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

#include <glm/glm.hpp>

namespace umfeld {
    class ShapeRendererBatch {
    public:
        virtual ~ShapeRendererBatch() = default;
        virtual void init()           = 0; // NOTE shaders + buffers
        // NOTE needs VP matrix. must be called to render batches at
        //      1. at end of frame
        //      2. before view or projection matrix are changed
        //      3. before downloading pixels from GPU
        virtual void flush(const glm::mat4& viewProj) = 0;
        virtual void beginShape(int shape)            = 0;
        virtual void vertex(const Vertex& v)          = 0;
        virtual void endShape(bool close_shape)       = 0;
    };
} // namespace umfeld