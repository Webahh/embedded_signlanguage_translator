import copy
import threading
import cv2 as cv
import numpy as np

from queue import Queue
from dataclasses import replace
from hand_pose_detector import LANDMARK_NAMES, POS_MAX, Hand

DEFAULT_WIDTH = 1280
DEFAULT_HEIGHT = 740


def draw_skeleton(img, pose: Hand, joints=True, bones=True, info=True):
    """
    Draws this hands skeleton onto the provided image.

    """

    # Make left hand joints blue and right hand joints red
    col_joint = (0, 0, 255) if pose.left_hand else (255, 0, 0)

    # Make the bones white
    col_bone = (255, 255, 255)

    def joint(p, pos):
        if joints:
            cv.circle(img, tuple(p), 4, col_joint, -1)
        if info:
            cv.addText(
                img,
                f"({pos[0]}, {pos[1]}, {pos[2]})",
                (p[0] + 5, p[1]),
                color=col_joint,
                pointSize=8,
                nameFont="NotoSans",
            )

    def bone(p1, p2):
        cv.line(img, tuple(p1), tuple(p2), col_bone, 2)

    def bone_mesh(*args):
        for index in range(len(args) - 1):
            bone(pose.landmark_pos[args[index]], pose.landmark_pos[args[index + 1]])

    if bones:
        bone_mesh(0, 1, 2, 3, 4)
        bone_mesh(5, 6, 7, 8)
        bone_mesh(9, 10, 11, 12)
        bone_mesh(13, 14, 15, 16)
        bone_mesh(0, 17, 18, 19, 20)
        bone_mesh(1, 5, 9, 13, 17)

    for i, p in enumerate(pose.landmark_pos):
        joint(p, pose.landmarks[LANDMARK_NAMES[i]])


def draw_debug_frame(
        pose: Hand, img=None, w=DEFAULT_WIDTH, h=DEFAULT_HEIGHT, **kwargs
) -> []:
    """
    Draw the hand data onto an image.
    Creates a black background image if no image was provided.
    Hand projection is adjusted for the given image dimensions
    returns the modified image
    """

    # Create a black image if no image was provided
    if img is None:
        img = np.zeros((h, w, 3), np.uint8)

    w, h = img.shape[1], img.shape[0]

    origin = (pose.wrist_pos) / POS_MAX * [w, h]

    landmark_pos = []
    for key in LANDMARK_NAMES:
        landmark_pos.append(origin + (pose.landmarks[key][[0, 1]] / POS_MAX * [w, h]))

    pose = replace(pose, landmark_pos=np.array(landmark_pos, dtype=int))
    draw_skeleton(img, pose, **kwargs)

    return img


class RunningVisalizer(Queue):
    """Provides some convinience shorthands for the visualizer"""

    def __init__(self, *args, **kwargs):
        self.__done = False
        super().__init__(*args, **kwargs)

    def send_pose(self, pose: [Hand]) -> bool:
        """Sends a hand pose to the visualizer to display, returns false if the visualizer terminated"""
        return self.send_img_pose(None, pose)

    def send_img_pose(self, img, pose: [Hand]) -> bool:
        """Sends an image and a hand pose to the visualizer to display, returns false if the visualizer terminated"""
        if self.__done:
            return False

        try:
            self.put((img, pose))
            return True
        except Exception as _:
            return False

    def terminate(self):
        """Terminates the visualizer"""
        try:
            self.__done = True
            self.shutdown(immediate=True)
        except Exception:
            # print("Visualizer terminated twice, ignoring second termination")
            pass


def visualize(joints=True, bones=True, info=True) -> RunningVisalizer:
    """
    Starts the data visualiszer as Background task.
    Paramters toggle debug data.

    """

    queue = RunningVisalizer()

    def visualize_task():
        while True:
            # Exit on Q
            if key := cv.waitKey(10):
                if key == ord("q"):
                    print("Q was pressed. Exiting!")
                    break

            # Get image from queue
            img, pose = None, None
            try:
                img, pose = queue.get(block=True)
            except:  # noqa: E722, Yes, i want to terminate on every exception...
                print("Visalizer queue is closed, shutting down visualizer")
                break

            hi_img = copy.deepcopy(img)

            for hand in pose:
                hi_img = draw_debug_frame(
                    hand, hi_img, joints=joints, bones=bones, info=info
                )

            queue.task_done()

            # Present the image
            cv.imshow(
                "Handpose Visualizer",
                hi_img
                if hi_img is not None
                else np.zeros((DEFAULT_HEIGHT, DEFAULT_WIDTH, 3), np.uint8),
            )

        cv.destroyWindow("Handpose Visualizer")
        queue.terminate()

    threading.Thread(target=visualize_task, daemon=True).start()
    return queue
