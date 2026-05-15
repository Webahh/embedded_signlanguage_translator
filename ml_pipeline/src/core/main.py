import time
import cv2 as cv

from hand_pose_detector import HandPoseDetector
from visualizer import visualize


def main():
    hand_pose = HandPoseDetector()
    visualizer = visualize(info=False)

    video = cv.VideoCapture(0)
    fps = video.get(cv.CAP_PROP_FPS)
    print(f"FPS: {fps}")

