import os

POS_MAX = 32767

LANDMARK_COUNT = 21
FEATURES_PER_HAND = 44
TOTAL_FEATURES = 88

HAND_PRESENCE_THRESHOLD = 0.5

CORE_DIR = os.path.dirname(
    os.path.abspath(__file__)
)

PROJECT_ROOT = os.path.abspath(
    os.path.join(
        CORE_DIR,
        "../../../..",
    )
)

MODEL_ROOT = os.path.join(
    PROJECT_ROOT,
    "models",
)

PALM_MODEL_PATH = os.path.join(
    MODEL_ROOT,
    "033_palm_detection_full_quant_pc_ff_od.tflite",
)

HAND_LANDMARK_MODEL_PATH = os.path.join(
    MODEL_ROOT,
    "033_hand_landmark_full_quant_pc_uf_handl.tflite",
)

GESTURE_MODEL_PATH = os.path.join(
    MODEL_ROOT,
    "fingeralphabet_model_int8.tflite",
)

CLASS_MAPPING_PATH = os.path.join(
    MODEL_ROOT,
    "class.pkl",
)

HAND_TEST_IMAGE_PATH = os.path.join(
    CORE_DIR,
    "hand_test.jpg",
)

HAND_TEST_IMAGE_2_PATH = os.path.join(
    CORE_DIR,
    "hand_test2.png",
)