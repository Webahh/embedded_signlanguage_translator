/*
 * ecblob_sections.h
 *
 *  Created on: 29.06.2026
 *      Author: Weber
 */

#ifndef ECBLOB_SECTIONS_H
#define ECBLOB_SECTIONS_H

#define ECBLOB_CONST_SECTION \
    __attribute__((section(".ecblobs"), aligned(8)))

#endif /* ECBLOB_SECTIONS_H */
