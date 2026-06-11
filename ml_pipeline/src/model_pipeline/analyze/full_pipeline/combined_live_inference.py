import cv2 as cv

from src.model_pipeline.core.config import (
    CAMERA_FRAME_HEIGHT,
    CAMERA_FRAME_WIDTH,
    CAMERA_INDEX,
    HAND_LANDMARK_MODEL_PATH,
    PALM_MODEL_PATH,
    SIGNLANGUAGE_MODEL_PATH,
)
from src.model_pipeline.tracking.tracker import MultiHandTracker
from src.model_pipeline.models.hand_landmark_detector import HandLandmarkDetector
from src.model_pipeline.models.sign_language_classifier import SignLanguageClassifier
from src.model_pipeline.postprocessing.palm_visualization import draw_detection


def run_live_inference() -> None:
    tracker = MultiHandTracker(PALM_MODEL_PATH, HAND_LANDMARK_MODEL_PATH)
    classifier = SignLanguageClassifier(SIGNLANGUAGE_MODEL_PATH)

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

            for track in tracker._tracks:
                if track.active and track.raw_landmarks is not None:
                    label, conf = classifier.predict(track.raw_landmarks)
                    track.sign_label = label
                    track.sign_confidence = conf

            for detection, label in zip(detections, tracker.last_handedness):
                draw_detection(frame, detection, scale, pad_left, pad_top, label=label)

            for lm in tracker.last_landmarks:
                points = [(int(lm[i, 0]), int(lm[i, 1])) for i in range(lm.shape[0])]
                HandLandmarkDetector.draw_landmarks(frame, points)

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

            y_offset = 35
            for sign_label, sign_conf in zip(tracker.last_sign_labels, tracker.last_sign_confidences):
                if sign_label:
                    text = f"Sign: {sign_label} ({sign_conf:.2f})"
                    cv.putText(
                        frame, text, (20, y_offset),
                        cv.FONT_HERSHEY_SIMPLEX, 0.8, (0, 255, 0), 2,
                    )
                    y_offset += 35

            cv.imshow("Combined Hand Tracking", frame)

            key = cv.waitKey(1) & 0xFF
            if key == ord("q"):
                break

    finally:
        camera.release()
        cv.destroyAllWindows()


if __name__ == "__main__":
    run_live_inference()
