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
        /**
         * draw all submitted shapes. `flush()` must be called at the end of each frame
         * and by default does so automatically. this behavior can be controlled with
         * `PGraphics::setAutoFlushShapes()`.
         *
         * 1. at end of a frame
         * 2. before view or projection matrix are changed ( e.g in `camera()` )
         * 3. before downloading pixels from GPU
         * 4. before calls to `background()` ( or at least clear shape buffer )
         * 5. before changing render modes
         *
         * @param view_matrix
         * @param projection_matrix
         */
        virtual void flush(const glm::mat4& view_matrix, const glm::mat4& projection_matrix) = 0;
        virtual ~UShapeRenderer()                                                            = default;
        virtual void init(PGraphics* g, const std::vector<PShader*>& shader_programs)        = 0; // NOTE init shaders + buffers
        virtual void submit_shape(UShape& shape)                                             = 0;
        virtual void set_shader_program(PShader* shader, ShaderProgramType shader_role)      = 0;


    protected:
        PGraphics*            graphics{nullptr};
        std::vector<PShader*> default_shader_programs;
    };
} // namespace umfeld