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
#include "UShape.h"
#include "UShapeRenderer.h"
#include "PShader.h"
#include "PGraphics.h"
#include "UmfeldFunctionsGraphics.h"

namespace umfeld {


    class UShapeRendererOpenGL_3 final : public UShapeRenderer {
    public:
        static constexpr uint16_t SHADER_PROGRAM_COLOR          = 0;
        static constexpr uint16_t SHADER_PROGRAM_TEXTURE        = 1;
        static constexpr uint16_t SHADER_PROGRAM_COLOR_LIGHTS   = 2;
        static constexpr uint16_t SHADER_PROGRAM_TEXTURE_LIGHTS = 3;
        static constexpr uint16_t SHADER_PROGRAM_POINT          = 4; // TODO implement
        static constexpr uint16_t SHADER_PROGRAM_LINE           = 5; // TODO implement
        static constexpr uint16_t NUM_SHADER_PROGRAMS           = 6;
        static constexpr uint16_t FALLBACK_MODEL_MATRIX_ID      = 0;
        static constexpr uint16_t PER_VERTEX_TRANSFORM_ID_START = 1;

        static_assert(Vertex::DEFAULT_TRANSFORM_ID == FALLBACK_MODEL_MATRIX_ID);

        enum ShapeCenterComputeStrategy {
            ZERO_CENTER,
            AXIS_ALIGNED_BOUNDING_BOX,
            CENTER_OF_MASS,
        };

        ~UShapeRendererOpenGL_3() override {}
        void init(PGraphics* g, const std::vector<PShader*>& shader_programs) override;
        void submit_shape(UShape& s) override;
        void flush(const glm::mat4& view_matrix, const glm::mat4& projection_matrix) override;

    private:
        static constexpr int      DEFAULT_NUM_TEXTURES = 16;
        static constexpr uint32_t NO_SHADER_PROGRAM    = -1;
        static constexpr uint16_t MAX_TRANSFORMS       = 256;
        // NOTE check `GL_MAX_UNIFORM_BLOCK_SIZE`
        //      ```c
        //      GLuint maxUBOSize;
        //      glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &maxUBOSize);
        //      static constexpr uint16_t MAX_TRANSFORMS = std::min(1024, maxUBOSize / (int)sizeof(glm::mat4));
        //      ```

        struct TextureBatch {
            std::vector<UShape*> shapes;
            uint32_t             max_vertices{0};
            uint16_t             texture_id{TEXTURE_NONE};
        };

        struct FrameState {
            GLuint        cached_texture_id{UINT32_MAX};
            ShaderProgram cached_shader_program{.id = NO_SHADER_PROGRAM};
            bool          cached_transparent_shape_enabled{false};
            bool          cached_require_buffer_resize{false};
            uint32_t      cached_max_vertices_per_draw{0};
            // TODO add more states like blend, depth_write/test …

            void reset() {
                cached_texture_id                = UINT32_MAX;
                cached_shader_program            = {.id = NO_SHADER_PROGRAM};
                cached_transparent_shape_enabled = false;
                cached_require_buffer_resize     = false;
                cached_max_vertices_per_draw     = 0;
            }
        };

        GLuint                     vbo         = 0;
        GLuint                     ubo         = 0; // NOTE + FYI UBOs are supported in OpenGL ES 3.0+ and OpenGL 3.1+
        GLuint                     default_vao = 0;
        ShaderProgram              shader_color{};
        ShaderProgram              shader_texture{};
        ShaderProgram              shader_color_lights{};
        ShaderProgram              shader_texture_lights{};
        ShaderProgram              shader_point{}; // TODO implement
        ShaderProgram              shader_line{};  // TODO implement
        std::vector<UShape>        shapes;
        ShapeCenterComputeStrategy shape_center_compute_strategy = ZERO_CENTER;
        FrameState                 frame_state_cache{};
        int                        frame_light_shapes_count{0};       // NOTE set in 'submit_shape'
        int                        frame_transparent_shapes_count{0}; // NOTE set in 'submit_shape'
        int                        frame_opaque_shapes_count{0};      // NOTE set in 'submit_shape'
        int                        frame_textured_shapes_count{0};    // NOTE set in 'submit_shape'
        std::vector<Vertex>        current_vertex_buffer;

        void                 init_shaders(const std::vector<PShader*>& shader_programms);
        void                 init_buffers();
        void                 computeShapeCenter(UShape& s) const;
        static void          enable_depth_testing();
        static void          enable_blending();
        static void          disable_blending();
        static void          enable_depth_buffer_writing();
        static void          disable_depth_buffer_writing();
        static void          disable_depth_testing();
        void                 prepare_next_flush_frame();
        void                 print_frame_info(const std::vector<UShape>& processed_point_shapes, const std::vector<UShape>& processed_line_shapes, const std::vector<UShape>& processed_triangle_shapes) const;
        void                 bind_default_vertex_array() const;
        static void          unbind_default_vertex_array();
        void                 enable_flat_shaders_and_bind_texture(GLuint& current_shader_program_id, unsigned texture_id) const;
        void                 enable_light_shaders_and_bind_texture(GLuint& current_shader_program_id, unsigned texture_id) const;
        static void          setup_uniform_blocks(const std::string& shader_name, GLuint program);
        static bool          uniform_exists(const GLuint loc) { return loc != ShaderUniforms::NOT_FOUND; }
        void                 set_per_frame_default_shader_uniforms(const glm::mat4& view_projection_matrix, const glm::mat4& view_matrix, int frame_has_light_shapes, int frame_has_transparent_shapes, int frame_has_opaque_shapes) const;
        static void          set_light_uniforms(const ShaderUniforms& uniforms, const LightingState& lighting);
        const ShaderProgram& get_shader_program_cached() const;
        bool                 use_shader_program_cached(const ShaderProgram& required_shader_program);
        static bool          set_uniform_model_matrix(const UShape& shape, const ShaderProgram& shader_program);
        void                 set_point_size_and_line_width(const UShape& shape) const;
        static bool          uniform_available(const GLuint loc) { return loc != ShaderUniforms::UNINITIALIZED && loc != ShaderUniforms::NOT_FOUND; }
        void                 flush_sort_by_z_order(const std::vector<UShape>& point_shapes, const std::vector<UShape>& line_shapes, std::vector<UShape>& triangulated_shapes, const glm::mat4& view_matrix, const glm::mat4& projection_matrix);
        void                 flush_submission_order(const std::vector<UShape>& shapes, const glm::mat4& view_matrix, const glm::mat4& projection_matrix);
        void                 flush_immediately(const std::vector<UShape>& shapes, const glm::mat4& view_matrix, const glm::mat4& projection_matrix);
        void                 process_shapes_z_order(std::vector<UShape>& processed_point_shapes, std::vector<UShape>& processed_line_shapes, std::vector<UShape>& processed_triangle_shapes);
        void                 process_shapes_submission_order(std::vector<UShape>& processed_shapes);
        static void          convert_point_shape_to_triangles(std::vector<UShape>& processed_triangle_shapes, UShape& point_shape);
        static void          convert_point_shape_for_shader(std::vector<UShape>& processed_point_shapes, UShape& point_shape);
        void                 process_point_shape_z_order(std::vector<UShape>& processed_triangle_shapes, std::vector<UShape>& processed_point_shapes, UShape& point_shape) const;
        void                 process_point_shape_submission_order(std::vector<UShape>& processed_shape_batch, UShape& point_shape) const;
        void                 convert_stroke_shape_to_triangles_2D(std::vector<UShape>& processed_triangle_shapes, UShape& stroke_shape) const;
        static void          convert_stroke_shape_to_triangles_3D_tube(std::vector<UShape>& processed_triangle_shapes, UShape& stroke_shape);
        static void          convert_stroke_shape_for_native(UShape& stroke_shape);
        static void          process_stroke_shape_for_line_shader(const UShape& stroke_shape, std::vector<Vertex>& line_vertices);
        static void          convert_stroke_shape_for_line_shader(std::vector<UShape>& processed_line_shapes, UShape& stroke_shape);
        static void          convert_stroke_shape_for_barycentric_shader(std::vector<UShape>& processed_line_shapes, UShape& stroke_shape);
        static void          convert_stroke_shape_for_geometry_shader(std::vector<UShape>& processed_line_shapes, UShape& stroke_shape);
        void                 process_stroke_shapes_z_order(std::vector<UShape>& processed_triangle_shapes, std::vector<UShape>& processed_stroke_shapes, UShape& stroke_shape) const;
        static void          process_stroke_shape_for_native(std::vector<UShape>& processed_shape_batch, UShape& stroke_shape);
        void                 process_stroke_shapes_submission_order(std::vector<UShape>& processed_stroke_shapes, UShape& stroke_shape) const;
        static size_t        estimate_triangle_count(const UShape& s);
        static void          convert_shapes_to_triangles_and_set_transform_id(const UShape& s, std::vector<Vertex>& out, uint16_t transformID);
        void                 render_batch(const std::vector<UShape*>& shapes_to_render);
        void                 update_and_draw_vertex_buffer(const UShape& shape);
        void                 render_shape(const UShape& shape);
        static size_t               calculate_line_shader_vertex_count(const UShape& stroke_shape);
    };
} // namespace umfeld