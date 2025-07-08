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

#if defined(OPENGL_3_3_CORE) || defined(OPENGL_ES_3_0)

#include <vector>

#include "UmfeldSDLOpenGL.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Umfeld.h"
#include "PGraphicsOpenGL.h"
#include "PGraphicsOpenGL_3_3_core.h"
#include "Vertex.h"
#include "Geometry.h"
#include "VertexBuffer.h"
#include "PShader.h"
#include "ShaderSourceColorTexture.h"
#include "ShaderSourceColorTextureLights.h"
#include "ShaderSourceLine.h"
#include "ShaderSourcePoint.h"

#ifdef UMFELD_PGRAPHICS_OPENGL_3_3_CORE_ERRORS
#define UMFELD_PGRAPHICS_OPENGL_3_3_CORE_CHECK_ERRORS(msg) \
    do {                                                   \
        checkOpenGLError(msg);                             \
    } while (0)
#else
#define UMFELD_PGRAPHICS_OPENGL_3_3_CORE_CHECK_ERRORS(msg)
#endif


using namespace umfeld;

PGraphicsOpenGL_3_3_core::PGraphicsOpenGL_3_3_core(const bool render_to_offscreen) : PImage(0, 0) {
    this->render_to_offscreen = render_to_offscreen;
}

void PGraphicsOpenGL_3_3_core::IMPL_background(const float a, const float b, const float c, const float d) {
    glClearColor(a, b, c, d);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void PGraphicsOpenGL_3_3_core::IMPL_bind_texture(const int bind_texture_id) {
    if (bind_texture_id != texture_id_current) {
        texture_id_current = bind_texture_id;
        glBindTexture(GL_TEXTURE_2D, texture_id_current); // NOTE this should be the only glBindTexture ( except for initializations )
    }
}

void PGraphicsOpenGL_3_3_core::IMPL_set_texture(PImage* img) {
    if (img == nullptr) {
        IMPL_bind_texture(texture_id_solid_color);
        return;
    }
    // NOTE identical >>>
    if (shape_has_begun) {
        console("`texture()` can only be called right before `beginShape(...)`. ( note, this is different from the original processing )");
        return;
    }

    // TODO move this to own method and share with `texture()`
    // TODO make MIPMAP optional
    if (img->texture_id == TEXTURE_NOT_GENERATED) {
        OGL_generate_and_upload_image_as_texture(img, true);
        if (img->texture_id == TEXTURE_NOT_GENERATED) {
            error("image cannot create texture.");
            return;
        }
    }

    IMPL_bind_texture(img->texture_id);
    // TODO so this is interesting: we could leave the texture bound and require the client
    //      to unbind it with `texture_unbind()` or should `endShape()` always reset to
    //      `texture_id_solid_color` with `texture_unbind()`.
    // NOTE identical <<<
}

void PGraphicsOpenGL_3_3_core::add_line_quad(const Vertex& p0, const Vertex& p1, float thickness, std::vector<Vertex>& out) {
    // glm::vec3 dir = glm::normalize(p1 - p0);
    glm::vec3 dir = p1.position - p0.position; // NOTE no need to noralize, the shader will do it

    // The shader will use this direction to compute screen-space offset
    glm::aligned_vec4 normal(dir, thickness);
    // glm::aligned_vec4 color(0.5f, 0.85f, 1.0f, 1.0f); // or pass in if needed

    // glm::aligned_vec4 pos0(p0.position);
    // glm::aligned_vec4 pos1(p1.position);

    // These 6 vertices form two triangles of the line quad
    Vertex v0, v1, v2, v3;

    v0.position = p0.position;
    v1.position = p1.position;
    v2.position = p0.position;
    v3.position = p1.position;

    v0.normal = normal;
    v1.normal = normal;
    normal.w  = -thickness;
    v2.normal = normal;
    v3.normal = normal;

    v0.color = p0.color;
    v1.color = p1.color;
    v2.color = p0.color;
    v3.color = p1.color;

    // Add first triangle
    out.push_back(v0);
    out.push_back(v1);
    out.push_back(v2);

    // Add second triangle
    out.push_back(v2);
    out.push_back(v1);
    out.push_back(v3);
}

/**
 * implement this method for respective renderer e.g
 *
 * - OpenGL_3_3_core + OpenGL_ES_3_0 ( shader based, buffered mode, vertex array objects )
 * - OpenGL_2_0 ( fixed function pipeline, immediate mode, vertex buffer arrays )
 * - SDL2
 *
 * and maybe later vulkan, metal, etc.
 *
 * @param line_strip_vertices
 * @param line_strip_closed
 */
void PGraphicsOpenGL_3_3_core::IMPL_emit_shape_stroke_line_strip(std::vector<Vertex>& line_strip_vertices, const bool line_strip_closed) {
    // NOTE relevant information for this method
    //     - closed
    //     - stroke_weight
    //     - stroke_join
    //     - stroke_cap
    //     - (shader_id)
    //     - (texture_id)

    // NOTE this is a very central method! up until here everything should have been done in generic PGraphics.
    //      - vertices are in model space
    //      - vertices are in line strip channels ( i.e not triangulated or anything yet )
    //      - decide on rendering mode ( triangulated, native, etcetera )
    //      - this method is usually accessed from `endShape()`

    // TODO maybe add stroke recorder here ( need to transform vertices to world space )

    if (render_mode == RENDER_MODE_BUFFERED) {
        if (stroke_render_mode == STROKE_RENDER_MODE_TRIANGULATE_2D) {
            std::vector<Vertex> line_vertices;
            triangulate_line_strip_vertex(line_strip_vertices,
                                          line_strip_closed,
                                          line_vertices);
            // TODO collect `line_vertices` and render as `GL_TRIANGLES` at end of frame
        }
        if (stroke_render_mode == STROKE_RENDER_MODE_NATIVE) {
            // TODO collect `line_strip_vertices` and render as `GL_LINE_STRIP` at end of frame
        }
    }

    if (render_mode == RENDER_MODE_IMMEDIATE) {
        // TODO add other render modes:
        //      - STROKE_RENDER_MODE_TUBE_3D
        //      - STROKE_RENDER_MODE_BARYCENTRIC_SHADER
        //      - STROKE_RENDER_MODE_GEOMETRY_SHADER
        if (stroke_render_mode == STROKE_RENDER_MODE_TRIANGULATE_2D) {

            std::vector<Vertex> line_vertices;
            triangulate_line_strip_vertex(line_strip_vertices,
                                          line_strip_closed,
                                          line_vertices);
            if (custom_shader != nullptr) {
                UMFELD_EMIT_WARNING("strokes with render mode 'STROKE_RENDER_MODE_TRIANGULATE_2D' are not supported with custom shaders");
            }
            // NOTE not happy about this hack … but `triangulate_line_strip_vertex` already applies model matrix
            shader_fill_texture->use();
            shader_fill_texture->set_uniform(SHADER_UNIFORM_MODEL_MATRIX, glm::mat4(1.0f));
            OGL3_render_vertex_buffer(vertex_buffer, GL_TRIANGLES, line_vertices);
        }
        if (stroke_render_mode == STROKE_RENDER_MODE_NATIVE) {
            shader_fill_texture->use();
            OGL3_render_vertex_buffer(vertex_buffer, GL_LINE_STRIP, line_strip_vertices);
        }
        if (stroke_render_mode == STROKE_RENDER_MODE_TUBE_3D) {
            const std::vector<Vertex> line_vertices = generate_tube_mesh(line_strip_vertices,
                                                                         stroke_weight / 2.0f,
                                                                         line_strip_closed,
                                                                         color_stroke);
            shader_fill_texture->use();
            OGL3_render_vertex_buffer(vertex_buffer, GL_TRIANGLES, line_vertices);
        }
        if (stroke_render_mode == STROKE_RENDER_MODE_GEOMETRY_SHADER) {
            static bool emit_warning = true;
            if (emit_warning) {
                emit_warning = false;
                warning("STROKE_RENDER_MODE_GEOMETRY_SHADER is not implemented yet.");
            }
        }
        if (stroke_render_mode == STROKE_RENDER_MODE_LINE_SHADER) {
            // TODO this MUST be optimized! it is not efficient to update all uniforms every time
            shader_stroke->use();
            update_shader_matrices(shader_stroke);
            shader_stroke->set_uniform("viewport", glm::vec4(0, 0, width, height));
            shader_stroke->set_uniform("perspective", 1);
            constexpr float scale_factor = 0.99f;
            shader_stroke->set_uniform("scale", glm::vec3(scale_factor, scale_factor, scale_factor));

            const float         stroke_weight_half = stroke_weight / 2.0f;
            std::vector<Vertex> line_strip_vertices_expanded;
            for (size_t i = 0; i + 1 < line_strip_vertices.size(); i++) {
                Vertex p0 = line_strip_vertices[i];
                Vertex p1 = line_strip_vertices[i + 1];
                add_line_quad(p0, p1, stroke_weight_half, line_strip_vertices_expanded);
            }
            if (line_strip_closed) {
                Vertex p0 = line_strip_vertices.back();
                Vertex p1 = line_strip_vertices.front();
                add_line_quad(p0, p1, stroke_weight_half, line_strip_vertices_expanded);
            }
            OGL3_render_vertex_buffer(vertex_buffer, GL_TRIANGLES, line_strip_vertices_expanded);
        }
    }

    /*
     * OpenGL ES 3.1 is stricter:
     *
     * 1. No GL_LINES, GL_LINE_STRIP, or GL_LINE_LOOP support in core spec!
     * 2. No glLineWidth support at all.
     * 3. Only GL_TRIANGLES, GL_TRIANGLE_STRIP, and GL_TRIANGLE_FAN are guaranteed.
     *
     * i.e GL_LINES + GL_LINE_STRIP must be emulated
     */
}

void PGraphicsOpenGL_3_3_core::IMPL_emit_shape_fill_triangles(std::vector<Vertex>& triangle_vertices) {
    // NOTE relevant information for this method
    //     - vertex ( i.e position, normal, color, tex_coord )
    //     - textured_id ( current id or solid color )
    //     - shader_id ( default or no shader )

    // NOTE this is a very central method! up until here everything should have been done in generic PGraphics.
    //      - vertices are in model space
    //      ```C
    //      const glm::mat4 modelview = model_matrix;
    //      for (auto& p: transformed_vertices) {
    //          p.position = glm::vec4(modelview * p.position);
    //      }
    //      ```

    // NOTE this is the magic place. here we can do everything we want with the triangle_vertices
    //      e.g export to PDF or SVG, or even do some post processing.
    //      ideally up until here everything could stay in PGraphics i.e all client side drawing
    //      like point, line, rect, ellipse and begin-end-shape

    // TODO maybe add triangle recorder here ( need to transform vertices to world space )

    if (render_mode == RENDER_MODE_BUFFERED) {
        // TODO collect recorded_triangles and current texture information for retained mode here.
        //      - maybe sort by transparency ( and by depth )
        //      - maybe sort transparent recorded_triangles by depth
        //      - maybe sort by fill and stroke
        //      ```C
        //      add_stroke_vertex_xyz_rgba(position, color, tex_coord); // applies transformation
        //      ... // add more triangle_vertices … maybe optimize by adding multiple triangle_vertices at once
        //      RM_add_texture_id_to_render_batch(triangle_vertices,num_vertices,batch_texture_id);
        //      ```
    }
    if (render_mode == RENDER_MODE_IMMEDIATE) {
        if (custom_shader != nullptr) {
            custom_shader->use();
            update_shader_matrices(custom_shader);
            // TODO this is very hack-ish ...
            if (custom_shader == shader_fill_texture_lights) {
                shader_fill_texture_lights->set_uniform("normalMatrix", glm::mat3(glm::transpose(glm::inverse(g->view_matrix * g->model_matrix))));
            }
        } else {
            shader_fill_texture->use();
            update_shader_matrices(shader_fill_texture);
        }
        OGL3_render_vertex_buffer(vertex_buffer, GL_TRIANGLES, triangle_vertices);
    }
}

void PGraphicsOpenGL_3_3_core::IMPL_emit_shape_stroke_points(std::vector<Vertex>& point_vertices, float point_size) {
    if (render_mode == RENDER_MODE_BUFFERED) {
    }
    if (render_mode == RENDER_MODE_IMMEDIATE) {
        if (point_render_mode == POINT_RENDER_MODE_SHADER) {
            shader_point->use();
            update_shader_matrices(shader_point);
            // TODO this MUST be optimized! it is not efficient to update all uniforms every time
            shader_point->set_uniform("viewport", glm::vec4(0, 0, width, height));
            shader_point->set_uniform("perspective", 1);
            std::vector<Vertex> point_vertices_expanded;

            for (size_t i = 0; i + 1 < point_vertices.size(); i++) {
                Vertex v0, v1, v2, v3;

                v0 = point_vertices[i];
                v1 = point_vertices[i];
                v2 = point_vertices[i];
                v3 = point_vertices[i];

                v0.normal.x = 0;
                v0.normal.y = 0;

                v1.normal.x = point_size;
                v1.normal.y = 0;

                v2.normal.x = point_size;
                v2.normal.y = point_size;

                v3.normal.x = 0;
                v3.normal.y = point_size;

                point_vertices_expanded.push_back(v0);
                point_vertices_expanded.push_back(v1);
                point_vertices_expanded.push_back(v2);

                point_vertices_expanded.push_back(v0);
                point_vertices_expanded.push_back(v2);
                point_vertices_expanded.push_back(v3);
            }
            OGL3_render_vertex_buffer(vertex_buffer, GL_TRIANGLES, point_vertices_expanded);
        }
    }
}

// TODO could move this to a shared method in `PGraphics` and use beginShape(TRIANGLES)
void PGraphicsOpenGL_3_3_core::debug_text(const std::string& text, const float x, const float y) {
    const std::vector<Vertex> triangle_vertices = debug_font.generate(text, x, y, glm::vec4(color_fill));
    push_texture_id();
    IMPL_bind_texture(debug_font.textureID);
    shader_fill_texture->use();
    update_shader_matrices(shader_fill_texture);
    OGL3_render_vertex_buffer(vertex_buffer, GL_TRIANGLES, triangle_vertices);
    pop_texture_id();
}

/* --- UTILITIES --- */

void PGraphicsOpenGL_3_3_core::beginDraw() {
    if (render_mode == RENDER_MODE_SHAPE) {
        static bool warning_once = true;
        if (warning_once) {
            warning("render_mode is set to RENDER_MODE_SHAPE. this is not implemented yet.");
            warning("switching to RENDER_MODE_IMMEDIATE.");
            warning_once = false;
            render_mode  = RENDER_MODE_IMMEDIATE;
        }
    }
    if (render_to_offscreen) {
        store_fbo_state();
    }
    noLights();
    resetShader();
    PGraphicsOpenGL::beginDraw();
    texture_id_current = TEXTURE_NONE;
    IMPL_bind_texture(texture_id_solid_color);
}

void PGraphicsOpenGL_3_3_core::endDraw() {
    if (render_mode == RENDER_MODE_BUFFERED) {
        // TODO flush collected vertices
        // RM_flush_fill();
        // RM_flush_stroke();
        // void PGraphicsOpenGL_3_3_core::RM_flush_stroke() {
        //     if (stroke_vertices_xyz_rgba.empty()) {
        //         return;
        //     }
        //
        //     // glEnable(GL_BLEND);
        //     // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        //
        //     glBindBuffer(GL_ARRAY_BUFFER, stroke_VBO_xyz_rgba);
        //     const unsigned long size = stroke_vertices_xyz_rgba.size() * sizeof(float);
        //     glBufferSubData(GL_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(size), stroke_vertices_xyz_rgba.data());
        //
        //     glUseProgram(stroke_shader_program);
        //
        //     // Upload matrices
        //     const GLint projLoc = glGetUniformLocation(stroke_shader_program, "uProjection");
        //     glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection_matrix)); // or projection2D
        //
        //     const GLint viewLoc = glGetUniformLocation(stroke_shader_program, "uViewMatrix");
        //     glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view_matrix));
        //
        //     const GLint matrixLoc = glGetUniformLocation(stroke_shader_program, SHADER_UNIFORM_MODEL_MATRIX);
        //     glUniformMatrix4fv(matrixLoc, 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));
        //
        //     glBindVertexArray(stroke_VAO_xyz_rgba);
        //     glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(stroke_vertices_xyz_rgba.size()) / NUM_STROKE_VERTEX_ATTRIBUTES_XYZ_RGBA);
        //     glBindVertexArray(0);
        //
        //     stroke_vertices_xyz_rgba.clear();
        // }
        //
        // void PGraphicsOpenGL_3_3_core::RM_flush_fill() {
        //     if (fill_vertices_xyz_rgba_uv.empty()) {
        //         return;
        //     }
        //
        //     // glEnable(GL_BLEND);
        //     // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        //
        //     glBindBuffer(GL_ARRAY_BUFFER, fill_VBO_xyz_rgba_uv);
        //     const unsigned long size = fill_vertices_xyz_rgba_uv.size() * sizeof(float);
        //     glBufferSubData(GL_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(size), fill_vertices_xyz_rgba_uv.data());
        //
        //     glUseProgram(fill_shader_program);
        //
        //     // Upload matrices
        //     const GLint projLoc = glGetUniformLocation(fill_shader_program, "uProjection");
        //     glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection_matrix));
        //
        //     const GLint viewLoc = glGetUniformLocation(fill_shader_program, "uViewMatrix");
        //     glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view_matrix));
        //
        //     const GLint modelLoc = glGetUniformLocation(fill_shader_program, SHADER_UNIFORM_MODEL_MATRIX);
        //     glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));
        //
        //     // Bind textures per batch
        //     glBindVertexArray(fill_VAO_xyz_rgba_uv);
        //     for (const auto& batch: renderBatches) {
        //         glBindTexture(GL_TEXTURE_2D, batch.texture_id); // NOTE no need to use `IMPL_bind_texture()`
        //         glDrawArrays(GL_TRIANGLES, batch.start_index, batch.num_vertices);
        //     }
        //     glBindTexture(GL_TEXTURE_2D, 0); // NOTE no need to use `IMPL_bind_texture()`
        //     glBindVertexArray(0);
        //
        //     fill_vertices_xyz_rgba_uv.clear();
        //     renderBatches.clear();
        // }
    }

    PGraphicsOpenGL::endDraw();
}

void PGraphicsOpenGL_3_3_core::render_framebuffer_to_screen(const bool use_blit) {
    // modern OpenGL framebuffer rendering method
    if (use_blit) {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer.id);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBlitFramebuffer(0, 0, framebuffer.width, framebuffer.height,
                          0, 0, framebuffer.width, framebuffer.height,
                          GL_COLOR_BUFFER_BIT, GL_LINEAR); // TODO maybe GL_NEAREST is enough
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    } else {
        warning("TODO only blitting supported atm … `render_framebuffer_to_screen` needs to implement this ... may re-use existing shader");
        // glBindFramebuffer(GL_FRAMEBUFFER, 0);
        // glDisable(GL_DEPTH_TEST);
        // glDisable(GL_BLEND);
        //
        // glUseProgram(shaderProgram);
        // glBindVertexArray(screenVAO);
        //
        // bind_framebuffer_texture();
        // glUniform1i(glGetUniformLocation(shaderProgram, "screenTexture"), 0);
        //
        // glDrawArrays(GL_TRIANGLES, 0, 6);
        //
        // glBindVertexArray(0);
        // glUseProgram(0);
    }
}

void PGraphicsOpenGL_3_3_core::hint(const uint16_t property) {
    // TODO @MERGE
    switch (property) {
        case ENABLE_SMOOTH_LINES:
#ifndef OPENGL_ES_3_0
            glEnable(GL_LINE_SMOOTH);
            glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
#endif
            break;
        case DISABLE_SMOOTH_LINES:
#ifndef OPENGL_ES_3_0
            glDisable(GL_LINE_SMOOTH);
            glHint(GL_LINE_SMOOTH_HINT, GL_FASTEST);
#endif
            break;
        case ENABLE_DEPTH_TEST:
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_LESS);
            break;
        case DISABLE_DEPTH_TEST:
            glDisable(GL_DEPTH_TEST);
            break;
        default:
            break;
    }
}

void PGraphicsOpenGL_3_3_core::upload_texture(PImage*         img,
                                              const uint32_t* pixel_data,
                                              const int       width,
                                              const int       height,
                                              const int       offset_x,
                                              const int       offset_y,
                                              const bool      mipmapped) {
    if (img == nullptr) {
        return;
    }

    if (pixel_data == nullptr) {
        return;
    }

    if (img->texture_id < TEXTURE_VALID_ID) {
        OGL_generate_and_upload_image_as_texture(img, mipmapped); // NOTE texture binding and unbinding is handled here properly
        console("PGraphics / `upload_texture` texture has not been initialized yet … trying to initialize");
        if (img->texture_id < TEXTURE_VALID_ID) {
            error("PGraphics / `upload_texture` failed to create texture");
            return;
        }
        console("texture is now initialized.");
        if (offset_x > 0 || offset_y > 0) {
            console("PGraphics / `upload_texture` offset was ignored");
        }
        return; // NOTE this should be fine, as the texture is now initialized
    }

    // Check if the provided width, height, and offsets are within the valid range
    if (width <= 0 || height <= 0) {
        error("PGraphics / `upload_texture` invalid width or height");
        return;
    }

    if (offset_x < 0 || offset_y < 0 || offset_x + width > img->width || offset_y + height > img->height) {
        error("PGraphics / `upload_texture` parameters exceed image dimensions");
        return;
    }

    const int tmp_bound_texture = texture_id_current;
    IMPL_bind_texture(img->texture_id);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glTexSubImage2D(GL_TEXTURE_2D,
                    0, offset_x, offset_y,
                    width, height,
                    UMFELD_DEFAULT_INTERNAL_PIXEL_FORMAT,
                    UMFELD_DEFAULT_TEXTURE_PIXEL_TYPE,
                    pixel_data);

    IMPL_bind_texture(tmp_bound_texture);
}

void PGraphicsOpenGL_3_3_core::download_texture(PImage* img) {
    if (img == nullptr) {
        return;
    }
    if (img->pixels == nullptr) {
        return;
    }
    if (img->texture_id < TEXTURE_VALID_ID) {
        return;
    }

#ifndef OPENGL_ES_3_0
    const int tmp_bound_texture = texture_id_current;
    IMPL_bind_texture(img->texture_id);
    glGetTexImage(GL_TEXTURE_2D, 0,
                  UMFELD_DEFAULT_INTERNAL_PIXEL_FORMAT,
                  UMFELD_DEFAULT_TEXTURE_PIXEL_TYPE,
                  img->pixels);
    IMPL_bind_texture(tmp_bound_texture);
#else
    static bool emit_warning = true;
    if (emit_warning) {
        emit_warning = false;
        warning("PGraphics / `download_texture` not implemented for OpenGL ES 3.0");
    }
#endif
}

void PGraphicsOpenGL_3_3_core::init(uint32_t*  pixels,
                                    const int  width,
                                    const int  height,
                                    const bool generate_mipmap) {
    (void) generate_mipmap;                // TODO should this always be ignored?
    const int msaa_samples = antialiasing; // TODO not cool to take this from Umfeld

    // TODO create shader system with `get_versioned_source(string)` for:
    //     - point shader
    //     - line shader
    //     - triangle shader ( with texture )
    //     - @maybe triangle shader ( without texture )
    //     maybe remove "transform on CPU" and use vertex shader for this

    shader_fill_texture        = loadShader(shader_source_color_texture.get_vertex_source(), shader_source_color_texture.get_fragment_source());
    shader_fill_texture_lights = loadShader(shader_source_color_texture_lights.get_vertex_source(), shader_source_color_texture_lights.get_fragment_source());
    shader_stroke              = loadShader(shader_source_line.get_vertex_source(), shader_source_line.get_fragment_source());
    shader_point               = loadShader(shader_source_point.get_vertex_source(), shader_source_point.get_fragment_source());

    if (shader_fill_texture == nullptr) {
        error("Failed to load default fill shader.");
    }
    if (shader_fill_texture_lights == nullptr) {
        error("Failed to load default light shader.");
    }
    if (shader_stroke == nullptr) {
        error("Failed to load default stroke shader.");
    } else {
        set_stroke_render_mode(STROKE_RENDER_MODE_LINE_SHADER);
    }
    if (shader_point == nullptr) {
        error("Failed to load default point shader.");
    } else {
        set_point_render_mode(POINT_RENDER_MODE_SHADER);
    }

    this->width        = width;
    this->height       = height;
    framebuffer.width  = width;
    framebuffer.height = height;
    framebuffer.msaa   = render_to_offscreen && msaa_samples > 0;

    if (render_to_offscreen) {
        console("creating offscreen buffer.");
        console("framebuffer: ", framebuffer.width, "×", framebuffer.height);

        glGenFramebuffers(1, &framebuffer.id);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.id);
        glGenTextures(1, &framebuffer.texture_id);

        console("creating framebuffer texture: ", framebuffer.texture_id);

#ifdef OPENGL_ES_3_0
        if (framebuffer.msaa) {
            warning("MSAA not supported in OpenGL ES 3.0 ... disabling MSAA.");
            framebuffer.msaa = false;
        }
#endif
        if (framebuffer.msaa) {
            console("using multisample anti-aliasing (MSAA)");

            GLint maxSamples;
            glGetIntegerv(GL_MAX_SAMPLES, &maxSamples);
            console(format_label("Max supported MSAA samples"), maxSamples);

            GLuint    msaaDepthBuffer;
            const int samples = std::min(msaa_samples, maxSamples); // Number of MSAA samples
            console(format_label("number of used MSAA samples"), samples);
            glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, framebuffer.texture_id);
            UMFELD_PGRAPHICS_OPENGL_3_3_CORE_CHECK_ERRORS("glBindTexture");
#ifndef OPENGL_ES_3_0
            glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE,
                                    samples,
                                    UMFELD_DEFAULT_INTERNAL_PIXEL_FORMAT,
                                    framebuffer.width,
                                    framebuffer.height,
                                    GL_TRUE);
#endif
            UMFELD_PGRAPHICS_OPENGL_3_3_CORE_CHECK_ERRORS("glTexImage2DMultisample");
            glFramebufferTexture2D(GL_FRAMEBUFFER,
                                   GL_COLOR_ATTACHMENT0,
                                   GL_TEXTURE_2D_MULTISAMPLE,
                                   framebuffer.texture_id, 0);
            UMFELD_PGRAPHICS_OPENGL_3_3_CORE_CHECK_ERRORS("glFramebufferTexture2D");
            // Create Multisampled Depth Buffer
            glGenRenderbuffers(1, &msaaDepthBuffer);
            glBindRenderbuffer(GL_RENDERBUFFER, msaaDepthBuffer);
            glRenderbufferStorageMultisample(GL_RENDERBUFFER,
                                             samples,
                                             GL_DEPTH24_STENCIL8,
                                             framebuffer.width,
                                             framebuffer.height);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER,
                                      GL_DEPTH_ATTACHMENT,
                                      GL_RENDERBUFFER,
                                      msaaDepthBuffer);
        } else {
            console("using standard framebuffer object");
            glBindTexture(GL_TEXTURE_2D, framebuffer.texture_id); // NOTE no need to use `IMPL_bind_texture()`
            glTexImage2D(GL_TEXTURE_2D,
                         0,
                         UMFELD_DEFAULT_INTERNAL_PIXEL_FORMAT,
                         framebuffer.width,
                         framebuffer.height,
                         0,
                         UMFELD_DEFAULT_INTERNAL_PIXEL_FORMAT,
                         UMFELD_DEFAULT_TEXTURE_PIXEL_TYPE,
                         nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glFramebufferTexture2D(GL_FRAMEBUFFER,
                                   GL_COLOR_ATTACHMENT0,
                                   GL_TEXTURE_2D,
                                   framebuffer.texture_id,
                                   0);
            GLuint depthBuffer;
            glGenRenderbuffers(1, &depthBuffer);
            glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, framebuffer.width, framebuffer.height);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuffer);
        }

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            error("ERROR Framebuffer is not complete!");
        }

        glViewport(0, 0, framebuffer.width, framebuffer.height);
        glClearColor(0, 0, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        if (framebuffer.msaa) {
            glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0); // NOTE no need to use `IMPL_bind_texture()`
        } else {
            glBindTexture(GL_TEXTURE_2D, 0); // NOTE no need to use `IMPL_bind_texture()`
        }
        texture_id = framebuffer.texture_id; // TODO maybe get rid of one of the texture_id variables
    }

    // static_assert(sizeof(Vertex) == 64, "Vertex struct must be 64 bytes"); // NOTE check this on other systems
    console(format_label("'Vertex' struct size"), sizeof(Vertex), " bytes");

    OGL3_create_solid_color_texture();
    texture_id_current = TEXTURE_NONE;
    IMPL_bind_texture(texture_id_solid_color);
}

/* additional */

void PGraphicsOpenGL_3_3_core::OGL3_create_solid_color_texture() {
    GLuint texture_id;
    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id); // NOTE no need to use `IMPL_bind_texture()`

    constexpr unsigned char whitePixel[4] = {255, 255, 255, 255}; // RGBA: White
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 UMFELD_DEFAULT_INTERNAL_PIXEL_FORMAT,
                 1, 1,
                 0,
                 UMFELD_DEFAULT_INTERNAL_PIXEL_FORMAT,
                 GL_UNSIGNED_BYTE,
                 whitePixel);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0); // NOTE no need to use `IMPL_bind_texture()`

    texture_id_solid_color = texture_id;
}

// void PGraphicsOpenGL_3_3_core::OGL3_tranform_model_matrix_and_render_vertex_buffer(VertexBuffer&              vertex_buffer,
//                                                                                    const GLenum               primitive_mode,
//                                                                                    const std::vector<Vertex>& shape_vertices) const {
//     static bool _emit_warning_only_once = false;
//     if (primitive_mode != GL_TRIANGLES && primitive_mode != GL_LINE_STRIP) {
//         if (!_emit_warning_only_once) {
//             warning("this test is just for development purposes: only GL_TRIANGLES and GL_LINE_STRIP are supposed to be used atm.");
//             warning("( warning only once )");
//             _emit_warning_only_once = true;
//         }
//     }
//
//     if (shape_vertices.empty()) {
//         return;
//     }
//
// #ifdef VERTICES_CLIENT_SIDE_TRANSFORM
//     // NOTE depending on the number of vertices transformation are handle on the GPU
//     //      i.e all shapes *up to* quads are transformed on CPU
//     static constexpr int MAX_NUM_VERTICES_CLIENT_SIDE_TRANSFORM = 0;
//     bool                 mModelMatrixTransformOnGPU             = false;
//     std::vector<Vertex>  transformed_vertices                   = shape_vertices; // NOTE make copy
//     if (model_matrix_dirty) {
//         if (shape_vertices.size() <= MAX_NUM_VERTICES_CLIENT_SIDE_TRANSFORM) {
//             const glm::mat4 modelview = model_matrix;
//             for (auto& p: transformed_vertices) {
//                 p.position = glm::vec4(modelview * p.position);
//             }
//             console("transforming model matrix on CPU for ", shape_vertices.size(), " vertices.");
//         } else {
//             mModelMatrixTransformOnGPU = true;
//         }
//     }
//     if (mModelMatrixTransformOnGPU) {
//         update_shader_matrices(current_shader);
//     }
//     OGL3_render_vertex_buffer(vertex_buffer, primitive_mode, transformed_vertices);
// #else
// #ifdef UMFELD_OGL33_RESET_MATRICES_ON_SHADER
//     if (mModelMatrixTransformOnGPU) {
//         reset_shader_matrices(current_shader);
//     }
// #endif // UMFELD_OGL33_RESET_MATRICES_ON_SHADER
//     // TODO check if it is necessary to update *all* shader matrices here ( i.e model, view, projection )
//     //      `update_shader_matrices(current_shader);`
//     shader_fill_texture->use();
//     if (shader_fill_texture->has_model_matrix) {
//         shader_fill_texture->set_uniform(SHADER_UNIFORM_MODEL_MATRIX, model_matrix);
//     }
//
//     OGL3_render_vertex_buffer(vertex_buffer, primitive_mode, shape_vertices);
// #endif // VERTICES_CLIENT_SIDE_TRANSFORM
// }

void PGraphicsOpenGL_3_3_core::OGL3_render_vertex_buffer(VertexBuffer& vertex_buffer, const GLenum primitive_mode, const std::vector<Vertex>& shape_vertices) {
    if (shape_vertices.empty()) {
        return;
    }
    vertex_buffer.clear();
    vertex_buffer.add_vertices(shape_vertices);
    vertex_buffer.set_shape(primitive_mode, false);
    vertex_buffer.draw();
}

void PGraphicsOpenGL_3_3_core::update_shader_matrices(PShader* shader) const {
    if (shader == nullptr) { return; }
    if (shader->has_model_matrix) {
        shader->set_uniform(SHADER_UNIFORM_MODEL_MATRIX, model_matrix);
    }
    if (shader->has_view_matrix) {
        shader->set_uniform(SHADER_UNIFORM_VIEW_MATRIX, view_matrix);
    }
    if (shader->has_projection_matrix) {
        shader->set_uniform(SHADER_UNIFORM_PROJECTION_MATRIX, projection_matrix);
    }
    if (shader->has_texture_unit) {
        shader->set_uniform(SHADER_UNIFORM_TEXTURE_UNIT, 0);
    }
}

void PGraphicsOpenGL_3_3_core::reset_shader_matrices(PShader* shader) {
    if (shader == nullptr) { return; }
    if (shader->has_model_matrix) {
        shader->set_uniform(SHADER_UNIFORM_MODEL_MATRIX, glm::mat4(1.0f));
    }
    if (shader->has_view_matrix) {
        shader->set_uniform(SHADER_UNIFORM_VIEW_MATRIX, glm::mat4(1.0f));
    }
    if (shader->has_projection_matrix) {
        shader->set_uniform(SHADER_UNIFORM_PROJECTION_MATRIX, glm::mat4(1.0f));
    }
    if (shader->has_texture_unit) {
        shader->set_uniform(SHADER_UNIFORM_TEXTURE_UNIT, 0);
    }
}

void PGraphicsOpenGL_3_3_core::mesh(VertexBuffer* mesh_shape) {
    UMFELD_PGRAPHICS_OPENGL_3_3_CORE_CHECK_ERRORS("mesh() begin");
    if (mesh_shape == nullptr) {
        return;
    }
    if (custom_shader != nullptr) {
        custom_shader->use();
        update_shader_matrices(custom_shader);
    } else {
        shader_fill_texture->use();
        UMFELD_PGRAPHICS_OPENGL_3_3_CORE_CHECK_ERRORS("mesh() use shader");
        update_shader_matrices(shader_fill_texture);
        UMFELD_PGRAPHICS_OPENGL_3_3_CORE_CHECK_ERRORS("mesh() update shader matrices");
    }
    // TODO is there a way to also draw this with line shader?
    mesh_shape->draw();
    UMFELD_PGRAPHICS_OPENGL_3_3_CORE_CHECK_ERRORS("mesh() end");
#ifdef UMFELD_OGL33_RESET_MATRICES_ON_SHADER
    reset_shader_matrices(current_shader);
#endif
}

PShader* PGraphicsOpenGL_3_3_core::loadShader(const std::string& vertex_code, const std::string& fragment_code, const std::string& geometry_code) {
    const auto shader = new PShader();
    const bool result = shader->load(vertex_code, fragment_code, geometry_code);
    if (!result) {
        error("failed to load shader: ", vertex_code, " ", fragment_code, " ", geometry_code);
        delete shader;
        return nullptr;
    }
    return shader;
}

void PGraphicsOpenGL_3_3_core::shader(PShader* shader) {
    if (shader == nullptr) {
        resetShader();
        return;
    }
    custom_shader = shader;
    custom_shader->use();
    update_shader_matrices(custom_shader);
}

void PGraphicsOpenGL_3_3_core::resetShader() {
    custom_shader = nullptr;
    // update_all_shader_matrices();
}

bool PGraphicsOpenGL_3_3_core::read_framebuffer(std::vector<unsigned char>& pixels) {
    store_fbo_state();
    if (framebuffer.msaa) {
        // NOTE this is a bit tricky. when the offscreen FBO is a multisample FBO ( MSAA ) we need to resolve it first
        //      i.e blit it into the color buffer of the default framebuffer. otherwise we can just read from the
        //      offscreen FBO.
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer.id);
        glBlitFramebuffer(0, 0, framebuffer.width, framebuffer.height,
                          0, 0, framebuffer.width, framebuffer.height,
                          GL_COLOR_BUFFER_BIT, GL_LINEAR);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    } else {
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.id); // non-MSAA FBO or default
    }
    const bool success = OGL_read_framebuffer(framebuffer, pixels);
    restore_fbo_state();
    return success;
}

void PGraphicsOpenGL_3_3_core::store_fbo_state() {
    glGetIntegerv(GL_CURRENT_PROGRAM, &previous_shader);
    glGetIntegerv(GL_VIEWPORT, previous_viewport);
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &previously_bound_read_FBO);
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &previously_bound_draw_FBO);
}

void PGraphicsOpenGL_3_3_core::bind_fbo() {
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.id);
}

void PGraphicsOpenGL_3_3_core::restore_fbo_state() {
    glBindFramebuffer(GL_READ_FRAMEBUFFER, previously_bound_read_FBO);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, previously_bound_draw_FBO);
    glViewport(previous_viewport[0], previous_viewport[1], previous_viewport[2], previous_viewport[3]);
    glUseProgram(previous_shader);
}

void PGraphicsOpenGL_3_3_core::update_all_shader_matrices() const {
    if (custom_shader != nullptr) {
        custom_shader->use();
        update_shader_matrices(custom_shader);
    } else {
        shader_fill_texture->use();
        update_shader_matrices(shader_fill_texture);
        shader_fill_texture_lights->use();
        update_shader_matrices(shader_fill_texture_lights);
        shader_stroke->use();
        update_shader_matrices(shader_stroke);
        shader_point->use();
        update_shader_matrices(shader_point);
    }
}


void PGraphicsOpenGL_3_3_core::camera(const float eyeX, const float eyeY, const float eyeZ, const float centerX, const float centerY, const float centerZ, const float upX, const float upY, const float upZ) {
    PGraphics::camera(eyeX, eyeY, eyeZ, centerX, centerY, centerZ, upX, upY, upZ);
    update_all_shader_matrices();
}

void PGraphicsOpenGL_3_3_core::frustum(const float left, const float right, const float bottom, const float top, const float near, const float far) {
    PGraphics::frustum(left, right, bottom, top, near, far);
    update_all_shader_matrices();
}

void PGraphicsOpenGL_3_3_core::ortho(const float left, const float right, const float bottom, const float top, const float near, const float far) {
    PGraphics::ortho(left, right, bottom, top, near, far);
    update_all_shader_matrices();
}

void PGraphicsOpenGL_3_3_core::perspective(const float fovy, const float aspect, const float near, const float far) {
    PGraphics::perspective(fovy, aspect, near, far);
    update_all_shader_matrices();
}

/* --- LIGHTS --- */

void PGraphicsOpenGL_3_3_core::noLights() {
    lightCount                   = 0;
    currentLightSpecular         = glm::vec3(0.0f);
    currentLightFalloffConstant  = 1.0f;
    currentLightFalloffLinear    = 0.0f;
    currentLightFalloffQuadratic = 0.0f;
    resetShader();
}

void PGraphicsOpenGL_3_3_core::lights() {
    enableLighting();

    // shader_fill_texture_lights->set_uniform(SHADER_UNIFORM_MODEL_MATRIX, g->model_matrix);
    // shader_fill_texture_lights->set_uniform(SHADER_UNIFORM_VIEW_MATRIX, g->view_matrix);
    // shader_fill_texture_lights->set_uniform(SHADER_UNIFORM_PROJECTION_MATRIX, g->projection_matrix);
    // shader_fill_texture_lights->set_uniform(SHADER_UNIFORM_TEXTURE_UNIT, 0);
    // shader_fill_texture_lights->set_uniform("normalMatrix", glm::mat3(glm::transpose(glm::inverse(g->view_matrix * g->model_matrix))));
    // NOTE ^^^ this is done in `IMPL_emit_shape_fill_triangles`
    // shader_fill_texture_lights->set_uniform("texMatrix", glm::mat4(1.0f)); // or a real matrix if you’re transforming texCoords

    ambient(0.5f, 0.5f, 0.5f);
    specular(0.5f, 0.5f, 0.5f);
    emissive(0.1f, 0.1f, 0.1f);
    shininess(64.0f);
    lightFalloff(1, 0, 0);
    lightSpecular(0, 0, 0);

    ambientLight(128 / 255.0f, 128 / 255.0f, 128 / 255.0f);
    directionalLight(128 / 255.0f, 128 / 255.0f, 128 / 255.0f, 0, 0, 1); // TODO why is this (0, 0, 1) and not (0, 0, -1) as described in the documentation?
}

void PGraphicsOpenGL_3_3_core::ambientLight(const float r, const float g, const float b, const float x, const float y, const float z) {
    enableLighting();
    if (lightCount >= MAX_LIGHTS) {
        return;
    }

    lightType[lightCount] = AMBIENT;

    setLightPosition(lightCount, x, y, z, false);
    setLightNormal(lightCount, 0, 0, 0);

    setLightAmbient(lightCount, r, g, b);
    setNoLightDiffuse(lightCount);
    setNoLightSpecular(lightCount);
    setNoLightSpot(lightCount);
    setLightFalloff(lightCount, currentLightFalloffConstant,
                    currentLightFalloffLinear,
                    currentLightFalloffQuadratic);

    lightCount++;
    updateShaderLighting();
}

void PGraphicsOpenGL_3_3_core::directionalLight(const float r, const float g, const float b, const float nx, const float ny, const float nz) {
    enableLighting();
    if (lightCount >= MAX_LIGHTS) {
        return;
    }

    lightType[lightCount] = DIRECTIONAL;

    setLightPosition(lightCount, 0, 0, 0, true); // directional = true
    setLightNormal(lightCount, nx, ny, nz);

    setNoLightAmbient(lightCount);
    setLightDiffuse(lightCount, r, g, b);
    setLightSpecular(lightCount, currentLightSpecular.r,
                     currentLightSpecular.g,
                     currentLightSpecular.b);
    setNoLightSpot(lightCount);
    setNoLightFalloff(lightCount);

    lightCount++;
    updateShaderLighting();
}

void PGraphicsOpenGL_3_3_core::pointLight(const float r, const float g, const float b, const float x, const float y, const float z) {
    enableLighting();
    if (lightCount >= MAX_LIGHTS) {
        return;
    }

    lightType[lightCount] = POINT;

    setLightPosition(lightCount, x, y, z, false);
    setLightNormal(lightCount, 0, 0, 0);

    setNoLightAmbient(lightCount);
    setLightDiffuse(lightCount, r, g, b);
    setLightSpecular(lightCount,
                     currentLightSpecular.r,
                     currentLightSpecular.g,
                     currentLightSpecular.b);
    setNoLightSpot(lightCount);
    setLightFalloff(lightCount,
                    currentLightFalloffConstant,
                    currentLightFalloffLinear,
                    currentLightFalloffQuadratic);

    lightCount++;
    updateShaderLighting();
}

void PGraphicsOpenGL_3_3_core::spotLight(const float r, const float g, const float b, const float x, const float y, const float z,
                                         const float nx, const float ny, const float nz, const float angle, const float concentration) {
    enableLighting();
    if (lightCount >= MAX_LIGHTS) {
        return;
    }

    lightType[lightCount] = SPOT;

    setLightPosition(lightCount, x, y, z, false);
    setLightNormal(lightCount, nx, ny, nz);

    setNoLightAmbient(lightCount);
    setLightDiffuse(lightCount, r, g, b);
    setLightSpecular(lightCount, currentLightSpecular.r,
                     currentLightSpecular.g,
                     currentLightSpecular.b);
    setLightSpot(lightCount, angle, concentration);
    setLightFalloff(lightCount, currentLightFalloffConstant,
                    currentLightFalloffLinear,
                    currentLightFalloffQuadratic);

    lightCount++;
    updateShaderLighting();
}

void PGraphicsOpenGL_3_3_core::lightFalloff(const float constant, const float linear, const float quadratic) {
    currentLightFalloffConstant  = constant;
    currentLightFalloffLinear    = linear;
    currentLightFalloffQuadratic = quadratic;
}

void PGraphicsOpenGL_3_3_core::lightSpecular(const float r, const float g, const float b) {
    currentLightSpecular = glm::vec3(r, g, b);
}

void PGraphicsOpenGL_3_3_core::ambient(const float r, const float g, const float b) {
    if (shader_fill_texture_lights) {
        shader_fill_texture_lights->set_uniform("ambient", glm::vec4(r, g, b, 1.0f));
    }
}

void PGraphicsOpenGL_3_3_core::specular(const float r, const float g, const float b) {
    if (shader_fill_texture_lights) {
        shader_fill_texture_lights->set_uniform("specular", glm::vec4(r, g, b, 1.0f));
    }
}

void PGraphicsOpenGL_3_3_core::emissive(const float r, const float g, const float b) {
    if (shader_fill_texture_lights) {
        shader_fill_texture_lights->set_uniform("emissive", glm::vec4(r, g, b, 1.0f));
    }
}

void PGraphicsOpenGL_3_3_core::shininess(const float s) {
    if (shader_fill_texture_lights) {
        shader_fill_texture_lights->set_uniform("shininess", s);
    }
}

void PGraphicsOpenGL_3_3_core::enableLighting() {
    shader(shader_fill_texture_lights);
}

void PGraphicsOpenGL_3_3_core::setLightPosition(const int num, const float x, const float y, const float z, const bool directional) {
    // TODO Transform position by current modelview matrix
    //      For now, assuming world space coordinates
    lightPositions[num] = glm::vec4(x, y, z, directional ? 0.0f : 1.0f);
}

void PGraphicsOpenGL_3_3_core::setLightNormal(const int num, const float dx, const float dy, const float dz) {
    // NOTE normalize the direction vector
    glm::vec3 normal(dx, dy, dz);
    if (glm::length(normal) > 0.0f) {
        normal = glm::normalize(normal);
    }
    lightNormals[num] = normal;
}

void PGraphicsOpenGL_3_3_core::setLightAmbient(const int num, const float r, const float g, const float b) {
    lightAmbientColors[num] = glm::vec3(r, g, b);
}

void PGraphicsOpenGL_3_3_core::setNoLightAmbient(const int num) {
    lightAmbientColors[num] = glm::vec3(0.0f);
}

void PGraphicsOpenGL_3_3_core::setLightDiffuse(const int num, const float r, const float g, const float b) {
    lightDiffuseColors[num] = glm::vec3(r, g, b);
}

void PGraphicsOpenGL_3_3_core::setNoLightDiffuse(const int num) {
    lightDiffuseColors[num] = glm::vec3(0.0f);
}

void PGraphicsOpenGL_3_3_core::setLightSpecular(const int num, const float r, const float g, const float b) {
    lightSpecularColors[num] = glm::vec3(r, g, b);
}

void PGraphicsOpenGL_3_3_core::setNoLightSpecular(const int num) {
    lightSpecularColors[num] = glm::vec3(0.0f);
}

void PGraphicsOpenGL_3_3_core::setLightFalloff(const int num, const float constant, const float linear, const float quadratic) {
    lightFalloffCoeffs[num] = glm::vec3(constant, linear, quadratic);
}

void PGraphicsOpenGL_3_3_core::setNoLightFalloff(const int num) {
    lightFalloffCoeffs[num] = glm::vec3(1.0f, 0.0f, 0.0f);
}

void PGraphicsOpenGL_3_3_core::setLightSpot(const int num, const float angle, const float concentration) {
    lightSpotParams[num] = glm::vec2(std::max(0.0f, std::cos(angle)), concentration);
}

void PGraphicsOpenGL_3_3_core::setNoLightSpot(const int num) {
    lightSpotParams[num] = glm::vec2(-1.0f, 0.0f); // -1 disables spotlight
}

void PGraphicsOpenGL_3_3_core::updateShaderLighting() const {
    if (!shader_fill_texture_lights) {
        return;
    }

    // TODO check if this is the best place to update the shader matrices
    shader_fill_texture_lights->set_uniform("normalMatrix", glm::mat3(glm::transpose(glm::inverse(g->view_matrix * g->model_matrix))));
    shader_fill_texture_lights->set_uniform("texMatrix", glm::mat4(1.0f)); // or a real matrix if you’re transforming texCoords

    // update light count
    shader_fill_texture_lights->set_uniform("lightCount", lightCount);

    // update all light uniforms for the current lights
    for (int i = 0; i < lightCount; i++) {
        std::string indexStr = "[" + std::to_string(i) + "]";

        shader_fill_texture_lights->set_uniform("lightPosition" + indexStr, lightPositions[i]);
        shader_fill_texture_lights->set_uniform("lightNormal" + indexStr, lightNormals[i]);
        shader_fill_texture_lights->set_uniform("lightAmbient" + indexStr, lightAmbientColors[i]);
        shader_fill_texture_lights->set_uniform("lightDiffuse" + indexStr, lightDiffuseColors[i]);
        shader_fill_texture_lights->set_uniform("lightSpecular" + indexStr, lightSpecularColors[i]);
        shader_fill_texture_lights->set_uniform("lightFalloff" + indexStr, lightFalloffCoeffs[i]);
        shader_fill_texture_lights->set_uniform("lightSpot" + indexStr, lightSpotParams[i]);
    }

    UMFELD_PGRAPHICS_OPENGL_3_3_CORE_CHECK_ERRORS("updateShaderLighting");
}

void PGraphicsOpenGL_3_3_core::texture_filter(const TextureFilter filter) {
    switch (filter) {
        case NEAREST:
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            break;
        case LINEAR:
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            break;
        case MIPMAP:
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            break;
        default:
            error("Unknown texture filter type");
            break;
    }
}

#endif // UMFELD_PGRAPHICS_OPENGLV33_CPP