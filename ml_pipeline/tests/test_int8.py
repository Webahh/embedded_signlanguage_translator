import sys
import pickle
import numpy as np
import tensorflow as tf

from dataclasses import dataclass

MODEL_PATH = "model/model_int8.tflite"
DATA_PATH = "resources/training_data.pkl"


@dataclass(frozen=True)
class TrainingData:
    label_count: int
    labels: dict
    labels_inv: dict
    training_labels: np.ndarray
    training_inputs: np.ndarray


# Register so pickle can find it
sys.modules["src.model.model"] = type(sys)("src.model.model")
sys.modules["src.model.model"].TrainingData = TrainingData


def load_training_data(path):
    with open(path, "rb") as f:
        data = pickle.load(f)
    return data.labels_inv, data.training_inputs, data.training_labels


def main():
    labels_inv, training_inputs, training_labels = load_training_data(DATA_PATH)
    print(f"Loaded {len(training_inputs)} samples, {len(labels_inv)} classes")

    interpreter = tf.lite.Interpreter(model_path=MODEL_PATH)
    interpreter.allocate_tensors()

    input_details = interpreter.get_input_details()[0]
    output_details = interpreter.get_output_details()[0]

    in_scale, in_zp = input_details["quantization"]
    out_scale, out_zp = output_details["quantization"]

    correct = 0
    total = 1000
    test_indices = np.random.choice(len(training_inputs), total, replace=False)

    for i, idx in enumerate(test_indices):
        inp_float = training_inputs[idx].astype(np.float32)
        inp_quant = (inp_float / in_scale + in_zp).astype(np.uint8)
        inp_quant = inp_quant.reshape(1, 88, 1)

        interpreter.set_tensor(input_details["index"], inp_quant)
        interpreter.invoke()
        out_quant = interpreter.get_tensor(output_details["index"])[0]
        out_float = (out_quant.astype(np.float32) - out_zp) * out_scale

        pred_idx = int(np.argmax(out_float))
        true_idx = int(training_labels[idx])
        if pred_idx == true_idx:
            correct += 1

        if i < 5 or (i < 20 and pred_idx != true_idx):
            pred_label = labels_inv.get(pred_idx, "?")
            true_label = labels_inv.get(true_idx, "?")
            conf = out_float[pred_idx]
            marker = "✓" if pred_idx == true_idx else "✗"
            print(f"[{i+1:3d}] {marker} pred={pred_label}({pred_idx:2d}) "
                  f"true={true_label}({true_idx:2d})  conf={conf:.4f}")

    accuracy = correct / total * 100
    print(f"\nTested {total} samples — Accuracy: {correct}/{total} ({accuracy:.2f}%)")

    print("\nClass-wise confidence on a single sample:")
    sample_idx = test_indices[0]
    inp_float = training_inputs[sample_idx].astype(np.float32)
    inp_quant = (inp_float / in_scale + in_zp).astype(np.uint8).reshape(1, 88, 1)
    interpreter.set_tensor(input_details["index"], inp_quant)
    interpreter.invoke()
    out_quant = interpreter.get_tensor(output_details["index"])[0]
    out_float = (out_quant.astype(np.float32) - out_zp) * out_scale
    true_label = labels_inv[int(training_labels[sample_idx])]
    print(f"True label: {true_label}")
    for c in range(len(labels_inv)):
        label = labels_inv.get(c, "?")
        bar = "█" * int(out_float[c] * 100)
        print(f"  {c:2d} {label}: {out_float[c]:.4f} {bar}")


if __name__ == "__main__":
    main()
