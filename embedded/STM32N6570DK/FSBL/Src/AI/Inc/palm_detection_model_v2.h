/**
  ******************************************************************************
  * @file    palm_detection_model_v2.h
  * @author  STEdgeAI
  * @date    2026-06-25 13:22:06
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
#ifndef LL_ATON_PALM_DETECTION_MODEL_V2_H
#define LL_ATON_PALM_DETECTION_MODEL_V2_H

/******************************************************************************/
#define LL_ATON_PALM_DETECTION_MODEL_V2_C_MODEL_NAME        "palm_detection_model_v2"
#define LL_ATON_PALM_DETECTION_MODEL_V2_ORIGIN_MODEL_NAME   "node_033_palm_detection_full_quant_pc_ff_od"

/************************** USER ALLOCATED IOs ********************************/
// No user allocated inputs
// No user allocated outputs

/************************** INPUTS ********************************************/
#define LL_ATON_PALM_DETECTION_MODEL_V2_IN_NUM        (1)    // Total number of input buffers
// Input buffer 1 -- Input_0_out_0
#define LL_ATON_PALM_DETECTION_MODEL_V2_IN_1_ALIGNMENT   (32)
#define LL_ATON_PALM_DETECTION_MODEL_V2_IN_1_SIZE_BYTES  (442368)

/************************** OUTPUTS *******************************************/
#define LL_ATON_PALM_DETECTION_MODEL_V2_OUT_NUM        (2)    // Total number of output buffers
// Output buffer 1 -- Transpose_348_out_0
#define LL_ATON_PALM_DETECTION_MODEL_V2_OUT_1_ALIGNMENT   (32)
#define LL_ATON_PALM_DETECTION_MODEL_V2_OUT_1_SIZE_BYTES  (8064)
// Output buffer 2 -- Transpose_338_out_0
#define LL_ATON_PALM_DETECTION_MODEL_V2_OUT_2_ALIGNMENT   (32)
#define LL_ATON_PALM_DETECTION_MODEL_V2_OUT_2_SIZE_BYTES  (145152)

#endif /* LL_ATON_PALM_DETECTION_MODEL_V2_H */
