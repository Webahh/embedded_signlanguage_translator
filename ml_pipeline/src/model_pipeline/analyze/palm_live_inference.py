import cv2 as cv

from src.model_pipeline.core.config import (
    CAMERA_INDEX,
    PALM_MODEL_PATH,
)
from src.model_pipeline.models.palm_detector import (
    PalmDetector,
)
from src.model_pipeline.postprocessing.palm_visualization import (
    draw_detection,
)


def run_live_inference() -> None:
    detector = PalmDetector(
        PALM_MODEL_PATH
    )

    camera = cv.VideoCapture(
        CAMERA_INDEX
    )

    camera.set(
        cv.CAP_PROP_FRAME_WIDTH,
        1280,
    )

    camera.set(
        cv.CAP_PROP_FRAME_HEIGHT,
        720,
    )

    if not camera.isOpened():
        raise RuntimeError(
            "Webcam konnte nicht geöffnet werden."
        )

    try:
        while True:
            success, frame = camera.read()

            if not success:
                break

            (
                detections,
                scale,
                pad_left,
                pad_top,
            ) = detector.detect(frame)

            for detection in detections:
                draw_detection(
                    frame,
                    detection,
                    scale,
                    pad_left,
                    pad_top,
                )

            cv.putText(
                frame,
                f"Palms: {len(detections)}",
                (20, 35),
                cv.FONT_HERSHEY_SIMPLEX,
                0.8,
                (255, 255, 255),
                2,
            )

            cv.imshow(
                "Palm Live Inference",
                frame,
            )

            if cv.waitKey(1) & 0xFF == ord("q"):
                break

    finally:
        camera.release()
        cv.destroyAllWindows()


if __name__ == "__main__":
    run_live_inference()