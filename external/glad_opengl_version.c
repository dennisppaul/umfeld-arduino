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

#if defined(OPENGL_2_0)
#include "glad_opengl_20/src/gl.c.keep"
#elif defined(OPENGL_ES_3_0)
#include "glad_opengles_30/src/gl.c.keep"
#elif defined(OPENGL_3_3_CORE)
#include "glad_opengl_33/src/gl.c.keep"
#else
#warning "OpenGL version not defined. for GLAD to work it must be either OPENGL_2_0, OPENGL_ES_3_0 or OPENGL_3_3_CORE."
#endif
