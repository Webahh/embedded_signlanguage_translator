import os
import pickle
import time
from src.gesture_generation.gesture import Gesture
from visualizer import visualize

label_filter = "L"
augment_filter = ""  # e.g. augment_filter = "mirr"
gesture_dir = "../../resources/gestures"

gesture_files = [
    os.path.join(gesture_dir, f)
    for f in os.listdir(gesture_dir)
    if f.endswith(".pkl") and f.startswith(label_filter + "_" + augment_filter)
]

gestures = []
for path in gesture_files:
    with open(path, "rb") as f:
        gesture: Gesture = pickle.load(f)
        gestures.append(gesture)

# shortest length for viz frame sync
min_len = min(len(g.frames) for g in gestures)
fps = gestures[0].fps if gestures else 30.0

print(f"{len(gestures)} Gesten geladen mit min. {min_len} synchronen Frames.")
visualizer = visualize(info=False)

for frame_idx in range(min_len):
    all_hands = []

    for g in gestures:
        all_hands.extend(g.frames[frame_idx])

    visualizer.send_pose(all_hands)
    time.sleep(1.0 / fps)

visualizer.terminate()
