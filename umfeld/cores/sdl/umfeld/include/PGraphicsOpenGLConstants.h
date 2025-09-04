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

#include "UmfeldSDLOpenGL.h" // TODO move to cpp implementation

namespace umfeld {
    static constexpr GLint UMFELD_DEFAULT_TEXTURE_PIXEL_TYPE    = GL_UNSIGNED_BYTE;
    static constexpr GLint UMFELD_DEFAULT_INTERNAL_PIXEL_FORMAT = GL_RGBA8;
    static constexpr GLint UMFELD_DEFAULT_EXTERNAL_PIXEL_FORMAT = GL_RGBA;
} // namespace umfeld