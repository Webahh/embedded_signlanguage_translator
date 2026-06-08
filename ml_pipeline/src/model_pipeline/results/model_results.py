from dataclasses import dataclass

import numpy as np


@dataclass(frozen=True)
class HandLandmarkResult:
    """
    Result returned by the hand-landmark model.
    """

    image_landmarks: np.ndarray
    world_landmarks: np.ndarray
    presence_score: float
    handedness_score: float


@dataclass(frozen=True)
class GestureResult:
    """
    Result returned by the gesture classifier.
    """

    class_index: int
    label: str
    confidence: float
    probabilities: np.ndarray