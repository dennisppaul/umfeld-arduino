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

#include "PGraphicsOpenGL.h"

#include "Umfeld.h"
#include "UmfeldFunctionsGraphics.h"

namespace umfeld {
    void PGraphicsOpenGL::blendMode(const int mode) {
        glEnable(GL_BLEND);
        switch (mode) {
            case REPLACE:
                glBlendEquation(GL_FUNC_ADD);
                glBlendFunc(GL_ONE, GL_ZERO);
                break;
            case BLEND:
                glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
                glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
                                    GL_ONE, GL_ONE);
                break;
            case ADD:
                glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
                glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE,
                                    GL_ONE, GL_ONE);
                break;
            case SUBTRACT:
                glBlendEquationSeparate(GL_FUNC_REVERSE_SUBTRACT, GL_FUNC_ADD);
                glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE,
                                    GL_ONE, GL_ONE);
                break;
            case LIGHTEST:
                glBlendEquationSeparate(GL_MAX, GL_FUNC_ADD);
                glBlendFuncSeparate(GL_ONE, GL_ONE,
                                    GL_ONE, GL_ONE);
                break;
            case DARKEST:
                glBlendEquationSeparate(GL_MIN, GL_FUNC_ADD);
                glBlendFuncSeparate(GL_ONE, GL_ONE,
                                    GL_ONE, GL_ONE);
                break;
            case MULTIPLY:
                glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
                glBlendFuncSeparate(GL_ZERO, GL_SRC_COLOR,
                                    GL_ONE, GL_ONE);
                break;
            case SCREEN:
                glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
                glBlendFuncSeparate(GL_ONE_MINUS_DST_COLOR, GL_ONE,
                                    GL_ONE, GL_ONE);
                break;
            case EXCLUSION:
                glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
                glBlendFuncSeparate(GL_ONE_MINUS_DST_COLOR, GL_ONE_MINUS_SRC_COLOR,
                                    GL_ONE, GL_ONE);
                break;
            // not possible in fixed-function blending
            case DIFFERENCE_BLEND:
            case OVERLAY:
            case HARD_LIGHT:
            case SOFT_LIGHT:
            case DODGE:
            case BURN:
                // optionally: issue a warning here
                glBlendEquation(GL_FUNC_ADD);
                glBlendFunc(GL_ONE, GL_ZERO); // fallback: REPLACE
                break;
            default:
                // fallback: BLEND
                glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
                glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
                                    GL_ONE, GL_ONE);
                break;
        }
    }

    void PGraphicsOpenGL::OGL_bind_texture(const int texture_id) {
        glActiveTexture(GL_TEXTURE0 + DEFAULT_ACTIVE_TEXTURE_UNIT);
        glBindTexture(GL_TEXTURE_2D, texture_id);
        // TODO optimize by also passing the current texture ID i.e `texture_id_current`
        // if (texture_id != texture_id_current) {
        //     glActiveTexture(GL_TEXTURE0 + DEFAULT_ACTIVE_TEXTURE_UNIT);
        //     glBindTexture(GL_TEXTURE_2D, texture_id);
        //     glBindTexture(GL_TEXTURE_2D, texture_id_current); // NOTE this should be the only glBindTexture ( except for initializations )
        // }
    }

    bool PGraphicsOpenGL::OGL_read_framebuffer(const FrameBufferObject& framebuffer, std::vector<unsigned char>& pixels) {
        const int _width  = framebuffer.width;
        const int _height = framebuffer.height;
        pixels.resize(_width * _height * DEFAULT_BYTES_PER_PIXELS);
        glPixelStorei(GL_PACK_ALIGNMENT, 4);
        glReadPixels(0, 0, _width, _height,
                     UMFELD_DEFAULT_EXTERNAL_PIXEL_FORMAT,
                     UMFELD_DEFAULT_TEXTURE_PIXEL_TYPE,
                     pixels.data());
        return true;
    }

    bool PGraphicsOpenGL::OGL_generate_and_upload_image_as_texture(PImage* image) {
        if (image == nullptr) {
            error_in_function("image is nullptr");
            return false;
        }

        if (image->pixels == nullptr) {
            error_in_function("pixel data is nullptr");
            return false;
        }

        if (image->width <= 0 || image->height <= 0) {
            error_in_function("invalid width or height");
            return false;
        }

        // generate texture ID
        GLuint mTextureID;
        glGenTextures(1, &mTextureID);

        if (mTextureID == 0) {
            error_in_function("texture ID generation failed");
            return false;
        }

        image->texture_id = static_cast<int>(mTextureID);
        // const int tmp_bound_texture = texture_id_current;
        OGL_bind_texture(image->texture_id);

        // set texture parameters
        if (image->get_auto_generate_mipmap()) {
            OGL_texture_wrap(CLAMP_TO_EDGE, glm::vec4(0, 0, 0, 0));
            OGL_texture_filter(MIPMAP);
        } else {
            OGL_texture_wrap(CLAMP_TO_EDGE, glm::vec4(0, 0, 0, 0));
            OGL_texture_filter(LINEAR);
        }

        // load image data
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
        glTexImage2D(GL_TEXTURE_2D,
                     0,
                     UMFELD_DEFAULT_INTERNAL_PIXEL_FORMAT,
                     static_cast<GLint>(image->width),
                     static_cast<GLint>(image->height),
                     0,
                     UMFELD_DEFAULT_EXTERNAL_PIXEL_FORMAT,
                     UMFELD_DEFAULT_TEXTURE_PIXEL_TYPE,
                     image->pixels);

        if (image->get_auto_generate_mipmap()) {
            glGenerateMipmap(GL_TEXTURE_2D); // NOTE this works on macOS … but might not work on all platforms
        }

        // OGL_bind_texture(tmp_bound_texture);
        return true;
    }

    void PGraphicsOpenGL::OGL_texture_filter(const TextureFilter filter) {
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
                error_in_function("unknown texture filter type");
                break;
        }
    }

    void PGraphicsOpenGL::OGL_texture_wrap(const TextureWrap wrap, const glm::vec4 color_stroke) {
        switch (wrap) {
            case REPEAT:
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                break;
            case CLAMP_TO_EDGE:
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                break;
            case MIRRORED_REPEAT:
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
                break;
            case CLAMP_TO_BORDER:
#ifndef OPENGL_ES_3_0
                // NOTE this is not supported in OpenGL ES 3.0
                {
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
                    const float borderColor[] = {color_stroke.r, color_stroke.g, color_stroke.b, color_stroke.a};
                    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
                }
#endif
                break;
            default:
                error_in_function("unknown texture wrap type");
                break;
        }
    }

    void PGraphicsOpenGL::texture_filter(const TextureFilter filter) {
        OGL_texture_filter(filter);
    }

    void PGraphicsOpenGL::texture_wrap(const TextureWrap wrap, const glm::vec4 color_stroke) {
        OGL_texture_wrap(wrap, color_stroke);
    }
} // namespace umfeld
