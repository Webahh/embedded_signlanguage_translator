import numpy as np

from src.model_pipeline.results.model_results import PalmDetection, ROI
from src.model_pipeline.models.palm_detector import PalmDetector
from src.model_pipeline.models.hand_landmark_detector import HandLandmarkDetector
from src.model_pipeline.tracking.roi_utils import pd_box_to_roi, decode_landmark, landmarks_to_roi
from src.model_pipeline.core.config import (
    MAX_HANDS,
    PRESENCE_THRESHOLD,
    MODEL_SIZE,
)
from typing import Optional


class _Track:
    def __init__(self) -> None:
        self.active = False
        self.roi: Optional[ROI] = None
        self.box: Optional[PalmDetection] = None
        self.landmarks: Optional[np.ndarray] = None


class MultiHandTracker:
    def __init__(
        self,
        palm_model_path: str,
        landmark_model_path: str,
        max_hands: int = MAX_HANDS,
    ) -> None:
        self._palm = PalmDetector(palm_model_path)
        self._landmark = HandLandmarkDetector(landmark_model_path)
        self._max_hands = max_hands
        self._tracks = [_Track() for _ in range(max_hands)]
        self._last_scale = 0.0
        self._last_pad_left = 0
        self._last_pad_top = 0

    @property
    def is_tracking(self) -> bool:
        return any(t.active for t in self._tracks)

    @property
    def active_count(self) -> int:
        return sum(1 for t in self._tracks if t.active)

    @property
    def last_landmarks(self) -> list[np.ndarray]:
        return [t.landmarks for t in self._tracks if t.active and t.landmarks is not None]

    def step(
        self,
        frame: np.ndarray,
    ) -> tuple[list[PalmDetection], float, int, int]:
        active_tracks = [t for t in self._tracks if t.active]
        tracked_ok = []
        lost_track = False

        for track in active_tracks:
            landmarks, presence = self._landmark.detect(frame, track.roi)
            if presence >= PRESENCE_THRESHOLD and landmarks is not None:
                decoded = self._decode_landmarks_to_frame(landmarks, track.roi)
                track.landmarks = decoded
                next_roi, next_box_pixel = landmarks_to_roi(decoded)
                track.box = self._pixel_box_to_normalized(next_box_pixel)
                track.roi = next_roi
                tracked_ok.append(track)
            else:
                track.active = False
                track.landmarks = None
                lost_track = True

        fully_tracked = self.active_count >= self._max_hands
        if fully_tracked and not lost_track:
            result = [t.box for t in self._tracks if t.active]
            return result, self._last_scale, self._last_pad_left, self._last_pad_top

        detections, scale, pad_left, pad_top = self._palm.detect(frame)
        self._last_scale = scale
        self._last_pad_left = pad_left
        self._last_pad_top = pad_top

        detections = self._filter_overlapping(detections, tracked_ok)

        empty_slots = [t for t in self._tracks if not t.active]
        for i, detection in enumerate(detections):
            if i >= len(empty_slots):
                break
            roi = pd_box_to_roi(detection, scale, pad_left, pad_top)
            empty_slots[i].roi = roi
            empty_slots[i].box = detection
            empty_slots[i].active = True

        result = [t.box for t in self._tracks if t.active]
        return result, scale, pad_left, pad_top

    @staticmethod
    def _decode_landmarks_to_frame(
        landmarks: np.ndarray,
        roi: ROI,
    ) -> np.ndarray:
        decoded = np.empty_like(landmarks)
        for i in range(landmarks.shape[0]):
            x, y = decode_landmark(landmarks[i, 0], landmarks[i, 1], roi)
            decoded[i, 0] = x
            decoded[i, 1] = y
        return decoded

    def _pixel_box_to_normalized(self, box_pixel: PalmDetection) -> PalmDetection:
        orig_box = box_pixel.box
        orig_kps = box_pixel.keypoints
        new_box = np.empty(4, dtype=np.float32)
        for i in range(4):
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
    def _filter_overlapping(
        detections: list[PalmDetection],
        existing: list[_Track],
        iou_threshold: float = 0.2,
    ) -> list[PalmDetection]:
        if not existing:
            return detections
        filtered = []
        for det in detections:
            too_close = False
            for track in existing:
                if track.box is not None and PalmDetector._calculate_iou(det.box, track.box.box) > iou_threshold:
                    too_close = True
                    break
            if not too_close:
                filtered.append(det)
        return filtered
