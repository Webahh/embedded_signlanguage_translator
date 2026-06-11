import numpy as np

from src.model_pipeline.core.config import SIGN_CLASSES
from src.model_pipeline.runtime.interpreter import load_model


class SignLanguageClassifier:
    def __init__(self, model_path: str) -> None:
        self._interpreter = load_model(model_path)
        self._input_details = self._interpreter.get_input_details()[0]
        self._output_details = self._interpreter.get_output_details()[0]

        self._expected_input_size = int(self._input_details["shape"][1])
        self._input_dtype = self._input_details["dtype"]

    def predict(self, landmarks: np.ndarray) -> tuple[str, float]:
        if landmarks.shape != (21, 2):
            return "", 0.0

        flat = landmarks.reshape(-1).astype(np.float32)
        if flat.shape[0] != self._expected_input_size:
            return "", 0.0

        input_tensor = np.expand_dims(flat, axis=0)
        if self._input_dtype == np.uint8:
            scale, zero_point = self._input_details["quantization"]
            input_tensor = (input_tensor / scale + zero_point).astype(np.uint8)

        self._interpreter.set_tensor(self._input_details["index"], input_tensor)
        self._interpreter.invoke()

        raw_output = self._interpreter.get_tensor(self._output_details["index"])[0]

        if raw_output.dtype == np.uint8:
            scale, zero_point = self._output_details["quantization"]
            scores = (raw_output.astype(np.float32) - zero_point) * scale
        else:
            scores = raw_output.astype(np.float32)

        best_index = int(np.argmax(scores))
        confidence = float(scores[best_index])

        if best_index < len(SIGN_CLASSES):
            return SIGN_CLASSES[best_index], confidence
        return "", 0.0
