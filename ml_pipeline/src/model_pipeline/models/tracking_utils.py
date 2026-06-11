import numpy as np

from src.model_pipeline.results.model_results import PalmDetection, ROI
from src.model_pipeline.postprocessing.palm_visualization import model_to_original_point

def pd_box_to_roi(
    detection: PalmDetection,
    scale: float,
    pad_left: int,
    pad_top: int,
    image_shape: tuple[int, int, int],
) -> ROI:
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

    rotation = np.pi * 0.5 - np.arctan2(-(kp2_y - kp0_y), kp2_x - kp0_x)
    rotation = _normalize_angle(rotation)

    shift_x = 0.0
    shift_y = -0.5
    roi_scale = 2.6

    roi = ROI(cx=cx, cy=cy, w=w, h=h, rotation=rotation)
    _roi_shift_and_scale(roi, shift_x, shift_y, roi_scale, roi_scale)

    return roi


def decode_landmark(
    lm_x: float,
    lm_y: float,
    roi: ROI,
) -> tuple[float, float]:
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
    indices = np.array([0, 1, 2, 3, 5, 6, 9, 10, 13, 14, 17, 18])
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

    pd_to_ld_idx = [0, 5, 9, 13, 17, 1, 2]
    keypoints = decoded_landmarks[pd_to_ld_idx]

    box = np.array([
        cx - w / 2.0, cy - h / 2.0,
        cx + w / 2.0, cy + h / 2.0,
    ], dtype=np.float32)

    palm_box = PalmDetection(
        index=0,
        score=1.0,
        box=box,
        keypoints=keypoints,
    )

    shift_x = 0.0
    shift_y = -0.1
    track_scale = 2.0

    roi = ROI(cx=cx, cy=cy, w=w, h=h, rotation=rotation)
    _roi_shift_and_scale(roi, shift_x, shift_y, track_scale, track_scale)

    return roi, palm_box


def _normalize_angle(angle: float) -> float:
    return angle - 2 * np.pi * np.floor((angle - (-np.pi)) / (2 * np.pi))


def _roi_shift_and_scale(roi: ROI, shift_x: float, shift_y: float, scale_x: float, scale_y: float) -> None:
    sx = roi.w * shift_x * np.cos(roi.rotation) - roi.h * shift_y * np.sin(roi.rotation)
    sy = roi.w * shift_x * np.sin(roi.rotation) + roi.h * shift_y * np.cos(roi.rotation)
    roi.cx += sx
    roi.cy += sy
    long_side = max(roi.w, roi.h)
    roi.w = long_side * scale_x
    roi.h = long_side * scale_y
