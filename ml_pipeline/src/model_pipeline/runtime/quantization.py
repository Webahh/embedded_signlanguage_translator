import numpy as np


def quantize_tensor(
    values: np.ndarray,
    tensor_details: dict,
) -> np.ndarray:
    """
    Converts floating-point values into the quantized representation
    expected by a model tensor.
    """

    values = np.asarray(values)

    dtype = tensor_details["dtype"]

    if np.issubdtype(dtype, np.floating):
        return values.astype(dtype)

    scale, zero_point = tensor_details["quantization"]

    if scale <= 0:
        raise ValueError(
            f"Ungültige Quantisierung für Tensor "
            f"'{tensor_details['name']}': "
            f"scale={scale}, zero_point={zero_point}"
        )

    quantized = np.round(
        values / scale + zero_point
    )

    limits = np.iinfo(dtype)

    quantized = np.clip(
        quantized,
        limits.min,
        limits.max,
    )

    return quantized.astype(dtype)


def dequantize_tensor(
    values: np.ndarray,
    tensor_details: dict,
) -> np.ndarray:
    """
    Converts a quantized model tensor into float32 values.
    """

    values = np.asarray(values)

    if np.issubdtype(values.dtype, np.floating):
        return values.astype(np.float32)

    scale, zero_point = tensor_details["quantization"]

    if scale <= 0:
        raise ValueError(
            f"Ungültige Quantisierung für Tensor "
            f"'{tensor_details['name']}': "
            f"scale={scale}, zero_point={zero_point}"
        )

    return (
        values.astype(np.float32) - zero_point
    ) * scale