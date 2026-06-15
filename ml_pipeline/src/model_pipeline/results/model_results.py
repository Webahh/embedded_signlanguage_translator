# Data classes for model pipeline outputs.
#
# PalmDetection: A detected hand bounding box with keypoints (frozen dataclass).
# ROI: A rotated region of interest used to crop and rectify hand images.

import numpy as np
from dataclasses import dataclass


@dataclass(frozen=True)
class PalmDetection:
    """A palm or hand bounding box detection

    NOTE: Although frozen, the contained numpy arrays are still mutable
    Use .copy() for safe access if modification is required

    Attributes:
        index: Anchor index that produced this detection
        score: Detection confidence (after sigmoid)
        box: Bounding box in xyxy format (normalized [0,1] or pixel)
        keypoints: Palm keypoint coordinates (NUM_PALM_KEYPOINTS x 2)
    """
    index: int
    score: float
    box: np.ndarray
    keypoints: np.ndarray


@dataclass
class ROI:
    """Rotated region of interest

    Attributes:
        cx, cy: Center of the ROI in image pixel coordinates
        w, h: Width and height of the ROI in pixels
        rotation: Rotation angle of the ROI in radians
    """
    cx: float
    cy: float
    w: float
    h: float
    rotation: float
