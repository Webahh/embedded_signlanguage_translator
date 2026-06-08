from typing import Optional

import numpy as np

from src.model_pipeline.core.config import (
    FEATURES_PER_HAND,
    LANDMARK_COUNT,
    POS_MAX,
    TOTAL_FEATURES,
)


def convert_landmarks_to_training_format(
    normalized_image_landmarks: np.ndarray,
) -> tuple[np.ndarray, np.ndarray]:
    """
    Converts normalized image landmarks into the representation used
    during gesture-model training.

    Returns:
        relative_landmarks:
            Shape [21, 3], relative to the wrist.

        absolute_wrist:
            Shape [2], absolute normalized wrist position scaled to
            the int16 range.
    """

    landmarks = np.asarray(
        normalized_image_landmarks,
        dtype=np.float32,
    )

    expected_shape = (
        LANDMARK_COUNT,
        3,
    )

    if landmarks.shape != expected_shape:
        raise ValueError(
            f"Erwartet wurde Landmark-Shape "
            f"{expected_shape}, "
            f"erhalten: {landmarks.shape}"
        )

    scaled = np.clip(
        landmarks * POS_MAX,
        -POS_MAX,
        POS_MAX,
    ).astype(np.int16)

    absolute_wrist = scaled[0, :2].copy()

    # int32 prevents overflow during subtraction.
    relative_landmarks = (
        scaled.astype(np.int32)
        - scaled[0].astype(np.int32)
    )

    relative_landmarks = np.clip(
        relative_landmarks,
        -POS_MAX,
        POS_MAX,
    ).astype(np.int16)

    return (
        relative_landmarks,
        absolute_wrist,
    )


def create_empty_hand_features() -> np.ndarray:
    """
    Creates the original Hand.empty() representation.

    Layout:
        21 relative x/y landmark pairs = 42 values
        absolute empty wrist           = 2 values

        total                          = 44 values
    """

    features = np.zeros(
        FEATURES_PER_HAND,
        dtype=np.int16,
    )

    features[-2:] = [
        -100,
        -100,
    ]

    return features


def create_hand_features(
    relative_landmarks: np.ndarray,
    absolute_wrist: np.ndarray,
) -> np.ndarray:
    """
    Creates the 44 features of one hand.

    Exact order:
        landmark 0 x/y
        landmark 1 x/y
        ...
        landmark 20 x/y
        absolute wrist x/y
    """

    relative_landmarks = np.asarray(
        relative_landmarks
    )

    absolute_wrist = np.asarray(
        absolute_wrist
    )

    expected_landmark_shape = (
        LANDMARK_COUNT,
        3,
    )

    if relative_landmarks.shape != expected_landmark_shape:
        raise ValueError(
            f"Erwartet wurde Landmark-Shape "
            f"{expected_landmark_shape}, "
            f"erhalten: {relative_landmarks.shape}"
        )

    if absolute_wrist.shape != (2,):
        raise ValueError(
            f"Erwartet wurde Wrist-Shape (2,), "
            f"erhalten: {absolute_wrist.shape}"
        )

    # Corresponds to the original flattened implementation:
    #
    # [num for coord in hand for num in coord[:-1]]
    relative_xy = (
        relative_landmarks[:, :2]
        .reshape(-1)
    )

    features = np.concatenate(
        [
            relative_xy,
            absolute_wrist,
        ]
    )

    if features.shape != (FEATURES_PER_HAND,):
        raise RuntimeError(
            f"Eine Hand muss "
            f"{FEATURES_PER_HAND} Features erzeugen, "
            f"erhalten: {features.size}"
        )

    return features.astype(np.int16)


def create_gesture_features(
    left_landmarks: Optional[np.ndarray],
    left_wrist: Optional[np.ndarray],
    right_landmarks: Optional[np.ndarray],
    right_wrist: Optional[np.ndarray],
) -> np.ndarray:
    """
    Creates the complete gesture input.

    Exact order:
        left hand  = 44 values
        right hand = 44 values

        total      = 88 values
    """

    left_features = _create_optional_hand_features(
        left_landmarks,
        left_wrist,
    )

    right_features = _create_optional_hand_features(
        right_landmarks,
        right_wrist,
    )

    features = np.concatenate(
        [
            left_features,
            right_features,
        ]
    )

    if features.shape != (TOTAL_FEATURES,):
        raise RuntimeError(
            f"Gesture-Input muss "
            f"{TOTAL_FEATURES} Werte enthalten, "
            f"erhalten: {features.size}"
        )

    return features.astype(np.int16)


def _create_optional_hand_features(
    landmarks: Optional[np.ndarray],
    wrist: Optional[np.ndarray],
) -> np.ndarray:
    """
    Creates features for one detected or missing hand.
    """

    if landmarks is None:
        return create_empty_hand_features()

    if wrist is None:
        raise ValueError(
            "Für erkannte Landmarks fehlt "
            "die absolute Wrist-Position."
        )

    return create_hand_features(
        landmarks,
        wrist,
    )