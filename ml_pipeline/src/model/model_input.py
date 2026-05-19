import numpy as np
import pickle
import os
import copy

from os import path
from src.core.hand_pose_detector import Hand, LANDMARK_NAMES
from src.gesture_generation.gesture import Gesture


def joints(hand: Hand) -> []:
    return [hand.landmarks[name] for name in LANDMARK_NAMES]


class ModelInput:
    def __init__(self, left: Hand, right: Hand):
        self._input_matrix = np.zeros(shape=(2, 22, 3), dtype=np.int16)

        left_wrist = [left.wrist_pos[0], left.wrist_pos[1], 0]
        right_wrist = [right.wrist_pos[0], right.wrist_pos[1], 0]
        left_joints = joints(left)
        right_joints = joints(right)

        left_joints.append(left_wrist)
        right_joints.append(right_wrist)

        self._input_matrix[0] = np.array(left_joints)
        self._input_matrix[1] = np.array(right_joints)

    def flattened(self):
        return [num for hand in self.mat for coord in hand for num in coord[:-1]]

    @property
    def mat(self):
        return self._input_matrix

    @staticmethod
    def from_hands(hands: [Hand]):
        """
        Takes an array with any number of hands in it and converts it into a Modelinput
        with a left and a right hand, adding empty Hands if neccessary.
        """

        left = Hand.empty(left=True)
        right = Hand.empty(left=False)

        for hand in hands:
            if hand.left_hand:
                left = hand
            else:
                right = hand

        return ModelInput(left=left, right=right)

    @staticmethod
    def from_gesture(gesture: Gesture):
        """
        Takes a gesture and turns it's frames into an array of ModelInputs.
        the label is also returned.

        * Returns: (label: str, [ModelInput])

        """

        inputs = [ModelInput.from_hands(hands) for hands in gesture.frames]
        return (gesture.label, inputs)

    def interpolate_to(self, other, stops: int) -> []:
        delta = other.mat - self.mat
        new_inputs = [self]

        for stop in range(stops):
            new_input = copy.deepcopy(self)
            new_input._input_matrix = self.mat + delta * (1 / stops * stop)
            new_inputs.append(new_input)

        return new_inputs


def load_training_data(data_dir: str) -> [(str, [ModelInput])]:
    data = []

    for fname in os.listdir(data_dir):
        p = path.join(data_dir, fname)
        try:
            with open(p, "rb") as f:
                data.append(ModelInput.from_gesture(pickle.load(f)))
        except Exception as e:
            print(f"Failed to load '{p}', due to {e}")

    return data


EMPTY_FRAME = ModelInput.from_hands(
    [Hand.empty(left=True), Hand.empty(left=False)]
).flattened()


def flatten_frames(seq):
    return [f.flattened() for f in seq]


def add_empty_frames(seq, until):
    delta = until - len(seq)
    return (
        seq
        + [
            EMPTY_FRAME,
        ]
        * delta
    )
