import numpy as np

from src.model_pipeline.core.config import (
    LANDMARK_COUNT,
)
from src.model_pipeline.preprocessing.landmark_preprocessing import (
    prepare_hand_landmark_input,
)
from src.model_pipeline.results.model_results import (
    HandLandmarkResult,
)
from src.model_pipeline.runtime.interpreter import (
    load_interpreter,
)
from src.model_pipeline.runtime.quantization import (
    dequantize_tensor,
)


class HandLandmarkModel:
    """
    Wrapper around the LiteRT hand-landmark model.
    """

    IMAGE_LANDMARK_OUTPUT = "Identity:0"
    WORLD_LANDMARK_OUTPUT = "Identity_3:0"
    PRESENCE_OUTPUT = "Identity_2:0"
    HANDEDNESS_OUTPUT = "Identity_1:0"

    def __init__(
        self,
        model_path: str,
    ) -> None:
        self._interpreter = load_interpreter(
            model_path
        )

        input_details = (
            self._interpreter.get_input_details()
        )

        if len(input_details) != 1:
            raise RuntimeError(
                "Das Hand-Landmark-Modell muss "
                "genau einen Eingang besitzen."
            )

        self._input_details = input_details[0]

        self._output_details = {
            details["name"]: details
            for details
            in self._interpreter.get_output_details()
        }

        self._validate_outputs()

    def predict(
        self,
        hand_roi: np.ndarray,
    ) -> HandLandmarkResult:
        """
        Runs hand-landmark inference for one hand ROI.
        """

        input_tensor = prepare_hand_landmark_input(
            hand_roi,
            self._input_details,
        )

        self._interpreter.set_tensor(
            self._input_details["index"],
            input_tensor,
        )

        self._interpreter.invoke()

        outputs = self._read_outputs()

        image_landmarks = outputs[
            self.IMAGE_LANDMARK_OUTPUT
        ].reshape(
            LANDMARK_COUNT,
            3,
        )

        world_landmarks = outputs[
            self.WORLD_LANDMARK_OUTPUT
        ].reshape(
            LANDMARK_COUNT,
            3,
        )

        presence_score = float(
            outputs[
                self.PRESENCE_OUTPUT
            ][0, 0]
        )

        handedness_score = float(
            outputs[
                self.HANDEDNESS_OUTPUT
            ][0, 0]
        )

        return HandLandmarkResult(
            image_landmarks=image_landmarks,
            world_landmarks=world_landmarks,
            presence_score=presence_score,
            handedness_score=handedness_score,
        )

    def _read_outputs(
        self,
    ) -> dict[str, np.ndarray]:
        outputs = {}

        for name, details in self._output_details.items():
            raw_output = self._interpreter.get_tensor(
                details["index"]
            )

            outputs[name] = dequantize_tensor(
                raw_output,
                details,
            )

        return outputs

    def _validate_outputs(
        self,
    ) -> None:
        required_outputs = {
            self.IMAGE_LANDMARK_OUTPUT,
            self.WORLD_LANDMARK_OUTPUT,
            self.PRESENCE_OUTPUT,
            self.HANDEDNESS_OUTPUT,
        }

        available_outputs = set(
            self._output_details.keys()
        )

        missing_outputs = (
            required_outputs - available_outputs
        )

        if missing_outputs:
            raise RuntimeError(
                "Dem Hand-Landmark-Modell fehlen "
                f"erwartete Ausgänge: {missing_outputs}"
            )

    @property
    def interpreter(self):
        """
        Exposes the interpreter for analysis/debugging.
        """

        return self._interpreter