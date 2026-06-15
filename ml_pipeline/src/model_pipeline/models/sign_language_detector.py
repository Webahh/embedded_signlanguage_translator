# Sign language classification model wrapper
#
# Takes up to two hands' worth of landmark coordinates (21 landmarks each,
# wrist-relative, normalized to image dimensions) and classifies the gesture
# into one of the ASL fingerspelling letter classes (A-Y, excluding J and Z)
#
# The model is an int8 quantized TFLite network that expects a specific input
# layout: 88 uint8 values encoding left/right hand landmarks stacked flat

import os
import pickle
import numpy as np

from src.model_pipeline.runtime.interpreter import load_model


POS_MAX = 32767
NUM_LANDMARKS = 21


class SignLanguageDetector:
    """TFLite wrapper for the sign language classifier

    Handles landmark normalization (image-relative -> wrist-relative),
    int16 quantization, and label decoding from a companion class.pkl file
    """

    def __init__(self, model_path: str) -> None:
        self._interpreter = load_model(model_path)
        self._input_details = self._interpreter.get_input_details()[0]
        self._output_details = self._interpreter.get_output_details()[0]
        self._in_scale, self._in_zp = self._input_details["quantization"]
        self._out_scale, self._out_zp = self._output_details["quantization"]
        self._labels_inv = self._load_labels(model_path)

    @staticmethod
    def _load_labels(model_path: str) -> dict[int, str]:
        """Load the label-to-index mapping from a pickled class.pkl file

        The pickle stores a model object with a _labels_inv attribute.
        A lightweight stub class is injected into __main__ to satisfy
        pickle's dependency on the original class definition during
        deserialization
        """
        model_dir = os.path.dirname(model_path)
        class_path = os.path.join(model_dir, "class.pkl")

        class _ModelStub:
            def __init__(self, **kwargs):
                for k, v in kwargs.items():
                    setattr(self, k, v)

        import __main__
        __main__.Model = _ModelStub

        with open(class_path, "rb") as f:
            obj = pickle.load(f)
        return obj._labels_inv

    @staticmethod
    def _normalize_to_image(
        landmarks_pixel: np.ndarray,
        image_width: int,
        image_height: int,
    ) -> np.ndarray:
        """Convert pixel-space landmarks to [0, 1] image-relative coordinates"""
        normalized = landmarks_pixel.astype(np.float32)
        normalized[:, 0] /= image_width
        normalized[:, 1] /= image_height
        return np.clip(normalized, 0.0, 1.0)

    @staticmethod
    def _to_wrist_relative_int16(
        landmarks_01: np.ndarray,
    ) -> tuple[np.ndarray, np.ndarray]:
        """Convert normalized landmarks to wrist-relative int16 coordinates

        The wrist landmark (index 0) is subtracted from all points, and the
        result is scaled to the int16 range [-32767, 32767]
        """
        scaled = (landmarks_01 * POS_MAX).astype(np.int16)
        wrist = scaled[0].copy()
        relative = scaled - wrist
        return relative, wrist

    @staticmethod
    def _empty_hand() -> tuple[np.ndarray, np.ndarray]:
        """Return a sentinel 'no hand' representation

        Used when fewer than two hands are detected; the model is trained
        to ignore zeroed wrist-relative landmarks
        """
        relative = np.zeros((NUM_LANDMARKS, 2), dtype=np.int16)
        wrist = np.array([-100, -100], dtype=np.int16)
        return relative, wrist

    def _build_input_tensor(
        self,
        left_relative: np.ndarray,
        left_wrist: np.ndarray,
        right_relative: np.ndarray,
        right_wrist: np.ndarray,
    ) -> np.ndarray:
        """Stack left/right hand data into the model's expected 88-element input

        Layout: 21 landmarks x 2 (x,y) + 1 wrist x 2 = 44 per hand,
        concatenated for both hands = 88. Quantized to uint8
        """
        left_joints = np.vstack([
            left_relative,
            [left_wrist[0], left_wrist[1]],
        ])
        right_joints = np.vstack([
            right_relative,
            [right_wrist[0], right_wrist[1]],
        ])
        flat = np.concatenate([
            left_joints.flatten(),
            right_joints.flatten(),
        ])

        x = flat.astype(np.float32) / POS_MAX
        x_quant = (x / self._in_scale + self._in_zp).astype(np.uint8)
        return x_quant.reshape(1, 88, 1)

    def _infer(
        self,
        hands: list[tuple[np.ndarray, float]],
        image_width: int,
        image_height: int,
    ) -> np.ndarray:
        """Run inference and return dequantized output logits

        Args:
            hands: List of (landmarks_21x2, handedness) tuples
            image_width, image_height: Frame dimensions for normalization

        Returns:
            Float array of class logits
        """
        left_rel, left_wrist = self._empty_hand()
        right_rel, right_wrist = self._empty_hand()

        for landmarks_pixel, handedness in hands:
            lm_01 = self._normalize_to_image(landmarks_pixel, image_width, image_height)
            # Mirror X axis: model expects left/right in a fixed orientation
            lm_01[:, 0] = 1.0 - lm_01[:, 0]
            rel, wrist = self._to_wrist_relative_int16(lm_01)
            if handedness < 0.5:
                left_rel, left_wrist = rel, wrist
            else:
                right_rel, right_wrist = rel, wrist

        input_tensor = self._build_input_tensor(
            left_rel, left_wrist,
            right_rel, right_wrist,
        )

        self._interpreter.set_tensor(self._input_details["index"], input_tensor)
        self._interpreter.invoke()

        out_quant = self._interpreter.get_tensor(self._output_details["index"])[0]
        return (out_quant.astype(np.float32) - self._out_zp) * self._out_scale

    def predict(
        self,
        hands: list[tuple[np.ndarray, float]],
        image_width: int,
        image_height: int,
    ) -> dict[str, float]:
        """Classify the hand gesture

        Args:
            hands: List of (landmarks_21x2, handedness) tuples
            image_width, image_height: Frame dimensions

        Returns:
            Dictionary mapping class label to confidence score
        """
        out_float = self._infer(hands, image_width, image_height)
        return {
            self._labels_inv[i]: float(out_float[i])
            for i in range(len(out_float))
        }
