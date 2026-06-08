import numpy as np

from src.model_pipeline.runtime.interpreter import (
    load_interpreter,
)


class PalmDetectionModel:
    """
    Wrapper around the palm-detection model.

    The model is already loaded, but its preprocessing and
    postprocessing still need to be implemented.
    """

    def __init__(
        self,
        model_path: str,
    ) -> None:
        self._interpreter = load_interpreter(
            model_path
        )

        self._input_details = (
            self._interpreter.get_input_details()
        )

        self._output_details = (
            self._interpreter.get_output_details()
        )

    def predict(
        self,
        frame: np.ndarray,
    ) -> list[dict]:
        """
        Detects palms in a complete camera frame.

        Still required:
            - input preprocessing
            - anchor generation
            - bounding-box decoding
            - confidence decoding
            - non-maximum suppression
            - hand ROI calculation
        """

        if frame is None or frame.size == 0:
            raise ValueError(
                "Das Eingabebild ist leer."
            )

        raise NotImplementedError(
            "Palm-Detection-Postprocessing "
            "ist noch nicht implementiert."
        )

    @property
    def interpreter(self):
        """
        Exposes the interpreter for analysis/debugging.
        """

        return self._interpreter