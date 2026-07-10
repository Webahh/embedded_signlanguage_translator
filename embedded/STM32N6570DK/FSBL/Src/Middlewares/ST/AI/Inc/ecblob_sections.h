/*
 * ecblob_sections.h
 *
 *  Created on: 29.06.2026
 *      Author: Weber
 */

#ifndef ECBLOB_SECTIONS_H
#define ECBLOB_SECTIONS_H

#define ECBLOB_CONST_SECTION \
    __attribute__((section(".ecblobs_const"), aligned(64)))

#define ECBLOB_RUNTIME_SECTION \
    __attribute__((section(".ecblobs_runtime"), aligned(64)))

#endif /* ECBLOB_SECTIONS_H */
