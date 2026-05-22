import os

import cv2
import sys
import uuid
import pickle
import multiprocessing

from src.gesture_generation.gesture import Gesture
from src.core.hand_pose_detector import HandPoseDetector, Hand, POS_MAX
from src.augmentation.augmentation_pipeline import AugmentationPipeline
from src.augmentation.pipeline_functions import pip_func_mirror, pip_func_random_translate, pip_func_random_zoom

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.abspath(os.path.join(SCRIPT_DIR, "../.."))

detector = HandPoseDetector()


def process_video(video_path: str, label: str) -> Gesture:
    cap = cv2.VideoCapture(video_path)
    fps = cap.get(cv2.CAP_PROP_FPS)
    frames = []

    print(f"Processing video {label}...")

    while True:
        ret, frame = cap.read()
        if not ret:
            break

        hands = detector.detect(frame)

        # Always have a left and right hand in each frame, if there are no hands detected, use empty hands...
        left = Hand.empty(left=True)
        right = Hand.empty(left=False)

        for hand in hands:
            if hand.left_hand:
                left = hand
            else:
                right = hand

        frames.append([left, right])

    cap.release()

    print(f"Finished processing video {label}")

    return [Gesture(label=label, frames=frames, fps=fps).upscale_fps()]


def save_gesture(savedir: str, gesture: Gesture, augtype: str = "orig"):
    uid = uuid.uuid4().hex[:4]
    filename = f"{gesture.label}_{augtype}_{uid}.pkl"
    path = os.path.join(savedir, filename)

    with open(path, "wb") as f:
        pickle.dump(gesture, f)

    print(f"Gespeichert: {path}")


def delete_old_gestures(output_dir):
    os.makedirs(output_dir, exist_ok=True)

    for file in os.listdir(output_dir):
        if file.endswith(".pkl"):
            os.remove(os.path.join(output_dir, file))

    print(f"Deleted old gestures in {output_dir}.")


def generate_gestures(
        video_dir=os.path.join(PROJECT_ROOT, "resources/videos_prototype"),
        output_dir=os.path.join(PROJECT_ROOT, "resources/gestures")
):
    delete_old_gestures(output_dir)

    if not os.path.isdir(video_dir):
        print(
            f"No video-gestures found in '{video_dir}'. No new gestures will be generated."
        )
        return

    import re

    training_data = []

    # Accept single letters A-Z and the special labels SCH, Ä, Ö, Ü
    label_pattern = r"(SCH|[A-ZÄÖÜ])"

    for file in os.listdir(video_dir):

        if file.startswith("alph_fw_") and file.endswith(".mp4"):
            match = re.match(f"alph_fw_{label_pattern}", file, re.IGNORECASE)

            if match:
                label = match.group(1).upper()
                training_data.append((os.path.join(video_dir, file), label))
                print(f"{file} → Label: {label}")
            else:
                print(f"Warnung: Konnte kein Label aus Datei '{file}' extrahieren.")

        elif file.startswith("alph_og_") and file.endswith(".mp4"):
            match = re.match(rf"alph_og_{label_pattern}", file, re.IGNORECASE)

            if match:
                label = match.group(1).upper()
                training_data.append((os.path.join(video_dir, file), label))
                print(f"{file} → Label: {label}")
            else:
                print(f"Warnung: Konnte kein Label aus Datei '{file}' extrahieren.")
    base_gestures = []

    if "-s" in sys.argv or "--single-thread" in sys.argv:
        for video, label in training_data:
            base_gestures.append(process_video(video, label))
    else:
        # Multiprocessing for videos
        cpu_count = multiprocessing.cpu_count()
        num_processes = max(1, int(cpu_count * 0.8))
        with multiprocessing.get_context("spawn").Pool(processes=num_processes) as pool:
            base_gestures = pool.starmap(process_video, training_data)

    print("Video processing finished, running augmentation pipeline")

    base_gestures = [g for parts in base_gestures for g in parts if len(g.frames)]

    pipeline = AugmentationPipeline()
    pipeline.add("mirr", pip_func_mirror)
    pipeline.add("rtrans", pip_func_random_translate, count=2, max_offset=(POS_MAX // 3))
    pipeline.add("rzoom", pip_func_random_zoom, count=5, min_factor=0.5, max_factor=1.5)

    for gesture in base_gestures:
        save_gesture(output_dir, gesture, augtype="orig")

    def augment_gesture(gesture):
        print(f"Augmenting {gesture.label}...")
        for aug_gesture, augtype in pipeline.augment(gesture):
            save_gesture(output_dir, aug_gesture, augtype=augtype)

    for gesture in base_gestures:
        augment_gesture(gesture)

    print(
        f"{len(base_gestures)} Augmentation done and saved."
    )


if __name__ == "__main__":
    generate_gestures()
