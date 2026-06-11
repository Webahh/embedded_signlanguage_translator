import os

SCRIPT_DIR = os.path.dirname(
    os.path.abspath(__file__)
)

MODEL_ROOT = os.path.abspath(
    os.path.join(
        SCRIPT_DIR,
        "../../../../models",
    )
)

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
        "../ml_pipeline/model/model_int8.tflite"
    )
)

TEST_IMAGE_PATH = os.path.join(
    MODEL_ROOT,
    "../ml_pipeline/src/model_pipeline/utils/hand_test.jpg"
)

CAMERA_INDEX = 0
CAMERA_FRAME_WIDTH = 1280
CAMERA_FRAME_HEIGHT = 720

MODEL_SIZE = 192

SCORE_THRESHOLD = 0.5
IOU_THRESHOLD = 0.4

PRESENCE_THRESHOLD = 0.5
MAX_HANDS = 2
HAND_LANDMARK_WIDTH = 224
HAND_LANDMARK_HEIGHT = 224

SIGMOID_CLIP_MIN = -100.0
SIGMOID_CLIP_MAX = 100.0

IMAGE_NORMALIZE_DIVISOR = 255.0

NUM_LANDMARKS = 21
NUM_PALM_KEYPOINTS = 7
PALM_KEYPOINT_OFFSET = 4

HANDEDNESS_THRESHOLD = 0.5
DEFAULT_HANDEDNESS = 0.5

CENTER_ROI_DEFAULT_SCALE = 0.7
LANDMARK_BOX_SCORE = 1.0

LANDMARK_BBOX_INDICES = [0, 1, 2, 3, 5, 6, 9, 10, 13, 14, 17, 18]
PALM_TO_LANDMARK_INDEX_MAP = [0, 5, 9, 13, 17, 1, 2]

PALM_ROI_SHIFT_X = 0.0
PALM_ROI_SHIFT_Y = -0.5
PALM_ROI_SCALE = 2.6

LANDMARK_ROI_SHIFT_X = 0.0
LANDMARK_ROI_SHIFT_Y = -0.1
LANDMARK_ROI_SCALE = 2.0

TRACK_MERGE_IOU = 0.5
FILTER_OVERLAP_IOU = 0.2

SIGN_CLASSES = list("ABCDEFGHIKLMNOPQRSTUVWXY")
