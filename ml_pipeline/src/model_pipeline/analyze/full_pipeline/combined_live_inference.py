import cv2 as cv

from src.model_generation.core.display import ConfidenceDisplay
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
from src.model_pipeline.models.sign_language_detector import SignLanguageDetector
from src.model_pipeline.postprocessing.palm_visualization import draw_detection
from src.model_pipeline.analyze.full_pipeline.dashboard import Dashboard, ModeState


def run_live_inference() -> None:
    tracker = MultiHandTracker(PALM_MODEL_PATH, HAND_LANDMARK_MODEL_PATH)
    classifier = SignLanguageDetector(SIGNLANGUAGE_MODEL_PATH)
    display = ConfidenceDisplay()

    dash = Dashboard()
    cfg = dash.config
    cfg.palm_count = "0"
    cfg.status_text = "DETECTING"

    camera = cv.VideoCapture(CAMERA_INDEX)
    camera.set(cv.CAP_PROP_FRAME_WIDTH, CAMERA_FRAME_WIDTH)
    camera.set(cv.CAP_PROP_FRAME_HEIGHT, CAMERA_FRAME_HEIGHT)

    if not camera.isOpened():
        raise RuntimeError("Opening Camera failed!")

    window_name = "Combined Hand Tracking"

    cv.namedWindow(window_name)
    dash.setup(window_name)

    try:
        while not cfg.quit_requested:
            success, frame = camera.read()
            if not success:
                break

            detections, scale, pad_left, pad_top = tracker.step(
                frame,
                score_threshold=cfg.score_threshold,
                iou_threshold=cfg.iou_threshold,
            )

            cfg.palm_count = str(len(detections))
            cfg.status_text = f"TRACKING ({tracker.active_count})" if tracker.is_tracking else "DETECTING"

            if cfg.show_palm:
                for detection, handedness in zip(detections, tracker.handedness_scores):
                    hand_label = "R" if handedness > cfg.handedness_threshold else "L"
                    draw_detection(frame, detection, scale, pad_left, pad_top, label=hand_label)

            if cfg.mode >= ModeState.HAND and cfg.show_landmarks:
                for lm in tracker.last_landmarks:
                    points = [(int(lm[i, 0]), int(lm[i, 1])) for i in range(lm.shape[0])]
                    HandLandmarkDetector.draw_landmarks(frame, points)

            if cfg.mode >= ModeState.SIGN and cfg.show_sign:
                tables = []
                for track in tracker.active_tracks:
                    hand_label = "R" if track.handedness > cfg.handedness_threshold else "L"
                    confidences = classifier.predict(
                        [(track.landmarks, track.handedness)],
                        frame.shape[1], frame.shape[0],
                    )
                    tables.append((hand_label, confidences))

                # Position tables horizontal in the top-right corner
                gap = 8
                next_x = None
                for hand_label, confidences in reversed(tables):
                    pw, _ = display.estimate_table_size(confidences, threshold=cfg.sign_threshold, with_title=True)
                    fw = frame.shape[1]
                    if next_x is None:
                        x = fw - pw - 15
                    else:
                        x = next_x - pw - gap
                    nx, _ = display.draw_confidence_table(
                        frame, confidences,
                        x_offset=x, y_offset=15, title=hand_label,
                        threshold=cfg.sign_threshold,
                    )
                    next_x = x

            dash.draw_info(frame)
            panel = dash.render(frame.shape[0])

            dash._x_offset = frame.shape[1]
            combined = cv.hconcat([frame, panel])

            cv.imshow(window_name, combined)

            key = cv.waitKey(1) & 0xFF
            if key == ord("q"):
                cfg.quit_requested = True
            if key == ord("m"):
                cfg.mode = (cfg.mode + 1) % 3
                print(f"Mode: {['Palm', 'Hand', 'Sign'][cfg.mode]}")

    finally:
        camera.release()
        cv.destroyAllWindows()


if __name__ == "__main__":
    run_live_inference()
