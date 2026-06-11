import numpy as np

from src.model_pipeline.results.model_results import PalmDetection, ROI
from src.model_pipeline.models.palm_detector import PalmDetector
from src.model_pipeline.models.hand_landmark_detector import HandLandmarkDetector
from src.model_pipeline.tracking.roi_utils import pd_box_to_roi, landmarks_to_roi
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
        self.handedness: float = 0.5


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

    @property
    def last_handedness(self) -> list[str]:
        return ["R" if t.handedness > 0.5 else "L" for t in self._tracks if t.active and t.landmarks is not None]

    def step(
        self,
        frame: np.ndarray,
    ) -> tuple[list[PalmDetection], float, int, int]:
        lost_track = False

        for track in [t for t in self._tracks if t.active]:
            landmarks, presence, handedness = self._landmark.detect(frame, track.roi)
            if presence >= PRESENCE_THRESHOLD and landmarks is not None:
                decoded = self._decode_landmarks_to_frame(landmarks, track.roi)
                track.landmarks = decoded
                track.handedness = handedness
                next_roi, next_box_pixel = landmarks_to_roi(decoded)
                track.box = self._pixel_box_to_normalized(next_box_pixel)
                track.roi = next_roi
            else:
                track.active = False
                track.landmarks = None
                lost_track = True

        active = [t for t in self._tracks if t.active]
        for i in range(len(active)):
            for j in range(i + 1, len(active)):
                if active[i].box is None or active[j].box is None:
                    continue
                if PalmDetector._calculate_iou(active[i].box.box, active[j].box.box) > 0.5:
                    if active[i].box.score >= active[j].box.score:
                        active[j].active = False
                    else:
                        active[i].active = False

        if self.active_count >= self._max_hands and not lost_track:
            return [t.box for t in self._tracks if t.active], self._last_scale, self._last_pad_left, self._last_pad_top

        detections, scale, pad_left, pad_top = self._palm.detect(frame)
        self._last_scale = scale
        self._last_pad_left = pad_left
        self._last_pad_top = pad_top

        detections = _filter_overlapping(detections, [t for t in self._tracks if t.active])

        empty_slots = [t for t in self._tracks if not t.active]
        for i, detection in enumerate(detections):
            if i >= len(empty_slots):
                break
            roi = pd_box_to_roi(detection, scale, pad_left, pad_top)
            empty_slots[i].roi = roi
            empty_slots[i].box = detection
            empty_slots[i].active = True

        return [t.box for t in self._tracks if t.active], scale, pad_left, pad_top

    @staticmethod
    def _decode_landmarks_to_frame(landmarks: np.ndarray, roi: ROI) -> np.ndarray:
        dx = (landmarks[:, 0] - 0.5) * roi.w
        dy = (landmarks[:, 1] - 0.5) * roi.h
        cos_r = np.cos(roi.rotation)
        sin_r = np.sin(roi.rotation)
        decoded = np.empty_like(landmarks)
        decoded[:, 0] = roi.cx + dx * cos_r - dy * sin_r
        decoded[:, 1] = roi.cy + dx * sin_r + dy * cos_r
        return decoded

    def _pixel_box_to_normalized(self, box_pixel: PalmDetection) -> PalmDetection:
        box = box_pixel.box.copy()
        box[0::2] = (box[0::2] * self._last_scale + self._last_pad_left) / MODEL_SIZE
        box[1::2] = (box[1::2] * self._last_scale + self._last_pad_top) / MODEL_SIZE
        kps = box_pixel.keypoints.copy()
        kps[:, 0] = (kps[:, 0] * self._last_scale + self._last_pad_left) / MODEL_SIZE
        kps[:, 1] = (kps[:, 1] * self._last_scale + self._last_pad_top) / MODEL_SIZE
        return PalmDetection(index=box_pixel.index, score=box_pixel.score, box=box, keypoints=kps)


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
