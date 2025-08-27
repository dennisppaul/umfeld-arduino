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

#include "UShapeRendererOpenGL_3.h"


#include <string>
#include <unordered_map>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "UmfeldTypes.h"

namespace umfeld {
    class PGraphics;
    struct UShape;

    // ReSharper disable once CppClassCanBeFinal
    class PShader {
    public:
        PShader();
        virtual ~PShader();

        virtual void init_uniforms();
        virtual void update_uniforms(const glm::mat4& model_matrix, const glm::mat4& view_matrix, const glm::mat4& projection_matrix, const uint32_t texture_unit);

        void     set_uniform(const std::string& name, int value);
        void     set_uniform(const std::string& name, int value_a, int value_b);
        void     set_uniform(const std::string& name, float value);
        void     set_uniform(const std::string& name, float value_a, float value_b);
        void     set_uniform(const std::string& name, const glm::vec2& value);
        void     set_uniform(const std::string& name, const glm::vec3& value);
        void     set_uniform(const std::string& name, const glm::vec4& value);
        void     set_uniform(const std::string& name, const glm::mat3& value);
        void     set_uniform(const std::string& name, const glm::mat4& value);
        void     check_uniform_location(const std::string& name) const;
        void     set_auto_update_uniforms(const bool value) { auto_update_uniforms = value; }
        bool     load(const std::string& vertex_code, const std::string& fragment_code, const std::string& geometry_code = "");
        void     use();
        void     unuse();
        uint32_t get_program_id() const { return program.id; }
        bool     is_bound() const { return in_use; }

        bool debug_uniform_location = true;

    private:
        std::unordered_map<std::string, int32_t> uniformLocations;
        bool                                     in_use{false};
        ShaderProgram                            program;
        bool                                     auto_update_uniforms{true};

        int32_t getUniformLocation(const std::string& name);
    };

} // namespace umfeld
