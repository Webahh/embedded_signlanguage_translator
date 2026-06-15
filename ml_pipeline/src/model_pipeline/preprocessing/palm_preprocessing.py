# Palm detection model input preparation
#
# Resizes the input image to the model's expected square size (MODEL_SIZE)
# using letterbox padding (preserving aspect ratio with zero-padding),
# converts BGR to RGB, and normalizes pixel values to [0, 1]

import cv2 as cv
import numpy as np

from src.model_pipeline.core.config import IMAGE_NORMALIZE_DIVISOR, MODEL_SIZE


def prepare_input(
    image: np.ndarray,
) -> tuple[np.ndarray, float, int, int]:
    """Prepare a full-frame image for palm model inference

    The image is resized with aspect ratio preservation and zero-padded
    to a square (MODEL_SIZE x MODEL_SIZE). Returns the normalized input
    tensor along with the scale factor and padding amounts needed to
    map model-space coordinates back to the original image

    Args:
        image: Input BGR image (height x width x 3)

    Returns:
        Tuple of (input_tensor, scale, pad_left, pad_top)
        - input_tensor: float32 tensor of shape (1, MODEL_SIZE, MODEL_SIZE, 3)
        - scale: ratio of original pixels to model pixels
        - pad_left, pad_top: number of zero-padding pixels on left/top edges
    """
    height, width = image.shape[:2]

    scale = min(MODEL_SIZE / width, MODEL_SIZE / height)
    resized_width = int(round(width * scale))
    resized_height = int(round(height * scale))
    resized = cv.resize(image, (resized_width, resized_height))

    pad_left = (MODEL_SIZE - resized_width) // 2
    pad_top = (MODEL_SIZE - resized_height) // 2
    pad_right = MODEL_SIZE - resized_width - pad_left
    pad_bottom = MODEL_SIZE - resized_height - pad_top

    padded = cv.copyMakeBorder(
        resized,
        pad_top,
        pad_bottom,
        pad_left,
        pad_right,
        cv.BORDER_CONSTANT,
        value=(0, 0, 0),
    )

    rgb = cv.cvtColor(padded, cv.COLOR_BGR2RGB)
    normalized = rgb.astype(np.float32) / IMAGE_NORMALIZE_DIVISOR
    input_tensor = np.expand_dims(normalized, axis=0)

    return input_tensor, scale, pad_left, pad_top
