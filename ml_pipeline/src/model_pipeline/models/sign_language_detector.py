import os
import pickle
import numpy as np

from src.model_pipeline.runtime.interpreter import load_model


POS_MAX = 32767
NUM_LANDMARKS = 21


class SignLanguageDetector:
    def __init__(self, model_path: str) -> None:
        self._interpreter = load_model(model_path)
        self._input_details = self._interpreter.get_input_details()[0]
        self._output_details = self._interpreter.get_output_details()[0]
        self._in_scale, self._in_zp = self._input_details["quantization"]
        self._out_scale, self._out_zp = self._output_details["quantization"]
        self._labels_inv = self._load_labels(model_path)

    @staticmethod
    def _load_labels(model_path: str) -> dict[int, str]:
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
        normalized = landmarks_pixel.astype(np.float32)
        normalized[:, 0] /= image_width
        normalized[:, 1] /= image_height
        return np.clip(normalized, 0.0, 1.0)

    @staticmethod
    def _to_wrist_relative_int16(
        landmarks_01: np.ndarray,
    ) -> tuple[np.ndarray, np.ndarray]:
        scaled = (landmarks_01 * POS_MAX).astype(np.int16)
        wrist = scaled[0].copy()
        relative = scaled - wrist
        return relative, wrist

    @staticmethod
    def _empty_hand() -> tuple[np.ndarray, np.ndarray]:
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
        left_rel, left_wrist = self._empty_hand()
        right_rel, right_wrist = self._empty_hand()

        for landmarks_pixel, handedness in hands:
            lm_01 = self._normalize_to_image(landmarks_pixel, image_width, image_height)
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
        out_float = self._infer(hands, image_width, image_height)
        return {
            self._labels_inv[i]: float(out_float[i])
            for i in range(len(out_float))
        }
