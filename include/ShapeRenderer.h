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

#include <vector>
#include <glm/glm.hpp>
#include "UmfeldConstants.h"
#include "Vertex.h"
#include "Shape.h"
#include "PGraphics.h"

namespace umfeld {
    class ShapeRenderer {
    public:
        virtual ~ShapeRenderer()                                          = default;
        virtual void init(PGraphics* g, std::vector<int> shader_programs) = 0; // NOTE init shaders + buffers
        virtual void beginShape(ShapeMode        mode,
                                bool             filled,
                                bool             transparent,
                                uint32_t         texture_id,
                                const glm::mat4& model_transform_matrix)  = 0;
        virtual void vertex(const Vertex& v)                              = 0;
        virtual void setVertices(std::vector<Vertex>&& vertices)          = 0;
        virtual void setVertices(const std::vector<Vertex>& vertices)     = 0;
        virtual void endShape(bool closed)                                = 0;
        virtual void submitShape(Shape& s)                  = 0;
        virtual void flush(const glm::mat4& view_projection_matrix)       = 0;
        // NOTE `flush()` needs VP matrix and must be called to render batches at
        //      1. at end of frame
        //      2. before view or projection matrix are changed
        //      3. before downloading pixels from GPU

        bool enable_lighting{false};
    protected:
        PGraphics* graphics{nullptr};
    };
} // namespace umfeld