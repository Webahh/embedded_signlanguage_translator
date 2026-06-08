import numpy as np

from ai_edge_litert.interpreter import Interpreter

from src.model_pipeline.results.model_results import (
    HandLandmarkResult,
)


def print_model_details(
    name: str,
    interpreter: Interpreter,
) -> None:
    """
    Prints input and output tensor information.
    """

    print(f"\n{name}")
    print("=" * len(name))

    print("Inputs:")

    for tensor in interpreter.get_input_details():
        print(
            f"  Name:          {tensor['name']}\n"
            f"  Shape:         {tensor['shape']}\n"
            f"  Datentyp:      {tensor['dtype']}\n"
            f"  Quantisierung: {tensor['quantization']}\n"
            f"  Index:         {tensor['index']}\n"
        )

    print("Outputs:")

    for tensor in interpreter.get_output_details():
        print(
            f"  Name:          {tensor['name']}\n"
            f"  Shape:         {tensor['shape']}\n"
            f"  Datentyp:      {tensor['dtype']}\n"
            f"  Quantisierung: {tensor['quantization']}\n"
            f"  Index:         {tensor['index']}\n"
        )


def print_landmark_result(
    result: HandLandmarkResult,
) -> None:
    """
    Prints the values returned by the hand-landmark model.
    """

    print("\nHand-landmark result")
    print("====================")

    print(
        f"Presence:   {result.presence_score}"
    )

    print(
        f"Handedness: {result.handedness_score}"
    )

    print("\nImage landmarks:")
    print(result.image_landmarks)

    print("\nWorld landmarks:")
    print(result.world_landmarks)


def print_array_summary(
    name: str,
    values: np.ndarray,
) -> None:
    """
    Prints shape and value statistics for an array.
    """

    values = np.asarray(values)

    print(f"\n{name}")
    print("=" * len(name))

    print("Shape:", values.shape)
    print("Dtype:", values.dtype)
    print("Min:", float(np.min(values)))
    print("Max:", float(np.max(values)))
    print("Mean:", float(np.mean(values)))