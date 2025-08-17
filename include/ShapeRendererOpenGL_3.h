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
#include "PShader.h"
#include "PGraphics.h"
#include "UmfeldFunctionsGraphics.h"

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

        enum ShapeCenterComputeStrategy {
            ZERO_CENTER,
            AXIS_ALIGNED_BOUNDING_BOX,
            CENTER_OF_MASS,
        };

        ~ShapeRendererOpenGL_3() override {}
        void init(PGraphics* g, std::vector<int> shader_programs) override;
        // void beginShape(ShapeMode mode, bool filled, bool transparent, uint32_t texture_id, const glm::mat4& model_transform_matrix) override;
        // void vertex(const Vertex& v) override;
        // void setVertices(std::vector<Vertex>&& vertices) override;
        // void setVertices(const std::vector<Vertex>& vertices) override;
        // void endShape(bool closed) override;
        void submitShape(Shape& s) override;
        int  set_texture(PImage* img) override;
        void flush(const glm::mat4& view_matrix, const glm::mat4& projection_matrix) override;
        void handle_point_shape(std::vector<Shape>& processed_triangle_shapes, std::vector<Shape>& processed_point_shapes, Shape& point_shape) const;
        void handle_stroke_shape(std::vector<Shape>& processed_triangle_shapes, std::vector<Shape>& processed_line_shapes, Shape& stroke_shape) const;
        void set_custom_shader(PShader* shader) override { custom_shader = shader; }

    private:
        static constexpr uint32_t NO_SHADER_PROGRAM = -1;
        static constexpr uint16_t MAX_TRANSFORMS    = 256;
        // NOTE check `GL_MAX_UNIFORM_BLOCK_SIZE`
        //      ```c
        //      GLuint maxUBOSize;
        //      glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &maxUBOSize);
        //      static constexpr uint16_t MAX_TRANSFORMS = std::min(1024, maxUBOSize / (int)sizeof(glm::mat4));
        //      ```

        struct TextureBatch {
            std::vector<Shape*> opaque_shapes;
            std::vector<Shape*> transparent_shapes;
            std::vector<Shape*> light_shapes;
            uint32_t            max_vertices{0};
            uint16_t            texture_id{TEXTURE_NONE};
        };

        // cached uniform locations
        struct ShaderUniforms {
            enum UNIFORM_LOCATION_STATE : GLuint {
                UNINITIALIZED = static_cast<GLuint>(-2),
                NOT_FOUND     = GL_INVALID_INDEX, // NOTE result delivered by OpenGL s `glGetUniformLocation()`
                INITIALIZED   = 0,                // NOTE `0` is the first valid value
            };
            GLuint uViewProj = UNINITIALIZED;
            GLuint uTexture  = UNINITIALIZED;

            // Lighting uniforms
            GLuint uView         = UNINITIALIZED;
            GLuint normalMatrix  = UNINITIALIZED;
            GLuint ambient       = UNINITIALIZED;
            GLuint specular      = UNINITIALIZED;
            GLuint emissive      = UNINITIALIZED;
            GLuint shininess     = UNINITIALIZED;
            GLuint lightCount    = UNINITIALIZED;
            GLuint lightPosition = UNINITIALIZED;
            GLuint lightNormal   = UNINITIALIZED;
            GLuint lightAmbient  = UNINITIALIZED;
            GLuint lightDiffuse  = UNINITIALIZED;
            GLuint lightSpecular = UNINITIALIZED;
            GLuint lightFalloff  = UNINITIALIZED;
            GLuint lightSpot     = UNINITIALIZED;
        };

        ShaderUniforms     shader_uniforms_color;
        ShaderUniforms     shader_uniforms_texture;
        ShaderUniforms     shader_uniforms_color_lights;
        ShaderUniforms     shader_uniforms_texture_lights;
        ShaderUniforms     shader_uniforms_point; // TODO implement
        ShaderUniforms     shader_uniforms_line;  // TODO implement
        GLuint             vbo                            = 0;
        GLuint             ubo                            = 0;
        GLuint             vao                            = 0;
        GLuint             shader_programm_texture        = 0;
        GLuint             shader_programm_color          = 0;
        GLuint             shader_programm_texture_lights = 0;
        GLuint             shader_programm_color_lights   = 0;
        GLuint             point_shader_program           = 0; // TODO implement
        GLuint             line_shader_program            = 0; // TODO implement
        std::vector<Shape> shapes;
        // Shape                      current_shape;
        ShapeCenterComputeStrategy shape_center_compute_strategy = ZERO_CENTER;
        std::vector<Vertex>        flush_frame_vertices;
        std::vector<glm::mat4>     flush_frame_matrices;
        uint32_t                   max_vertices_per_batch{0};
        bool                       initialized_vbo_buffer{false};
        PShader*                   custom_shader{nullptr};
        int                        frame_light_shapes_count{0};
        int                        frame_transparent_shapes_count{0};
        int                        frame_opaque_shapes_count{0};

        static void   setup_uniform_blocks(const std::string& shader_name, GLuint program);
        static bool   evaluate_shader_uniforms(const std::string& shader_name, const ShaderUniforms& uniforms);
        void          initShaders(const std::vector<int>& shader_programm_id);
        void          initBuffers();
        static size_t estimate_triangle_count(const Shape& s);
        static void   convert_shapes_to_triangles(const Shape& s, std::vector<Vertex>& out, uint16_t transformID);
        void          render_batch(const std::vector<Shape*>& shapes_to_render);
        void          computeShapeCenter(Shape& s) const;
        void          enable_depth_testing() const;
        static void   disable_depth_testing();
        void          flush_sort_by_z_order(std::vector<Shape>& shapes,
                                            const glm::mat4&    view_matrix,
                                            const glm::mat4&    projection_matrix);
        void          flush_submission_order(std::vector<Shape>& shapes,
                                             const glm::mat4&    view_matrix,
                                             const glm::mat4&    projection_matrix);
        void          flush_immediately(std::vector<Shape>& shapes,
                                        const glm::mat4&    view_matrix,
                                        const glm::mat4&    projection_matrix);
        void          flush_processed_shapes(const std::vector<Shape>& processed_point_shapes, const std::vector<Shape>& processed_line_shapes, std::vector<Shape>& processed_triangle_shapes, const glm::mat4& view_matrix, const glm::mat4& projection_matrix);
        void          reset_flush_frame();
        void          process_shapes(std::vector<Shape>& processed_point_shapes, std::vector<Shape>& processed_line_shapes, std::vector<Shape>& processed_triangle_shapes);
        static void   updateShaderLighting();
        void          set_per_frame_shader_uniforms(const glm::mat4& view_projection_matrix, int frame_has_light_shapes, int frame_has_transparent_shapes, int frame_has_opaque_shapes) const;
        void          enable_default_shader(unsigned texture_id) const;
        void          enable_light_shader(unsigned texture_id) const;
        static bool   uniform_exists(const GLuint loc) { return loc != ShaderUniforms::NOT_FOUND; }
        static void   set_light_uniforms(const ShaderUniforms& uniforms, const LightingState& lighting);

        static void bind_texture(const int texture_id) {
            // TODO check if `glActiveTexture` needs to be done every time
            glActiveTexture(GL_TEXTURE0 + PGraphicsOpenGL::DEFAULT_ACTIVE_TEXTURE_UNIT);
            glBindTexture(GL_TEXTURE_2D, texture_id); // NOTE this should be the only glBindTexture ( except for initializations )
        }
    };
} // namespace umfeld