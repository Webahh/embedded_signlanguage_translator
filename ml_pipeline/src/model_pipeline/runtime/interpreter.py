import os

from ai_edge_litert.interpreter import Interpreter


def load_model(model_path: str) -> Interpreter:
    if not os.path.isfile(model_path):
        raise FileNotFoundError(
            f"Couldnt find the model: {model_path}"
        )

    interpreter = Interpreter(
        model_path=model_path
    )

    interpreter.allocate_tensors()

    return interpreter
