/**
 * @file    app.h
 * @author  Weber
 * @date    05.06.2026
 * @brief   Application initialisation and main loop
 *
 * Usage
 * -----
 * 1. app_init()   – initialise all subsystems
 * 2. app_run()    – start scheduler (never returns)
 */

#ifndef APP_H
#define APP_H

#include "ui.h"

void app_init();
void app_run();

extern UI_Drawer_TypeDef _drawer;

#endif /* APP_H */
