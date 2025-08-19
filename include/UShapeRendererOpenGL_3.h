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
#include "UShapeRenderer.h"
#include "UShape.h"
#include "PShader.h"
#include "PGraphics.h"
#include "UmfeldFunctionsGraphics.h"

namespace umfeld {
    // cached uniform locations
    struct ShaderUniforms {
        enum UNIFORM_LOCATION_STATE : GLuint {
            UNINITIALIZED = static_cast<GLuint>(-2),
            NOT_FOUND     = GL_INVALID_INDEX, // NOTE result delivered by OpenGL s `glGetUniformLocation()`
            INITIALIZED   = 0,                // NOTE `0` is the first valid value
        };
        GLuint uViewProj = UNINITIALIZED;
        GLuint uTexture  = UNINITIALIZED;
        // lighting uniforms
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

    class UShapeRendererOpenGL_3 final : public UShapeRenderer {
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

        ~UShapeRendererOpenGL_3() override {}
        void init(PGraphics* g, std::vector<PShader*> shader_programs) override;
        void submit_shape(UShape& s) override;
        void handle_point_shape(std::vector<UShape>& processed_triangle_shapes, std::vector<UShape>& processed_point_shapes, UShape& point_shape) const;
        void handle_stroke_shape(std::vector<UShape>& processed_triangle_shapes, std::vector<UShape>& processed_line_shapes, UShape& stroke_shape) const;
        // int  texture_update_and_bind(PImage* img) override;
        // void set_custom_shader(PShader* shader) override { custom_shader = shader; }
        void flush(const glm::mat4& view_matrix, const glm::mat4& projection_matrix) override;

    private:
        static constexpr int      DEFAULT_NUM_TEXTURES = 64;
        static constexpr uint32_t NO_SHADER_PROGRAM    = -1;
        static constexpr uint16_t MAX_TRANSFORMS       = 256;
        // NOTE check `GL_MAX_UNIFORM_BLOCK_SIZE`
        //      ```c
        //      GLuint maxUBOSize;
        //      glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &maxUBOSize);
        //      static constexpr uint16_t MAX_TRANSFORMS = std::min(1024, maxUBOSize / (int)sizeof(glm::mat4));
        //      ```

        struct TextureBatch {
            std::vector<UShape*> opaque_shapes;
            std::vector<UShape*> transparent_shapes;
            std::vector<UShape*> light_shapes;
            uint32_t            max_vertices{0};
            uint16_t            texture_id{TEXTURE_NONE};
        };

        ShaderUniforms             shader_uniforms_color;
        ShaderUniforms             shader_uniforms_texture;
        ShaderUniforms             shader_uniforms_color_lights;
        ShaderUniforms             shader_uniforms_texture_lights;
        ShaderUniforms             shader_uniforms_point; // TODO implement
        ShaderUniforms             shader_uniforms_line;  // TODO implement
        GLuint                     vbo                            = 0;
        GLuint                     ubo                            = 0; // NOTE FYI UBOs are supported in OpenGL ES 3.0+ and OpenGL 3.1+
        GLuint                     default_vao                    = 0;
        GLuint                     shader_programm_texture        = 0;
        GLuint                     shader_programm_color          = 0;
        GLuint                     shader_programm_texture_lights = 0;
        GLuint                     shader_programm_color_lights   = 0;
        GLuint                     point_shader_program           = 0; // TODO implement
        GLuint                     line_shader_program            = 0; // TODO implement
        std::vector<UShape>         shapes;
        ShapeCenterComputeStrategy shape_center_compute_strategy = ZERO_CENTER;
        std::vector<Vertex>        flush_frame_vertices;
        std::vector<glm::mat4>     flush_frame_matrices;
        uint32_t                   max_vertices_per_batch{0};
        bool                       initialized_vbo_buffer{false};
        // PShader*                   custom_shader{nullptr};
        int                        frame_light_shapes_count{0};
        int                        frame_transparent_shapes_count{0};
        int                        frame_opaque_shapes_count{0};

        static void   setup_uniform_blocks(const std::string& shader_name, GLuint program);
        static bool   evaluate_shader_uniforms(const std::string& shader_name, const ShaderUniforms& uniforms);
        void          init_shaders(const std::vector<PShader*>& shader_programms);
        void          init_buffers();
        static size_t estimate_triangle_count(const UShape& s);
        static void   convert_shapes_to_triangles(const UShape& s, std::vector<Vertex>& out, uint16_t transformID);
        void          render_batch(const std::vector<UShape*>& shapes_to_render);
        void          render_shape(const UShape& s);
        void          computeShapeCenter(UShape& s) const;
        void          enable_depth_testing() const;
        static void   disable_depth_testing();
        void          flush_sort_by_z_order(std::vector<UShape>& shapes, const glm::mat4& view_matrix, const glm::mat4& projection_matrix);
        void          flush_submission_order(std::vector<UShape>& shapes, const glm::mat4& view_matrix, const glm::mat4& projection_matrix);
        void          flush_immediately(std::vector<UShape>& shapes, const glm::mat4& view_matrix, const glm::mat4& projection_matrix);
        void          flush_processed_shapes(const std::vector<UShape>& processed_point_shapes, const std::vector<UShape>& processed_line_shapes, std::vector<UShape>& processed_triangle_shapes, const glm::mat4& view_matrix, const glm::mat4& projection_matrix);
        void          reset_flush_frame();
        void          print_frame_info(const std::vector<UShape>& processed_point_shapes, const std::vector<UShape>& processed_line_shapes, const std::vector<UShape>& processed_triangle_shapes) const;
        void          process_shapes(std::vector<UShape>& processed_point_shapes, std::vector<UShape>& processed_line_shapes, std::vector<UShape>& processed_triangle_shapes);
        void          set_per_frame_shader_uniforms(const glm::mat4& view_projection_matrix, int frame_has_light_shapes, int frame_has_transparent_shapes, int frame_has_opaque_shapes) const;
        void          enable_flat_shaders_and_bind_texture(GLuint& current_shader_program_id, unsigned texture_id) const;
        void          enable_light_shaders_and_bind_texture(GLuint& current_shader_program_id, unsigned texture_id) const;
        void          bind_default_vertex_buffer() const;
        static void          unbind_default_vertex_buffer();
        static bool   uniform_exists(const GLuint loc) { return loc != ShaderUniforms::NOT_FOUND; }
        static void   set_light_uniforms(const ShaderUniforms& uniforms, const LightingState& lighting);
    };
} // namespace umfeld