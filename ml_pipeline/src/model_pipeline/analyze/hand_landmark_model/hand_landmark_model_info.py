import numpy as np
import cv2 as cv

from src.model_pipeline.core.config import HAND_LANDMARK_MODEL_PATH, TEST_IMAGE_PATH
from src.model_pipeline.runtime.interpreter import load_model


def print_tensor_details(title: str, tensors: list[dict], ) -> None:
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
    image: np.ndarray,
    input_details: dict,
) -> np.ndarray:
    if image is None or image.size == 0:
        raise ValueError(
            "Das Testbild ist leer."
        )

    input_shape = input_details["shape"]

    input_height = int(input_shape[1])
    input_width = int(input_shape[2])

    resized = cv.resize(
        image,
        (input_width, input_height),
    )

    rgb = cv.cvtColor(
        resized,
        cv.COLOR_BGR2RGB,
    )

    tensor = np.expand_dims(
        rgb.astype(np.uint8),
        axis=0,
    )

    return tensor


def run_model(
        interpreter,
        image: np.ndarray,
) -> dict[str, np.ndarray]:
    input_details = (
        interpreter.get_input_details()[0]
    )

    output_details = (
        interpreter.get_output_details()
    )

    input_tensor = prepare_input(
        image,
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


def main() -> None:
    interpreter = load_model(
        HAND_LANDMARK_MODEL_PATH
    )

    print_tensor_details(
        "Inputs",
        interpreter.get_input_details(),
    )

    print_tensor_details(
        "Outputs",
        interpreter.get_output_details(),
    )

    image = cv.imread(
        TEST_IMAGE_PATH
    )

    if image is None:
        raise FileNotFoundError(
            f"Testbild nicht gefunden: "
            f"{TEST_IMAGE_PATH}"
        )

    outputs = run_model(
        interpreter,
        image,
    )

    print_output_summary(
        outputs
    )

    empty_image = np.zeros(
        (224, 224, 3),
        dtype=np.uint8,
    )

    empty_outputs = run_model(
        interpreter,
        empty_image,
    )

    print_output_summary(
        empty_outputs
    )

if __name__ == "__main__":
    main()
