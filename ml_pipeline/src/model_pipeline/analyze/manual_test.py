import cv2 as cv

from src.model_pipeline.analyze.model_debug import (
    print_landmark_result,
)
from src.model_pipeline.core.config import (
    HAND_PRESENCE_THRESHOLD,
    HAND_TEST_IMAGE_2_PATH,
)
from src.model_pipeline.core.ml_pipeline import (
    MLModelPipeline,
)
from src.model_pipeline.preprocessing.landmark_preprocessing import (
    normalize_image_landmarks,
)


def main() -> None:
    pipeline = MLModelPipeline()

    image = cv.imread(
        HAND_TEST_IMAGE_2_PATH
    )

    if image is None:
        raise FileNotFoundError(
            f"Testbild wurde nicht gefunden: "
            f"{HAND_TEST_IMAGE_2_PATH}"
        )

    landmark_result = pipeline.detect_landmarks(
        image
    )

    print_landmark_result(
        landmark_result
    )

    if (
        landmark_result.presence_score
        < HAND_PRESENCE_THRESHOLD
    ):
        print(
            "\nKeine Hand sicher erkannt."
        )
        return

    normalized_landmarks = normalize_image_landmarks(
        landmark_result.image_landmarks
    )

    # Temporary manual assignment.
    # The test image is currently treated as a right hand.
    gesture_result = (
        pipeline.classify_from_image_landmarks(
            left_image_landmarks=None,
            right_image_landmarks=normalized_landmarks,
        )
    )

    print("\nGesture Classification")
    print("======================")

    print(
        "Class index:",
        gesture_result.class_index,
    )

    print(
        "Label:",
        gesture_result.label,
    )

    print(
        "Confidence:",
        gesture_result.confidence,
    )

    print("Probabilities:")
    print(
        gesture_result.probabilities
    )


if __name__ == "__main__":
    main()