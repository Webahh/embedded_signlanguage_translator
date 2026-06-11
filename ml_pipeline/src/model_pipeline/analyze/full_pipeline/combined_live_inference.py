import numpy as np
import cv2 as cv

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


def draw_sign_panel(
        frame: np.ndarray,
        results: dict[str, tuple[str, float]],
) -> None:
    h, w = frame.shape[:2]
    x0 = w - 220
    y0 = 10
    line_h = 30

    for i, (hand, (label, conf)) in enumerate(results.items()):
        text = f"{hand}: {label} ({conf:.2f})" if label else f"{hand}: ---"
        cv.putText(
            frame,
            text,
            (x0, y0 + i * line_h + line_h - 5),
            cv.FONT_HERSHEY_SIMPLEX,
            1.0,
            (0, 255, 0) if label else (255, 255, 255),
            2,
        )


def run_live_inference() -> None:
    tracker = MultiHandTracker(PALM_MODEL_PATH, HAND_LANDMARK_MODEL_PATH)
    classifier = SignLanguageDetector(SIGNLANGUAGE_MODEL_PATH)

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

            sign_results = {"L": ("", 0.0), "R": ("", 0.0)}
            for track in tracker._tracks:
                if track.active and track.landmarks is not None:
                    label, conf = classifier.predict(
                        [(track.landmarks, track.handedness)],
                        frame.shape[1], frame.shape[0],
                    )
                    hand = "R" if track.handedness > HANDEDNESS_THRESHOLD else "L"
                    sign_results[hand] = (label, conf)

            for detection, hand in zip(detections, tracker.last_handedness):
                draw_detection(frame, detection, scale, pad_left, pad_top, label=hand)

            for lm in tracker.last_landmarks:
                points = [(int(lm[i, 0]), int(lm[i, 1])) for i in range(lm.shape[0])]
                HandLandmarkDetector.draw_landmarks(frame, points)

            draw_sign_panel(frame, sign_results)

            status = f"TRACKING ({tracker.active_count})" if tracker.is_tracking else "DETECTING"
            cv.putText(
                frame,
                f"Mode: {status}  Palms: {len(detections)}",
                (20, 35),
                cv.FONT_HERSHEY_SIMPLEX,
                0.8,
                (0, 255, 255) if tracker.is_tracking else (255, 255, 255),
                2,
            )

            cv.imshow("Combined Hand Tracking", frame)

            key = cv.waitKey(1) & 0xFF
            if key == ord("q"):
                break

    finally:
        camera.release()
        cv.destroyAllWindows()


if __name__ == "__main__":
    run_live_inference()
