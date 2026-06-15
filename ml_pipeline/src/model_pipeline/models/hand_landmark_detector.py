# Hand landmark detection model wrapper
#
# Given a cropped hand ROI (from palm detection), predicts 21 3D landmarks
# per the MediaPipe hand landmark convention. Also outputs a hand presence
# score and optional handedness (left/right) prediction

import cv2 as cv
import numpy as np

from src.model_pipeline.core.config import DEFAULT_HANDEDNESS, NUM_LANDMARKS
from src.model_pipeline.results.model_results import ROI
from src.model_pipeline.runtime.interpreter import load_model


# MediaPipe hand skeleton connections (21 landmarks, 20 edges + 1 wrist-to-pinky)
HAND_CONNECTIONS = [
    (0, 1), (1, 2), (2, 3), (3, 4),
    (0, 5), (5, 6), (6, 7), (7, 8),
    (5, 9), (9, 10), (10, 11), (11, 12),
    (9, 13), (13, 14), (14, 15), (15, 16),
    (13, 17), (17, 18), (18, 19), (19, 20),
    (0, 17),
]


class HandLandmarkDetector:
    """TFLite wrapper for MediaPipe-style hand landmark detection

    Operates on a cropped and rotated hand ROI (provided by palm detection
    or previous-frame tracking) and returns normalized landmark coordinates,
    presence confidence, and handedness score
    """

    def __init__(self, model_path: str) -> None:
        self._interpreter = load_model(model_path)
        self._input_details = self._interpreter.get_input_details()[0]
        self._output_details = {
            output["name"]: output
            for output in self._interpreter.get_output_details()
        }
        self._input_height = int(self._input_details["shape"][1])
        self._input_width = int(self._input_details["shape"][2])

    def detect(
        self,
        frame: np.ndarray,
        roi: ROI,
    ) -> tuple[np.ndarray, float, float]:
        """Run landmark detection on a hand ROI cropped from the frame

        Args:
            frame: Full BGR video frame
            roi: Region of interest defining the hand bounding box and rotation

        Returns:
            Tuple of (landmarks, presence_score, handedness)
            landmarks is an (NUM_LANDMARKS, 2) array of (x, y) normalized
            within the ROI
            Returns (None, 0.0, DEFAULT_HANDEDNESS) on failure
        """
        cropped = self._crop_roi(frame, roi)
        if cropped is None:
            return None, 0.0, DEFAULT_HANDEDNESS

        input_tensor = self._prepare_input(cropped)
        self._interpreter.set_tensor(self._input_details["index"], input_tensor)
        self._interpreter.invoke()

        presence_score = float(
            self._interpreter.get_tensor(
                self._output_details["Identity_1:0"]["index"]
            )[0, 0]
        )

        try:
            handedness = float(
                self._interpreter.get_tensor(
                    self._output_details["Identity_2:0"]["index"]
                )[0, 0]
            )
        except KeyError:
            # Older model variants may not have a handedness output head
            handedness = DEFAULT_HANDEDNESS

        raw_landmarks = self._interpreter.get_tensor(
            self._output_details["Identity:0"]["index"]
        ).reshape(NUM_LANDMARKS, 3)

        # Normalize landmark coordinates from model input space to [0, 1]
        landmarks = np.empty((NUM_LANDMARKS, 2), dtype=np.float32)
        for i in range(NUM_LANDMARKS):
            landmarks[i, 0] = raw_landmarks[i, 0] / self._input_width
            landmarks[i, 1] = raw_landmarks[i, 1] / self._input_width

        return landmarks, presence_score, handedness

    def _crop_roi(
        self,
        frame: np.ndarray,
        roi: ROI,
    ) -> np.ndarray:
        """Extract and rectify the hand ROI via affine warp

        Computes the affine transform that maps the rotated ROI rectangle
        onto the model's square input, adding a 10% margin to avoid edge
        truncation
        """
        frame_height, frame_width = frame.shape[:2]

        cos_r = abs(np.cos(roi.rotation))
        sin_r = abs(np.sin(roi.rotation))
        margin_w = int((roi.w * cos_r + roi.h * sin_r) * 1.1)
        margin_h = int((roi.w * sin_r + roi.h * cos_r) * 1.1)

        src_points = np.float32([
            [-roi.w / 2, -roi.h / 2],
            [roi.w / 2, -roi.h / 2],
            [roi.w / 2, roi.h / 2],
        ]).reshape(-1, 1, 2)

        cos_r = np.cos(roi.rotation)
        sin_r = np.sin(roi.rotation)
        rot_mat = np.float32([[cos_r, -sin_r], [sin_r, cos_r]])
        src_points = np.dot(src_points.reshape(-1, 2), rot_mat.T)
        src_points[:, 0] += roi.cx
        src_points[:, 1] += roi.cy

        dst_points = np.float32([
            [0, 0],
            [self._input_width, 0],
            [self._input_width, self._input_height],
        ])

        transform = cv.getAffineTransform(src_points, dst_points)
        cropped = cv.warpAffine(
            frame, transform,
            (self._input_width, self._input_height),
            flags=cv.INTER_LINEAR,
            borderMode=cv.BORDER_CONSTANT,
            borderValue=(0, 0, 0),
        )

        return cropped

    def _prepare_input(self, image: np.ndarray) -> np.ndarray:
        """Convert BGR crop to RGB uint8 batched tensor"""
        rgb = cv.cvtColor(image, cv.COLOR_BGR2RGB)
        return np.expand_dims(rgb.astype(np.uint8), axis=0)

    @staticmethod
    def draw_landmarks(
        frame: np.ndarray,
        points: list[tuple[int, int]],
    ) -> None:
        """Draw hand skeleton connections and landmark indices on the frame

        Args:
            frame: BGR image to draw on (modified in place)
            points: List of 21 (x, y) pixel coordinates
        """
        for start_index, end_index in HAND_CONNECTIONS:
            cv.line(
                frame,
                points[start_index],
                points[end_index],
                (0, 255, 0),
                2,
            )

        for index, point in enumerate(points):
            cv.circle(frame, point, 4, (0, 0, 255), -1)
            cv.putText(
                frame,
                str(index),
                (point[0] + 5, point[1] - 5),
                cv.FONT_HERSHEY_SIMPLEX,
                0.4,
                (255, 255, 255),
                1,
            )
