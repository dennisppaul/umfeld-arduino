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

#include <SDL3/SDL.h>
#include "UmfeldConstants.h"

namespace umfeld {

    class PGraphics;
    class PImage {
    public:
        explicit PImage(const std::string& filepath);
        PImage(const uint8_t* raw_byte_data, uint32_t length);
        PImage(int width, int height);
        PImage(const unsigned char* raw_byte_pixel_data, int width, int height, uint8_t channels);
        PImage();
        virtual ~PImage();

        explicit PImage(const PImage* other);

        // Deep copy constructor
        PImage(const PImage& other);
        // Deep copy assignment
        PImage& operator=(const PImage& other);
        // Move constructor
        PImage(PImage&& other) noexcept;
        // Move assignment
        PImage& operator=(PImage&& other) noexcept;

        // explicit deep copy helper
        PImage copy() const { return PImage(*this); }

        virtual void loadPixels(PGraphics* graphics);
        virtual void init(uint32_t* pixels, int width, int height);

        void updatePixels(PGraphics* graphics);
        void updatePixels(PGraphics* graphics, int x, int y, int w, int h);
        void update(PGraphics* graphics, const uint32_t* pixel_data);
        void update(PGraphics* graphics, const uint32_t* pixel_data, int width, int height, int offset_x, int offset_y);
        void update(PGraphics* graphics, const float* pixel_data, int width, int height, int offset_x, int offset_y);

        void set(const uint16_t x, const uint16_t y, const uint32_t c) const {
            if (x >= static_cast<uint16_t>(width) || y >= static_cast<uint16_t>(height)) {
                return;
            }
            pixels[y * static_cast<uint16_t>(width) + x] = c;
        }

        uint32_t get(const uint16_t x, const uint16_t y) const {
            if (x >= static_cast<uint16_t>(width) || y >= static_cast<uint16_t>(height)) {
                return 0;
            }
            const uint32_t c = pixels[y * static_cast<uint16_t>(width) + x];
            return c;
        }

        void set_auto_generate_mipmap(bool generate_mipmap) { auto_generate_mipmap = generate_mipmap; }
        bool get_auto_generate_mipmap() const { return auto_generate_mipmap; }

        void set_texture_wrap(TextureWrap wrap) {
            if (wrap != texture_wrap) {
                texture_wrap       = wrap;
                texture_wrap_dirty = true;
            }
        }
        TextureWrap get_texture_wrap() const { return texture_wrap; }
        bool        is_texture_wrap_dirty() const { return texture_wrap_dirty; }
        void        set_texture_wrap_clean() { texture_wrap_dirty = false; }

        void set_texture_filter(TextureFilter filter) {
            if (filter != texture_filter) {
                texture_filter       = filter;
                texture_filter_dirty = true;
            }
        }
        TextureFilter get_texture_filter() const { return texture_filter; }
        bool          is_texture_filter_dirty() const { return texture_filter_dirty; }
        void          set_texture_filter_clean() { texture_filter_dirty = false; }

        float                    width;
        float                    height;
        uint32_t*                pixels;
        static constexpr uint8_t channels         = DEFAULT_BYTES_PER_PIXELS;
        bool                     flip_y_texcoords = false;
        int                      texture_id       = TEXTURE_NOT_GENERATED;

    protected:
        bool          auto_generate_mipmap{false};
        bool          clean_up_pixel_buffer{false};
        TextureWrap   texture_wrap{CLAMP_TO_EDGE};
        bool          texture_wrap_dirty{true};
        TextureFilter texture_filter{LINEAR};
        bool          texture_filter_dirty{true};

        void update_full_internal(PGraphics* graphics);

    public:
        static uint32_t*    convert_bytes_to_pixels(int width, int height, int channels, const unsigned char* data);
        static PImage       convert_SDL_Surface_to_PImage(SDL_Surface* surface);
        static SDL_Surface* convert_PImage_to_SDL_Surface(const PImage& image);
    };
} // namespace umfeld
