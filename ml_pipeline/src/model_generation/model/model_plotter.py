import os
import pickle
import matplotlib.pyplot as plt

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.abspath(os.path.join(SCRIPT_DIR, "../../.."))


def plot_history():
    history_path = os.path.join(PROJECT_ROOT, "model/history.pkl")

    with open(history_path, "rb") as f:
        history = pickle.load(f)

    # Accuracy
    plt.figure(figsize=(10, 5))
    plt.plot(history["sparse_categorical_accuracy"])
    plt.plot(history["val_sparse_categorical_accuracy"])

    plt.title("Model Accuracy")
    plt.ylabel("Accuracy")
    plt.xlabel("Epoch")
    plt.legend(["Train", "Validation"])
    plt.grid(True)

    plt.show()

    # Loss
    plt.figure(figsize=(10, 5))
    plt.plot(history["loss"])
    plt.plot(history["val_loss"])

    plt.title("Model Loss")
    plt.ylabel("Loss")
    plt.xlabel("Epoch")
    plt.legend(["Train", "Validation"])
    plt.grid(True)

    plt.show()


if __name__ == "__main__":
    plot_history()
