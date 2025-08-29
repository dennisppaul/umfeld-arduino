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
#include "UShape.h"
#include "PGraphics.h"

namespace umfeld {
    class PShader;

    class UShapeRenderer {
    public:
        virtual ~UShapeRenderer()                                               = default;
        virtual void init(PGraphics* g, const std::vector<PShader*>& shader_programs) = 0; // NOTE init shaders + buffers
        // virtual void beginShape(ShapeMode        mode,
        //                         bool             filled,
        //                         bool             transparent,
        //                         uint32_t         texture_id,
        //                         const glm::mat4& model_transform_matrix)  = 0;
        // virtual void vertex(const Vertex& v)                              = 0;
        // virtual void setVertices(std::vector<Vertex>&& vertices)          = 0;
        // virtual void setVertices(const std::vector<Vertex>& vertices)     = 0;
        // virtual void endShape(bool closed)                                = 0;
        virtual void submit_shape(UShape& s)   = 0;
        // virtual int  texture_update_and_bind(PImage* img) = 0; // TODO move to PGraohicsOpenGL3 and rename to `prepare_texture()`?
        // virtual void set_custom_shader(PShader* shader) = 0;

        // NOTE `flush()` needs VP matrix and must be called to render batches at
        //      1. at end of frame
        //      2. before view or projection matrix are changed
        //      3. before downloading pixels from GPU
        //      4. before calls to `background()` ( or at least reject shapes? )
        virtual void flush(const glm::mat4& view_matrix, const glm::mat4& projection_matrix) = 0;

    protected:
        PGraphics* graphics{nullptr};
        std::vector<PShader*> default_shader_programs;
        // bool       shape_in_progress = false;
    };
} // namespace umfeld