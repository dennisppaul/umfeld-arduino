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

namespace umfeld {
    class ShapeRendererBatchOpenGL_3 final : public ShapeRenderer {
    public:
        enum SHAPE_CENTER_COMPUTE_STRATEGY {
            ZERO_CENTER,
            AXIS_ALIGNED_BOUNDING_BOX,
            CENTER_OF_MASS,
        };

        ~ShapeRendererBatchOpenGL_3() override {}
        void init() override;
        void beginShape(ShapeMode mode, bool filled, bool transparent, uint32_t texture_id, const glm::mat4& model_transform_matrix) override;
        void vertex(const Vertex& v) override;
        void setVertices(std::vector<Vertex>&& vertices) override;
        void setVertices(const std::vector<Vertex>& vertices) override;
        void endShape(bool closed) override;
        void submitShape(Shape& s) override;
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
            std::vector<Shape*> opaqueShapes;
            std::vector<Shape*> transparentShapes;
        };

        // Cached uniform locations (avoid glGetUniformLocation every frame)
        struct ShaderUniforms {
            GLint uViewProj = -1;
            GLint uTex      = -1;
        };

        ShaderUniforms                texturedUniforms;
        ShaderUniforms                untexturedUniforms;
        GLuint                        vbo                     = 0;
        GLuint                        ubo                     = 0;
        GLuint                        vao                     = 0;
        GLuint                        texturedShaderProgram   = 0;
        GLuint                        untexturedShaderProgram = 0;
        std::vector<Shape>            shapes;
        Shape                         currentShape;
        SHAPE_CENTER_COMPUTE_STRATEGY shape_center_compute_strategy = ZERO_CENTER;
        bool                          shapeInProgress               = false;
        std::vector<Vertex>           frameVertices;
        std::vector<glm::mat4>        frameMatrices;

        static GLuint compileShader(const char* src, GLenum type);
        GLuint createShaderProgram(const char* vsSrc, const char* fsSrc);
        static void   setupUniformBlocks(GLuint program);
        void   initShaders();
        void   initBuffers();
        static size_t estimateTriangleCount(const Shape& s);
        static void   tessellateToTriangles(const Shape& s, std::vector<Vertex>& out, uint16_t transformID);
        void   renderBatch(const std::vector<Shape*>& shapesToRender, const glm::mat4& viewProj, GLuint textureID);
    };
} // namespace umfeld