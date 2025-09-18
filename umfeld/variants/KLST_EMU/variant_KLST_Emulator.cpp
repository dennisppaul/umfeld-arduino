/*
 * Umfeld
 *
 * This file is part of the *Umfeld* library (https://github.com/dennisppaul/umfeld).
 * Copyright (c) 2025 Dennis P Paul.
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef BOARD_NAME_KLST_EMU

/* --- debugging define configuration --- */

#if !defined(UMFELD_SET_DEFAULT_CALLBACK) || UMFELD_SET_DEFAULT_CALLBACK != 0
#error "UMFELD_SET_DEFAULT_CALLBACK must be defined as 0"
#endif

#ifndef KLST_ENV
#error "need to define KLST_ENV"
#endif

/* --- emulator implementation --- */

#include "Arduino.h"
#include "Umfeld.h"

extern void klst_emulator_arguments(const std::vector<std::string>& args);
extern void klst_emulator_settings();
extern void klst_emulator_setup();
extern void klst_emulator_draw();
extern void klst_emulator_update();
extern void klst_emulator_audioEvent();
extern void klst_emulator_keyPressed();
extern void klst_emulator_keyReleased();

void umfeld_set_callbacks() {
    umfeld::set_arguments_callback(klst_emulator_arguments);
    umfeld::set_settings_callback(klst_emulator_settings);
    umfeld::set_setup_callback(klst_emulator_setup);
    umfeld::set_draw_callback(klst_emulator_draw);
    umfeld::set_update_callback(klst_emulator_update);
    umfeld::set_audioEvent_callback(klst_emulator_audioEvent);
    umfeld::set_keyPressed_callback(klst_emulator_keyPressed);
    umfeld::set_keyReleased_callback(klst_emulator_keyReleased);
}

#endif
