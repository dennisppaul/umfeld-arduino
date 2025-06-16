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

#include "PGraphicsOpenGL.h"
#include "VertexBuffer.h"

namespace umfeld {
    class PGraphicsOpenGL_3_3_core final : public PGraphicsOpenGL {
    public:
        explicit PGraphicsOpenGL_3_3_core(bool render_to_offscreen);

        /* --- OpenGL 3.3 specific implementation of shared methods --- */

        void IMPL_emit_shape_stroke_line_strip(std::vector<Vertex>& line_strip_vertices, bool line_strip_closed) override;
        void IMPL_emit_shape_fill_triangles(std::vector<Vertex>& triangle_vertices) override;
        void IMPL_emit_shape_stroke_points(std::vector<Vertex>& point_vertices, float point_size) override;

        void IMPL_background(float a, float b, float c, float d) override;
        void IMPL_bind_texture(int bind_texture_id) override;
        void IMPL_set_texture(PImage* img) override;

        void render_framebuffer_to_screen(bool use_blit = false) override;
        bool read_framebuffer(std::vector<unsigned char>& pixels) override;
        void store_fbo_state() override;
        void restore_fbo_state() override;
        void bind_fbo() override;
        void finish_fbo() override {}

        void upload_texture(PImage* img, const uint32_t* pixel_data, int width, int height, int offset_x, int offset_y, bool mipmapped) override;
        void download_texture(PImage* img) override;

        void beginDraw() override;
        void endDraw() override;

        void        init(uint32_t* pixels, int width, int height, bool generate_mipmap) override;
        std::string name() override {
#if defined(OPENGL_ES_3_0)
            return "PGraphicsOpenGL_ES_3_0";
#elif defined(OPENGL_3_3_CORE)
            return "PGraphicsOpenGL_3_3_core";
#else
            return "Unknown";
#endif
        }

        void hint(uint16_t property) override;
        void debug_text(const std::string& text, float x, float y) override; // TODO move to PGraphics ( use glBeginShape() )

        /* --- standard drawing functions --- */

        void     mesh(VertexBuffer* mesh_shape) override;
        void     shader(PShader* shader) override;
        PShader* loadShader(const std::string& vertex_code, const std::string& fragment_code, const std::string& geometry_code = "") override;
        void     resetShader() override;
        void     camera(float eyeX, float eyeY, float eyeZ, float centerX, float centerY, float centerZ, float upX, float upY, float upZ) override;
        void     frustum(float left, float right, float bottom, float top, float near, float far) override;
        void     ortho(float left, float right, float bottom, float top, float near, float far) override;
        void     perspective(float fovy, float aspect, float near, float far) override;
        void     lights() override;
        void     noLights() override;
        void     ambientLight(float r, float g, float b, float x = 0, float y = 0, float z = 0) override;
        void     directionalLight(float r, float g, float b, float nx, float ny, float nz) override;
        void     pointLight(float r, float g, float b, float x, float y, float z) override;
        void     spotLight(float r, float g, float b, float x, float y, float z, float nx, float ny, float nz, float angle, float concentration) override;
        void     lightFalloff(float constant, float linear, float quadratic) override;
        void     lightSpecular(float r, float g, float b) override;
        void     ambient(float r, float g, float b) override;
        void     specular(float r, float g, float b) override;
        void     emissive(float r, float g, float b) override;
        void     shininess(float s) override;

        PShader* shader_fill_texture{nullptr};
        PShader* shader_fill_texture_lights{nullptr};
        PShader* shader_stroke{nullptr};
        PShader* shader_point{nullptr};
        PShader* custom_shader{nullptr};

    private:
        struct RenderBatch {
            int    start_index;
            int    num_vertices;
            GLuint texture_id;

            RenderBatch(const int start, const int count, const GLuint texID)
                : start_index(start), num_vertices(count), texture_id(texID) {}
        };

        static constexpr bool    RENDER_POINT_AS_CIRCLE                 = true;
        static constexpr bool    RENDER_PRIMITVES_AS_SHAPES             = true;
        static constexpr uint8_t NUM_FILL_VERTEX_ATTRIBUTES_XYZ_RGBA_UV = 9;
        static constexpr uint8_t NUM_STROKE_VERTEX_ATTRIBUTES_XYZ_RGBA  = 7;
        GLuint                   texture_id_solid_color{};
        VertexBuffer             vertex_buffer{};
        std::vector<RenderBatch> renderBatches; // TODO revive for buffered mode
        GLint                    previously_bound_read_FBO = 0;
        GLint                    previously_bound_draw_FBO = 0;
        GLint                    previous_viewport[4]{};
        GLint                    previous_shader{0};

        /* --- lights --- */

        int                  lightCount = 0;
        static constexpr int MAX_LIGHTS = 8; // or whatever your PGL.MAX_LIGHTS equivalent is

        // Light type constants
        static constexpr int AMBIENT     = 0;
        static constexpr int DIRECTIONAL = 1;
        static constexpr int POINT       = 2;
        static constexpr int SPOT        = 3;

        // Light arrays
        int       lightType[MAX_LIGHTS]{};
        glm::vec4 lightPositions[MAX_LIGHTS]{};
        glm::vec3 lightNormals[MAX_LIGHTS]{};
        glm::vec3 lightAmbientColors[MAX_LIGHTS]{};
        glm::vec3 lightDiffuseColors[MAX_LIGHTS]{};
        glm::vec3 lightSpecularColors[MAX_LIGHTS]{};
        glm::vec3 lightFalloffCoeffs[MAX_LIGHTS]{};
        glm::vec2 lightSpotParams[MAX_LIGHTS]{};

        // Current light settings
        glm::vec3 currentLightSpecular         = glm::vec3(0.0f);
        float     currentLightFalloffConstant  = 1.0f;
        float     currentLightFalloffLinear    = 0.0f;
        float     currentLightFalloffQuadratic = 0.0f;

        void enableLighting();
        void setLightPosition(int num, float x, float y, float z, bool directional);
        void setLightNormal(int num, float dx, float dy, float dz);
        void setLightAmbient(int num, float r, float g, float b);
        void setNoLightAmbient(int num);
        void setLightDiffuse(int num, float r, float g, float b);
        void setNoLightDiffuse(int num);
        void setLightSpecular(int num, float r, float g, float b);
        void setNoLightSpecular(int num);
        void setLightFalloff(int num, float constant, float linear, float quadratic);
        void setNoLightFalloff(int num);
        void setLightSpot(int num, float angle, float concentration);
        void setNoLightSpot(int num);
        void updateShaderLighting() const;

        /* --- OpenGL 3.3 specific methods --- */

        // void        OGL3_tranform_model_matrix_and_render_vertex_buffer(VertexBuffer& vertex_buffer, GLenum primitive_mode, const std::vector<Vertex>& shape_vertices) const;
        static void OGL3_render_vertex_buffer(VertexBuffer& vertex_buffer, GLenum primitive_mode, const std::vector<Vertex>& shape_vertices);
        void        OGL3_create_solid_color_texture();
        void        update_all_shader_matrices() const;
        void        update_shader_matrices(PShader* shader) const;
        static void reset_shader_matrices(PShader* shader);
        static void add_line_quad(const Vertex& p0, const Vertex& p1, float thickness, std::vector<Vertex>& out);
    };
} // namespace umfeld
