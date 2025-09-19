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

#include "Umfeld.h"
#include "PFont.h"

using namespace umfeld;

PFont::PFont(const std::string& filepath,
             const int          font_size,
             const float        pixelDensity) : font_size(font_size) {
    if (!file_exists(filepath)) {
        error("PFont / file not found: '", filepath, "'");
        return;
    }

    const std::string character_atlas = character_atlas_default;
    (void) pixelDensity; // TODO implement pixel density

    /* init freetype and font struct */
    const char* filepath_c = filepath.c_str();
    FT_Init_FreeType(&freetype);
    font         = new FontData();
    font->buffer = hb_buffer_create();
    FT_New_Face(freetype, filepath_c, 0, &font->face);
    FT_Set_Pixel_Sizes(font->face, 0, font_size);
    font->hb_font = hb_ft_font_create(font->face, nullptr);

    create_font_atlas(*font, character_atlas);

    // TODO see if pixel density needs to or should be respected in the atlas

    // tex_id = create_font_texture(*font); // NOTE this is done in PGraphics
    width  = static_cast<float>(font->atlas_width);
    height = static_cast<float>(font->atlas_height);
    pixels = new uint32_t[static_cast<int>(width * height)];
    set_auto_generate_mipmap(true); // NOTE set mipmap generation to true by default
    copy_atlas_to_rgba(*font, reinterpret_cast<unsigned char*>(pixels));

    console(format_label("PFont"), "atlas created");
    console(format_label("PFont atlas size"), width, "×", height, " px");
    textSize(font_size);
    textLeading(font_size * 1.2f);
#ifdef PFONT_DEBUG_FONT
    DEBUG_save_font_atlas(*font, font_filepath + "--font_atlas.png");
    DEBUG_save_text(*font, "AVTAWaToVAWeYoyo Hamburgefonts", font_filepath + "--text.png");
#endif //PFONT_DEBUG_FONT
}

void PFont::outline(const std::string& text, std::vector<std::vector<glm::vec2>>& outlines) const {
    if (font == nullptr) {
        return;
    }

    hb_buffer_clear_contents(font->buffer);
    hb_buffer_add_utf8(font->buffer, text.c_str(), -1, 0, -1);
    hb_buffer_guess_segment_properties(font->buffer);
    hb_shape(font->hb_font, font->buffer, nullptr, 0);

    unsigned int           glyph_count;
    const hb_glyph_info_t* glyph_info = hb_buffer_get_glyph_infos(font->buffer, &glyph_count);

    constexpr FT_Outline_Funcs funcs{
        move_to_callback,
        line_to_callback,
        conic_to_callback,
        cubic_to_callback,
        0, 0};

    float pen_x = 0;
    float pen_y = 0;

    const auto* pos = hb_buffer_get_glyph_positions(font->buffer, nullptr);

    for (unsigned int i = 0; i < glyph_count; ++i) {
        const FT_UInt glyph_index = glyph_info[i].codepoint;
        FT_Load_Glyph(font->face, glyph_index, FT_LOAD_NO_BITMAP | FT_LOAD_NO_HINTING);
        if (font->face->glyph->format == FT_GLYPH_FORMAT_OUTLINE) {
            const float    text_scale = text_size / font_size;
            OutlineContext ctx(outlines, text_scale);

            // offset pen position before decomposition
            FT_Outline* outline = &font->face->glyph->outline;

            // move all outline points manually before decomposing
            for (int j = 0; j < outline->n_points; ++j) {
                outline->points[j].x += pen_x;
                outline->points[j].y += pen_y;
            }

            FT_Outline_Decompose(outline, &funcs, &ctx);

            // restore original outline (optional, in case reused)
            for (int j = 0; j < outline->n_points; ++j) {
                outline->points[j].x -= pen_x;
                outline->points[j].y -= pen_y;
            }
        }

        pen_x += pos[i].x_advance;
        pen_y += pos[i].y_advance;
    }
}

void PFont::draw(PGraphics* g, const std::string& text, const float x, const float y, const float z) {
    if (font == nullptr) {
        return;
    }
    if (g == nullptr || font_size == 0) {
        return;
    }

    const float text_scale = text_size / font_size;
    const float ascent     = font->ascent;
    const float descent    = font->descent;

    const std::vector<std::string> lines = split_lines(text); // see helper below

    float y_offset = -ascent;

    // vertical alignment adjustment (now considers multiple lines)
    const float total_height = lines.size() * text_leading;
    switch (text_align_y) {
        case TOP: y_offset += ascent; break;
        case CENTER: y_offset += ascent - total_height * 0.5f; break;
        case BOTTOM: y_offset -= total_height - descent; break;
        case BASELINE:
        default:
            break;
    }

    g->pushMatrix();
    g->translate(x, y, z);
    g->scale(text_scale, text_scale, 1);
    g->translate(0, y_offset, 0);

    g->push_texture_id();
    g->push_force_transparent();
    g->set_shape_force_transparent(true);

    g->texture(this);

    for (std::size_t i = 0; i < lines.size(); ++i) {
        const std::string& line       = lines[i];
        const float        line_width = get_text_width(*font, line);

        float x_offset = 0;
        switch (text_align_x) {
            case CENTER: x_offset -= line_width * 0.5f; break;
            case RIGHT: x_offset -= line_width; break;
            case LEFT:
            default:
                break;
        }

        generate_text_quads(*font, line, text_quads);
        // TODO maybe deactive stroke shape here to avoid outline artifacts
        g->pushMatrix();
        g->translate(x_offset, i * text_leading, 0); // baseline offset for current line
        g->beginShape(TRIANGLES);
        for (const auto& q: text_quads) {
            g->vertex(q.x0, q.y0, 0, q.u0, q.v0);
            g->vertex(q.x1, q.y1, 0, q.u1, q.v1);
            g->vertex(q.x2, q.y2, 0, q.u2, q.v2);

            g->vertex(q.x3, q.y3, 0, q.u3, q.v3);
            g->vertex(q.x0, q.y0, 0, q.u0, q.v0);
            g->vertex(q.x2, q.y2, 0, q.u2, q.v2);
        }
        g->endShape(CLOSE);
        g->popMatrix();
    }

    g->pop_texture_id();
    g->pop_force_transparent();
    g->popMatrix();
}

void PFont::generate_text_quads(const FontData&            font,
                                const std::string&         text,
                                std::vector<TexturedQuad>& quads) {
    quads.clear();
    hb_buffer_clear_contents(font.buffer);

    hb_buffer_add_utf8(font.buffer, text.c_str(), -1, 0, -1);
    hb_buffer_set_direction(font.buffer, HB_DIRECTION_LTR);
    hb_buffer_set_script(font.buffer, HB_SCRIPT_LATIN);
    hb_buffer_set_language(font.buffer, hb_language_from_string("en", 2));

    hb_shape(font.hb_font, font.buffer, nullptr, 0);

    unsigned int               glyph_count;
    const hb_glyph_info_t*     glyph_info = hb_buffer_get_glyph_infos(font.buffer, &glyph_count);
    const hb_glyph_position_t* glyph_pos  = hb_buffer_get_glyph_positions(font.buffer, &glyph_count);

    // Dynamically decide whether to preallocate
    if (text.length() > 100) { // Only preallocate if the text is long
        size_t estimated_quads = 0;
        for (unsigned int i = 0; i < glyph_count; i++) {
            uint32_t glyph_id = glyph_info[i].codepoint;
            if (glyph_id != ' ' && font.glyphs.find(glyph_id) != font.glyphs.end()) {
                estimated_quads++; // Only count valid glyphs (not spaces)
            }
        }
        quads.reserve(estimated_quads);
    }

    float      x = 0.0f;
    const auto y = static_cast<float>(font.ascent); // Baseline position

    for (unsigned int i = 0; i < glyph_count; i++) {
        uint32_t glyph_id = glyph_info[i].codepoint;

        auto it = font.glyphs.find(glyph_id);
        if (it == font.glyphs.end()) {
            // Handle spaces explicitly
            if (glyph_id == ' ') {
                x += static_cast<float>(glyph_pos[i].x_advance >> 6); // Move forward for spaces
            }
            continue; // Skip unsupported glyphs
        }

        const Glyph& g = it->second;

        float      x_pos = x + static_cast<float>(g.left + (glyph_pos[i].x_offset >> 6));
        float      y_pos = y - static_cast<float>(g.top + (glyph_pos[i].y_offset >> 6));
        const auto w     = static_cast<float>(g.width);
        const auto h     = static_cast<float>(g.height);

        // Compute texture coordinates
        float u0 = static_cast<float>(g.atlas_x) / static_cast<float>(font.atlas_width);
        float v0 = static_cast<float>(g.atlas_y) / static_cast<float>(font.atlas_height);
        float u1 = static_cast<float>(g.atlas_x + g.width) / static_cast<float>(font.atlas_width);
        float v1 = static_cast<float>(g.atlas_y + g.height) / static_cast<float>(font.atlas_height);

        // Add textured quad
        quads.emplace_back(
            x_pos, y_pos, u0, v0,         // Top-left
            x_pos + w, y_pos, u1, v0,     // Top-right
            x_pos + w, y_pos + h, u1, v1, // Bottom-right
            x_pos, y_pos + h, u0, v1      // Bottom-left
        );

        x += static_cast<float>(glyph_pos[i].x_advance >> 6); // Move forward
    }
}

void PFont::create_font_atlas(FontData& font, const std::string& characters_in_atlas) {
    if (font.face == nullptr || font.hb_font == nullptr) {
        error("font data not intizialized");
        return;
    }

    font.ascent   = static_cast<int>(font.face->size->metrics.ascender >> 6);
    font.descent  = static_cast<int>(-font.face->size->metrics.descender >> 6);
    font.line_gap = static_cast<int>(font.face->size->metrics.height >> 6);

    hb_buffer_clear_contents(font.buffer);
    // std::u16string s16 = utf8_to_utf16(characters_in_atlas);
    // std::vector<uint16_t> utf16_vec(s16.begin(), s16.end());
    // utf16_vec.push_back(0);  // Add null terminator
    // const uint16_t* raw_utf16 = utf16_vec.data();
    // hb_buffer_add_utf16(font.buffer, raw_utf16, -1, 0, -1);
    hb_buffer_add_utf8(font.buffer, characters_in_atlas.c_str(), -1, 0, -1);
    hb_buffer_set_direction(font.buffer, HB_DIRECTION_LTR);
    hb_buffer_set_script(font.buffer, HB_SCRIPT_LATIN);
    hb_buffer_set_language(font.buffer, hb_language_from_string("en", 2));

    hb_shape(font.hb_font, font.buffer, nullptr, 0);

    unsigned int           glyph_count;
    const hb_glyph_info_t* glyph_info = hb_buffer_get_glyph_infos(font.buffer, &glyph_count);

    int           current_x      = 0;
    int           current_y      = 0;
    int           max_row_height = 0;
    constexpr int atlas_width    = atlas_pixel_width;
    int           atlas_height   = 0;

    for (unsigned int i = 0; i < glyph_count; i++) {
        FT_Load_Glyph(font.face, glyph_info[i].codepoint, FT_LOAD_RENDER);
        const FT_GlyphSlot glyph = font.face->glyph;

        if (font.glyphs.find(glyph_info[i].codepoint) == font.glyphs.end()) {
            Glyph g;
            g.width   = static_cast<int>(glyph->bitmap.width);
            g.height  = static_cast<int>(glyph->bitmap.rows);
            g.left    = glyph->bitmap_left;
            g.top     = glyph->bitmap_top;
            g.advance = static_cast<int>(glyph->advance.x) >> 6;

            // Ensure correct copying of bitmap buffer
            if (glyph->bitmap.buffer) {
                g.bitmap.assign(glyph->bitmap.buffer, glyph->bitmap.buffer + g.width * g.height);
            } else {
                g.bitmap.assign(g.width * g.height, 0);
            }

            if (current_x + g.width + atlas_character_padding > atlas_width) {
                current_x = 0;
                current_y += max_row_height + atlas_character_padding;
                max_row_height = 0;
            }

            g.atlas_x = current_x;
            g.atlas_y = current_y;

            current_x += g.width + atlas_character_padding;
            max_row_height = std::max(max_row_height, g.height);

            font.glyphs[glyph_info[i].codepoint] = g;
        }
    }

    atlas_height      = current_y + max_row_height; // Fix last row height
    font.atlas_width  = atlas_width;
    font.atlas_height = atlas_height;
    font.atlas.resize(atlas_width * atlas_height, 0);

    for (const auto& pair: font.glyphs) {
        const Glyph& g = pair.second;
        for (int y = 0; y < g.height; y++) {
            for (int x = 0; x < g.width; x++) {
                const int atlas_x                           = g.atlas_x + x;
                const int atlas_y                           = g.atlas_y + y;
                font.atlas[atlas_y * atlas_width + atlas_x] = g.bitmap[y * g.width + x];
            }
        }
    }
}

#ifdef PFONT_DEBUG_FONT
#define STB_IMAGE_WRITE_IMPLEMENTATION // TODO why does this cause a duplicate symbol error?
#include "stb_image_write.h"
#endif //PFONT_DEBUG_FONT

#ifdef PFONT_DEBUG_FONT
void PFont::DEBUG_save_font_atlas(const FontData& font, const std::string& output_path) const {
    // Convert the grayscale atlas to an RGBA image for better visibility
    std::vector<unsigned char> atlas_rgba(font.atlas_width * font.atlas_height * 4, 255);
    for (int y = 0; y < font.atlas_height; y++) {
        for (int x = 0; x < font.atlas_width; x++) {
            const unsigned char val = font.atlas[y * font.atlas_width + x];
            const int           idx = (y * font.atlas_width + x) * 4;

            // Store grayscale value into RGBA channels (transparent text on white fond)
            atlas_rgba[idx + 0] = 255; // R
            atlas_rgba[idx + 1] = 255; // G
            atlas_rgba[idx + 2] = 255; // B
            atlas_rgba[idx + 3] = val; // A (fully opaque)

            // // Store grayscale value into RGBA channels (white text on black background)
            // atlas_rgba[idx + 0] = val; // R
            // atlas_rgba[idx + 1] = val; // G
            // atlas_rgba[idx + 2] = val; // B
            // atlas_rgba[idx + 3] = 255; // A (fully opaque)
        }
    }

    // Save as PNG
    stbi_write_png(output_path.c_str(), font.atlas_width, font.atlas_height, 4, atlas_rgba.data(), font.atlas_width * 4);
    console("Font atlas saved to: ", output_path);
}

void PFont::DEBUG_save_text(const FontData&    font,
                            const char*        text,
                            const std::string& outputfile) const {

    hb_buffer_clear_contents(font.buffer);
    hb_buffer_add_utf8(font.buffer, text, -1, 0, -1);
    hb_buffer_set_direction(font.buffer, HB_DIRECTION_LTR);
    hb_buffer_set_script(font.buffer, HB_SCRIPT_LATIN);
    hb_buffer_set_language(font.buffer, hb_language_from_string("en", 2));

    // Use the pre-existing HarfBuzz font
    hb_shape(font.hb_font, font.buffer, nullptr, 0);

    unsigned int               glyph_count;
    const hb_glyph_info_t*     glyph_info = hb_buffer_get_glyph_infos(font.buffer, &glyph_count);
    const hb_glyph_position_t* glyph_pos  = hb_buffer_get_glyph_positions(font.buffer, &glyph_count);

    const int max_height      = font.ascent + font.descent;
    int       total_advance   = 0;
    int       trimmed_advance = 0; // Tracks actual needed width

    // Compute max width while ignoring missing glyphs
    for (unsigned int i = 0; i < glyph_count; i++) {
        uint32_t glyph_id = glyph_info[i].codepoint;
        if (glyph_id == ' ' || font.glyphs.find(glyph_id) != font.glyphs.end()) {
            trimmed_advance += glyph_pos[i].x_advance >> 6; // Count only valid glyphs and spaces
        }
        total_advance += glyph_pos[i].x_advance >> 6; // Original width
    }

    total_advance = trimmed_advance; // Trim unused space

    std::vector<unsigned char> image;
    image.assign(total_advance * max_height, 0);

    int       x = 0;
    const int y = font.ascent; // Baseline position

    for (unsigned int i = 0; i < glyph_count; i++) {
        uint32_t glyph_id = glyph_info[i].codepoint;

        auto it = font.glyphs.find(glyph_id);
        if (it == font.glyphs.end()) {
            if (glyph_id == ' ') {
                x += glyph_pos[i].x_advance >> 6; // Move forward for spaces
            }
            continue;
        }

        const Glyph& g = it->second;

        const int x_pos = x + g.left + (glyph_pos[i].x_offset >> 6);
        const int y_pos = y - g.top + (glyph_pos[i].y_offset >> 6);

        for (int row = 0; row < g.height; row++) {
            for (int col = 0; col < g.width; col++) {
                const int atlas_x = g.atlas_x + col;
                const int atlas_y = g.atlas_y + row;
                const int img_x   = x_pos + col;
                const int img_y   = y_pos + row;

                if (img_x >= 0 && img_x < total_advance && img_y >= 0 && img_y < max_height) {
                    if (atlas_x >= 0 && atlas_x < font.atlas_width &&
                        atlas_y >= 0 && atlas_y < font.atlas_height) {
                        unsigned char val                    = font.atlas[atlas_y * font.atlas_width + atlas_x];
                        image[img_y * total_advance + img_x] = std::max(image[img_y * total_advance + img_x], val);
                    }
                }
            }
        }

        x += glyph_pos[i].x_advance >> 6; // Move forward
    }

    // Save image
    //     stbi_write_png(output_path, total_advance, max_height, 1, image.data(), total_advance);
    std::vector<unsigned char> image_rgba(image.size() * 4, 255);
    for (int i = 0; i < image.size(); ++i) {
        image_rgba[i * 4 + 0] = 255;
        image_rgba[i * 4 + 1] = 255;
        image_rgba[i * 4 + 2] = 255;
        image_rgba[i * 4 + 3] = image[i];
    }
    stbi_write_png(outputfile.c_str(), total_advance, max_height, 4, image_rgba.data(), total_advance * 4);
}
#endif //PFONT_DEBUG_FONT
