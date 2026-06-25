/**
  ******************************************************************************
  * @file    hand_landmark_model_v2.h
  * @author  STEdgeAI
  * @date    2026-06-25 13:22:33
  * @brief   Minimal description of the generated c-implemention of the network
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  ******************************************************************************
  */
#ifndef LL_ATON_HAND_LANDMARK_MODEL_V2_H
#define LL_ATON_HAND_LANDMARK_MODEL_V2_H

/******************************************************************************/
#define LL_ATON_HAND_LANDMARK_MODEL_V2_C_MODEL_NAME        "hand_landmark_model_v2"
#define LL_ATON_HAND_LANDMARK_MODEL_V2_ORIGIN_MODEL_NAME   "node_033_hand_landmark_full_quant_pc_uf_handl"

/************************** USER ALLOCATED IOs ********************************/
// No user allocated inputs
// No user allocated outputs

/************************** INPUTS ********************************************/
#define LL_ATON_HAND_LANDMARK_MODEL_V2_IN_NUM        (1)    // Total number of input buffers
// Input buffer 1 -- Input_0_out_0
#define LL_ATON_HAND_LANDMARK_MODEL_V2_IN_1_ALIGNMENT   (32)
#define LL_ATON_HAND_LANDMARK_MODEL_V2_IN_1_SIZE_BYTES  (150528)

/************************** OUTPUTS *******************************************/
#define LL_ATON_HAND_LANDMARK_MODEL_V2_OUT_NUM        (4)    // Total number of output buffers
// Output buffer 1 -- Dequantize_223_out_0
#define LL_ATON_HAND_LANDMARK_MODEL_V2_OUT_1_ALIGNMENT   (32)
#define LL_ATON_HAND_LANDMARK_MODEL_V2_OUT_1_SIZE_BYTES  (4)
// Output buffer 2 -- Dequantize_229_out_0
#define LL_ATON_HAND_LANDMARK_MODEL_V2_OUT_2_ALIGNMENT   (32)
#define LL_ATON_HAND_LANDMARK_MODEL_V2_OUT_2_SIZE_BYTES  (252)
// Output buffer 3 -- Dequantize_219_out_0
#define LL_ATON_HAND_LANDMARK_MODEL_V2_OUT_3_ALIGNMENT   (32)
#define LL_ATON_HAND_LANDMARK_MODEL_V2_OUT_3_SIZE_BYTES  (4)
// Output buffer 4 -- Dequantize_226_out_0
#define LL_ATON_HAND_LANDMARK_MODEL_V2_OUT_4_ALIGNMENT   (32)
#define LL_ATON_HAND_LANDMARK_MODEL_V2_OUT_4_SIZE_BYTES  (252)

#endif /* LL_ATON_HAND_LANDMARK_MODEL_V2_H */
