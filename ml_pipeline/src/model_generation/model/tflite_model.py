import os
import pickle
import numpy as np
import tensorflow as tf

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.abspath(os.path.join(SCRIPT_DIR, "../../.."))


#   Pickle stub: lets class.pkl deserialize without importing the
#   full Keras/MediaPipe pipeline
class _ModelStub:
    """Minimal stand-in for src.model.model.Model during pickle loading."""
    def __init__(self, **kwargs):
        for k, v in kwargs.items():
            setattr(self, k, v)


def _load_labels_inv(class_path):
    # Both class.pkl and training_data.pkl were created by running model.py
    # as __main__, so pickle references __main__.Model and __main__.TrainingData.
    import __main__
    __main__.Model = _ModelStub
    if not hasattr(__main__, "TrainingData"):
        __main__.TrainingData = _ModelStub

    with open(class_path, "rb") as f:
        obj = pickle.load(f)
    return obj._labels_inv

class TFLiteModel:
    def __init__(self, interpreter, labels_inv):
        self._interpreter = interpreter
        self._labels_inv = labels_inv

        self._input_details = interpreter.get_input_details()[0]
        self._output_details = interpreter.get_output_details()[0]

        self._in_scale, self._in_zp = self._input_details["quantization"]
        self._out_scale, self._out_zp = self._output_details["quantization"]

        self.label_count = len(labels_inv)

    def label_from_index(self, index: int) -> str:
        return self._labels_inv[index]

    def infer(self, buffer) -> dict:
        POS_MAX = 32767
        x = np.array(buffer, dtype=np.float32) / POS_MAX

        x_quant = (x / self._in_scale + self._in_zp).astype(np.uint8)
        x_quant = x_quant.reshape(1, 88, 1)

        self._interpreter.set_tensor(self._input_details["index"], x_quant)
        self._interpreter.invoke()

        out_quant = self._interpreter.get_tensor(self._output_details["index"])[0]
        out_float = (out_quant.astype(np.float32) - self._out_zp) * self._out_scale

        return {
            self.label_from_index(i): float(conf)
            for i, conf in enumerate(out_float)
        }

    @staticmethod
    def load(dir=os.path.join(PROJECT_ROOT, "model")):
        tflite_path = os.path.join(dir, "model_int8.tflite")
        class_path = os.path.join(dir, "class.pkl")

        labels_inv = _load_labels_inv(class_path)
        interpreter = tf.lite.Interpreter(model_path=tflite_path)
        interpreter.allocate_tensors()

        return TFLiteModel(interpreter, labels_inv)

    @staticmethod
    def load_custom(tflite_path, class_path):
        labels_inv = _load_labels_inv(class_path)
        interpreter = tf.lite.Interpreter(model_path=tflite_path)
        interpreter.allocate_tensors()

        return TFLiteModel(interpreter, labels_inv)
