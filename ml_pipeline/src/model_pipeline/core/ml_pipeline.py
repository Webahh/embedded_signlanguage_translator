from typing import Optional

import numpy as np

from src.model_pipeline.core.config import (
    CLASS_MAPPING_PATH,
    GESTURE_MODEL_PATH,
    HAND_LANDMARK_MODEL_PATH,
    PALM_MODEL_PATH,
)
from src.model_pipeline.models.gesture_classifier import (
    GestureClassifier,
)
from src.model_pipeline.models.hand_landmark_model import (
    HandLandmarkModel,
)
from src.model_pipeline.models.palm_detection_model import (
    PalmDetectionModel,
)
from src.model_pipeline.preprocessing.gesture_features import (
    convert_landmarks_to_training_format,
    create_gesture_features,
)
from src.model_pipeline.results.model_results import (
    GestureResult,
    HandLandmarkResult,
)


class MLModelPipeline:
    """
    Connects palm detection, hand landmarks and gesture classification.
    """

    def __init__(
        self,
    ) -> None:
        self.palm_model = PalmDetectionModel(
            PALM_MODEL_PATH
        )

        self.hand_landmark_model = HandLandmarkModel(
            HAND_LANDMARK_MODEL_PATH
        )

        self.gesture_classifier = GestureClassifier(
            GESTURE_MODEL_PATH,
            CLASS_MAPPING_PATH,
        )

    def detect_palms(
        self,
        frame: np.ndarray,
    ) -> list[dict]:
        """
        Runs palm detection on a complete camera frame.
        """

        return self.palm_model.predict(
            frame
        )

    def detect_landmarks(
        self,
        hand_roi: np.ndarray,
    ) -> HandLandmarkResult:
        """
        Runs hand-landmark inference on one hand ROI.
        """

        return self.hand_landmark_model.predict(
            hand_roi
        )

    def classify_from_image_landmarks(
        self,
        left_image_landmarks: Optional[np.ndarray],
        right_image_landmarks: Optional[np.ndarray],
    ) -> GestureResult:
        """
        Converts normalized full-image landmarks into the original
        88-feature format and runs gesture classification.
        """

        left_relative = None
        left_wrist = None

        right_relative = None
        right_wrist = None

        if left_image_landmarks is not None:
            (
                left_relative,
                left_wrist,
            ) = convert_landmarks_to_training_format(
                left_image_landmarks
            )

        if right_image_landmarks is not None:
            (
                right_relative,
                right_wrist,
            ) = convert_landmarks_to_training_format(
                right_image_landmarks
            )

        features = create_gesture_features(
            left_landmarks=left_relative,
            left_wrist=left_wrist,
            right_landmarks=right_relative,
            right_wrist=right_wrist,
        )

        return self.gesture_classifier.predict(
            features
        )

    def classify_features(
        self,
        features: np.ndarray,
    ) -> GestureResult:
        """
        Runs the gesture classifier with an already prepared
        88-value feature vector.
        """

        return self.gesture_classifier.predict(
            features
        )