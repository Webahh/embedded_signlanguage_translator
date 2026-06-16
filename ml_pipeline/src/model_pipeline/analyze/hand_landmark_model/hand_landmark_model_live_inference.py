import cv2 as cv
import numpy as np

from src.model_pipeline.core.camera import ThreadedVideoCapture
from src.model_pipeline.core.config import (
    CAMERA_FRAME_HEIGHT,
    CAMERA_FRAME_WIDTH,
    CAMERA_INDEX,
    CENTER_ROI_DEFAULT_SCALE,
    HANDEDNESS_THRESHOLD,
    HAND_LANDMARK_HEIGHT,
    HAND_LANDMARK_MODEL_PATH,
    HAND_LANDMARK_WIDTH,
    NUM_LANDMARKS,
    PRESENCE_THRESHOLD,
)
from src.model_pipeline.runtime.interpreter import (
    load_model,
)

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


def prepare_input(
        image: np.ndarray,
        input_details: dict,
) -> np.ndarray:
    resized = cv.resize(
        image,
        (
            HAND_LANDMARK_WIDTH,
            HAND_LANDMARK_HEIGHT,
        ),
    )

    rgb = cv.cvtColor(
        resized,
        cv.COLOR_BGR2RGB,
    )

    tensor = np.expand_dims(
        rgb.astype(np.uint8),
        axis=0,
    )

    return tensor


def create_center_roi(
        frame: np.ndarray,
        scale: float = CENTER_ROI_DEFAULT_SCALE,
) -> tuple[np.ndarray, tuple[int, int, int, int]]:
    frame_height, frame_width = frame.shape[:2]

    roi_size = int(
        min(frame_width, frame_height) * scale
    )

    roi_x = (
                    frame_width - roi_size
            ) // 2

    roi_y = (
                    frame_height - roi_size
            ) // 2

    roi = frame[
          roi_y:roi_y + roi_size,
          roi_x:roi_x + roi_size,
          ]

    return roi, (
        roi_x,
        roi_y,
        roi_size,
        roi_size,
    )


def roi_landmarks_to_frame_points(
        landmarks: np.ndarray,
        roi_x: int,
        roi_y: int,
        roi_width: int,
        roi_height: int,
) -> list[tuple[int, int]]:
    points = []

    for landmark in landmarks:
        x = int(
            roi_x
            + landmark[0] / HAND_LANDMARK_WIDTH
            * roi_width
        )

        y = int(
            roi_y
            + landmark[1] / HAND_LANDMARK_HEIGHT
            * roi_height
        )

        points.append((x, y))

    return points


def draw_landmarks(
        frame: np.ndarray,
        points: list[tuple[int, int]],
) -> None:
    for start_index, end_index in HAND_CONNECTIONS:
        cv.line(
            frame,
            points[start_index],
            points[end_index],
            (0, 255, 0),
            2,
        )

    for index, point in enumerate(points):
        cv.circle(
            frame,
            point,
            4,
            (0, 0, 255),
            -1,
        )

        cv.putText(
            frame,
            str(index),
            (
                point[0] + 5,
                point[1] - 5,
            ),
            cv.FONT_HERSHEY_SIMPLEX,
            0.4,
            (255, 255, 255),
            1,
        )


def determine_hand(
        handedness_score: float,
) -> str:
    if handedness_score >= HANDEDNESS_THRESHOLD:
        return "Right"

    return "Left"


def run_live_inference() -> None:
    interpreter = load_model(
        HAND_LANDMARK_MODEL_PATH
    )

    input_details = (
        interpreter.get_input_details()[0]
    )

    output_details = {
        output["name"]: output
        for output in interpreter.get_output_details()
    }

    camera = ThreadedVideoCapture(
        CAMERA_INDEX
    )

    camera.set(
        cv.CAP_PROP_FRAME_WIDTH,
        CAMERA_FRAME_WIDTH,
    )

    camera.set(
        cv.CAP_PROP_FRAME_HEIGHT,
        CAMERA_FRAME_HEIGHT,
    )

    if not camera.isOpened():
        raise RuntimeError(
            "Webcam konnte nicht geöffnet werden."
        )

    camera.start()

    try:
        while True:
            success, frame = camera.read()

            if not success or frame is None:
                continue

            roi, (
                roi_x,
                roi_y,
                roi_width,
                roi_height,
            ) = create_center_roi(
                frame
            )

            input_tensor = prepare_input(
                roi,
                input_details,
            )

            interpreter.set_tensor(
                input_details["index"],
                input_tensor,
            )

            interpreter.invoke()

            presence_score = float(
                interpreter.get_tensor(
                    output_details[
                        "Identity_1:0"
                    ]["index"]
                )[0, 0]
            )

            handedness_score = float(
                interpreter.get_tensor(
                    output_details[
                        "Identity_2:0"
                    ]["index"]
                )[0, 0]
            )

            image_landmarks = interpreter.get_tensor(
                output_details[
                    "Identity:0"
                ]["index"]
            ).reshape(NUM_LANDMARKS, 3)

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

            detected_hand = "Unknown"

            if presence_score >= PRESENCE_THRESHOLD:
                detected_hand = determine_hand(
                    handedness_score
                )

                points = (
                    roi_landmarks_to_frame_points(
                        landmarks=image_landmarks,
                        roi_x=roi_x,
                        roi_y=roi_y,
                        roi_width=roi_width,
                        roi_height=roi_height,
                    )
                )

                draw_landmarks(
                    frame,
                    points,
                )

            cv.putText(
                frame,
                f"Presence: {presence_score:.4f}",
                (20, 40),
                cv.FONT_HERSHEY_SIMPLEX,
                0.8,
                (255, 255, 255),
                2,
            )

            cv.putText(
                frame,
                f"Handedness: {handedness_score:.4f}",
                (20, 80),
                cv.FONT_HERSHEY_SIMPLEX,
                0.8,
                (255, 255, 255),
                2,
            )

            cv.putText(
                frame,
                f"Hand: {detected_hand}",
                (20, 120),
                cv.FONT_HERSHEY_SIMPLEX,
                0.8,
                (255, 255, 255),
                2,
            )

            cv.imshow(
                "Hand Landmark Live Inference",
                frame,
            )

            key = cv.waitKey(1) & 0xFF

            if key == ord("q"):
                break

    finally:
        camera.release()
        cv.destroyAllWindows()


if __name__ == "__main__":
    run_live_inference()
