import copy
import cv2 as cv
import numpy as np
import mediapipe as mp

from dataclasses import dataclass

LANDMARK_NAMES = [
    "WRIST",
    "THUMB_CMC",
    "THUMB_MCP",
    "THUMB_IP",
    "THUMB_TIP",
    "INDEX_FINGER_MCP",
    "INDEX_FINGER_PIP",
    "INDEX_FINGER_DIP",
    "INDEX_FINGER_TIP",
    "MIDDLE_FINGER_MCP",
    "MIDDLE_FINGER_PIP",
    "MIDDLE_FINGER_DIP",
    "MIDDLE_FINGER_TIP",
    "RING_FINGER_MCP",
    "RING_FINGER_PIP",
    "RING_FINGER_DIP",
    "RING_FINGER_TIP",
    "PINKY_FINGER_MCP",
    "PINKY_FINGER_PIP",
    "PINKY_FINGER_DIP",
    "PINKY_FINGER_TIP",
]
POS_MAX = 32767  # max value at i16


def create_hand_bounding_box(hand_landmarks, width, height):
    # creates a bounding box around the hand
    np_landmarks = np.empty((0, 2), int)

    for _, landmark in enumerate(hand_landmarks.landmark):
        # Get the x and y location of each landmark while clamping it to be inside the image dimensions
        x = min(int(landmark.x * width), width - 1)
        y = min(int(landmark.y * height), height - 1)
        np_landmarks = np.append(np_landmarks, [np.array((x, y))], axis=0)

    return cv.boundingRect(np_landmarks)


def find_wrist_pos(hand_landmarks):
    return np.array(
        [int(hand_landmarks.landmark[0].x * POS_MAX),
         int(hand_landmarks.landmark[0].y * POS_MAX)]
    )


def normalize_landmarks(hand_landmarks):
    """Converts the landmark data to i16 with each point being relative to the wrist"""

    # Convert all landmark data to i16
    int_hand_landmarks = np.empty((21, 3), np.int16)
    clamp_range = POS_MAX

    def clamp(coord):
        return max(-clamp_range, min(clamp_range, int(coord * clamp_range)))

    for index, landmark in enumerate(hand_landmarks.landmark):
        x = clamp(landmark.x)
        y = clamp(landmark.y)
        z = clamp(landmark.z)

        int_hand_landmarks[index] = np.array([x, y, z])

    wrist = copy.deepcopy(int_hand_landmarks[0])
    normalized = int_hand_landmarks - wrist

    landmarks = {}
    for i, p in enumerate(normalized):
        landmarks[LANDMARK_NAMES[i]] = p

    return landmarks


def find_landmark_pos(hand_landmarks, width, height):
    ret = np.empty((21, 2), int)

    for index, landmark in enumerate(hand_landmarks.landmark):
        x = min(int(landmark.x * width), width - 1)
        y = min(int(landmark.y * height), height - 1)

        ret[index] = np.array([x, y])

    return ret


@dataclass(frozen=True)
class Hand:
    # Is this the left or the right hand?
    left_hand: bool

    # Wrist position scaled to int16 range
    wrist_pos: [int, int]

    # Hand landmarks relative to the wrist
    # Reference: https://ai.google.dev/static/edge/mediapipe/images/solutions/hand-landmarks.png
    landmarks: {}

    # Area in px where the hand is located relative within the img (x, y, width, height)
    hand_area: (int, int, int, int)

    # landmark positions in px within the image
    landmark_pos: []

    def __post_init__(self):
        super().__setattr__("left_hand", bool(self.left_hand))

    def is_empty(self):
        return self.wrist_pos[0] == -100 and self.wrist_pos[1] == -100

    @staticmethod
    def empty(left: bool):
        return Hand(
            left_hand=left,
            wrist_pos=np.array([-100, -100]),
            landmarks={name: np.array([0, 0, 0]) for name in LANDMARK_NAMES},
            hand_area=(0, 0, 0, 0),
            landmark_pos=[],
        )


class HandPoseDetector:
    def __init__(self):
        hands = mp.solutions.hands

        self.__hands = hands.Hands(
            static_image_mode=True,
            max_num_hands=2,
            min_detection_confidence=0.7,
            min_tracking_confidence=0.5,
        )

    def detect(self, img, *args, **kwargs) -> [Hand]:
        img = cv.cvtColor(img, cv.COLOR_BGR2RGB)
        img.flags.writeable = False

        results = self.__hands.process(img, *args, **kwargs)
        width, height = img.shape[1], img.shape[0]

        detected_hands = []

        if results.multi_hand_landmarks is not None:
            for hand_landmarks, handedness in zip(results.multi_hand_landmarks, results.multi_handedness):
                hand = Hand(
                    left_hand=bool("Left" in str(handedness)),
                    wrist_pos=find_wrist_pos(hand_landmarks),
                    landmarks=normalize_landmarks(hand_landmarks),
                    hand_area=create_hand_bounding_box(hand_landmarks, width, height),
                    landmark_pos=find_landmark_pos(hand_landmarks, width, height),
                )

                detected_hands.append(hand)

        img.flags.writeable = True

        return detected_hands
