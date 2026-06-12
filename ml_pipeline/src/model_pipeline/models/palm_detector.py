import numpy as np

from src.model_pipeline.models.palm_anchors import PALM_ANCHORS
from src.model_pipeline.preprocessing.palm_preprocessing import prepare_input
from src.model_pipeline.results.model_results import PalmDetection
from src.model_pipeline.runtime.interpreter import load_model
from src.model_pipeline.core.config import (
    IOU_THRESHOLD,
    MODEL_SIZE,
    NUM_PALM_KEYPOINTS,
    PALM_KEYPOINT_OFFSET,
    SCORE_THRESHOLD,
    SIGMOID_CLIP_MAX,
    SIGMOID_CLIP_MIN,
)


class PalmDetector:
    def __init__(self, model_path: str) -> None:
        self._interpreter = load_model(model_path)
        self._input_details = self._interpreter.get_input_details()[0]
        self._output_details = self._interpreter.get_output_details()

    def detect(
        self,
        image: np.ndarray,
        score_threshold: float | None = None,
        iou_threshold: float | None = None,
    ) -> tuple[list[PalmDetection], float, int, int]:
        score_threshold = score_threshold if score_threshold is not None else SCORE_THRESHOLD
        iou_threshold = iou_threshold if iou_threshold is not None else IOU_THRESHOLD

        input_tensor, scale, pad_left, pad_top = prepare_input(image)

        self._interpreter.set_tensor(self._input_details["index"], input_tensor)
        self._interpreter.invoke()

        raw_scores = self._interpreter.get_tensor(self._output_details[0]["index"])[0, :, 0]
        raw_boxes = self._interpreter.get_tensor(self._output_details[1]["index"])[0]

        probabilities = self._sigmoid(raw_scores)
        valid_indices = np.where(probabilities >= score_threshold)[0]

        detections = [
            self._decode_detection(
                index=int(index),
                score=float(probabilities[index]),
                raw=raw_boxes[index],
            )
            for index in valid_indices
        ]

        detections = self._non_max_suppression(detections, iou_threshold)

        return detections, scale, pad_left, pad_top

    @staticmethod
    def _sigmoid(values: np.ndarray) -> np.ndarray:
        values = np.clip(values, SIGMOID_CLIP_MIN, SIGMOID_CLIP_MAX)
        return 1.0 / (1.0 + np.exp(-values))

    @staticmethod
    def _decode_detection(index: int, score: float, raw: np.ndarray) -> PalmDetection:
        anchor_x, anchor_y = PALM_ANCHORS[index]

        center_x = raw[0] / MODEL_SIZE + anchor_x
        center_y = raw[1] / MODEL_SIZE + anchor_y
        width = raw[2] / MODEL_SIZE
        height = raw[3] / MODEL_SIZE

        box = np.array([
            center_x - width / 2.0,
            center_y - height / 2.0,
            center_x + width / 2.0,
            center_y + height / 2.0,
        ], dtype=np.float32)

        keypoints = np.empty((NUM_PALM_KEYPOINTS, 2), dtype=np.float32)
        for keypoint_index in range(NUM_PALM_KEYPOINTS):
            offset = PALM_KEYPOINT_OFFSET + keypoint_index * 2
            keypoints[keypoint_index, 0] = raw[offset] / MODEL_SIZE + anchor_x
            keypoints[keypoint_index, 1] = raw[offset + 1] / MODEL_SIZE + anchor_y

        return PalmDetection(index=index, score=score, box=box, keypoints=keypoints)

    @staticmethod
    def _calculate_iou(box_a: np.ndarray, box_b: np.ndarray) -> float:
        x1 = max(box_a[0], box_b[0])
        y1 = max(box_a[1], box_b[1])
        x2 = min(box_a[2], box_b[2])
        y2 = min(box_a[3], box_b[3])

        intersection = max(0.0, x2 - x1) * max(0.0, y2 - y1)
        area_a = max(0.0, box_a[2] - box_a[0]) * max(0.0, box_a[3] - box_a[1])
        area_b = max(0.0, box_b[2] - box_b[0]) * max(0.0, box_b[3] - box_b[1])
        union = area_a + area_b - intersection

        if union <= 0.0:
            return 0.0
        return float(intersection / union)

    def _non_max_suppression(self, detections: list[PalmDetection], iou_threshold: float = IOU_THRESHOLD) -> list[PalmDetection]:
        remaining = sorted(detections, key=lambda d: d.score, reverse=True)
        selected = []
        while remaining:
            best = remaining.pop(0)
            selected.append(best)
            remaining = [
                c for c in remaining
                if self._calculate_iou(best.box, c.box) < iou_threshold
            ]
        return selected

    def debug_best_detection(
        self,
        image: np.ndarray,
    ) -> tuple[PalmDetection, float, int, int]:
        input_tensor, scale, pad_left, pad_top = prepare_input(image)
        self._interpreter.set_tensor(self._input_details["index"], input_tensor)
        self._interpreter.invoke()

        raw_scores = self._interpreter.get_tensor(self._output_details[0]["index"])[0, :, 0]
        raw_boxes = self._interpreter.get_tensor(self._output_details[1]["index"])[0]

        probabilities = self._sigmoid(raw_scores)
        best_index = int(np.argmax(probabilities))
        best_detection = self._decode_detection(
            index=best_index,
            score=float(probabilities[best_index]),
            raw=raw_boxes[best_index],
        )
        return best_detection, scale, pad_left, pad_top
