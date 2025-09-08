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

#pragma once

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>

#include VARIANT_H

/* sketch */

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

// Note `main()` is handled via `SDL_MAIN_USE_CALLBACKS` define in platform.txt