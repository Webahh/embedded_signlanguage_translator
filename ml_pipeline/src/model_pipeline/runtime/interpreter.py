import os

from ai_edge_litert.interpreter import Interpreter


def load_interpreter(
    model_path: str,
) -> Interpreter:
    """
    Loads a LiteRT model and allocates all tensors.
    """

    if not os.path.isfile(model_path):
        raise FileNotFoundError(
            f"LiteRT-Modell wurde nicht gefunden: "
            f"{model_path}"
        )

    interpreter = Interpreter(
        model_path=model_path
    )

    interpreter.allocate_tensors()

    return interpreter