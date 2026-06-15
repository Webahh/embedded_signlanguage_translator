# TFLite model loading utility.
#
# Thin wrapper around ai_edge_litert.Interpreter that validates the
# file path exists before allocating tensors.

import os

from ai_edge_litert.interpreter import Interpreter


def load_model(model_path: str) -> Interpreter:
    """Load a TFLite model from disk and allocate its tensors

    Args:
        model_path: Absolute or relative path to the .tflite file

    Returns:
        Initialized Interpreter with allocated tensors

    Raises:
        FileNotFoundError: If the model file does not exist
    """
    if not os.path.isfile(model_path):
        raise FileNotFoundError(
            f"Couldnt find the model: {model_path}"
        )

    interpreter = Interpreter(model_path=model_path)
    interpreter.allocate_tensors()

    return interpreter
