import os
import sys
import tensorflow as tf
from keras.utils import plot_model

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "../ml_pipeline"))

from src.model_generation.model.model import Model

model_obj = Model.load()
model = model_obj._model

output_dir = os.path.join(os.path.dirname(__file__), "images")
os.makedirs(output_dir, exist_ok=True)
output_path = os.path.join(output_dir, "model_architecture.png")

plot_model(
    model,
    to_file=output_path,
    show_shapes=True,
    show_layer_names=True,
    show_layer_activations=True,
    dpi=150
)
print(f"Gespeichert: {output_path}")