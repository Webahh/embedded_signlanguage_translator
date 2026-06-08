import cv2 as cv
import numpy as np

from src.model_pipeline.core.config import (
    PALM_MODEL_PATH,
    SCORE_THRESHOLD,
)
from src.model_pipeline.models.palm_detector import (
    PalmDetector,
)
from src.model_pipeline.preprocessing.palm_preprocessing import (
    prepare_input,
)
from src.model_pipeline.runtime.interpreter import (
    load_model,
)


TEST_IMAGE_PATH = "hand_test.jpg"


def sigmoid(value: float) -> float:
    return float(
        1.0 / (1.0 + np.exp(-value))
    )


def print_tensor_details(
    title: str,
    tensors: list[dict],
) -> None:
    print(f"\n{title}")
    print("=" * len(title))

    for tensor in tensors:
        print(
            f"Name:          {tensor['name']}\n"
            f"Shape:         {tensor['shape']}\n"
            f"Dtype:         {tensor['dtype']}\n"
            f"Quantisierung: {tensor['quantization']}\n"
            f"Index:         {tensor['index']}\n"
        )


def analyze_raw_outputs(
    image: np.ndarray,
) -> None:
    interpreter = load_model(
        PALM_MODEL_PATH
    )

    input_details = (
        interpreter.get_input_details()[0]
    )

    output_details = (
        interpreter.get_output_details()
    )

    input_tensor, _, _, _ = prepare_input(
        image
    )

    interpreter.set_tensor(
        input_details["index"],
        input_tensor,
    )

    interpreter.invoke()

    raw_scores = interpreter.get_tensor(
        output_details[0]["index"]
    )[0, :, 0]

    raw_boxes = interpreter.get_tensor(
        output_details[1]["index"]
    )[0]

    probabilities = 1.0 / (
        1.0 + np.exp(-raw_scores)
    )

    best_index = int(
        np.argmax(probabilities)
    )

    valid_indices = np.where(
        probabilities >= SCORE_THRESHOLD
    )[0]

    print("\nRaw output analysis")
    print("===================")

    print(
        f"Best anchor index: {best_index}"
    )

    print(
        f"Best logit: "
        f"{raw_scores[best_index]:.6f}"
    )

    print(
        f"Best probability: "
        f"{probabilities[best_index]:.6f}"
    )

    print(
        f"Anchors above threshold: "
        f"{len(valid_indices)}"
    )

    print("\nBest raw box/keypoints:")
    print(
        raw_boxes[best_index]
    )


def analyze_postprocessing(
    image: np.ndarray,
) -> None:
    detector = PalmDetector(
        PALM_MODEL_PATH
    )

    detections, _, _, _ = detector.detect(
        image
    )

    print("\nPostprocessing")
    print("==============")

    print(
        f"Detections after NMS: "
        f"{len(detections)}"
    )

    for detection in detections:
        print(
            f"\nAnchor index: {detection.index}\n"
            f"Score:        {detection.score:.6f}\n"
            f"Box:          {detection.box}\n"
            f"Keypoints:\n{detection.keypoints}"
        )


def main() -> None:
    interpreter = load_model(
        PALM_MODEL_PATH
    )

    print_tensor_details(
        "Inputs",
        interpreter.get_input_details(),
    )

    print_tensor_details(
        "Outputs",
        interpreter.get_output_details(),
    )

    image = cv.imread(
        TEST_IMAGE_PATH
    )

    if image is None:
        raise FileNotFoundError(
            f"Testbild nicht gefunden: "
            f"{TEST_IMAGE_PATH}"
        )

    analyze_raw_outputs(
        image
    )

    analyze_postprocessing(
        image
    )


if __name__ == "__main__":
    main()