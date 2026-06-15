# ROI (Region of Interest) utility functions.
#
# Converts between palm detection results, landmark results, and the
# rotated ROI format used to crop-and-rectify hand regions for the
# landmark model. Also provides coordinate decoding from ROI space
# back to image pixel space.

import numpy as np

from src.model_pipeline.core.config import (
    LANDMARK_BBOX_INDICES,
    LANDMARK_BOX_SCORE,
    LANDMARK_ROI_SCALE,
    LANDMARK_ROI_SHIFT_X,
    LANDMARK_ROI_SHIFT_Y,
    PALM_ROI_SCALE,
    PALM_ROI_SHIFT_X,
    PALM_ROI_SHIFT_Y,
    PALM_TO_LANDMARK_INDEX_MAP,
)
from src.model_pipeline.results.model_results import PalmDetection, ROI
from src.model_pipeline.postprocessing.palm_visualization import model_to_original_point


def pd_box_to_roi(
    detection: PalmDetection,
    scale: float,
    pad_left: int,
    pad_top: int,
) -> ROI:
    """Convert a palm detection result to a cropped ROI for the landmark model.

    Decodes the normalized box and keypoints back to original pixel space,
    computes the palm rotation angle from landmarks 0 and 2, and applies
    the configured shift/scale to produce the final landmark model ROI.

    Args:
        detection: Palm detection result.
        scale, pad_left, pad_top: Letterbox params from preprocessing.

    Returns:
        ROI centered on the palm, rotated, and expanded for landmark detection.
    """
    x1, y1 = model_to_original_point(detection.box[0], detection.box[1], scale, pad_left, pad_top)
    x2, y2 = model_to_original_point(detection.box[2], detection.box[3], scale, pad_left, pad_top)

    cx = (x1 + x2) / 2.0
    cy = (y1 + y2) / 2.0
    w = float(x2 - x1)
    h = float(y2 - y1)

    kp0_x, kp0_y = model_to_original_point(
        detection.keypoints[0, 0], detection.keypoints[0, 1],
        scale, pad_left, pad_top,
    )
    kp2_x, kp2_y = model_to_original_point(
        detection.keypoints[2, 0], detection.keypoints[2, 1],
        scale, pad_left, pad_top,
    )

    # Compute rotation from the line between palm keypoints 0 and 2
    rotation = np.pi * 0.5 - np.arctan2(-(kp2_y - kp0_y), kp2_x - kp0_x)
    rotation = _normalize_angle(rotation)

    roi = ROI(cx=cx, cy=cy, w=w, h=h, rotation=rotation)
    _roi_shift_and_scale(roi, PALM_ROI_SHIFT_X, PALM_ROI_SHIFT_Y, PALM_ROI_SCALE, PALM_ROI_SCALE)

    return roi


def decode_landmark(
    lm_x: float,
    lm_y: float,
    roi: ROI,
) -> tuple[float, float]:
    """Decode a single normalized ROI-space landmark to image pixel coordinates.

    Reverses the ROI cropping: landmark is offset from ROI center, rotated,
    and scaled by ROI dimensions.

    Args:
        lm_x, lm_y: Normalized landmark coordinates in [0, 1] relative to ROI.
        roi: The ROI defining the crop coordinate system.

    Returns:
        (x, y) pixel coordinates in the original image frame.
    """
    dx = (lm_x - 0.5) * roi.w
    dy = (lm_y - 0.5) * roi.h
    cos_r = np.cos(roi.rotation)
    sin_r = np.sin(roi.rotation)
    x = roi.cx + dx * cos_r - dy * sin_r
    y = roi.cy + dx * sin_r + dy * cos_r
    return x, y


def landmarks_to_roi(
    decoded_landmarks: np.ndarray,
) -> tuple[ROI, PalmDetection]:
    """Derive a new tracking ROI and fake palm detection from decoded landmarks

    Uses a subset of landmark indices (LANDMARK_BBOX_INDICES) to compute
    the bounding box, and landmarks 0 and 9 to compute orientation.
    The resulting ROI is shifted/scaled for the next frame's landmark model

    Args:
        decoded_landmarks: (21, 2) array of landmark positions in image pixels

    Returns:
        Tuple of (next_frame_ROI, PalmDetection proxy with box and keypoints
        in pixel space)
    """
    indices = np.array(LANDMARK_BBOX_INDICES)
    selected = decoded_landmarks[indices]
    min_xy = selected.min(axis=0)
    max_xy = selected.max(axis=0)
    cx = (max_xy[0] + min_xy[0]) / 2.0
    cy = (max_xy[1] + min_xy[1]) / 2.0
    w = max_xy[0] - min_xy[0]
    h = max_xy[1] - min_xy[1]

    x0, y0 = decoded_landmarks[0]
    x9, y9 = decoded_landmarks[9]
    rotation = np.pi * 0.5 - np.arctan2(-(y9 - y0), x9 - x0)
    rotation = _normalize_angle(rotation)

    pd_to_ld_idx = np.array(PALM_TO_LANDMARK_INDEX_MAP)
    keypoints = decoded_landmarks[pd_to_ld_idx]

    box = np.array([
        cx - w / 2.0, cy - h / 2.0,
        cx + w / 2.0, cy + h / 2.0,
    ], dtype=np.float32)

    palm_box = PalmDetection(
        index=0,
        score=LANDMARK_BOX_SCORE,
        box=box,
        keypoints=keypoints,
    )

    roi = ROI(cx=cx, cy=cy, w=w, h=h, rotation=rotation)
    _roi_shift_and_scale(roi, LANDMARK_ROI_SHIFT_X, LANDMARK_ROI_SHIFT_Y, LANDMARK_ROI_SCALE, LANDMARK_ROI_SCALE)

    return roi, palm_box


def _normalize_angle(angle: float) -> float:
    """Normalize an angle to the range [-pi, pi)"""
    return angle - 2 * np.pi * np.floor((angle - (-np.pi)) / (2 * np.pi))


def _roi_shift_and_scale(roi: ROI, shift_x: float, shift_y: float, scale_x: float, scale_y: float) -> None:
    """Apply a shift and scale transform to an ROI in place

    Translates the ROI center by (shift_x, shift_y) relative to its
    axes, then expands the shorter side to match the longer side and
    scales by the given factors
    """
    sx = roi.w * shift_x * np.cos(roi.rotation) - roi.h * shift_y * np.sin(roi.rotation)
    sy = roi.w * shift_x * np.sin(roi.rotation) + roi.h * shift_y * np.cos(roi.rotation)
    roi.cx += sx
    roi.cy += sy
    long_side = max(roi.w, roi.h)
    roi.w = long_side * scale_x
    roi.h = long_side * scale_y
