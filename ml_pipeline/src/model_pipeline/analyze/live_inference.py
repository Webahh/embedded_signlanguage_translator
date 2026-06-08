import time
import cv2 as cv
import numpy as np

from src.model_pipeline.core.config import (
    HAND_PRESENCE_THRESHOLD,
)
from src.model_pipeline.core.ml_pipeline import (
    MLModelPipeline,
)
from src.model_pipeline.preprocessing.landmark_preprocessing import (
    normalize_image_landmarks,
    roi_landmarks_to_image_coordinates,
)

CAMERA_INDEX = 0

ROI_SCALE = 0.75

CONFIDENCE_THRESHOLD = 0.5

# Vorläufige Zuordnung, bis Handedness sicher ausgewertet wird.
TEST_HAND_IS_LEFT = False


# MediaPipe-Landmark-Verbindungen
HAND_CONNECTIONS = [
    (0, 1),
    (1, 2),
    (2, 3),
    (3, 4),

    (0, 5),
    (5, 6),
    (6, 7),
    (7, 8),

    (5, 9),
    (9, 10),
    (10, 11),
    (11, 12),

    (9, 13),
    (13, 14),
    (14, 15),
    (15, 16),

    (13, 17),
    (17, 18),
    (18, 19),
    (19, 20),

    (0, 17),
]


def create_center_roi(
    frame: np.ndarray,
    scale: float,
) -> tuple[np.ndarray, tuple[int, int, int, int]]:
    """
    Schneidet eine quadratische ROI aus der Bildmitte aus.

    Returns:
        roi:
            Ausgeschnittenes Bild.

        roi_rect:
            Tuple (x, y, width, height).
    """

    if not 0.0 < scale <= 1.0:
        raise ValueError(
            f"ROI_SCALE muss zwischen 0 und 1 liegen, "
            f"erhalten: {scale}"
        )

    frame_height, frame_width = frame.shape[:2]

    roi_size = int(
        min(frame_width, frame_height) * scale
    )

    roi_x = (frame_width - roi_size) // 2
    roi_y = (frame_height - roi_size) // 2

    roi = frame[
        roi_y:roi_y + roi_size,
        roi_x:roi_x + roi_size,
    ]

    return (
        roi,
        (
            roi_x,
            roi_y,
            roi_size,
            roi_size,
        ),
    )


def draw_roi(
    frame: np.ndarray,
    roi_rect: tuple[int, int, int, int],
) -> None:
    roi_x, roi_y, roi_width, roi_height = roi_rect

    cv.rectangle(
        frame,
        (roi_x, roi_y),
        (
            roi_x + roi_width,
            roi_y + roi_height,
        ),
        (255, 255, 255),
        2,
    )


def draw_landmarks(
    frame: np.ndarray,
    landmarks: np.ndarray,
) -> None:
    """
    Zeichnet normalisierte Full-Image-Landmarks.
    """

    frame_height, frame_width = frame.shape[:2]

    points = []

    for landmark in landmarks:
        x = int(landmark[0] * frame_width)
        y = int(landmark[1] * frame_height)

        points.append((x, y))

    for start_index, end_index in HAND_CONNECTIONS:
        cv.line(
            frame,
            points[start_index],
            points[end_index],
            (255, 255, 255),
            2,
        )

    for point in points:
        cv.circle(
            frame,
            point,
            4,
            (255, 255, 255),
            -1,
        )


def draw_text(
    frame: np.ndarray,
    label: str,
    confidence: float,
    presence: float,
    fps: float,
) -> None:
    cv.putText(
        frame,
        f"Gesture: {label}",
        (20, 35),
        cv.FONT_HERSHEY_SIMPLEX,
        0.9,
        (255, 255, 255),
        2,
    )

    cv.putText(
        frame,
        f"Confidence: {confidence:.3f}",
        (20, 70),
        cv.FONT_HERSHEY_SIMPLEX,
        0.7,
        (255, 255, 255),
        2,
    )

    cv.putText(
        frame,
        f"Presence: {presence:.3f}",
        (20, 100),
        cv.FONT_HERSHEY_SIMPLEX,
        0.7,
        (255, 255, 255),
        2,
    )

    cv.putText(
        frame,
        f"FPS: {fps:.1f}",
        (20, 130),
        cv.FONT_HERSHEY_SIMPLEX,
        0.7,
        (255, 255, 255),
        2,
    )


def run_live_inference() -> None:
    pipeline = MLModelPipeline()

    camera = cv.VideoCapture(
        CAMERA_INDEX
    )

    if not camera.isOpened():
        raise RuntimeError(
            f"Kamera {CAMERA_INDEX} konnte "
            f"nicht geöffnet werden."
        )

    previous_time = time.perf_counter()

    try:
        while True:
            success, frame = camera.read()

            if not success:
                print(
                    "Kamerabild konnte nicht gelesen werden."
                )
                break

            display_frame = frame.copy()

            roi, roi_rect = create_center_roi(
                frame,
                ROI_SCALE,
            )

            draw_roi(
                display_frame,
                roi_rect,
            )

            label = "No hand"
            confidence = 0.0
            presence = 0.0

            landmark_result = (
                pipeline.detect_landmarks(
                    roi
                )
            )

            presence = (
                landmark_result.presence_score
            )

            if (
                presence
                >= HAND_PRESENCE_THRESHOLD
            ):
                roi_landmarks = (
                    normalize_image_landmarks(
                        landmark_result.image_landmarks
                    )
                )

                (
                    roi_x,
                    roi_y,
                    roi_width,
                    roi_height,
                ) = roi_rect

                frame_height, frame_width = (
                    frame.shape[:2]
                )

                image_landmarks = (
                    roi_landmarks_to_image_coordinates(
                        roi_landmarks=roi_landmarks,
                        roi_x=roi_x,
                        roi_y=roi_y,
                        roi_width=roi_width,
                        roi_height=roi_height,
                        image_width=frame_width,
                        image_height=frame_height,
                    )
                )

                draw_landmarks(
                    display_frame,
                    image_landmarks,
                )

                if TEST_HAND_IS_LEFT:
                    gesture_result = (
                        pipeline
                        .classify_from_image_landmarks(
                            left_image_landmarks=(
                                image_landmarks
                            ),
                            right_image_landmarks=None,
                        )
                    )
                else:
                    gesture_result = (
                        pipeline
                        .classify_from_image_landmarks(
                            left_image_landmarks=None,
                            right_image_landmarks=(
                                image_landmarks
                            ),
                        )
                    )

                confidence = (
                    gesture_result.confidence
                )

                if (
                    confidence
                    >= CONFIDENCE_THRESHOLD
                ):
                    label = gesture_result.label
                else:
                    label = "Unknown"

            current_time = time.perf_counter()

            elapsed = (
                current_time - previous_time
            )

            previous_time = current_time

            fps = (
                1.0 / elapsed
                if elapsed > 0.0
                else 0.0
            )

            draw_text(
                display_frame,
                label=label,
                confidence=confidence,
                presence=presence,
                fps=fps,
            )

            cv.imshow(
                "Embedded Sign Language - Live Inference",
                display_frame,
            )

            key = cv.waitKey(1) & 0xFF

            if key == ord("q"):
                break

    finally:
        camera.release()
        cv.destroyAllWindows()


if __name__ == "__main__":
    run_live_inference()