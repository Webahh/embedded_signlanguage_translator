import cv2 as cv
import numpy as np

from src.model_pipeline.core.config import MODEL_SIZE


def letterbox(image: np.ndarray, ) -> tuple[np.ndarray, float, int, int]:
    height, width = image.shape[:2]

    scale = min(MODEL_SIZE / width, MODEL_SIZE / height, )
    resized_width = int(round(width * scale))
    resized_height = int(round(height * scale))
    resized = cv.resize(image, (resized_width, resized_height), )

    pad_left = (MODEL_SIZE - resized_width) // 2
    pad_top = (MODEL_SIZE - resized_height) // 2
    pad_right = (MODEL_SIZE - resized_width - pad_left)
    pad_bottom = (MODEL_SIZE - resized_height - pad_top)

    output = cv.copyMakeBorder(
        resized,
        pad_top,
        pad_bottom,
        pad_left,
        pad_right,
        cv.BORDER_CONSTANT,
        value=(0, 0, 0),
    )

    return output, scale, pad_left, pad_top


def prepare_input(
    image: np.ndarray,
) -> tuple[np.ndarray, float, int, int]:
    image_height, image_width = image.shape[:2]

    output_width = MODEL_SIZE

    output_height = int(
        MODEL_SIZE * image_height / image_width
    )

    if output_height > MODEL_SIZE:
        output_height = MODEL_SIZE

    resized = cv.resize(
        image,
        (
            output_width,
            output_height,
        ),
    )

    rgb = cv.cvtColor(
        resized,
        cv.COLOR_BGR2RGB,
    )

    canvas = np.zeros(
        (
            MODEL_SIZE,
            MODEL_SIZE,
            3,
        ),
        dtype=np.float32,
    )

    canvas[
        0:output_height,
        0:output_width,
    ] = rgb.astype(np.float32) / 255.0

    input_tensor = np.expand_dims(
        canvas,
        axis=0,
    )

    scale = output_width / image_width

    pad_left = 0
    pad_top = 0

    return (
        input_tensor,
        scale,
        pad_left,
        pad_top,
    )