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

#include "UmfeldSDLOpenGL.h"
#include "ShapeRenderer.h"
#include "Shape.h"
#include "PGraphics.h"

namespace umfeld {
    class ShapeRendererOpenGL_3 final : public ShapeRenderer {
    public:
        static constexpr uint16_t SHADER_PROGRAM_COLOR          = 0;
        static constexpr uint16_t SHADER_PROGRAM_TEXTURE        = 1;
        static constexpr uint16_t SHADER_PROGRAM_COLOR_LIGHTS   = 2;
        static constexpr uint16_t SHADER_PROGRAM_TEXTURE_LIGHTS = 3;
        static constexpr uint16_t SHADER_PROGRAM_POINT          = 4; // TODO implement
        static constexpr uint16_t SHADER_PROGRAM_LINE           = 5; // TODO implement
        static constexpr uint16_t NUM_SHADER_PROGRAMS           = 6;

        enum SHAPE_CENTER_COMPUTE_STRATEGY {
            ZERO_CENTER,
            AXIS_ALIGNED_BOUNDING_BOX,
            CENTER_OF_MASS,
        };

        ~ShapeRendererOpenGL_3() override {}
        void init(PGraphics* g, std::vector<int> shader_programs) override;
        void beginShape(ShapeMode mode, bool filled, bool transparent, uint32_t texture_id, const glm::mat4& model_transform_matrix) override;
        void vertex(const Vertex& v) override;
        void setVertices(std::vector<Vertex>&& vertices) override;
        void setVertices(const std::vector<Vertex>& vertices) override;
        void endShape(bool closed) override;
        void submitShape(Shape& s) override;
        int  set_texture(PImage* img) override;
        void flush(const glm::mat4& view_projection_matrix) override;
        void handle_point_shape(std::vector<Shape>& processed_triangle_shapes, std::vector<Shape>& processed_point_shapes, Shape& point_shape) const;
        void handle_stroke_shape(std::vector<Shape>& processed_triangle_shapes, std::vector<Shape>& processed_line_shapes, Shape& stroke_shape) const;

    private:
        static constexpr uint16_t MAX_TRANSFORMS = 256;
        // NOTE check `GL_MAX_UNIFORM_BLOCK_SIZE`
        //      ```c
        //      GLint maxUBOSize;
        //      glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &maxUBOSize);
        //      static constexpr uint16_t MAX_TRANSFORMS = std::min(1024, maxUBOSize / (int)sizeof(glm::mat4));
        //      ```

        struct TextureBatch {
            uint16_t            texture_id{TEXTURE_NONE};
            std::vector<Shape*> opaque_shapes;
            std::vector<Shape*> transparent_shapes;
        };

        // cached uniform locations
        struct ShaderUniforms {
            enum UNIFORM_LOCATION_STATE {
                UNINITIALIZED = -2,
                NOT_FOUND     = -1, // NOTE result delivered by OpenGL s `glGetUniformLocation()`
                INITIALIZED   = 0,
            };
            GLint uViewProj = UNINITIALIZED;
            GLint uTexture  = UNINITIALIZED;
        };

        // TODO implement for lighting, point and line shader
        ShaderUniforms                texturedUniforms;
        ShaderUniforms                untexturedUniforms;
        GLuint                        vbo                             = 0;
        GLuint                        ubo                             = 0;
        GLuint                        vao                             = 0;
        GLuint                        shader_programm_texture           = 0;
        GLuint                        shader_programm_color         = 0;
        GLuint                        shader_programm_texture_lights   = 0; // TODO implement
        GLuint                        shader_programm_color_lights = 0; // TODO implement
        GLuint                        pointShaderProgram              = 0; // TODO implement
        GLuint                        lineShaderProgram               = 0; // TODO implement
        std::vector<Shape>            shapes;
        Shape                         currentShape;
        SHAPE_CENTER_COMPUTE_STRATEGY shape_center_compute_strategy = ZERO_CENTER;
        std::vector<Vertex>           frame_vertices;
        std::vector<glm::mat4>        frame_matrices;

        //        static GLuint compileShader(const char* src, GLenum type);
        //        static GLuint createShaderProgram(const char* vsSrc, const char* fsSrc);
        static void   setupUniformBlocks(GLuint program);
        static bool   evaluate_shader_uniforms(const std::string& shader_name, const ShaderUniforms& uniforms);
        void          initShaders(const std::vector<int>& shader_programm_id);
        void          initBuffers();
        static size_t estimate_triangle_count(const Shape& s);
        static void   convert_shapes_to_triangles(const Shape& s, std::vector<Vertex>& out, uint16_t transformID);
        void          render_batch(const std::vector<Shape*>& shapes_to_render, const glm::mat4& view_projection_matrix, GLuint texture_id);
        void          computeShapeCenter(Shape& s) const;
        void          flush_sort_by_z_order(std::vector<Shape>& shapes, const glm::mat4& view_projection_matrix);
        void          flush_submission_order(std::vector<Shape>& shapes, const glm::mat4& view_projection_matrix);
        void          flush_immediately(std::vector<Shape>& shapes, const glm::mat4& view_projection_matrix);
        void          flush_processed_shapes(std::vector<Shape>& processed_point_shapes, std::vector<Shape>& processed_line_shapes, std::vector<Shape>& processed_triangle_shapes, const glm::mat4& view_projection_matrix);
        void          process_shapes(std::vector<Shape>& processed_point_shapes, std::vector<Shape>& processed_line_shapes, std::vector<Shape>& processed_triangle_shapes);

        static void bind_texture(const int texture_id) {
            glActiveTexture(GL_TEXTURE0 + PGraphicsOpenGL::DEFAULT_ACTIVE_TEXTURE_UNIT);
            glBindTexture(GL_TEXTURE_2D, texture_id); // NOTE this should be the only glBindTexture ( except for initializations )
        }
    };
} // namespace umfeld