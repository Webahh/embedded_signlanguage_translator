import cv2 as cv
import numpy as np

from src.model_pipeline.core.config import (
    LANDMARK_COUNT,
)
from src.model_pipeline.runtime.quantization import (
    quantize_tensor,
)


def prepare_hand_landmark_input(
    roi_bgr: np.ndarray,
    input_details: dict,
) -> np.ndarray:
    """
    Converts a BGR image or hand ROI into the input representation
    expected by the hand-landmark model.
    """

    if roi_bgr is None or roi_bgr.size == 0:
        raise ValueError(
            "Die Hand-ROI ist leer."
        )

    expected_shape = input_details["shape"]

    input_height = int(expected_shape[1])
    input_width = int(expected_shape[2])

    resized = cv.resize(
        roi_bgr,
        (input_width, input_height),
    )

    rgb = cv.cvtColor(
        resized,
        cv.COLOR_BGR2RGB,
    )

    normalized = (
        rgb.astype(np.float32) / 255.0
    )

    normalized = np.expand_dims(
        normalized,
        axis=0,
    )

    return quantize_tensor(
        normalized,
        input_details,
    )


def normalize_image_landmarks(
    landmarks: np.ndarray,
    input_width: int = 224,
    input_height: int = 224,
) -> np.ndarray:
    """
    Converts landmark coordinates from the hand-landmark input size
    into normalized coordinates.

    Input:
        Shape: [21, 3]
        x/y coordinates based on the 224x224 model input.

    Output:
        Shape: [21, 3]
        Normalized coordinates.
    """

    normalized = np.asarray(
        landmarks,
        dtype=np.float32,
    ).copy()

    expected_shape = (
        LANDMARK_COUNT,
        3,
    )

    if normalized.shape != expected_shape:
        raise ValueError(
            f"Erwartet wurde Landmark-Shape "
            f"{expected_shape}, "
            f"erhalten: {normalized.shape}"
        )

    normalized[:, 0] /= float(input_width)
    normalized[:, 1] /= float(input_height)

    # The model scales z approximately like x.
    normalized[:, 2] /= float(input_width)

    return normalized


def roi_landmarks_to_image_coordinates(
    roi_landmarks: np.ndarray,
    roi_x: float,
    roi_y: float,
    roi_width: float,
    roi_height: float,
    image_width: int,
    image_height: int,
) -> np.ndarray:
    """
    Converts normalized ROI landmarks into normalized coordinates
    of the complete input image.

    This currently supports an axis-aligned ROI. A rotated ROI will
    later require an inverse affine transformation.
    """

    image_landmarks = np.asarray(
        roi_landmarks,
        dtype=np.float32,
    ).copy()

    expected_shape = (
        LANDMARK_COUNT,
        3,
    )

    if image_landmarks.shape != expected_shape:
        raise ValueError(
            f"Erwartet wurde Landmark-Shape "
            f"{expected_shape}, "
            f"erhalten: {image_landmarks.shape}"
        )

    if image_width <= 0 or image_height <= 0:
        raise ValueError(
            "Die Bildgröße muss größer als null sein."
        )

    if roi_width <= 0 or roi_height <= 0:
        raise ValueError(
            "Die ROI-Größe muss größer als null sein."
        )

    image_landmarks[:, 0] = (
        roi_x
        + image_landmarks[:, 0] * roi_width
    ) / float(image_width)

    image_landmarks[:, 1] = (
        roi_y
        + image_landmarks[:, 1] * roi_height
    ) / float(image_height)

    image_landmarks[:, 2] = (
        image_landmarks[:, 2] * roi_width
    ) / float(image_width)

    return image_landmarks