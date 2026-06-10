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
) -> None:
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
    cv.putText(
        frame,
        f"{detection.score:.3f}",
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
