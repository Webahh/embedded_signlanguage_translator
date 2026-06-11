# ROI-Based Tracking

## Concept

The ROI (Region of Interest) is a rotated rectangle in pixel space that defines where a model should look. <br>
Instead of processing the full frame, a model only sees a small cropped and rotated patch around one tracked object. <br>
This leads to speed improvements by reducing full-frame detection calls and lowering false positives.

## ROI Dataclass

```python
@dataclass
class ROI:
    cx: float       # center x (pixels)
    cy: float       # center y (pixels)
    w: float        # width (pixels)
    h: float        # height (pixels)
    rotation: float # angle (radians, normalized to [-π, π])
```

## Pipeline

```
  Detection Model ->  detection_to_roi() ->  ROI
                                              │
                                        Landmark Model
                                              │
                                       decode_landmark()
                                              │
                                       landmarks_to_roi()
                                              │
                                           new ROI -> next frame
```

See [ROI Lifecycle](./roi_lifecycle.md) for the full lifecycle of a single ROI from creation to destruction.

### 1. Detection -> ROI

A detection model outputs a normalized bounding box `[x1, y1, x2, y2]` and keypoints (0–1 range). <br>
These are converted to pixel coordinates using the letterbox scale/pad values.

The rotation is derived from a pair of keypoints that define the object's orientation.

```python
rotation = π/2 - arctan2(-(kp2_y - kp0_y), kp2_x - kp0_x)
```

A **shift** and **scale** is applied to make the ROI larger and centred on the object:

| Step | `shift_x` | `shift_y` | `scale_x` | `scale_y` |
|------|-----------|-----------|-----------|-----------|
| Detection -> ROI | 0.0 | –0.5 | 2.6 | 2.6 |

The shift is rotated into the ROI's local coordinate system before being applied.

### 2. Landmark Decoding

The landmark model outputs normalized `(x, y)` coordinates **relative to the ROI crop** - i.e., values in [0, 1] where (0, 0) is top-left of the rotated crop.

`decode_landmark` converts these back to **absolute pixel coordinates** on the original frame:

```python
dx = (lm_x - 0.5) * roi.w
dy = (lm_y - 0.5) * roi.h
x = roi.cx + dx * cos(rot) - dy * sin(rot)
y = roi.cy + dx * sin(rot) + dy * cos(rot)
```

This is a standard 2D rotation + translation: subtract the crop center, scale by ROI dimensions, rotate by `-roi.rotation`, and offset by ROI center.

### 3. Landmarks -> Next ROI

The decoded landmarks represent the best estimate of the object's position. A new bounding box is computed from a subset of stable landmarks (avoiding noisy points like fingertips).

The new rotation is derived from two stable landmarks that define the orientation.

A subset of landmark indices is extracted to match the detection model's keypoint format for visualization.

Shift/scale for the update step is gentler than the initial creation:

| Step | `shift_x` | `shift_y` | `scale_x` | `scale_y` |
|------|-----------|-----------|-----------|-----------|
| Update | 0.0 | –0.1 | 2.0 | 2.0 |

This keeps the ROI tightly cropped around the object once tracking is stable.

### 4. Angle Normalization

All rotations are clamped to `[-π, π]` to avoid drift over multiple frames:

```python
def normalize_angle(angle):
    return angle - 2π * floor((angle + π) / (2π))
```

## Visual Summary

```
┌───────────────────────────────────┐
│         Original Frame            │
│      ┌────────────────────┐       │
│      │   ROI (rotated)    │       │
│      │   ┌──────────┐     │       │
│      │   │ landmark │     │       │
│      │   │  crop    │     │       │
│      │   └──────────┘     │       │
│      │                    │       │
│      └────────────────────┘       │
└───────────────────────────────────┘
```

The ROI is updated frame-to-frame via `landmarks_to_roi`, so the landmark model always sees the object in a consistent orientation.
