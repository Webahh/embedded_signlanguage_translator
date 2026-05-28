import os
import pickle
from warnings import showwarning

import keras.models
import tensorflow as tf
import numpy as np

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
FOLDER_ROOT = os.path.abspath(os.path.join(SCRIPT_DIR, "../../model/"))
MODEL_DIR = os.path.join(FOLDER_ROOT, "model.keras")
TRAINING_DATA_PATH = os.path.abspath(os.path.join(SCRIPT_DIR, "../../resources/training_data.pkl"))


def representative_dataset():
    """Yields calibration samples matching model input shape (88, 1)

    Uses real normalized training data for best quantization accuracy,
    falls back to uniform random noise in the typical [-1, 1] range
    """
    try:
        with open(TRAINING_DATA_PATH, "rb") as f:
            data = pickle.load(f)

        inputs = data.training_inputs  # shape (N, 88), float32, already /POS_MAX
        # Reshape to (N, 88, 1) to match model Input(shape=(88, 1))
        inputs = inputs.reshape(-1, 88, 1)
        for i in range(min(len(inputs), 500)):
            yield [inputs[i : i + 1]]

        print("Using trainging data")

    except (FileNotFoundError, AttributeError, pickle.UnpicklingError):
        print("Using Fallback noise data")

        # Fallback: uniform noise in [-1, 1] — covers the real value range
        for _ in range(100):
            sample = np.random.uniform(-1.0, 1.0, (1, 88, 1)).astype(np.float32)
            yield [sample]


if __name__ == "__main__":
    # Load model
    model = keras.models.load_model(MODEL_DIR)

    # Setup converter
    converter = tf.lite.TFLiteConverter.from_keras_model(model)

    converter.optimizations = [tf.lite.Optimize.DEFAULT]

    converter.representative_dataset = representative_dataset

    # INT8 conversion
    converter.target_spec.supported_ops = [tf.lite.OpsSet.TFLITE_BUILTINS_INT8]

    # Input/Output have to be INT8
    converter.inference_input_type = tf.uint8
    converter.inference_output_type = tf.uint8

    # Convert
    tflite_model = converter.convert()

    # Save
    output_path = os.path.join(FOLDER_ROOT, "model_int8.tflite")
    with open(output_path, "wb") as f:
        f.write(tflite_model)

    print(f"Saved INT8 model to: {output_path}")