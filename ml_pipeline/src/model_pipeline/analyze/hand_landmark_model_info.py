from src.model_pipeline.core.config import (
    HAND_LANDMARK_MODEL_PATH,
)
from src.model_pipeline.runtime.interpreter import (
    load_model,
)


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


if __name__ == "__main__":
    main()