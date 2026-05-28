import os
import sys
import pickle
import numpy as np

from dataclasses import dataclass

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.abspath(os.path.join(SCRIPT_DIR, ".."))
sys.path.insert(0, PROJECT_ROOT)

from src.model.tflite_model import TFLiteModel


@dataclass(frozen=True)
class _TrainingData:
    label_count: int
    labels: dict
    labels_inv: dict
    training_labels: np.ndarray
    training_inputs: np.ndarray


# training_data.pkl was created by running model.py as __main__,
# so pickle references __main__.TrainingData
import __main__
__main__.TrainingData = _TrainingData

DATA_PATH = os.path.join(PROJECT_ROOT, "resources/training_data.pkl")


def main():
    with open(DATA_PATH, "rb") as f:
        data = pickle.load(f)

    training_inputs = data.training_inputs
    training_labels = data.training_labels
    labels_inv = data.labels_inv

    print(f"Loaded {len(training_inputs)} samples, {len(labels_inv)} classes")

    model = TFLiteModel.load()
    print(f"TFLite model: {model.label_count} classes\n")

    correct = 0
    total = 1000
    indices = np.random.choice(len(training_inputs), total, replace=False)

    POS_MAX = 32767

    for i, idx in enumerate(indices):
        # training_inputs are pre-normalized (/POS_MAX); infer() normalizes
        # raw values, so multiply back to raw int16 range
        buffer = (training_inputs[idx] * POS_MAX).astype(np.int16).tolist()
        confidences = model.infer(buffer)

        pred_label = max(confidences, key=confidences.get)
        true_label = labels_inv[int(training_labels[idx])]
        conf = confidences[pred_label]

        if pred_label == true_label:
            correct += 1

        if i < 5 or (i < 20 and pred_label != true_label):
            marker = "✓" if pred_label == true_label else "✗"
            print(f"[{i+1:3d}] {marker} pred={pred_label} "
                  f"true={true_label}  conf={conf:.4f}")

    print(f"\nTested {total} samples — Accuracy: {correct}/{total} ({correct/total*100:.2f}%)")


if __name__ == "__main__":
    main()
