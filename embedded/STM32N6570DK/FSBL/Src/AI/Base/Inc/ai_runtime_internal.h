/*
 * ai_runtime_internal.h
 *
 *  Created on: 01.07.2026
 *      Author: Weber
 */

#ifndef AI_RUNTIME_INTERNAL_H
#define AI_RUNTIME_INTERNAL_H

#include <stdbool.h>
#include "ll_aton_runtime.h"

bool AI_RuntimeRunNetwork(NN_Instance_TypeDef *network);
AI_RunStepStatus_TypeDef AI_RuntimeRunNetworkStep(NN_Instance_TypeDef *network);


#endif /* AI_RUNTIME_INTERNAL_H */
