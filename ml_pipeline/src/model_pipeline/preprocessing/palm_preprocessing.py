import cv2 as cv
import numpy as np

from src.model_pipeline.core.config import MODEL_SIZE


def prepare_input(
    image: np.ndarray,
) -> tuple[np.ndarray, float, int, int]:
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
    normalized = rgb.astype(np.float32) / 255.0
    input_tensor = np.expand_dims(normalized, axis=0)

    return input_tensor, scale, pad_left, pad_top