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

#warning "BOARD_NAME_KLST_EMU is defined"

#ifndef UMFELD_SET_DEFAULT_CALLBACK
#error "need to to define UMFELD_SET_DEFAULT_CALLBACK"
#endif

#if UMFELD_SET_DEFAULT_CALLBACK
#error "UMFELD_SET_DEFAULT_CALLBACK must be false"
#endif

#ifndef KLST_ENV
#error "need to define KLST_ENV"
#endif

/* --- emulator implementation --- */

#include "Umfeld.h"
// #include "KlangstromEmulator.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#ifndef WEAK
#define WEAK __attribute__((weak))
#endif

    extern void setup(void);
    extern void loop(void);

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

namespace umfeld_klst_emulator {
    void emulator_settings() {
        umfeld::size(1024, 768);
        umfeld::warning("-> starting emulator");
    }

    void emulator_setup() {
        setup();
    }

    void emulator_draw() {
        loop(); // TODO or better put this in loop?!?
    }
}

void umfeld_set_callbacks() {
    umfeld::set_settings_callback(umfeld_klst_emulator::emulator_settings);
    umfeld::set_setup_callback(umfeld_klst_emulator::emulator_setup);
    umfeld::set_draw_callback(umfeld_klst_emulator::emulator_draw);
//    umfeld::set_audioEvent_callback(audioEvent);
}

#endif
