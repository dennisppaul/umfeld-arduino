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
        static constexpr uint16_t SHADER_PROGRAM_UNTEXTURED       = 0;
        static constexpr uint16_t SHADER_PROGRAM_TEXTURED         = 1;
        static constexpr uint16_t SHADER_PROGRAM_UNTEXTURED_LIGHT = 2; // TODO implement
        static constexpr uint16_t SHADER_PROGRAM_TEXTURED_LIGHT   = 3; // TODO implement
        static constexpr uint16_t SHADER_PROGRAM_POINT            = 4; // TODO implement
        static constexpr uint16_t SHADER_PROGRAM_LINE             = 5; // TODO implement
        static constexpr uint16_t NUM_SHADER_PROGRAMS             = 6;

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

    private:
        static constexpr uint16_t MAX_TRANSFORMS = 256;
        // NOTE check `GL_MAX_UNIFORM_BLOCK_SIZE`
        //      ```c
        //      GLint maxUBOSize;
        //      glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &maxUBOSize);
        //      static constexpr uint16_t MAX_TRANSFORMS = std::min(1024, maxUBOSize / (int)sizeof(glm::mat4));
        //      ```

        struct TextureBatch {
            uint16_t            textureID{TEXTURE_NONE};
            std::vector<Shape*> opaque_shapes;
            std::vector<Shape*> transparent_shapes;
        };

        // Cached uniform locations (avoid glGetUniformLocation every frame)
        struct ShaderUniforms {
            GLint uViewProj = -1;
            GLint uTex      = -1;
        };

        // TODO implement for lighting, point and line shader
        ShaderUniforms                texturedUniforms;
        ShaderUniforms                untexturedUniforms;
        GLuint                        vbo                             = 0;
        GLuint                        ubo                             = 0;
        GLuint                        vao                             = 0;
        GLuint                        texturedShaderProgram           = 0;
        GLuint                        untexturedShaderProgram         = 0;
        GLuint                        texturedLightingShaderProgram   = 0; // TODO implement
        GLuint                        untexturedLightingShaderProgram = 0; // TODO implement
        GLuint                        pointShaderProgram              = 0; // TODO implement
        GLuint                        lineShaderProgram               = 0; // TODO implement
        std::vector<Shape>            shapes;
        Shape                         currentShape;
        SHAPE_CENTER_COMPUTE_STRATEGY shape_center_compute_strategy = ZERO_CENTER;
        std::vector<Vertex>           frameVertices;
        std::vector<glm::mat4>        frameMatrices;

        //        static GLuint compileShader(const char* src, GLenum type);
        //        static GLuint createShaderProgram(const char* vsSrc, const char* fsSrc);
        static void   setupUniformBlocks(GLuint program);
        void          initShaders(const std::vector<int>& shader_programm_id);
        void          initBuffers();
        static size_t estimateTriangleCount(const Shape& s);
        static void   tessellateToTriangles(const Shape& s, std::vector<Vertex>& out, uint16_t transformID);
        void          renderBatch(const std::vector<Shape*>& shapesToRender, const glm::mat4& viewProj, GLuint textureID);
        void          computeShapeCenter(Shape& s) const;
        void          flush_sort_by_z_order(const glm::mat4& view_projection_matrix);
        void          flush_submission_order(const glm::mat4& view_projection_matrix);
        void          flush_immediately(const glm::mat4& view_projection_matrix);

        static void bind_texture(const int texture_id) {
            glActiveTexture(GL_TEXTURE0 + PGraphicsOpenGL::DEFAULT_ACTIVE_TEXTURE_UNIT);
            glBindTexture(GL_TEXTURE_2D, texture_id); // NOTE this should be the only glBindTexture ( except for initializations )
        }
    };
} // namespace umfeld