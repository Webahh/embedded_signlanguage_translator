import os
import pickle
import numpy as np

from src.model_pipeline.models.sign_language_detector import SignLanguageDetector
from src.model_pipeline.runtime.interpreter import load_model
from src.model_pipeline.core.config import SIGNLANGUAGE_MODEL_PATH


def _load_labels_inv(model_path: str) -> dict[int, str]:
    model_dir = os.path.dirname(model_path)
    class_path = os.path.join(model_dir, "class.pkl")

    class _ModelStub:
        def __init__(self, **kwargs):
            for k, v in kwargs.items():
                setattr(self, k, v)

    import __main__
    __main__.Model = _ModelStub

    with open(class_path, "rb") as f:
        obj = pickle.load(f)
    return obj._labels_inv


LABELS_INV = _load_labels_inv(SIGNLANGUAGE_MODEL_PATH)


def print_tensor_details(
        title: str,
        tensors: list[dict],
) -> None:
    print(f"\n{title}")
    print("=" * len(title))

    for tensor in tensors:
        print(
            f"Name:          {tensor['name']}\n"
            f"Shape:         {tensor['shape']}\n"
            f"Dtype:         {tensor['dtype']}\n"
            f"Quantisierung: {tensor['quantization']}\n"
            f"Index:         {tensor['index']}\n"
        )


def prepare_input(
        features: np.ndarray,
        input_details: dict,
) -> np.ndarray:
    input_size = int(input_details["shape"][1])

    if features.ndim != 1 or features.shape[0] != input_size:
        raise ValueError(
            f"Erwartete Input-Größe {input_size}, "
            f"erhalten {features.shape}."
        )

    tensor = np.expand_dims(
        features.astype(np.float32),
        axis=0,
    )

    if input_details["dtype"] == np.uint8:
        scale, zero_point = input_details["quantization"]
        tensor = (tensor / scale + zero_point).astype(np.uint8)

    return tensor.reshape(input_details["shape"])


def run_model(
        interpreter,
        features: np.ndarray,
) -> dict[str, np.ndarray]:
    input_details = (
        interpreter.get_input_details()[0]
    )

    output_details = (
        interpreter.get_output_details()
    )

    input_tensor = prepare_input(
        features,
        input_details,
    )

    interpreter.set_tensor(
        input_details["index"],
        input_tensor,
    )

    interpreter.invoke()

    outputs = {}

    for details in output_details:
        outputs[details["name"]] = (
            interpreter.get_tensor(
                details["index"]
            )
        )

    return outputs


def print_output_summary(
        outputs: dict[str, np.ndarray],
) -> None:
    print("\nOutput summary")
    print("==============")

    for name, values in outputs.items():
        values = np.asarray(values)

        print(
            f"{name}\n"
            f"  Shape: {values.shape}\n"
            f"  Min:   {float(np.min(values)):.8f}\n"
            f"  Max:   {float(np.max(values)):.8f}\n"
            f"  Mean:  {float(np.mean(values)):.8f}\n"
        )


def generate_dummy_input(
        input_size: int,
) -> np.ndarray:
    return np.zeros(
        input_size,
        dtype=np.float32,
    )


def analyze_with_input(
        label: str,
        features: np.ndarray,
) -> None:
    interpreter = load_model(
        SIGNLANGUAGE_MODEL_PATH
    )

    output_details = (
        interpreter.get_output_details()
    )

    outputs = run_model(
        interpreter,
        features,
    )

    print(f"\nRaw output analysis ({label})")
    print("=" * (24 + len(label)))

    print_output_summary(
        outputs
    )

    scores = outputs[output_details[0]["name"]][0]

    if scores.dtype == np.uint8:
        scale, zero_point = output_details[0]["quantization"]
        scores = (scores.astype(np.float32) - zero_point) * scale

    best_index = int(np.argmax(scores))

    print(
        f"Best class index: {best_index}\n"
        f"Best score:       {float(scores[best_index]):.6f}\n"
    )

    if best_index in LABELS_INV:
        print(
            f"Predicted sign:   {LABELS_INV[best_index]}"
        )

    print("\nTop-5 scores:")

    top5 = np.argsort(scores)[-5:][::-1]

    for idx in top5:
        class_name = (
            LABELS_INV[idx]
            if idx in LABELS_INV
            else "?"
        )
        print(
            f"  {idx:2d} ({class_name}): "
            f"{float(scores[idx]):.6f}"
        )


def analyze_postprocessing() -> None:
    detector = SignLanguageDetector(
        SIGNLANGUAGE_MODEL_PATH
    )

    img_w = 640
    img_h = 480

    # Dummy left hand: fingers extended upward
    left_landmarks = np.array([
        [320, 400], [300, 350], [290, 320], [285, 300], [280, 280],
        [340, 350], [350, 310], [355, 290], [360, 270],
        [360, 340], [370, 300], [375, 280], [380, 260],
        [380, 350], [390, 320], [395, 300], [400, 280],
        [340, 380], [350, 360], [355, 340], [360, 320],
    ], dtype=np.float32)

    # Dummy right hand: fist (all fingers curled)
    right_landmarks = np.array([
        [400, 380], [410, 390], [415, 395], [418, 398], [420, 400],
        [390, 390], [380, 400], [375, 405], [370, 410],
        [395, 395], [385, 405], [380, 410], [375, 415],
        [400, 390], [395, 400], [390, 405], [385, 410],
        [410, 385], [405, 395], [400, 400], [395, 405],
    ], dtype=np.float32)

    # Single hand (right only) — simulates tracking one hand
    confs = detector.predict(
        [(right_landmarks, 0.9)],
        img_w, img_h,
    )
    best = max(confs, key=confs.get) if confs else ""
    print(f"\nSingle hand (right):   {best} ({confs.get(best, 0):.4f})")

    # Single hand (left only)
    confs = detector.predict(
        [(left_landmarks, 0.3)],
        img_w, img_h,
    )
    best = max(confs, key=confs.get) if confs else ""
    print(f"Single hand (left):    {best} ({confs.get(best, 0):.4f})")

    # Two hands
    confs = detector.predict(
        [(left_landmarks, 0.3), (right_landmarks, 0.9)],
        img_w, img_h,
    )
    best = max(confs, key=confs.get) if confs else ""
    print(f"Two hands:             {best} ({confs.get(best, 0):.4f})")

    # No hands (empty prediction)
    confs = detector.predict([], img_w, img_h)
    best = max(confs, key=confs.get) if confs else ""
    print(f"No hands:              {best} ({confs.get(best, 0):.4f})")

    interpreter = load_model(
        SIGNLANGUAGE_MODEL_PATH
    )

    input_size = int(
        interpreter.get_input_details()[0]["shape"][1]
    )

    test_input = np.random.default_rng(42).uniform(
        0.0, 1.0,
        size=input_size,
    ).astype(np.float32)

    analyze_with_input(
        "random uniform",
        test_input,
    )


def main() -> None:
    interpreter = load_model(
        SIGNLANGUAGE_MODEL_PATH
    )

    input_size = int(
        interpreter.get_input_details()[0]["shape"][1]
    )

    print_tensor_details(
        "Inputs",
        interpreter.get_input_details(),
    )

    print_tensor_details(
        "Outputs",
        interpreter.get_output_details(),
    )

    print(
        f"\nModel input size: {input_size} floats"
    )

    dummy = generate_dummy_input(
        input_size
    )

    analyze_with_input(
        "zero input",
        dummy,
    )

    analyze_postprocessing()


if __name__ == "__main__":
    main()
