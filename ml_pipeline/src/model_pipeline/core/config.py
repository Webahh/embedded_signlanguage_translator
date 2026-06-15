# Central configuration for the sign language translation model pipeline.
# All model paths, thresholds, dimensions, and calibration constants are
# defined here to provide a single source of truth across the codebase

import os

SCRIPT_DIR = os.path.dirname(
    os.path.abspath(__file__)
)

# Root directory for TFLite model files (.tflite)
MODEL_ROOT = os.path.abspath(
    os.path.join(
        SCRIPT_DIR,
        "../../../../models",
    )
)

# -- Model paths -----------------------------------------------------------

PALM_MODEL_PATH = os.path.join(
    MODEL_ROOT,
    "033_palm_detection_full_quant_pc_ff_od.tflite",
)

HAND_LANDMARK_MODEL_PATH = os.path.join(
    MODEL_ROOT,
    "033_hand_landmark_full_quant_pc_uf_handl.tflite"
)

SIGNLANGUAGE_MODEL_PATH = os.path.abspath(
    os.path.join(
        MODEL_ROOT,
        "fingeralphabet_model_int8.tflite"
    )
)

# Test image for development and debugging
TEST_IMAGE_PATH = os.path.join(
    MODEL_ROOT,
    "../ml_pipeline/src/model_pipeline/utils/hand_test.jpg"
)

# -- Camera configuration --------------------------------------------------

CAMERA_INDEX = 0
CAMERA_FRAME_WIDTH = 1280
CAMERA_FRAME_HEIGHT = 720

# -- Model input dimensions -------------------------------------------------

MODEL_SIZE = 192                     # Palm model input square size (px)
HAND_LANDMARK_WIDTH = 224            # Landmark model input width (px)
HAND_LANDMARK_HEIGHT = 224           # Landmark model input height (px)

# -- Detection thresholds ---------------------------------------------------

SCORE_THRESHOLD = 0.5                # Minimum palm detection confidence
IOU_THRESHOLD = 0.4                  # NMS overlap threshold for palms
PRESENCE_THRESHOLD = 0.5             # Minimum hand presence confidence
HANDEDNESS_THRESHOLD = 0.5           # Threshold for right vs left hand
MAX_HANDS = 2                        # Maximum simultaneous tracked hands

# -- Sigmoid calibration ----------------------------------------------------

SIGMOID_CLIP_MIN = -100.0
SIGMOID_CLIP_MAX = 100.0

IMAGE_NORMALIZE_DIVISOR = 255.0      # Normalize uint8 pixels to [0,1]

# -- Landmark output dimensions ---------------------------------------------

NUM_LANDMARKS = 21                   # Landmarks per hand (MediaPipe spec)
NUM_PALM_KEYPOINTS = 7              # Keypoints in palm detection output
PALM_KEYPOINT_OFFSET = 4            # Box regressor offset in raw output

DEFAULT_HANDEDNESS = 0.5             # Default when model has no handedness head

# -- ROI (Region of Interest) parameters ------------------------------------

CENTER_ROI_DEFAULT_SCALE = 0.7
LANDMARK_BOX_SCORE = 1.0

# Indices used to compute bounding box from 21 landmarks
LANDMARK_BBOX_INDICES = [0, 1, 2, 3, 5, 6, 9, 10, 13, 14, 17, 18]
# Maps 7 palm keypoint indices to corresponding landmark indices
PALM_TO_LANDMARK_INDEX_MAP = [0, 5, 9, 13, 17, 1, 2]

# ROI shift/scale for palm detection -> landmark model cropping
PALM_ROI_SHIFT_X = 0.0
PALM_ROI_SHIFT_Y = -0.5
PALM_ROI_SCALE = 2.6

# ROI shift/scale for landmark -> next-frame tracking
LANDMARK_ROI_SHIFT_X = 0.0
LANDMARK_ROI_SHIFT_Y = -0.1
LANDMARK_ROI_SCALE = 2.0

# -- Tracking parameters ----------------------------------------------------

TRACK_MERGE_IOU = 0.5                # IOU threshold to merge duplicate tracks
FILTER_OVERLAP_IOU = 0.2             # IOU threshold to filter new detections

# -- Sign language classes (ASL fingerspelling, excluding J and Z) ----------

SIGN_CLASSES = list("ABCDEFGHIKLMNOPQRSTUVWXY")
