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

TEST_IMAGE_PATH = os.path.join(
    MODEL_ROOT,
    "../ml_pipeline/src/model_pipeline/utils/hand_test.jpg"
)

CAMERA_INDEX = 0
MODEL_SIZE = 192

SCORE_THRESHOLD = 0.5
IOU_THRESHOLD = 0.3