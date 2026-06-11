import cv2 as cv
import numpy as np

from src.model_pipeline.core.config import (
    CAMERA_INDEX,
    PALM_MODEL_PATH,
    HAND_LANDMARK_MODEL_PATH,
)
from src.model_pipeline.models.palm_detector import PalmDetector
from src.model_pipeline.models.hand_landmark_detector import HandLandmarkDetector
from src.model_pipeline.postprocessing.palm_visualization import draw_detection


def run_live_inference() -> None:
    palm_detector = PalmDetector(PALM_MODEL_PATH)
    hand_landmark = HandLandmarkDetector(HAND_LANDMARK_MODEL_PATH)

    camera = cv.VideoCapture(CAMERA_INDEX)
    camera.set(cv.CAP_PROP_FRAME_WIDTH, 1280)
    camera.set(cv.CAP_PROP_FRAME_HEIGHT, 720)

    if not camera.isOpened():
        raise RuntimeError("Opening Camera failed!")

    try:
        while True:
            success, frame = camera.read()
            if not success:
                break

            detections, scale, pad_left, pad_top = palm_detector.detect(
                frame, hand_landmark_detector=hand_landmark,
            )

            prefix = "ROI" if palm_detector.is_tracking else "PD"
            for detection in detections:
                draw_detection(frame, detection, scale, pad_left, pad_top)

            if palm_detector.last_landmarks is not None:
                lm = palm_detector.last_landmarks
                points = [(int(lm[i, 0]), int(lm[i, 1])) for i in range(lm.shape[0])]
                HandLandmarkDetector.draw_landmarks(frame, points)

            status = "TRACKING" if palm_detector.is_tracking else "DETECTING"
            cv.putText(
                frame,
                f"Mode: {status}  Palms: {len(detections)}",
                (20, 35),
                cv.FONT_HERSHEY_SIMPLEX,
                0.8,
                (0, 255, 255) if palm_detector.is_tracking else (255, 255, 255),
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