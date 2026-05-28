import os

import keras.models
import tensorflow as tf

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
FOLDER_ROOT = os.path.abspath(os.path.join(SCRIPT_DIR, "../../model/"))
MODEL_DIR = os.path.join(FOLDER_ROOT, "model.keras")

if __name__ == '__main__':
    model = keras.models.load_model(MODEL_DIR)
    converter = tf.lite.TFLiteConverter.from_keras_model(model)
    tflite_model = converter.convert()

    with open(os.path.join(FOLDER_ROOT, "model.tflite"), "wb") as f:
        f.write(tflite_model)