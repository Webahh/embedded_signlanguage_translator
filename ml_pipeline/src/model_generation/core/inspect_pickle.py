import os
import sys
import time
import pickle

import cv2 as cv
from dataclasses import replace
from src.model_generation.core.visualizer import visualize, draw_debug_frame
from src.model_generation.gesture_generation.gesture import Gesture


# ---------------------------------------------------------------------------
#  Configuration
# ---------------------------------------------------------------------------

LABEL = sys.argv[1] if len(sys.argv) > 1 else "W"
MAX_FILES = int(sys.argv[2]) if len(sys.argv) > 2 else 10
ORIG_INDEX = int(sys.argv[3]) if len(sys.argv) > 3 else -1
EXPORT = True

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.abspath(os.path.join(SCRIPT_DIR, "../../.."))
GESTURE_DIR = os.path.join(PROJECT_ROOT, "resources/gestures")

GREEN = (0, 255, 0)
ORANGE = (0, 165, 255)


# ---------------------------------------------------------------------------
#  Gesture file discovery & selection
# ---------------------------------------------------------------------------

all_files = [
    f for f in os.listdir(GESTURE_DIR)
    if f.endswith(".pkl") and f.startswith(LABEL + "_")
]

orig_files = sorted(f for f in all_files if "_orig_" in f)
aug_files  = sorted(f for f in all_files if "_orig_" not in f)

if ORIG_INDEX >= 0:
    if ORIG_INDEX >= len(orig_files):
        print(f"Original #{ORIG_INDEX} existiert nicht. Verfügbare Originale:")
        for i, f in enumerate(orig_files):
            print(f"  {i}: {f}")
        sys.exit(1)

    selected = orig_files[ORIG_INDEX]
    parent_uid = selected.split("_")[-1].split(".")[0]
    matching_aug = [f for f in aug_files if f"_{parent_uid}_" in f]
    gesture_files = [selected] + matching_aug
    print(
        f"Isoliere Original #{ORIG_INDEX}: {selected} "
        f"({len(matching_aug)} zugehörige Augmentierungen)"
    )
else:
    gesture_files = [orig_files[0]] + aug_files if orig_files else []
    if MAX_FILES >= 0:
        gesture_files = gesture_files[:MAX_FILES]


# ---------------------------------------------------------------------------
#  Load & colorize gestures
# ---------------------------------------------------------------------------

gestures = []
for fname in gesture_files:
    path = os.path.join(GESTURE_DIR, fname)
    with open(path, "rb") as f:
        gesture: Gesture = pickle.load(f)

    color = GREEN if "_orig_" in fname else ORANGE

    colored_frames = []
    for frame in gesture.frames:
        colored_frames.append([replace(hand, color=color) for hand in frame])
    gesture = replace(gesture, frames=colored_frames)

    gestures.append(gesture)

min_len = min(len(g.frames) for g in gestures)
fps = gestures[0].fps if gestures else 30.0

print(f"{len(gestures)} Gesten geladen mit min. {min_len} synchronen Frames.")


# ---------------------------------------------------------------------------
#  Export setup
# ---------------------------------------------------------------------------

if EXPORT:
    export_dir = os.path.join(PROJECT_ROOT, "model", f"export_{LABEL}")
    os.makedirs(export_dir, exist_ok=True)
    print(f"Exportiere Frames nach {export_dir}")


# ---------------------------------------------------------------------------
#  Visualization loop
# ---------------------------------------------------------------------------

visualizer = visualize(info=False)

for frame_idx in range(min_len):
    all_hands = []
    for g in gestures:
        all_hands.extend(g.frames[frame_idx])

    if EXPORT:
        img = None
        for hand in all_hands:
            img = draw_debug_frame(hand, img, info=False, joints=True, bones=True)
        cv.imwrite(os.path.join(export_dir, f"frame_{frame_idx:04d}.png"), img)

    visualizer.send_pose(all_hands)
    time.sleep(1.0 / fps)

visualizer.terminate()
