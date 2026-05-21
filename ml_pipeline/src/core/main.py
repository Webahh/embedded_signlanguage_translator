import time

import cv2 as cv

from core import display
from core.display import ConfidenceDisplay
from src.model.model import Model
from src.core.visualizer import visualize
from src.model.model_input import ModelInput
from src.core.hand_pose_detector import HandPoseDetector
from src.model.model_input_buffer import ModelInputBuffer

# # Use the Augmentation pipeline to build generate modified gestures, based on
# # a input gesture. This can be used to generate more training data based on existing data.

# from augment import AugmentationPipeline, mirror, translate
# from gesture import Gesture

# def augment_pipeline_example(gesture: Gesture) -> [Gesture]:
#     # Build the pipeline
#     pipeline = AugmentationPipeline()

#     # Create a mirrored clone
#     pipeline.add(mirror)

#     # Create copies moved a little bit up
#     pipeline.add(translate, offset=[0, -10000])

#     # Run the pipeline
#     gestures = pipeline.augment(gesture)

#     return gestures


# def hand_skeleton_example():
#     # Camera Setup
#     camera = cv.VideoCapture(0)
#     camera.set(cv.CAP_PROP_FRAME_WIDTH, 960 * 1.5)
#     camera.set(cv.CAP_PROP_FRAME_HEIGHT, 640 * 1.5)

#     # Get HandPoseDetector and Visualizer instances
#     hand_pose = HandPoseDetector()
#     visualizer = visualize(info=False)  # info=False => Dont display joint positions

#     while True:
#         # Read image from camera
#         ok, img = camera.read()
#         if not ok:
#             print("Failed to fetch frame from camera. Exiting!")
#             break

#         # Flip image and detect hand poses in it
#         img = cv.flip(img, 1)
#         hands = hand_pose.detect(img)

#         # NOTE: This is just for fun:
#         # Using the augmentation pipeline to generate 3 ghost hands if only one hand is visible.
#         # This has no practical application, but showcases the augmentation pipeline
#         if len(hands) == 1:
#             # Convert the hands to a gesture, since the AugmentationPipeline works on gestures
#             gesture = Gesture.from_hands(hands)

#             # Run the pipeline
#             gestures = augment_pipeline_example(gesture)

#             # Convert the gestures back to hands, so they can be visualized
#             hands = [g.to_hands()[0] for g in gestures]

#         # Use the visualizer to display the webcam image, aswell as the hand poses in it.
#         # If the Visualizer terminates, terminate this loop as well.
#         if not visualizer.send_img_pose(img, hands):
#             break

#         # Lock on 15 FPS
#         time.sleep(1.0 / 15.0)

#     # Cleanup
#     camera.release()
#     visualizer.terminate()

def main():
    # Get HandPoseDetector and Visualizer instances
    hand_pose = HandPoseDetector()
    visualizer = visualize(info=False)  # info=False => Dont display joint positions

    display = ConfidenceDisplay()
    latest_result = {
        "confidences": {}
    }

    # Setup the video
    video = cv.VideoCapture(0)
    fps = video.get(cv.CAP_PROP_FPS)
    print(f"FPS: {fps}")

    # Setup the model and its input buffer
    model = Model.load()

    def on_inference(confidences):
        latest_result["confidences"] = confidences

    buffer = ModelInputBuffer(model, on_inference)

    while True:
        # Read image from video
        ok, img = video.read()
        if not ok:
            print("Failed to fetch frame from camera. Exiting!")
            break

        # Flip image and detect hand poses in it
        # img = cv.flip(img, 1)
        hands = hand_pose.detect(img)

        model_input = ModelInput.from_hands(hands)
        buffer.push(model_input)

        img = display.draw_confidence_table(
            img,
            latest_result["confidences"],
        )

        # Use the visualizer to display the video, aswell as the hand poses in it.
        # If the Visualizer terminates, terminate this loop as well.
        if not visualizer.send_img_pose(img, hands):
            break

        time.sleep(1.0 / fps)

    # Cleanup
    buffer.destroy()
    video.release()
    visualizer.terminate()


if __name__ == "__main__":
    main()
