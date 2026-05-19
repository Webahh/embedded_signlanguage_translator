import os
import pickle
from dataclasses import dataclass

import numpy as np
import tensorflow as tf
from tensorflow.keras.layers import (
    Dense,
    Dropout,
    Flatten,
    Input,
)
from tensorflow.keras.models import Sequential
#from tensorflow.keras.utils import to_categorical

from src.core.hand_pose_detector import POS_MAX
from src.model.model_input import load_training_data

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.abspath(os.path.join(SCRIPT_DIR, "../.."))


@dataclass(frozen=True)
class TrainingData:
    label_count: int
    labels: {}
    labels_inv: {}
    training_labels: any
    training_inputs: any

    def save(self, path=os.path.join(PROJECT_ROOT, "resources/training_data.pkl")):
        with open(path, "wb") as f:
            pickle.dump(self, f)

    @staticmethod
    def load(path=os.path.join(PROJECT_ROOT, "resources/training_data.pkl")):
        with open(path, "rb") as f:
            return pickle.load(f)


def generate_training_data(path=os.path.join(PROJECT_ROOT, "resources/gestures")) -> TrainingData:
    training_data = load_training_data(path)

    training_labels = []
    training_inputs = []
    labels = {}
    labels_inv = {}
    label_count = 0

    for label, inputs in training_data:
        # Build a dictionary over all labels
        if label not in labels:
            print(f"Added Label '{label}' with id {label_count}")
            labels[label] = label_count
            labels_inv[label_count] = label
            label_count += 1

        print(f"Adding data to label '{label}' with id {labels[label]}")

        # Populate training data
        for input in inputs:
            # Dont have empty hands in training_data...
            if sum(input.flattened()) == -400:
                continue

            training_labels.append(labels[label])
            training_inputs.append(input.flattened())

    # training_labels = to_categorical(training_labels, num_classes=label_count)

    training_labels = np.array(training_labels, dtype=int)
    training_inputs = np.array(training_inputs, dtype=np.int16)

    # Shuffle training data
    shuffled = [i for i in range(len(training_labels))]
    np.random.shuffle(shuffled)

    for p1, p2 in enumerate(shuffled):
        training_labels[[p1, p2]] = training_labels[[p2, p1]]
        training_inputs[[p1, p2]] = training_inputs[[p2, p1]]

    return TrainingData(
        label_count, labels, labels_inv, training_labels, training_inputs
    )


class Model:
    def __init__(self, training_data: TrainingData):
        training_labels = training_data.training_labels
        training_inputs = training_data.training_inputs

        print(training_labels)
        print(training_inputs[0])

        self._label_count = training_data.label_count
        self._labels = training_data.labels
        self._labels_inv = training_data.labels_inv

        self._model = Sequential(
            [
                Input(shape=(88, 1)),
                Flatten(),
                Dense(88, activation="relu"),
                Dropout(0.25),
                Dense(128, activation="relu"),
                Dropout(0.5),
                # Dense(32, activation="relu"),
                # Dropout(0.5),
                Dense(training_data.label_count, activation="sigmoid"),
            ]
        )

        self._model.compile(
            optimizer=tf.keras.optimizers.Adam(learning_rate=0.00001),
            loss=tf.keras.losses.SparseCategoricalCrossentropy(),
            metrics=["sparse_categorical_accuracy"],
        )

        self._model.summary()
        self._model.fit(
            training_inputs,
            training_labels,
            epochs=100,
            validation_split=0.2,
            batch_size=128,
        )

    @property
    def label_count(self) -> int:
        return self._label_count

    def label_from_index(self, index: int) -> str:
        return self._labels_inv[index]

    def index_from_label(self, label: str) -> int:
        return self._labels[label]

    def save(self, dir=os.path.join(PROJECT_ROOT, "model")):
        pickle_path = os.path.join(dir, "class.pkl")
        model_path = os.path.join(dir, "model.keras")
        os.makedirs(dir, exist_ok=True)

        # Save the entire model outside of pickle
        self._model.save(model_path)

        # Save this class using pickle, but dont save the model in pickle
        model = self._model
        self._model = None

        with open(pickle_path, "wb") as f:
            pickle.dump(self, f)

        self._model = model

    def infer(self, buffer) -> (str, float):
        outputs = self._model.predict(np.array([buffer], dtype=np.int16), verbose=0)
        index, confidence = 0, -1000

        for output, conf in enumerate(outputs[0]):
            if conf > confidence:
                confidence = conf
                index = output

        return self.label_from_index(index), confidence

    @staticmethod
    def load(dir=os.path.join(PROJECT_ROOT, "model")):
        pickle_path = os.path.join(dir, "class.pkl")
        model_path = os.path.join(dir, "model.keras")

        # Read this class from pickle
        me = None
        with open(pickle_path, "rb") as f:
            me = pickle.load(f)

        # Load model from keras file
        model = tf.keras.models.load_model(model_path)
        me._model = model

        # Return instance
        return me


# If this file is not imported as a module, train and save model
if __name__ == "__main__":
    print("Generating new model")

    tdata = os.path.join(PROJECT_ROOT, "resources/training_data.pkl")
    gdata = os.path.join(PROJECT_ROOT, "resources/gestures")

    training_data = None
    if os.path.isfile(tdata):
        print("Found training data. Loading Training data...")
        training_data = TrainingData.load(tdata)
    else:
        print("Did not find training data set, preparing new one...")
        training_data = generate_training_data(gdata)
        training_data.save(tdata)

    model = Model(training_data)
    model.save()

    loaded_model = Model.load()
    assert model._model is not None
    assert loaded_model._model is not None
