# Multi-hand tracker that combines palm detection and landmark detection
# into a frame-to-frame tracking loop
#
# Architecture:
#   1. For each active track, run the landmark model on the stored ROI.
#      If the hand is still present, update landmarks and derive the
#      next-frame ROI from those landmarks. Otherwise mark the track lost
#   2. If tracks were lost or capacity allows, run the palm detector on
#      the full frame, filter out detections that overlap existing tracks,
#      and initialize new tracks
#   3. Merge duplicate tracks whose IoU exceeds the merge threshold

import numpy as np

from dataclasses import replace
from src.model_pipeline.results.model_results import PalmDetection, ROI
from src.model_pipeline.models.palm_detector import PalmDetector
from src.model_pipeline.models.hand_landmark_detector import HandLandmarkDetector
from src.model_pipeline.tracking.roi_utils import pd_box_to_roi, landmarks_to_roi
from src.model_pipeline.core.config import (
    DEFAULT_HANDEDNESS,
    FILTER_OVERLAP_IOU,
    HANDEDNESS_THRESHOLD,
    MAX_HANDS,
    MODEL_SIZE,
    PRESENCE_THRESHOLD,
    TRACK_MERGE_IOU,
)
from typing import Optional


class _roi_track:
    """Internal per-hand tracking state."""

    def __init__(self) -> None:
        self.active = False
        self.roi: Optional[ROI] = None
        self.box: Optional[PalmDetection] = None
        self.landmarks: Optional[np.ndarray] = None
        self.raw_landmarks: Optional[np.ndarray] = None
        self.handedness: float = DEFAULT_HANDEDNESS
        self.sign_label: str = ""
        self.sign_confidence: float = 0.0


class MultiHandTracker:
    """Orchestrates palm detection, landmark detection, and frame-to-frame tracking

    Maintains up to ``max_hands`` simultaneous tracks. Each track persists
    across frames via ROI propagation from the landmark output, with
    fallback to full-frame palm detection when tracking is lost
    """

    def __init__(
        self,
        palm_model_path: str,
        landmark_model_path: str,
        max_hands: int = MAX_HANDS,
    ) -> None:
        self._palm = PalmDetector(palm_model_path)
        self._landmark = HandLandmarkDetector(landmark_model_path)
        self._max_hands = max_hands
        self._tracks = [_roi_track() for _ in range(max_hands)]
        self._last_scale = 0.0
        self._last_pad_left = 0
        self._last_pad_top = 0

    # -- Public properties expose active track state as flat lists ----------

    @property
    def is_tracking(self) -> bool:
        """True if at least one hand is currently being tracked"""
        return any(t.active for t in self._tracks)

    @property
    def active_count(self) -> int:
        """Number of currently active hand tracks"""
        return sum(1 for t in self._tracks if t.active)

    @property
    def last_landmarks(self) -> list[np.ndarray]:
        """Decoded (image-pixel) landmarks for all active tracks"""
        return [t.landmarks for t in self._tracks if t.active and t.landmarks is not None]

    @property
    def active_tracks(self):
        """Active track objects that have valid landmarks"""
        return [t for t in self._tracks if t.active and t.landmarks is not None]

    @property
    def handedness_scores(self) -> list[float]:
        """Raw handedness scores for active tracks"""
        return [t.handedness for t in self._tracks if t.active and t.landmarks is not None]

    @property
    def last_handedness(self) -> list[str]:
        """Handedness labels ('L' or 'R') for active tracks"""
        return ["R" if t.handedness > HANDEDNESS_THRESHOLD else "L" for t in self._tracks if t.active and t.landmarks is not None]

    @property
    def last_raw_landmarks(self) -> list[np.ndarray]:
        """ROI-normalized raw landmarks for active tracks"""
        return [t.raw_landmarks for t in self._tracks if t.active and t.raw_landmarks is not None]

    @property
    def last_sign_labels(self) -> list[str]:
        """Sign classification labels for active tracks"""
        return [t.sign_label for t in self._tracks if t.active and t.landmarks is not None]

    @property
    def last_sign_confidences(self) -> list[float]:
        """Sign classification confidence scores for active tracks"""
        return [t.sign_confidence for t in self._tracks if t.active and t.landmarks is not None]

    def step(
        self,
        frame: np.ndarray,
        score_threshold: float | None = None,
        iou_threshold: float | None = None,
    ) -> tuple[list[PalmDetection], float, int, int]:
        """Advance tracking by one frame

        Step 1: Update existing tracks via landmark model
        Step 2: Merge duplicate tracks
        Step 3: If under capacity, run palm detector on full frame and
                initialize new tracks from fresh detections

        Args:
            frame: Current BGR video frame
            score_threshold: Palm detection score threshold (override)
            iou_threshold: Palm NMS IoU threshold (override)

        Returns:
            Tuple of (active_boxes, scale, pad_left, pad_top) where
            active_boxes is a list of normalized PalmDetection objects
            for all currently tracked hands
        """
        lost_track = False

        # -- Step 1: Update existing tracks via landmark model --------------
        for track in [t for t in self._tracks if t.active]:
            landmarks, presence, handedness = self._landmark.detect(frame, track.roi)
            if presence >= PRESENCE_THRESHOLD and landmarks is not None:
                track.raw_landmarks = landmarks.copy()
                decoded = self._decode_landmarks_to_frame(landmarks, track.roi)
                track.landmarks = decoded
                track.handedness = handedness
                next_roi, next_box_pixel = landmarks_to_roi(decoded)
                next_box_pixel = replace(next_box_pixel, score=track.box.score)
                track.box = self._pixel_box_to_normalized(next_box_pixel)
                track.roi = next_roi
            else:
                track.active = False
                track.landmarks = None
                track.raw_landmarks = None
                lost_track = True

        # -- Step 2: Merge duplicate tracks whose IoU exceeds threshold -----
        active = [t for t in self._tracks if t.active]
        for i in range(len(active)):
            for j in range(i + 1, len(active)):
                if active[i].box is None or active[j].box is None:
                    continue
                if PalmDetector._calculate_iou(active[i].box.box, active[j].box.box) > TRACK_MERGE_IOU:
                    if active[i].box.score >= active[j].box.score:
                        active[j].active = False
                    else:
                        active[i].active = False

        # If at capacity and no tracks lost, skip full-frame detection
        if self.active_count >= self._max_hands and not lost_track:
            return [t.box for t in self._tracks if t.active], self._last_scale, self._last_pad_left, self._last_pad_top

        # -- Step 3: Full-frame palm detection for new tracks ----------------
        detections, scale, pad_left, pad_top = self._palm.detect(frame, score_threshold, iou_threshold)
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
        """Convert normalized ROI-space landmarks back to image pixel coordinates

        Each landmark is offset from the ROI center, rotated by the ROI
        rotation angle, and scaled by ROI dimensions

        Args:
            landmarks: (21, 2) array normalized in [0, 1] relative to ROI
            roi: The ROI that defines the coordinate transform

        Returns:
            (21, 2) array of (x, y) positions in image pixel space
        """
        dx = (landmarks[:, 0] - 0.5) * roi.w
        dy = (landmarks[:, 1] - 0.5) * roi.h
        cos_r = np.cos(roi.rotation)
        sin_r = np.sin(roi.rotation)
        decoded = np.empty_like(landmarks)
        decoded[:, 0] = roi.cx + dx * cos_r - dy * sin_r
        decoded[:, 1] = roi.cy + dx * sin_r + dy * cos_r
        return decoded

    def _pixel_box_to_normalized(self, box_pixel: PalmDetection) -> PalmDetection:
        """Convert a pixel-space PalmDetection back to normalized model space

        Reverses the letterbox transform so that the detection is expressed
        in the same normalized coordinate system as palm model outputs
        """
        box = box_pixel.box.copy()
        box[0::2] = (box[0::2] * self._last_scale + self._last_pad_left) / MODEL_SIZE
        box[1::2] = (box[1::2] * self._last_scale + self._last_pad_top) / MODEL_SIZE
        kps = box_pixel.keypoints.copy()
        kps[:, 0] = (kps[:, 0] * self._last_scale + self._last_pad_left) / MODEL_SIZE
        kps[:, 1] = (kps[:, 1] * self._last_scale + self._last_pad_top) / MODEL_SIZE
        return PalmDetection(index=box_pixel.index, score=box_pixel.score, box=box, keypoints=kps)


def _filter_overlapping(
    detections: list[PalmDetection],
    existing: list[_roi_track],
    iou_threshold: float = FILTER_OVERLAP_IOU,
) -> list[PalmDetection]:
    """Filter out new detections that overlap too much with existing tracks

    Prevents the tracker from initializing a second track on the same hand

    Args:
        detections: New palm detections
        existing: Current active tracks
        iou_threshold: Maximum allowed IoU with any existing track

    Returns:
        Filtered list of detections
    """
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
