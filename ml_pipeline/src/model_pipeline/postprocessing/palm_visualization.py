# Visualization utilities for palm detection results
#
# Provides coordinate mapping from normalized model space back to original
# image pixel space, and drawing routines for bounding boxes and keypoints

import cv2 as cv
import numpy as np

from src.model_pipeline.core.config import MODEL_SIZE
from src.model_pipeline.results.model_results import PalmDetection


def model_to_original_point(
        x_normalized: float,
        y_normalized: float,
        scale: float,
        pad_left: int,
        pad_top: int,
) -> tuple[int, int]:
    """Convert a normalized model-space point back to original image pixels

    Reverses the letterbox transform applied by prepare_input()

    Args:
        x_normalized, y_normalized: Coordinates in [0, 1] model space
        scale: Image-to-model scale factor from preprocessing
        pad_left, pad_top: Letterbox padding from preprocessing

    Returns:
        (x, y) integer pixel coordinates in the original image
    """
    model_x = x_normalized * MODEL_SIZE
    model_y = y_normalized * MODEL_SIZE

    original_x = (model_x - pad_left) / scale
    original_y = (model_y - pad_top) / scale

    return int(round(original_x)), int(round(original_y)),


def draw_detection(
        frame: np.ndarray,
        detection: PalmDetection,
        scale: float,
        pad_left: int,
        pad_top: int,
        label: str = "",
) -> None:
    """Draw a palm detection bounding box, keypoints, and confidence label

    Args:
        frame: BGR image to draw on (modified in place)
        detection: PalmDetection result with box and keypoints
        scale, pad_left, pad_top: Letterbox parameters from preprocessing
        label: Optional text prepended to the confidence score
    """
    x1, y1 = model_to_original_point(
        detection.box[0],
        detection.box[1],
        scale,
        pad_left,
        pad_top,
    )

    x2, y2 = model_to_original_point(
        detection.box[2],
        detection.box[3],
        scale,
        pad_left,
        pad_top,
    )

    cv.rectangle(frame, (x1, y1), (x2, y2), (0, 255, 0), 2, )
    label_text = f"{detection.score:.3f}" if not label else f"{label} {detection.score:.3f}"
    cv.putText(
        frame,
        label_text,
        (x1, max(20, y1 - 10)),
        cv.FONT_HERSHEY_SIMPLEX,
        0.6,
        (0, 255, 0),
        2,
    )

    for index, keypoint in enumerate(
        detection.keypoints
    ):
        x, y = model_to_original_point(
            keypoint[0],
            keypoint[1],
            scale,
            pad_left,
            pad_top,
        )

        cv.circle(frame, (x, y), 5, (0, 0, 255), -1, )
        cv.putText(
            frame,
            str(index),
            (x + 5, y - 5),
            cv.FONT_HERSHEY_SIMPLEX,
            0.4,
            (255, 255, 255),
            1,
        )
