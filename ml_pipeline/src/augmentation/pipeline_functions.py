from src.core.hand_pose_detector import Hand, POS_MAX
from dataclasses import replace
import numpy as np
from src.gesture_generation.gesture import Gesture


def apply_on_gesture(function, gesture: Gesture) -> Gesture:
    """
    Applies a function on every hand in every frame of the gesture.
    Named Function should return a hand
    """
    new_frames = []
    for frame in gesture.frames:
        hands = []
        for hand in frame:
            if hand.is_empty():
                hands.append(Hand.empty(function(hand).left_hand))
            else:
                hands.append(function(hand))

        new_frames.append(hands)

    return replace(gesture, frames=new_frames)


def pip_func_translate(gesture: Gesture, offset: [int, int]) -> [Gesture]:
    """
    Takes a gesture and produces a copy of it with the given offset applied
    """
    def offset_hand(hand: Hand) -> Hand:
        return replace(hand, wrist_pos=np.array(hand.wrist_pos + offset))

    return [apply_on_gesture(offset_hand, gesture)]


def pip_func_random_translate(gesture: Gesture, max_offset: int = 10_000, count=10) -> [Gesture]:
    """
    Takes a gesture and produces a copy of it with a random offset applied
    """
    offsets = np.random.randint(-max_offset, max_offset + 1, size=(count, 2))
    return [pip_func_translate(gesture, off)[0] for off in offsets]


def pip_func_mirror(gesture) -> [Gesture]:
    """
    Takes a gesture and mirrors it, returns the mirrored version.
    """
    def mirror_hand(hand: Hand) -> Hand:
        mirrored_landmarks = {
            k: np.array([-v[0], v[1], v[2]]) for k, v in hand.landmarks.items()
        }

        off = int(POS_MAX / 2)
        mirrored_wrist = (hand.wrist_pos - off) * [-1, 1] + off

        return replace(
            hand,
            landmarks=mirrored_landmarks,
            left_hand=(not hand.left_hand),
            wrist_pos=mirrored_wrist,
        )

    return [apply_on_gesture(mirror_hand, gesture)]


def pip_func_scale(gesture: Gesture, factor: float = 1.1) -> [Gesture]:
    """
    Takes a gesture and returns a scaled copy with the given factor applied
    """

    def scale_hand(hand: Hand) -> Hand:
        scaled_landmarks = {
            k: np.array(v * factor, dtype=np.int16) for k, v in hand.landmarks.items()
        }
        scaled_wrist = np.array(hand.wrist_pos * factor, dtype=np.int16)
        return replace(hand, landmarks=scaled_landmarks, wrist_pos=scaled_wrist)

    return [apply_on_gesture(scale_hand, gesture)]


def pip_func_jitter(gesture: Gesture, noise_level: float = 5.0) -> [Gesture]:
    """
    simulates noises with random shifts (e.g. camera or handshaking)
    """
    def jitter_hand(hand: Hand) -> Hand:
        new_landmarks = {
            name: pos + np.random.normal(0, noise_level, size=3)
            for name, pos in hand.landmarks.items()
        }
        new_wrist = hand.wrist_pos + np.random.normal(0, noise_level, size=2)
        return replace(hand, landmarks=new_landmarks, wrist_pos=new_wrist)

    return [apply_on_gesture(jitter_hand, gesture)]


def pip_func_zoom(gesture: Gesture, scale_factor: float = 1.2) -> [Gesture]:
    """
    Moves the hand to the camera (z axis)
    """
    def zoom_hand(hand: Hand) -> Hand:
        new_landmarks = {
            name: pos * scale_factor for name, pos in hand.landmarks.items()
        }

        return replace(hand, landmarks=new_landmarks)

    return [apply_on_gesture(zoom_hand, gesture)]


def pip_func_random_zoom(gesture: Gesture, min_factor=0.8, max_factor=1.2, count=10) -> [Gesture]:
    """
    Random zoom, for a random position on the z axis
    """
    factors = np.random.uniform(min_factor, max_factor, size=count)
    return [pip_func_zoom(gesture, scale_factor=fac)[0] for fac in factors]


def pip_func_drop_frames(gesture: Gesture, drop_rate: float = 0.1) -> [Gesture]:
    """
    Removes analog to the drop_rate some frames
    """
    total = len(gesture.frames)
    keep_mask = np.random.rand(total) > drop_rate
    new_frames = [frame for i, frame in enumerate(gesture.frames) if keep_mask[i]]

    if not new_frames:
        new_frames = [gesture.frames[len(gesture.frames) // 2]]

    return [replace(gesture, frames=new_frames)]