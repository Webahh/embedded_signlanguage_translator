import numpy as np

from src.model_pipeline.models.palm_anchors import PALM_ANCHORS
from src.model_pipeline.preprocessing.palm_preprocessing import prepare_input
from src.model_pipeline.results.model_results import PalmDetection, ROI
from src.model_pipeline.models.tracking_utils import pd_box_to_roi, decode_landmark, landmarks_to_roi
from src.model_pipeline.runtime.interpreter import load_model
from src.model_pipeline.core.config import (
    IOU_THRESHOLD,
    MODEL_SIZE,
    SCORE_THRESHOLD,
    PRESENCE_THRESHOLD,
)
from typing import Optional


class PalmDetector:
    def __init__(self, model_path: str, ) -> None:
        self._interpreter = load_model(model_path)
        self._input_details = self._interpreter.get_input_details()[0]
        self._output_details = self._interpreter.get_output_details()

        self._is_tracking = False
        self._tracking_roi: Optional[ROI] = None
        self._tracking_box: Optional[PalmDetection] = None
        self._last_landmarks: Optional[np.ndarray] = None
        self._last_scale = 0.0
        self._last_pad_left = 0
        self._last_pad_top = 0

    @property
    def is_tracking(self) -> bool:
        return self._is_tracking

    @property
    def last_landmarks(self) -> Optional[np.ndarray]:
        return self._last_landmarks

    def detect(
        self,
        image: np.ndarray,
        hand_landmark_detector=None,
    ) -> tuple[
        list[PalmDetection],
        float,
        int,
        int,
    ]:
        if self._is_tracking and hand_landmark_detector is not None:
            landmarks, presence = hand_landmark_detector.detect(image, self._tracking_roi)
            if presence >= PRESENCE_THRESHOLD and landmarks is not None:
                decoded = self._decode_landmarks_to_frame(landmarks)
                self._last_landmarks = decoded
                next_roi, next_box_pixel = landmarks_to_roi(decoded)
                self._tracking_box = self._pixel_box_to_normalized(next_box_pixel)
                self._tracking_roi = next_roi
                return (
                    [self._tracking_box],
                    self._last_scale,
                    self._last_pad_left,
                    self._last_pad_top,
                )
            else:
                self._is_tracking = False
                self._last_landmarks = None

        (
            input_tensor,
            scale,
            pad_left,
            pad_top,
        ) = prepare_input(image)

        self._interpreter.set_tensor(self._input_details["index"], input_tensor, )
        self._interpreter.invoke()

        raw_scores = self._interpreter.get_tensor(self._output_details[0]["index"])[0, :, 0]
        raw_boxes = self._interpreter.get_tensor(self._output_details[1]["index"])[0]

        probabilities = self._sigmoid(raw_scores)
        valid_indices = np.where(probabilities >= SCORE_THRESHOLD)[0]

        detections = [
            self._decode_detection(
                index=int(index),
                score=float(probabilities[index]),
                raw=raw_boxes[index],
            )
            for index in valid_indices
        ]

        detections = self._non_max_suppression(
            detections
        )

        if detections and hand_landmark_detector is not None:
            best = detections[0]
            roi = pd_box_to_roi(best, scale, pad_left, pad_top, image.shape)
            self._tracking_roi = roi
            self._tracking_box = best
            self._last_scale = scale
            self._last_pad_left = pad_left
            self._last_pad_top = pad_top
            self._is_tracking = True

        return (
            detections,
            scale,
            pad_left,
            pad_top,
        )

    def _decode_landmarks_to_frame(
        self,
        landmarks: np.ndarray,
    ) -> np.ndarray:
        decoded = np.empty_like(landmarks)
        for i in range(landmarks.shape[0]):
            x, y = decode_landmark(landmarks[i, 0], landmarks[i, 1], self._tracking_roi)
            decoded[i, 0] = x
            decoded[i, 1] = y
        return decoded

    def _pixel_box_to_normalized(self, box_pixel: PalmDetection) -> PalmDetection:
        if self._tracking_box is None:
            return box_pixel

        orig_box = box_pixel.box
        orig_kps = box_pixel.keypoints

        new_box = np.empty(4, dtype=np.float32)
        for i in range(0, 4):
            coord = orig_box[i]
            if i % 2 == 0:
                model_coord = (coord * self._last_scale + self._last_pad_left) / MODEL_SIZE
            else:
                model_coord = (coord * self._last_scale + self._last_pad_top) / MODEL_SIZE
            new_box[i] = model_coord

        new_kps = np.empty_like(orig_kps)
        for k in range(orig_kps.shape[0]):
            new_kps[k, 0] = (orig_kps[k, 0] * self._last_scale + self._last_pad_left) / MODEL_SIZE
            new_kps[k, 1] = (orig_kps[k, 1] * self._last_scale + self._last_pad_top) / MODEL_SIZE

        return PalmDetection(
            index=box_pixel.index,
            score=box_pixel.score,
            box=new_box,
            keypoints=new_kps,
        )

    @staticmethod
    def _sigmoid(values: np.ndarray, ) -> np.ndarray:
        values = np.clip(values, -100.0, 100.0, )

        return 1.0 / (1.0 + np.exp(-values))

    @staticmethod
    def _decode_detection(index: int, score: float, raw: np.ndarray, ) -> PalmDetection:
        anchor_x, anchor_y = (PALM_ANCHORS[index])

        center_x = (raw[0] / MODEL_SIZE + anchor_x)
        center_y = (raw[1] / MODEL_SIZE + anchor_y)

        width = raw[2] / MODEL_SIZE
        height = raw[3] / MODEL_SIZE

        box = np.array(
            [
                center_x - width / 2.0,
                center_y - height / 2.0,
                center_x + width / 2.0,
                center_y + height / 2.0,
            ],
            dtype=np.float32,
        )

        keypoints = np.empty((7, 2), dtype=np.float32, )

        for keypoint_index in range(7):
            offset = (4 + keypoint_index * 2)

            keypoints[keypoint_index, 0] = (raw[offset] / MODEL_SIZE + anchor_x)
            keypoints[keypoint_index, 1] = (raw[offset + 1] / MODEL_SIZE + anchor_y)

        return PalmDetection(
            index=index,
            score=score,
            box=box,
            keypoints=keypoints,
        )

    @staticmethod
    def _calculate_iou(box_a: np.ndarray, box_b: np.ndarray, ) -> float:
        x1 = max(box_a[0], box_b[0])
        y1 = max(box_a[1], box_b[1])
        x2 = min(box_a[2], box_b[2])
        y2 = min(box_a[3], box_b[3])

        intersection_width = max(0.0, x2 - x1, )
        intersection_height = max(0.0, y2 - y1, )
        intersection = (intersection_width * intersection_height)

        area_a = max(0.0, box_a[2] - box_a[0]) * max(0.0, box_a[3] - box_a[1])
        area_b = max(0.0, box_b[2] - box_b[0]) * max(0.0, box_b[3] - box_b[1])

        union = area_a + area_b - intersection

        if union <= 0.0:
            return 0.0

        return float(intersection / union)

    def _non_max_suppression(self, detections: list[PalmDetection], ) -> list[PalmDetection]:
        remaining = sorted(
            detections,
            key=lambda detection: detection.score,
            reverse=True,
        )

        selected = []

        while remaining:
            best = remaining.pop(0)
            selected.append(best)

            remaining = [
                candidate
                for candidate in remaining
                if self._calculate_iou(
                    best.box,
                    candidate.box,
                ) < IOU_THRESHOLD
            ]

        return selected

    def debug_best_detection(
            self,
            image: np.ndarray,
    ) -> tuple[PalmDetection, float, int, int]:
        (
            input_tensor,
            scale,
            pad_left,
            pad_top,
        ) = prepare_input(image)

        self._interpreter.set_tensor(
            self._input_details["index"],
            input_tensor,
        )

        self._interpreter.invoke()

        raw_scores = self._interpreter.get_tensor(
            self._output_details[0]["index"]
        )[0, :, 0]

        raw_boxes = self._interpreter.get_tensor(
            self._output_details[1]["index"]
        )[0]

        probabilities = self._sigmoid(
            raw_scores
        )

        best_index = int(
            np.argmax(probabilities)
        )

        best_detection = self._decode_detection(
            index=best_index,
            score=float(probabilities[best_index]),
            raw=raw_boxes[best_index],
        )

        return (
            best_detection,
            scale,
            pad_left,
            pad_top,
        )