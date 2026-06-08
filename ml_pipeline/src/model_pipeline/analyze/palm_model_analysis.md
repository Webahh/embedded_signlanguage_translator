# Palm Detection Model Analysis

## Overview

This document describes the analysis of the
palm detection model used in the embedded
sign-language pipeline.



The model file is:

`033_palm_detection_full_quant_pc_ff_od.tflite`

The purpose of the model is to detect one or more palms
in an image. For every possible detection position, the 
model outputs:

- a confidence value,
- a palm bounding box,
- seven palm keypoints.

The output is not directly usable. It must first be decoded
with a predefined anchor list and filtered with Non-Maximum
Suppression.

## Non-Maximum Suppression

Several anchors may detect the same palm and therefore produce strongly
overlapping bounding boxes. Duplicate detection are removed using IoU-based
Non-Maximum Suppression.

IoU = intersection area / union area

An IoU value of `1.0` means that the boxes are identical, while a value of `0.0`
means that they do not overlap. The current IoU threshold is `0.3`

When two detections overlap by more than this threshold, the detection with the lower
confidence score is removed.

## Input

| Property     | Value              |
|--------------|--------------------|
| Name         | `input_1:0`        |
| Shape        | `[1, 192, 192, 3]` |
| Data type    | `float32`          |
| Quantization | none               |
| Color format | RGB                |
| Value range  | `0.0` to `1.0`     |

The dimensions mean:
- `1`: batch size
- `192`: image height
- `192`: image width
- `3`: RGB color channels

The model therefore expects exactly one RGB image with a resolution of `192 x 192`
pixels. The original image is resized with letterboxing to preserve its aspect ratio.


## Outputs

The model has two output tensors.

### Score output

| Property  | Value                 |
|-----------|-----------------------|
| Name      | `Identity_1:0`        |
| Shape     | `[1, 2016, 1]`        |
| Data type | `float32`             |
| Meaning   | Palm detection logits |

The model produces one raw score for each of the 2016 anchors.
The output shape means:

- `1`: batch size
- `2016`: number of anchors
- `1`: one score per anchor

The score is a logit and must be converted into a probability
using the sigmoid function.

### Regression output

| Property  | Value                           |
|-----------|---------------------------------|
| Name      | `Identity:0`                    |
| Shape     | `[1, 2016, 18]`                 |
| Data type | `float32`                       |
| Meaning   | bounding box and palm keypoints |

For every anchor, the model returns 18 regression values. The values are
structured as follows:

| Indices | Meaning                                |
|---------|----------------------------------------|
| 0       | bounding-box center x offset           |
| 1       | bounding-box center y offset           |
| 2       | bounding-box width                     |
| 3       | bounding-box height                    |
| 4-17    | seven keypoints with x and coordinates |

## Anchors

The model uses 2016 anchors. Each anchor contains a normalized center position:

- `anchor_x`
- `anchor_y`

The anchor values are stored in `palm_anchors.py` The raw model
outputs are offsets relative to these anchor centers. The output at 
index `i` must always be decoded with anchor `i`.

## Decoding

Bounding-box decoding:

    center_x = raw[0] / 192 + anchor_x
    center_y = raw[1] / 192 + anchor_y

    width  = raw[2] / 192
    height = raw[3] / 192

Keypoint decoding:

    keypoint_x = raw[offset] / 192 + anchor_x
    keypoint_y = raw[offset + 1] / 192 + anchor_y

## Keypoint order

The model returns seven palm keypoints. These keypoints are
later important for the calculation of the hand rotation and
creating a hand region of interest `ROI`.

Observed arrangement:

- Keypoint 0: wrist
- Keypoints 1–4: finger bases
- Keypoints 5–6: additional palm keypoints near the thumb side

## Score filtering

Current score threshold:

`0.5`

Several anchors can detect the same palm.

## Verified test result

For `hand_test.jpg`:

- Best anchor index: `1686`
- Best probability: approximately `0.966`
- Detections after NMS: `1`

The decoded bounding box and all seven keypoints were visually verified.

## Live inference

The live pipeline currently performs:

1. Capture HD webcam frame
2. Letterbox image to `192 × 192`
3. Run palm model
4. Apply sigmoid to logits
5. Decode anchors
6. Filter by score
7. Apply Non-Maximum Suppression
8. Map coordinates back to the original frame

Multiple palms can be detected in one frame.