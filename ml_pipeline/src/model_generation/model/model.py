import os
import pickle

import matplotlib.pyplot as plt
import numpy as np
import tensorflow as tf

from dataclasses import dataclass
from keras.models import Sequential
from keras.callbacks import EarlyStopping
from keras.layers import Dense, Dropout, Flatten, Input
from sklearn.metrics import ConfusionMatrixDisplay, classification_report, confusion_matrix
from sklearn.model_selection import train_test_split

from src.model_generation.core.hand_pose_detector import POS_MAX
from src.model_generation.model.model_plotter import plot_history
from src.model_generation.model.model_input import load_training_data

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.abspath(os.path.join(SCRIPT_DIR, "../../.."))


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

    # Order: NONE=0, then A..Z alphabetically, then SCH=last
    unique_labels = sorted(
        {label for label, _ in training_data},
        key=lambda lbl: (2, lbl) if lbl == "SCH" else (0, "") if lbl == "NONE" else (1, lbl),
    )

    labels = {}
    labels_inv = {}

    for label_count, label in enumerate(unique_labels):
        print(f"Added Label '{label}' with id {label_count}")
        labels[label] = label_count
        labels_inv[label_count] = label

    training_labels = []
    training_inputs = []

    for label, inputs in training_data:
        print(f"Adding data to label '{label}' with id {labels[label]}")

        # Populate training data
        for input in inputs:
            # Dont have empty hands in training_data...
            if sum(input.flattened()) == -400:
                continue

            # normalize input data
            flattened = np.array(input.flattened(), dtype=np.float32)
            flattened = flattened / POS_MAX

            training_labels.append(labels[label])
            training_inputs.append(flattened)

    training_labels = np.array(training_labels, dtype=int)
    training_inputs = np.array(training_inputs, dtype=np.float32)

    # Shuffle training data
    rng = np.random.default_rng(42)
    indicies = rng.permutation(len(training_labels))

    training_labels = training_labels[indicies]
    training_inputs = training_inputs[indicies]

    return TrainingData(
        len(unique_labels), labels, labels_inv, training_labels, training_inputs
    )


class Model:
    def __init__(self, training_data: TrainingData):
        training_labels = np.asarray(
            training_data.training_labels,
            dtype=np.int32,
        )

        training_inputs = np.asarray(
            training_data.training_inputs,
            dtype=np.float32,
        ).reshape(-1, 88, 1)

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
                Dense(training_data.label_count, activation="softmax"),
            ]
        )

        self._model.compile(
            optimizer=tf.keras.optimizers.Adam(learning_rate=0.00001),
            loss=tf.keras.losses.SparseCategoricalCrossentropy(),
            metrics=["sparse_categorical_accuracy"],
        )

        self._model.summary()

        train_val_inputs, test_inputs, train_val_labels, test_labels = (
            train_test_split(
                training_inputs,
                training_labels,
                test_size=0.20,
                random_state=42,
                stratify=training_labels,
            )
        )

        # Split the remaining 80% into:
        # 64% training and 16% validation.
        train_inputs, validation_inputs, train_labels, validation_labels = (
            train_test_split(
                train_val_inputs,
                train_val_labels,
                test_size=0.20,
                random_state=42,
                stratify=train_val_labels,
            )
        )

        print(f"Training samples:   {len(train_labels)}")
        print(f"Validation samples: {len(validation_labels)}")
        print(f"Test samples:       {len(test_labels)}")

        early_stopping = EarlyStopping(
            monitor='val_loss',
            patience=3,
            restore_best_weights=True,
        )

        history = self._model.fit(
            train_inputs,
            train_labels,
            epochs=20,
            validation_data=(validation_inputs, validation_labels),
            batch_size=128,
            callbacks=[early_stopping],
            shuffle=True,
        )

        os.makedirs(
            os.path.join(PROJECT_ROOT, "model"),
            exist_ok=True,
        )

        with open(
                os.path.join(PROJECT_ROOT, "model/history.pkl"),
                "wb",
        ) as file:
            pickle.dump(history.history, file)

        plot_history()

        self.evaluate_confusion_matrix(
            test_inputs=test_inputs,
            test_labels=test_labels,
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

    def infer(self, buffer) -> dict:
        """
        Runs inference on a single model input buffer.

        Returns:
            dict: {label: confidence}
        """

        x = np.array([buffer], dtype=np.float32)
        x = x / POS_MAX

        outputs = self._model.predict(x, verbose=0)[0]

        # Map each label to its confidence
        confidences = {
            self.label_from_index(i): float(conf)
            for i, conf in enumerate(outputs)
        }

        return confidences

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

    def evaluate_confusion_matrix(
            self,
            test_inputs: np.ndarray,
            test_labels: np.ndarray,
            output_dir: str = os.path.join(PROJECT_ROOT, "model"),
    ) -> np.ndarray:
        """
        Evaluates the model on an untouched test set and saves:
          - confusion_matrix.png
          - confusion_matrix_normalized.png
          - confusion_matrix.npy
          - classification_report.txt

        Rows represent true classes.
        Columns represent predicted classes.
        """

        os.makedirs(output_dir, exist_ok=True)

        test_inputs = np.asarray(test_inputs, dtype=np.float32)
        test_labels = np.asarray(test_labels, dtype=np.int32).reshape(-1)

        # Match Input(shape=(88, 1))
        if test_inputs.ndim == 2:
            test_inputs = np.expand_dims(test_inputs, axis=-1)

        probabilities = self._model.predict(test_inputs, verbose=0)
        predicted_labels = np.argmax(probabilities, axis=1)

        label_ids = np.arange(self._label_count)
        class_names = [
            self.label_from_index(int(index))
            for index in label_ids
        ]

        # Non-normalized confusion matrix
        cm = confusion_matrix(
            test_labels,
            predicted_labels,
            labels=label_ids,
        )

        np.save(
            os.path.join(output_dir, "confusion_matrix.npy"),
            cm,
        )

        figure, axis = plt.subplots(figsize=(12, 12))

        display = ConfusionMatrixDisplay(
            confusion_matrix=cm,
            display_labels=class_names,
        )

        display.plot(
            ax=axis,
            xticks_rotation=90,
            values_format="d",
            colorbar=False,
        )

        axis.set_title("Confusion Matrix – Test Data")
        figure.tight_layout()
        figure.savefig(
            os.path.join(output_dir, "confusion_matrix.png"),
            dpi=200,
            bbox_inches="tight",
        )
        plt.close(figure)

        # Row-normalized confusion matrix
        # Each row shows the prediction distribution for one true class.
        cm_normalized = confusion_matrix(
            test_labels,
            predicted_labels,
            labels=label_ids,
            normalize="true",
        )

        figure, axis = plt.subplots(figsize=(12, 12))

        display = ConfusionMatrixDisplay(
            confusion_matrix=cm_normalized,
            display_labels=class_names,
        )

        display.plot(
            ax=axis,
            xticks_rotation=90,
            values_format=".2f",
            colorbar=True,
        )

        axis.set_title("Normalized Confusion Matrix – Test Data")
        figure.tight_layout()
        figure.savefig(
            os.path.join(output_dir, "confusion_matrix_normalized.png"),
            dpi=200,
            bbox_inches="tight",
        )
        plt.close(figure)

        report = classification_report(
            test_labels,
            predicted_labels,
            labels=label_ids,
            target_names=class_names,
            digits=4,
            zero_division=0,
        )

        with open(
                os.path.join(output_dir, "classification_report.txt"),
                "w",
                encoding="utf-8",
        ) as file:
            file.write(report)

        test_loss, test_accuracy = self._model.evaluate(
            test_inputs,
            test_labels,
            verbose=0,
        )

        print("\nTest results")
        print(f"Loss:     {test_loss:.6f}")
        print(f"Accuracy: {test_accuracy:.4%}")

        print("\nConfusion matrix")
        print(cm)

        print("\nClassification report")
        print(report)

        return cm

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
