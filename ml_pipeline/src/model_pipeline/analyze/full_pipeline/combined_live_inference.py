import numpy as np
import cv2 as cv

from src.model_generation.core.display import ConfidenceDisplay
from src.model_pipeline.core.config import (
    CAMERA_FRAME_HEIGHT,
    CAMERA_FRAME_WIDTH,
    CAMERA_INDEX,
    HANDEDNESS_THRESHOLD,
    HAND_LANDMARK_MODEL_PATH,
    PALM_MODEL_PATH,
    SIGNLANGUAGE_MODEL_PATH,
)
from src.model_pipeline.tracking.tracker import MultiHandTracker
from src.model_pipeline.models.hand_landmark_detector import HandLandmarkDetector
from src.model_pipeline.models.sign_language_detector import SignLanguageDetector
from src.model_pipeline.postprocessing.palm_visualization import draw_detection


class Mode:
    PALM = 0
    PALM_LANDMARKS = 1
    PALM_LANDMARKS_SIGN = 2


MODE_NAMES = {
    Mode.PALM: "Palm",
    Mode.PALM_LANDMARKS: "Palm + Landmarks",
    Mode.PALM_LANDMARKS_SIGN: "Palm + Landmarks + Sign",
}


def run_live_inference() -> None:
    tracker = MultiHandTracker(PALM_MODEL_PATH, HAND_LANDMARK_MODEL_PATH)
    classifier = SignLanguageDetector(SIGNLANGUAGE_MODEL_PATH)
    display = ConfidenceDisplay()
    mode = Mode.PALM

    camera = cv.VideoCapture(CAMERA_INDEX)
    camera.set(cv.CAP_PROP_FRAME_WIDTH, CAMERA_FRAME_WIDTH)
    camera.set(cv.CAP_PROP_FRAME_HEIGHT, CAMERA_FRAME_HEIGHT)

    if not camera.isOpened():
        raise RuntimeError("Opening Camera failed!")

    try:
        while True:
            success, frame = camera.read()
            if not success:
                break

            detections, scale, pad_left, pad_top = tracker.step(frame)

            for detection, hand_label in zip(detections, tracker.last_handedness):
                draw_detection(frame, detection, scale, pad_left, pad_top, label=hand_label)

            if mode >= Mode.PALM_LANDMARKS:
                for lm in tracker.last_landmarks:
                    points = [(int(lm[i, 0]), int(lm[i, 1])) for i in range(lm.shape[0])]
                    HandLandmarkDetector.draw_landmarks(frame, points)

            if mode >= Mode.PALM_LANDMARKS_SIGN:
                for track in tracker._tracks:
                    if track.active and track.landmarks is not None:
                        hand = "R" if track.handedness > HANDEDNESS_THRESHOLD else "L"
                        confidences = classifier.predict(
                            [(track.landmarks, track.handedness)],
                            frame.shape[1], frame.shape[0],
                        )
                        frame = display.draw_confidence_table(
                            frame, confidences,
                            x_offset=10 if hand == "L" else None,
                            y_offset=10 if hand == "L" else 10,
                        )

            status = f"TRACKING ({tracker.active_count})" if tracker.is_tracking else "DETECTING"
            cv.putText(
                frame,
                f"Mode: {MODE_NAMES[mode]}  {status}  Palms: {len(detections)}",
                (20, 35),
                cv.FONT_HERSHEY_SIMPLEX,
                0.8,
                (0, 255, 255) if tracker.is_tracking else (255, 255, 255),
                2,
            )
            cv.putText(
                frame,
                "[M] cycle mode  [Q] quit",
                (20, frame.shape[0] - 15),
                cv.FONT_HERSHEY_SIMPLEX,
                0.5,
                (180, 180, 180),
                1,
            )

            cv.imshow("Combined Hand Tracking", frame)

            key = cv.waitKey(1) & 0xFF
            if key == ord("q"):
                break
            if key == ord("m"):
                next_mode = mode + 1
                if next_mode > Mode.PALM_LANDMARKS_SIGN:
                    next_mode = Mode.PALM
                mode = next_mode
                print(f"Mode: {MODE_NAMES[mode]}")

    finally:
        camera.release()
        cv.destroyAllWindows()


if __name__ == "__main__":
    run_live_inference()
