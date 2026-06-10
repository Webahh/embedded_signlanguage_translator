import numpy as np
from dataclasses import dataclass


@dataclass(frozen=True)
class PalmDetection:
    index: int
    score: float
    box: np.ndarray
    keypoints: np.ndarray
