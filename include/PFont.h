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

// #define PFONT_DEBUG_FONT

#include <string>
#include <codecvt>

#include <algorithm>
#include <vector>
#include <unordered_map>

#include "harfbuzz/hb.h"
#include "harfbuzz/hb-ft.h"
#include "ft2build.h"
#include FT_FREETYPE_H
#include FT_OUTLINE_H

#include "UmfeldFunctionsAdditional.h"
#include "PImage.h"

namespace umfeld {
    struct TexturedQuad {
        float x0, y0, u0, v0; // Top-left
        float x1, y1, u1, v1; // Top-right
        float x2, y2, u2, v2; // Bottom-right
        float x3, y3, u3, v3; // Bottom-left

        TexturedQuad(const float _x0, const float _y0, const float _u0, const float _v0,
                     const float _x1, const float _y1, const float _u1, const float _v1,
                     const float _x2, const float _y2, const float _u2, const float _v2,
                     const float _x3, const float _y3, const float _u3, const float _v3)
            : x0(_x0), y0(_y0), u0(_u0), v0(_v0),
              x1(_x1), y1(_y1), u1(_u1), v1(_v1),
              x2(_x2), y2(_y2), u2(_u2), v2(_v2),
              x3(_x3), y3(_y3), u3(_u3), v3(_v3) {}
    };

    class PFont final : public PImage {
        const std::string character_atlas_default = " ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()[]{}-_=+;:'\",<.>/?`~";

    public:
        explicit PFont(const std::string& filepath, int font_size, float pixelDensity = 1);
        static constexpr int atlas_pixel_width       = 512;
        static constexpr int atlas_character_padding = 2;
        const float          font_size;

        static PImage* create_image(const std::string& text) {
            error("PImage / implement `create_image`: ", text);
            // TODO implement creation of PImage from text
            return nullptr;
        }

        void textAlign(const int alignX) {
            text_align_x = alignX;
        }

        void textAlign(const int alignX, const int alignY) {
            text_align_x = alignX;
            text_align_y = alignY;
        }

        float textAscent() const {
            if (font == nullptr) {
                return 0.0f;
            }
            if (font_size == 0) {
                return 0.0f;
            }
            return font->ascent * (text_size / font_size);
        }

        float textDescent() const {
            if (font == nullptr) {
                return 0.0f;
            }
            if (font_size == 0) {
                return 0.0f;
            }
            return font->descent * (text_size / font_size);
        }

        float textWidth(const std::string& str) const {
            if (str.empty()) {
                return 0.0f;
            }
            if (font == nullptr) {
                return 0.0f;
            }
            if (font_size == 0) {
                return 0.0f;
            }

            const float text_scale = text_size / font_size;
            return get_text_width(*font, str) * text_scale;
        }

        void textSize(const float size) {
            text_size = size;
        }

        void textLeading(const float leading) {
            text_leading = leading;
        }

        ~PFont() override {
            hb_buffer_destroy(font->buffer);
            hb_font_destroy(font->hb_font);
            FT_Done_Face(font->face);
            delete font;
            FT_Done_FreeType(freetype);
            delete pixels;
        }

    private:
        struct Glyph {
            std::vector<unsigned char> bitmap;
            int                        width{};
            int                        height{};
            int                        left{};
            int                        top{};
            int                        advance{};
            int                        atlas_x{};
            int                        atlas_y{};
        };

        struct FontData {
            std::unordered_map<uint32_t, Glyph> glyphs;
            int                                 ascent{};
            int                                 descent{};
            int                                 line_gap{};
            int                                 atlas_width{};
            int                                 atlas_height{};
            std::vector<unsigned char>          atlas;
            FT_Face                             face{nullptr};
            hb_font_t*                          hb_font{nullptr};
            hb_buffer_t*                        buffer{nullptr};
        };

        std::vector<TexturedQuad> text_quads;
        FontData*                 font{nullptr};
        FT_Library                freetype{nullptr};
        float                     text_size{1};
        float                     text_leading{0};
        int                       text_align_x{LEFT};
        int                       text_align_y{BASELINE};

#ifdef PFONT_DEBUG_FONT
        void DEBUG_save_font_atlas(const FontData& font, const std::string& output_path) const;
        void DEBUG_save_text(const FontData& font, const char* text, const std::string& outputfile) const;
#endif //PFONT_DEBUG_FONT

        // static std::u16string utf8_to_utf16(std::string& utf8) {
        //     std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> convert;
        //     return convert.from_bytes(utf8);
        // }

        static void create_font_atlas(FontData& font, const std::string& characters_in_atlas);

        static void copy_atlas_to_rgba(const FontData& font, unsigned char* atlas_rgba) {
            for (int y = 0; y < font.atlas_height; y++) {
                for (int x = 0; x < font.atlas_width; x++) {
                    const unsigned char val = font.atlas[y * font.atlas_width + x];
                    const int           idx = (y * font.atlas_width + x) * 4;
                    // Store grayscale value into RGBA channels (transparent text on white fond)
                    atlas_rgba[idx + 0] = 255; // R
                    atlas_rgba[idx + 1] = 255; // G
                    atlas_rgba[idx + 2] = 255; // B
                    atlas_rgba[idx + 3] = val; // A (fully opaque)
                }
            }
        }

        static float get_text_width(const FontData& font, const std::string& text) {
            hb_buffer_clear_contents(font.buffer);

            hb_buffer_add_utf8(font.buffer, text.c_str(), -1, 0, -1);
            hb_buffer_set_direction(font.buffer, HB_DIRECTION_LTR);
            hb_buffer_set_script(font.buffer, HB_SCRIPT_LATIN);
            hb_buffer_set_language(font.buffer, hb_language_from_string("en", 2));

            hb_shape(font.hb_font, font.buffer, nullptr, 0);

            unsigned int               glyph_count;
            const hb_glyph_position_t* glyph_pos = hb_buffer_get_glyph_positions(font.buffer, &glyph_count);

            float width = 0.0f;
            for (unsigned int i = 0; i < glyph_count; ++i) {
                width += static_cast<float>(glyph_pos[i].x_advance) / 64.0f; // divide by 64 to convert from subpixels
            }

            return width;
        }

        static void generate_text_quads(const FontData& font, const std::string& text, std::vector<TexturedQuad>& quads);

        static std::vector<std::string> split_lines(const std::string& text) {
            std::vector<std::string> lines;
            std::istringstream       stream(text);
            std::string              line;
            while (std::getline(stream, line)) {
                lines.push_back(line);
            }
            return lines;
        }

    public:
        void draw(PGraphics* g, const std::string& text, float x, float y, float z = 0);

    private:
        struct OutlineContext {
            std::vector<std::vector<glm::vec2>>& outlines;
            glm::vec2                            current_point;
            float                                scale;

            OutlineContext(std::vector<std::vector<glm::vec2>>& out, const float scale)
                : outlines(out), current_point(), scale(scale) {
                outlines.emplace_back();
            }

            void move_to(const float x, const float y) {
                outlines.emplace_back();
                current_point = {x * scale, -y * scale}; // flip y and scale
                outlines.back().push_back(current_point);
            }

            void line_to(const float x, const float y) {
                current_point = {x * scale, -y * scale};
                outlines.back().push_back(current_point);
            }

            void conic_to(float cx, float cy, const float x, const float y) {
                // Approximate conic (quadratic Bézier) with straight line
                // TODO: replace with curve tessellation if needed
                current_point = {x * scale, -y * scale};
                outlines.back().push_back(current_point);
            }

            void cubic_to(float cx1, float cy1, float cx2, float cy2, const float x, const float y) {
                // Approximate cubic Bézier with straight line
                // TODO: replace with curve tessellation if needed
                current_point = {x * scale, -y * scale};
                outlines.back().push_back(current_point);
            }
        };

        static int move_to_callback(const FT_Vector* to, void* user) {
            auto* ctx = static_cast<OutlineContext*>(user);
            ctx->move_to(to->x / 64.0f, to->y / 64.0f);
            return 0;
        }

        static int line_to_callback(const FT_Vector* to, void* user) {
            auto* ctx = static_cast<OutlineContext*>(user);
            ctx->line_to(to->x / 64.0f, to->y / 64.0f);
            return 0;
        }

        static int conic_to_callback(const FT_Vector* control, const FT_Vector* to, void* user) {
            auto* ctx = static_cast<OutlineContext*>(user);
            ctx->conic_to(control->x / 64.0f, control->y / 64.0f, to->x / 64.0f, to->y / 64.0f);
            return 0;
        }

        static int cubic_to_callback(const FT_Vector* c1, const FT_Vector* c2, const FT_Vector* to, void* user) {
            auto* ctx = static_cast<OutlineContext*>(user);
            ctx->cubic_to(
                c1->x / 64.0f, c1->y / 64.0f,
                c2->x / 64.0f, c2->y / 64.0f,
                to->x / 64.0f, to->y / 64.0f);
            return 0;
        }

    public:
        void outline(const std::string& text, std::vector<std::vector<glm::vec2>>& outlines) const;
    };
} // namespace umfeld
