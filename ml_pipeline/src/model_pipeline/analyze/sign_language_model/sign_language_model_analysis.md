# Sign Language Model Analysis

## Overview

This document describes the analysis of the sign language
classification model used in the embedded sign-language pipeline.

The model file is:

`model_int8.tflite`

The purpose of the model is to classify a static hand gesture
into one of 26 sign-language classes (24 letters + 2 special
symbols). The model takes 88 normalized landmark features as
input (both hands combined) and outputs a probability
distribution over 26 classes.

The 24 supported letters are:

```
A, B, C, D, E, F, G, H, I, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y
```

Letters J and Z are excluded because they require motion
(dynamic gestures), which this static model cannot represent.

Two additional non-letter classes were added:

- **NONE** - represents the absence of a hand (empty or no gesture)
- **SCH** - represents the German sign-language trigraph "SCH"

## Training Pipeline

The model was trained in a separate pipeline under
`ml_pipeline/src/model_generation/`.

### Architecture

```
Sequential([
    Input(shape=(88, 1)),
    Flatten(),
    Dense(88, activation="relu"),
    Dropout(0.25),
    Dense(128, activation="relu"),
    Dropout(0.5),
    Dense(26, activation="softmax"),
])
```

- Optimizer: Adam (learning rate = 0.00001)
- Loss: Sparse Categorical Crossentropy
- Training: 20 epochs, 80/20 train/val split, batch size 128,
  early stopping (patience = 3)

### Quantization

The Keras model was converted to a fully INT8-quantized TFLite
model using a representative dataset of 500 real training samples:

```python
converter.target_spec.supported_ops = [tf.lite.OpsSet.TFLITE_BUILTINS_INT8]
converter.inference_input_type = tf.uint8
converter.inference_output_type = tf.uint8
```

## Input Preprocessing

The model does not receive raw pixel data. Instead, it
receives a flat 88-element feature vector constructed from
MediaPipe hand landmarks.

### Step-by-step construction

The following steps are performed for each detected hand:

1. **Normalise to image coordinates**

   The decoded pixel landmarks `(x, y)` are divided by the image
   width and height to produce values in `[0.0, 1.0]`.

2. **Scale to int16**

   Each coordinate is multiplied by `POS_MAX = 32767` and
   converted to `int16`.

3. **Wrist-relative offset**

   The wrist landmark (index 0) is subtracted from all 21
   landmarks. This makes the gesture translation-invariant.

4. **Append absolute wrist position**

   The original (pre-subtraction) wrist position is appended as a
   22nd joint. This carries the global hand position information.

5. **Drop Z coordinate**

   Only `x` and `y` are kept; `z` is discarded.

Both hands are combined in a fixed order:

```
[Left hand: 22 joints × 2 coords = 44 values]
  joint  0  (WRIST):               x, y   → always [0, 0] (wrist minus itself)
  joint  1  (THUMB_CMC):           x, y
  joint  2  (THUMB_MCP):           x, y
  joint  3  (THUMB_IP):            x, y
  joint  4  (THUMB_TIP):           x, y
  joint  5  (INDEX_FINGER_MCP):    x, y
  joint  6  (INDEX_FINGER_PIP):    x, y
  joint  7  (INDEX_FINGER_DIP):    x, y
  joint  8  (INDEX_FINGER_TIP):    x, y
  joint  9  (MIDDLE_FINGER_MCP):   x, y
  joint 10  (MIDDLE_FINGER_PIP):   x, y
  joint 11  (MIDDLE_FINGER_DIP):   x, y
  joint 12  (MIDDLE_FINGER_TIP):   x, y
  joint 13  (RING_FINGER_MCP):     x, y
  joint 14  (RING_FINGER_PIP):     x, y
  joint 15  (RING_FINGER_DIP):     x, y
  joint 16  (RING_FINGER_TIP):     x, y
  joint 17  (PINKY_MCP):           x, y
  joint 18  (PINKY_PIP):           x, y
  joint 19  (PINKY_DIP):           x, y
  joint 20  (PINKY_TIP):           x, y
  joint 21  (absolute wrist):      x, y

[Right hand: 22 joints × 2 coords = 44 values]
  ... same structure ...
```

**Total**: `2 × 22 × 2 = 88` values.

### Missing hands

If only one hand is detected, the missing hand uses sentinel
values:

- All 21 landmarks: `[0, 0]`
- Absolute wrist position: `[-100, -100]`

This matches the convention used during training.

### Float normalisation

After assembly, the 88 int16 values are:

1. Converted to `float32` and divided by `POS_MAX` (`32767`)
   to obtain values in approximately `[-1.0, 1.0]`.
2. Quantized to `uint8` using the model's input quantization
   parameters (`scale = 0.007842`, `zero_point = 127`).

## Input

| Property     | Value                              |
|--------------|------------------------------------|
| Name         | `serving_default_input_layer:0`    |
| Shape        | `[1, 88, 1]`                       |
| Data type    | `uint8`                            |
| Quantization | `(0.007842, 127)`                  |
| Value range  | `0` to `255`                       |

### Dimension breakdown

```
[1, 88, 1]
 │   │   └── channels  (always 1 - Keras 1D-input convention)
 │   └────── features  (88 values: 44 per hand)
 └────────── batch     (single-frame inference)
```

- **Axis 0 - batch** (`1`): The model processes one frame at a
  time. During training, batches of 128 were used, but the TFLite
  model is always invoked with a batch of 1 at runtime.

- **Axis 1 - features** (`88`): The core data dimension. These 88
  values encode both hands as described above:
  - Positions `0–43`: left hand (22 joints × 2 coords)
  - Positions `44–87`: right hand (22 joints × 2 coords)

- **Axis 2 - channels** (`1`): A vestige of Keras's shape
  convention. The Keras `Input(shape=(88, 1))` declares the
  input as a 1D sequence of length 88 with 1 channel per
  position. This is the same shape convention used for a
  monophonic audio signal or a single-channel time series.
  TFLite faithfully preserves this trailing dimension even
  though the data is fundamentally a flat feature vector.

### Memory layout

The 88 values are stored in row-major order:

```
tensor[0,   0, 0] = left  hand, joint  0 (WRIST),               x
tensor[0,   1, 0] = left  hand, joint  0 (WRIST),               y
tensor[0,   2, 0] = left  hand, joint  1 (THUMB_CMC),           x
tensor[0,   3, 0] = left  hand, joint  1 (THUMB_CMC),           y
...
tensor[0,  42, 0] = left  hand, joint 21 (absolute wrist),      x
tensor[0,  43, 0] = left  hand, joint 21 (absolute wrist),      y
tensor[0,  44, 0] = right hand, joint  0 (WRIST),               x
tensor[0,  45, 0] = right hand, joint  0 (WRIST),               y
...
tensor[0,  86, 0] = right hand, joint 21 (absolute wrist),      x
tensor[0,  87, 0] = right hand, joint 21 (absolute wrist),      y
```

Within each hand, joints are ordered according to the MediaPipe
hand topology:

| Joint range | Finger segment     |
|-------------|--------------------|
| 0           | WRIST              |
| 1–4         | THUMB (CMC–TIP)    |
| 5–8         | INDEX (MCP–TIP)    |
| 9–12        | MIDDLE (MCP–TIP)   |
| 13–16       | RING (MCP–TIP)     |
| 17–20       | PINKY (MCP–TIP)    |
| 21          | absolute wrist pos |

Joint 0 (WRIST) is always `[0, 0]` because the wrist-relative
normalisation subtracts the wrist from itself. The original
wrist position is carried separately at joint 21.

### Quantization interpretation

The uint8 values are dequantized by the interpreter as:

```
float_value = (uint8_value - zero_point) × scale
            = (uint8_value - 127)        × 0.007842
```

This maps:
- `uint8 0`   → float approx `-0.996`
- `uint8 127` → float `0.0`
- `uint8 255` → float approx `1.004`

The float range approximately `[-1.0, 1.0]` matches the
training-time normalisation where the int16 feature vector was
divided by `POS_MAX = 32767`.

## Output

| Property     | Value                                |
|--------------|--------------------------------------|
| Name         | `StatefulPartitionedCall_1:0`        |
| Shape        | `[1, 26]`                            |
| Data type    | `uint8`                              |
| Quantization | `(0.00390625, 0)`                    |
| Meaning      | class scores (one per class)         |

The 26 output values correspond to the 26 sign-language classes.
The label ordering is: NONE first (index 0), then A–Y alphabetically,
then SCH last (index 25):

| Index | Label | Index | Label | Index | Label |
|-------|-------|-------|-------|-------|-------|
| 0     | NONE  | 9     | I     | 18    | S     |
| 1     | A     | 10    | K     | 19    | T     |
| 2     | B     | 11    | L     | 20    | U     |
| 3     | C     | 12    | M     | 21    | V     |
| 4     | D     | 13    | N     | 22    | W     |
| 5     | E     | 14    | O     | 23    | X     |
| 6     | F     | 15    | P     | 24    | Y     |
| 7     | G     | 16    | Q     | 25    | SCH   |
| 8     | H     | 17    | R     |       |       |

This mapping is stored in `class.pkl` (adjacent to the
`model_int8.tflite` file) and is loaded at runtime by
`SignLanguageDetector._load_labels()`. After dequantization,
the class with the highest score is the predicted letter.

## Inference

The `SignLanguageDetector` wraps the full inference pipeline:

```
predict(hands: list[(landmarks_21x2, handedness)], image_w, image_h)
```

```python
# 1. Normalise pixel landmarks to [0, 1]
lm_01[:, 0] /= image_width
lm_01[:, 1] /= image_height

# 2. Scale to int16 and make wrist-relative
scaled = (lm_01 * 32767).astype(int16)
wrist = scaled[0]
relative = scaled - wrist

# 3. Append wrist as 22nd joint → (22, 2) per hand
# 4. Flatten left + right → 88 int16 values
# 5. Normalise to [-1, 1] and quantize to uint8
x = flat / 32767
x_quant = (x / 0.007842 + 127).astype(uint8)
x_quant = x_quant.reshape(1, 88, 1)

# 6. Run TFLite inference
interpreter.set_tensor(input_index, x_quant)
interpreter.invoke()

# 7. Dequantize output
out_float = (out_quant - 0) * 0.00390625

# 8. Look up label from class.pkl mapping
best_index = argmax(out_float)
return labels_inv[best_index], out_float[best_index]
```

## Limitations

- The model accepts only static gestures. Letters J and Z, which
  require motion, are not supported
- Two hands are always expected. A missing hand fills the
  corresponding slot with sentinel values, which can bias the
  prediction
- The **NONE** class (index 0) represents the absence of a hand
  and should be used to gate inference - a prediction of NONE
  indicates no valid gesture is present
- The **SCH** class (index 25) represents the German sign-language
  trigraph "SCH" (a single handshape, not a sequence)
- The confidence score is not calibrated. A threshold (e.g.
  `0.5` or higher) should be applied in practice to reject
  uncertain or invalid predictions
- The model was trained on MediaPipe landmarks from full-frame
  images. The live pipeline must decode ROI-relative landmarks
  back to image coordinates before normalisation, otherwise the
  scale of the wrist-relative offsets will be incorrect
