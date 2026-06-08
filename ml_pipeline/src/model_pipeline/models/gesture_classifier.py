import os
import pickle

import numpy as np

from src.model_pipeline.core.config import (
    POS_MAX,
    TOTAL_FEATURES,
)
from src.model_pipeline.results.model_results import (
    GestureResult,
)
from src.model_pipeline.runtime.interpreter import (
    load_interpreter,
)
from src.model_pipeline.runtime.quantization import (
    dequantize_tensor,
    quantize_tensor,
)


class LegacyModel:
    """
    Compatibility class for class.pkl files that stored the original
    model class as __main__.Model.

    Only the saved instance attributes, especially _labels_inv, are
    needed here.
    """

    pass


class ModelMappingUnpickler(
    pickle.Unpickler
):
    """
    Redirects the original __main__.Model reference from class.pkl
    to LegacyModel.
    """

    def find_class(
        self,
        module: str,
        name: str,
    ):
        if (
            module == "__main__"
            and name == "Model"
        ):
            return LegacyModel

        return super().find_class(
            module,
            name,
        )


def load_label_mapping(
    class_path: str,
) -> dict[int, str]:
    """
    Loads the label mapping from the original class.pkl file.
    """

    if not os.path.isfile(class_path):
        raise FileNotFoundError(
            f"Klassen-Zuordnung wurde nicht gefunden: "
            f"{class_path}"
        )

    with open(
        class_path,
        "rb",
    ) as file:
        saved_model = (
            ModelMappingUnpickler(file).load()
        )

    if not hasattr(
        saved_model,
        "_labels_inv",
    ):
        raise AttributeError(
            "Das geladene Objekt enthält kein "
            "'_labels_inv'-Attribut."
        )

    labels = saved_model._labels_inv

    if not isinstance(labels, dict):
        raise TypeError(
            "'_labels_inv' muss ein Dictionary sein."
        )

    return {
        int(index): str(label)
        for index, label in labels.items()
    }


class GestureClassifier:
    """
    Wrapper around the quantized gesture-classification model.
    """

    def __init__(
        self,
        model_path: str,
        class_mapping_path: str,
    ) -> None:
        self._interpreter = load_interpreter(
            model_path
        )

        input_details = (
            self._interpreter.get_input_details()
        )

        output_details = (
            self._interpreter.get_output_details()
        )

        if len(input_details) != 1:
            raise RuntimeError(
                "Das Gesture-Modell muss genau "
                "einen Eingang besitzen."
            )

        if len(output_details) != 1:
            raise RuntimeError(
                "Das Gesture-Modell muss genau "
                "einen Ausgang besitzen."
            )

        self._input_details = input_details[0]
        self._output_details = output_details[0]

        self._labels = load_label_mapping(
            class_mapping_path
        )

    def predict(
        self,
        features: np.ndarray,
    ) -> GestureResult:
        """
        Runs gesture classification for one 88-value feature vector.
        """

        features = np.asarray(
            features
        )

        expected_shape = (
            TOTAL_FEATURES,
        )

        if features.shape != expected_shape:
            raise ValueError(
                f"Erwartet wurde Feature-Shape "
                f"{expected_shape}, "
                f"erhalten: {features.shape}"
            )

        # Same normalization used during training.
        model_input = (
            features.astype(np.float32)
            / POS_MAX
        ).reshape(
            1,
            TOTAL_FEATURES,
            1,
        )

        quantized_input = quantize_tensor(
            model_input,
            self._input_details,
        )

        self._interpreter.set_tensor(
            self._input_details["index"],
            quantized_input,
        )

        self._interpreter.invoke()

        raw_output = self._interpreter.get_tensor(
            self._output_details["index"]
        )

        probabilities = dequantize_tensor(
            raw_output,
            self._output_details,
        )[0]

        class_index = int(
            np.argmax(probabilities)
        )

        confidence = float(
            probabilities[class_index]
        )

        label = self._labels.get(
            class_index,
            f"UNKNOWN_{class_index}",
        )

        return GestureResult(
            class_index=class_index,
            label=label,
            confidence=confidence,
            probabilities=probabilities,
        )

    @property
    def labels(
        self,
    ) -> dict[int, str]:
        return self._labels.copy()

    @property
    def interpreter(self):
        """
        Exposes the interpreter for analysis/debugging.
        """

        return self._interpreter