# Hand Landmark Detection Model Analysis

## Overview

This document describes the analysis of the hand landmark detection
model used in the embedded sign-language pipeline.

The model file is:

`033_hand_landmark_full_quant_pc_uf_handl.tflite`

The purpose of the model is to detect detailed hand landmarks
inside a cropped hand image. The model expects that the hand is
already roughly localized by the palm detection stage. The outputs are:

- a hand presence score
- a handedness score
- 21 image landmarks
- 21 world landmarks

The output is not directly usable for the gesture classifier. The image
landmarks must first be interpreted, normalized and mapped back to the 
original frame or to the expected gesture-model input format.

## Input

| Property     | Value                 |
|--------------|-----------------------|
| Name         | `new_input_tensor`    |
| Shape        | `[1, 224, 224, 3]`    |
| Data type    | `uint8`               |
| Quantization | `(0.003921568..., 0)` |
| Color format | RGB                   |
| Value range  | `0` to `255`          |

The dimensions mean:
- `1`: batch size
- `224`: image height
- `224`: image width
- `3` : RGB color channels

The model expects exactly one RGB image with a resolution of `224 × 224` pixels.
Although the input tensor uses `uint8`, the quantization scale is approximately `1 / 255`.
This means that the integer input values represent normalized values between `0.0` and `1.0`.

OpenCV loads images in BGR format. Therefore, the image must be converted from 
BGR to RGB before it is passed to the model.

## Input Preprocessing

The current test setup uses a fixed square Region of Interest, abbreviated as ROI.
ROI means Region of Interest. It is the selected image area that should contain the hand.

The preprocessing steps are:

1. Select a square ROI from the input frame.
2. Resize the ROI to `244 x 244`.
3. Convert the image from BGR to RGB
4. Convert the image to `uint8`.
5. Add the batch dimension.

The model does not receive the full camera frame directly in the final pipeline.
Instead, it should receive a hand crop generated from the palm detection result.

## Outputs

The model has four output tensors.

### Output: Identity_1:0 - Presence Score

| Property     | Value               |
|--------------|---------------------|
| Name         | `Identity_1:0`      |
| Shape        | `[1, 1]`            |
| Data type    | `float32`           |
| Quantization | none                |
| Meaning      | hand presence score |

This output indicates whether a valid hanf is present in the input crop.
In live tests, this value remained stable when a hand was inside the ROI
and changed when the input was invalid or empty.

### Output: Identity_2:0 - Handedness Score

| Property     | Value            |
|--------------|------------------|
| Name         | `Identity_2:0`   |
| Shape        | `[1, 1]`         |
| Data type    | `float32`        |
| Quantization | none             |
| Meaning      | handedness score |

This output indicates whether the detected hand is the left or right hand.

- Left hand -> approx. value `0.2422...`
- Right hand -> approx. value `0.9297...`

The handedness score should only be used if the presence score indicates
a valid hand.

### Output: Identity:0 - Image Hand Landmarks

| Property     | Value           |
|--------------|-----------------|
| Name         | `Identity:0`    |
| Shape        | `[1, 63]`       |
| Data type    | `float32`       |
| Quantization | none            |
| Meaning      | image landmarks |

This output contains 21 hand landmarks in image coordinates. The 63 values are
structured as:

*21 landmark x 3 values = 63 values*

Each landmark contains:

- `x`
- `y`
- `z`

So the landmark array can be reshaped to `[21, 3]`.

For visualization in the oroginal frame, the landmark coordinates must be mapped
from the ROI coordinate system back to the camera frame.

### Output: Identity_3:0 - World Hand Landmarks

| Property     | Value           |
|--------------|-----------------|
| Name         | `Identity_3:0`  |
| Shape        | `[1, 63]`       |
| Data type    | `float32`       |
| Quantization | none            |
| Meaning      | world landmarks |

This output contains 21 hand landmarks in a normalized world-coordinates
representation. The 63 values are structured in the same way as the
image landmarks.

The observed values are much smaller than the image landmark values and are
centered around zero. This output may be useful for 3D-aware processing, but the current gesture
pipeline primarily uses the image landmarks.

## Verified Test Result

A live test was performed with a fixed center ROI. The observed behavior was:

| Situation         | Identity_1:0                     | Identity_2:0                     |
|-------------------|----------------------------------|----------------------------------|
| Left hand in ROI  | stable hand-present value        | approx. `0.2422`                 |
| Right hand in ROI | stable hand-present value        | approx. `0.9297`                 |
| No hand in ROI    | unstable or background-dependent | unstable or background-dependent |

THe image landmarks from `Identity:0` were reshaped to [21, 3] and drawn into the ROI.
The 21 point were visually verified on the hand.

## Landmark Order

The model return 21 landmarks. The landmark order follows the common MediaPipe-style hand topology:

| Index | Landmark      |
|-------|---------------|
| 0     | wrist         |
| 1-4   | thumb         |
| 5-8   | index finger  |
| 9-12  | middle finger |
| 13-16 | ring finger   |
| 17-20 | pinky finger  |

The following connections were used for visualization:

- wrist -> thumb
- wrist -> index finger
- index finger -> middle finger
- middle finger -> ring finger
- ring finger -> pinky finger 
- wrist -> pinky finger


