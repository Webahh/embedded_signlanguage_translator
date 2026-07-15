# Model Parameters

This document describes all hyperparameters and model parameters across
the three models in the embedded sign-language translator pipeline,
along with the reasoning behind each choice.

## Table of Contents

1. [Sign Language Classifier (Custom-Trained)](#1-sign-language-classifier-custom-trained)
2. [Palm Detection Model (Pre-trained TFLite)](#2-palm-detection-model-pre-trained-tflite)
3. [Hand Landmark Detector (Pre-trained TFLite)](#3-hand-landmark-detector-pre-trained-tflite)
4. [Tracking & Pipeline Parameters](#4-tracking--pipeline-parameters)
5. [Data Augmentation Parameters](#5-data-augmentation-parameters)

---

## 1. Sign Language Classifier (Custom-Trained)

This is the only model whose architecture and training is defined in this project. The Model comes last within the Pipeline. It takes in the handlandmarks and turns these points into the representative signlanguage letter. 

### 1.1 Model Architecture Parameters

| Parameter     | Value                           | Location             |
| ------------- | ------------------------------- | -------------------- |
| Input shape   | `(88, 1)`                       | `model/model.py:103` |
| Dense layer 1 | 88 units, ReLU                  | `model/model.py:105` |
| Dropout 1     | 0.25                            | `model/model.py:106` |
| Dense layer 2 | 128 units, ReLU                 | `model/model.py:107` |
| Dropout 2     | 0.5                             | `model/model.py:108` |
| Output layer  | 26 units (label_count), Softmax | `model/model.py:109` |

The full architecture:

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

#### Why these choices were made

| Aspect                    | Reason                                                                                                                                                                                                     |
| ------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Sequential, small MLP     | The input is a flat 88-element feature vector derived from hand landmarks (not raw pixels), so a lightweight fully-connected network suffices. This keeps inference fast enough for embedded/real-time use |
| `Input(88, 1)`            | 88 features = 2 hands x 22 joints x 2 coords (x, y). The trailing `1` is a Keras 1D-input convention, matching the shape used for monophonic audio or single-channel time series                           |
| Dense(88) then Dense(128) | The first layer matches input dimensionality (88 to 88) to learn per-feature transformations, then expands to 128 to capture inter-joint relationships before the classification head                      |
| Dropout 0.25/0.5          | Progressive dropout (lighter early, heavier later) prevents overfitting on a relatively small dataset while preserving the earlier, more general features                                                  |
| Softmax output            | Multi-class classification over 26 classes. Softmax produces a probability distribution that sums to 1                                                                                                     |

### 1.2 Training Hyperparameters

| Parameter               | Value                           | Location             |
| ----------------------- | ------------------------------- | -------------------- |
| Optimizer               | Adam                            | `model/model.py:114` |
| Learning rate           | `0.00001`                       | `model/model.py:114` |
| Loss function           | Sparse Categorical Crossentropy | `model/model.py:115` |
| Metric                  | `sparse_categorical_accuracy`   | `model/model.py:116` |
| Epochs                  | 20                              | `model/model.py:129` |
| Validation split        | 0.2 (80/20 train/val)           | `model/model.py:130` |
| Batch size              | 128                             | `model/model.py:131` |
| Early stopping patience | 3                               | `model/model.py:122` |
| Early stopping monitor  | `val_loss`                      | `model/model.py:121` |

#### Why these choices were made

| Aspect                                | Reason                                                                                                                                                                                      |
| ------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Adam with lr=0.00001                  | A very low learning rate ensures stable convergence on a small dataset. Adam adapts per-parameter learning rates automatically, which helps when features have different scales             |
| Sparse Categorical Crossentropy       | Labels are integer-encoded (not one-hot vectors), so the sparse variant avoids unnecessary memory overhead from one-hot encoding                                                            |
| Epochs=20 + EarlyStopping(patience=3) | Caps training at 20 epochs but stops early if `val_loss` does not improve for 3 consecutive epochs. This prevents overfitting without requiring manual tuning of the exact number of epochs |
| Batch size=128                        | Balanced between gradient noise (too small a batch) and generalization (too large a batch). Works well for the dataset size produced by the augmentation pipeline                           |

### 1.3 Quantization Parameters

| Parameter                   | Value                              | Location                              |
| --------------------------- | ---------------------------------- | ------------------------------------- |
| Quantization type           | Full INT8                          | `model/keras_to_tflite.py:54`         |
| Input type                  | `uint8`                            | `model/keras_to_tflite.py:57`         |
| Output type                 | `uint8`                            | `model/keras_to_tflite.py:58`         |
| Input quantization          | `(scale=0.007842, zero_point=127)` | `sign_language_model_analysis.md:160` |
| Output quantization         | `(scale=0.00390625, zero_point=0)` | `sign_language_model_analysis.md:250` |
| Representative dataset size | 500 samples                        | `model/keras_to_tflite.py:28`         |

#### Why these choices were made

| Aspect                          | Reason                                                                                                                                                                                                                  |
| ------------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Full INT8 quantization          | Required for deployment on embedded hardware with integer-only inference (no floating-point unit). Uses 500 real training samples as a representative dataset for calibration to preserve accuracy during quantization. |
| Input zero_point=127            | Centers the uint8 range around 0 in float space, matching the training-time normalization to [-1, 1].                                                                                                                   |
| Output scale=0.00390625 (1/256) | Maps 256 uint8 output values evenly to class scores.                                                                                                                                                                    |

### 1.4 Input Data Parameters

| Parameter | Value | Location |
|---|---|---|
| `POS_MAX` | 32767 (int16 max) | `hand_pose_detector.py:31` |
| Num landmarks per hand | 21 (+ 1 absolute wrist = 22 joints) | `model_input.py:17` |
| Features per hand | 44 (22 joints x 2 coords) | `sign_language_model_analysis.md:132` |
| Total input features | 88 (2 hands x 44) | `model/model.py:103` |
| Num classes | 26 (NONE + A-Y + SCH) | `sign_language_model_analysis.md:48` |

#### Why these choices were made

| Aspect                       | Reason                                                                                                                                                                                 |
| ---------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| POS_MAX = 32767              | The maximum value of int16, used to scale normalized [0, 1] coordinates to the full int16 range before wrist-relative subtraction. This preserves precision during integer arithmetic. |
| 22 joints per hand           | The 21 standard MediaPipe hand landmarks plus 1 appended absolute wrist position, which carries global hand position information after wrist-relative subtraction.                     |
| 26 classes                   | 24 DGS letters (`A`-`SCH` excluding `J`, `Z`, `Ä`, `Ö` and `Ü` which require motion), plus NONE (no hand).                                                                             |
| Wrist-relative normalization | Makes gestures translation-invariant so the same gesture at different screen positions produces the same input vector.                                                                 |


---

## 2. Palm Detection Model (Pre-trained TFLite)

A MediaPipe-style palm detection model loaded as a pre-trained
TFLite file. Configuration is in `src/model_pipeline/core/config.py`.

### 2.1 Model Configuration Parameters

| Parameter | Value | Location |
|---|---|---|
| Model input size | 192x192 px | `config.py:52` |
| Score threshold | 0.5 | `config.py:58` |
| NMS IoU threshold | 0.4 | `config.py:59` |
| Sigmoid clip min | -100.0 | `config.py:66` |
| Sigmoid clip max | 100.0 | `config.py:67` |
| Num palm keypoints | 7 | `config.py:74` |
| Keypoint offset | 4 (box regressor) | `config.py:75` |

### 2.2 Why these choices were made

| Aspect                       | Reason                                                                                                                                                                                                                                      |
| ---------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| 192x192 input                | Standard MediaPipe palm detection resolution. Small enough for real-time inference on embedded devices, yet large enough for reliable palm localization.                                                                                    |
| Score threshold 0.5          | Filters out low-confidence false positives while retaining real palm detections. Higher values risk missing hands; lower values increase false positives.                                                                                   |
| IoU threshold 0.4 for NMS    | Aggressive non-maximum suppression to eliminate duplicate bounding boxes for the same palm. A lower threshold is used here than in tracking because the palm detector runs on the full frame where overlapping predictions are more common. |
| Sigmoid clipping [-100, 100] | Prevents numerical overflow in the sigmoid activation while covering the full useful activation range (sigmoid saturates well before                                                                                                        |

---

## 3. Hand Landmark Detector (Pre-trained TFLite)

A MediaPipe-style hand landmark model loaded as a pre-trained
TFLite file. Configuration is in `src/model_pipeline/core/config.py`.

### 3.1 Model Configuration Parameters

| Parameter | Value | Location |
|---|---|---|
| Model input width | 224 px | `config.py:53` |
| Model input height | 224 px | `config.py:54` |
| Num landmarks | 21 per hand | `config.py:73` |
| Presence threshold | 0.5 | `config.py:60` |
| Handedness threshold | 0.5 | `config.py:61` |
| Default handedness | 0.5 | `config.py:77` |
| Image normalize divisor | 255.0 | `config.py:69` |

### 3.2 Why these choices were made

| Aspect | Reason |
| --- | --- |
| 224x224 input | Standard MobileNet-v2-derived resolution used by MediaPipe hand models. Optimized for mobile/embedded inference with a good accuracy/speed trade-off. |
| 21 landmarks | The MediaPipe hand skeleton standard: wrist + 4 fingers x 4 joints + fingertip. This captures all meaningful hand geometry for static gesture recognition. |
| Presence threshold 0.5 | Filters out frames where the hand is partially occluded or absent, preventing unreliable landmark predictions from propagating into the classifier. |
| Handedness threshold 0.5 | Boundary between left (score <= 0.5) and right (score > 0.5) hand classification. |

---

## 4. Tracking & Pipeline Parameters

Parameters governing the multi-hand tracking loop and ROI
management. Defined in `src/model_pipeline/core/config.py`.

### 4.1 Camera Configuration

| Parameter | Value | Location |
|---|---|---|
| Camera index | 0 | `config.py:46` |
| Frame width | 1280 px | `config.py:47` |
| Frame height | 720 px | `config.py:48` |

### 4.2 Detection Thresholds

| Parameter | Value | Location |
|---|---|---|
| Score threshold | 0.5 | `config.py:58` |
| IoU threshold | 0.4 | `config.py:59` |
| Max hands | 2 | `config.py:62` |

### 4.3 ROI Parameters

| Parameter | Value | Location |
|---|---|---|
| Center ROI default scale | 0.7 | `config.py:81` |
| Landmark box score | 1.0 | `config.py:82` |
| Landmark bbox indices | `[0,1,2,3,5,6,9,10,13,14,17,18]` | `config.py:85` |
| Palm-to-landmark index map | `[0,5,9,13,17,1,2]` | `config.py:87` |
| Palm ROI shift (x, y) | (0.0, -0.5) | `config.py:90-91` |
| Palm ROI scale | 2.6 | `config.py:92` |
| Landmark ROI shift (x, y) | (0.0, -0.1) | `config.py:95-96` |
| Landmark ROI scale | 2.0 | `config.py:97` |

### 4.4 Tracking Parameters

| Parameter | Value | Location |
|---|---|---|
| Track merge IoU | 0.5 | `config.py:101` |
| Filter overlap IoU | 0.2 | `config.py:102` |

### 4.5 Why these choices were made

| Aspect | Reason |
| --- | --- |
| 1280x720 (HD) | Provides sufficient detail for hand detection at a distance while maintaining real-time frame rates on embedded hardware. |
| Max hands = 2 | ASL fingerspelling typically uses one or two hands. Two is the maximum the MediaPipe landmark model supports. |
| Palm ROI shift Y=-0.5, scale=2.6 | Shifts the crop upward (hands are typically in the lower half of the frame) and expands it significantly to prevent truncation of the hand and wrist. |
| Landmark ROI shift Y=-0.1, scale=2.0 | A tighter crop for frame-to-frame tracking (hand position is already known from the previous frame), but still 2x margin to accommodate inter-frame hand movement. |
| Track merge IoU 0.5 | Merges two tracks if they overlap more than 50%, preventing duplicate tracking of the same hand when the palm detector re-detects it. |
| Filter overlap IoU 0.2 | Prevents initializing a new track if a fresh palm detection overlaps more than 20% with an existing track, avoiding double-counting. |
| Landmark bbox indices | A subset of the 21 landmarks chosen to form a tight bounding box around the hand (excluding fingertips that can spread wide and distort the box). |

---

## 5. Data Augmentation Parameters

Parameters for the training data augmentation pipeline defined in
`src/model_generation/augmentation/pipeline_functions.py`.

### 5.1 Augmentation Functions

| Parameter | Value | Location |
|---|---|---|
| Random translate max_offset | 10,000 (int16 units) | `pipeline_functions.py:38` |
| Random translate count | 10 | `pipeline_functions.py:38` |
| Scale factor | 1.1 | `pipeline_functions.py:69` |
| Jitter noise level (std dev) | 5.0 | `pipeline_functions.py:84` |
| Zoom scale factor | 1.2 | `pipeline_functions.py:100` |
| Random zoom range | [0.8, 1.2] | `pipeline_functions.py:115` |
| Random zoom count | 10 | `pipeline_functions.py:115` |
| Drop frame rate | 0.1 (10%) | `pipeline_functions.py:123` |

### 5.2 Why these choices were made

| Aspect | Reason |
| --- | --- |
| Translate +/-10,000 | Simulates hand position variation within the int16 coordinate space (~30% of total range). This teaches the model to recognize gestures regardless of where the hand appears in the frame. |
| Mirror | Flips the gesture horizontally and swaps left/right hand labels. Doubles effective training data and teaches the model to handle both orientations. |
| Scale 1.1 | A small scaling factor simulates minor distance changes from the camera without distorting the gesture shape. |
| Jitter sigma=5.0 | Small Gaussian noise simulates camera jitter and MediaPipe landmark estimation noise. Keeps the model robust to imperfect landmark detection. |
| Zoom range [0.8, 1.2] | Simulates larger distance changes (moving the hand closer to or farther from the camera). Only the landmark positions are scaled, not the wrist position. |
| Drop frames 10% | Simulates occasional frame drops common in embedded camera systems. Ensures the model (and downstream processing) can handle missing frames gracefully. |
