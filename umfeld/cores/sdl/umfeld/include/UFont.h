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

#include <vector>

#include "Vertex.h"
#include "UFontPixels.h"

namespace umfeld {

    class UFont {
        static constexpr int _CHAR_WIDTH       = 8;
        static constexpr int _CHAR_HEIGHT      = 12;
        static constexpr int ATLAS_COLS        = 16;
        static constexpr int ATLAS_ROWS        = 8;
        static constexpr int FONT_ATLAS_WIDTH  = _CHAR_WIDTH * ATLAS_COLS;
        static constexpr int FONT_ATLAS_HEIGHT = _CHAR_HEIGHT * ATLAS_ROWS;

        std::unique_ptr<PImage> font_atlas;

        void generateFontAtlas() {
            font_atlas = std::make_unique<PImage>(FONT_ATLAS_WIDTH, FONT_ATLAS_HEIGHT);
            if (!font_atlas->pixels) {
                return;
            }

            for (uint32_t ascii_char = 32; ascii_char < 128; ++ascii_char) {
                const int charX = ((ascii_char - 32) % ATLAS_COLS) * _CHAR_WIDTH;
                const int charY = ((ascii_char - 32) / ATLAS_COLS) * _CHAR_HEIGHT;
                for (uint32_t row = 0; row < Font_7x10.height; row++) {
                    const uint16_t bits = Font_7x10.data[(ascii_char - 32) * Font_7x10.height + row];
                    for (uint32_t col = 0; col < Font_7x10.width; col++) {
                        const bool set = (bits << col) & 0x8000;
                        const int  px  = charX + col;
                        const int  py  = charY + row;
                        if (px >= FONT_ATLAS_WIDTH || py >= FONT_ATLAS_HEIGHT) {
                            continue;
                        }
                        // White glyph pixel or transparent; 0xAARRGGBB assumed
                        font_atlas->set(static_cast<uint16_t>(px),
                                        static_cast<uint16_t>(py),
                                        set ? 0xFFFFFFFFu : 0x00000000u);
                    }
                }
            }

            font_atlas->set_texture_filter(NEAREST);
            font_atlas->set_texture_wrap(CLAMP_TO_EDGE);
        }

    public:
        UFont() {
            generateFontAtlas();
        }

        PImage* atlas() const { return font_atlas.get(); }

        static std::vector<Vertex> generate(std::vector<Vertex>& vertices, const std::string& text, const float startX, const float startY, glm::vec4 color) {
            float x = startX, y = startY;
            for (const char c: text) {
                const int       index = static_cast<unsigned char>(c) - 32;
                float           u     = (index % ATLAS_COLS) * (1.0f / ATLAS_COLS);
                float           v     = (index / ATLAS_COLS) * (1.0f / ATLAS_ROWS);
                constexpr float uSize = 1.0f / ATLAS_COLS;
                constexpr float vSize = 1.0f / ATLAS_ROWS;

                vertices.emplace_back(x, y, 0, color.r, color.g, color.b, color.a, u, v);
                vertices.emplace_back(x + _CHAR_WIDTH, y, 0, color.r, color.g, color.b, color.a, u + uSize, v);
                vertices.emplace_back(x + _CHAR_WIDTH, y + _CHAR_HEIGHT, 0, color.r, color.g, color.b, color.a, u + uSize, v + vSize);

                vertices.emplace_back(x, y, 0, color.r, color.g, color.b, color.a, u, v);
                vertices.emplace_back(x + _CHAR_WIDTH, y + _CHAR_HEIGHT, 0, color.r, color.g, color.b, color.a, u + uSize, v + vSize);
                vertices.emplace_back(x, y + _CHAR_HEIGHT, 0, color.r, color.g, color.b, color.a, u, v + vSize);

                x += _CHAR_WIDTH;
            }
            return vertices;
        }
    };
} // namespace umfeld
