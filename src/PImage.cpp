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

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "Umfeld.h"
#include "PImage.h"
#include "PGraphics.h"

using namespace umfeld;

/**
* @brief minimal constructor for PImage. does not initialize any data.
*/
PImage::PImage() : width(0),
                   height(0),
                   pixels(nullptr) {}

/**
 *
 * @param width image width in pixels
 * @param height image height in pixels
 */
PImage::PImage(const int width, const int height) : width(static_cast<float>(width)),
                                                    height(static_cast<float>(height)),
                                                    pixels(nullptr) {
    const int length = width * height;
    if (length <= 0) {
        return;
    }
    pixels                = new uint32_t[length]{0x00000000};
    clean_up_pixel_buffer = true;
    PImage::init(pixels, width, height);
}

PImage::PImage(const uint8_t* raw_byte_pixel_data, const int width, const int height, const uint8_t channels) : width(static_cast<float>(width)),
                                                                                                                height(static_cast<float>(height)),
                                                                                                                pixels(nullptr) {
    if (width <= 0 || height <= 0) {
        error("failed to create image. dimension is not valid: width=", width, ", height=", height, ". must be greater than 0");
        return;
    }
    if (raw_byte_pixel_data != nullptr) {
        const int requested_channels = channels;
        pixels                       = convert_bytes_to_pixels(this->width, this->height, channels, raw_byte_pixel_data);
        if (requested_channels != channels) {
            warning("unsupported image channels, defaulting to RGBA forcing 4 color channels.");
        }
        PImage::init(pixels, this->width, this->height);
    } else {
        error("failed to create image. raw byte data is null");
        return;
    }
}

PImage::PImage(const uint8_t* raw_byte_data, const uint32_t length) : width(0),
                                                                      height(0),
                                                                      pixels(nullptr) {
    int      _width              = 0;
    int      _height             = 0;
    int      _channels           = 0;
    uint8_t* raw_pixel_byte_data = stbi_load_from_memory(raw_byte_data, length, &_width, &_height, &_channels, 0);
    console("creating image from raw image data: ", _width, "x", _height, " with ", _channels, " channels");
    pixels = convert_bytes_to_pixels(_width, _height, _channels, raw_pixel_byte_data);
    PImage::init(pixels, _width, _height);
    stbi_image_free(raw_pixel_byte_data);
}

PImage::PImage(const std::string& filepath) : width(0),
                                              height(0),
                                              pixels(nullptr) {
    if (!file_exists(filepath)) {
        error("file not found: '", filepath, "'");
        return;
    }

    int      _width              = 0;
    int      _height             = 0;
    int      _channels           = 0;
    uint8_t* raw_pixel_byte_data = stbi_load(filepath.c_str(), &_width, &_height, &_channels, 0);
    if (raw_pixel_byte_data) {
        pixels = convert_bytes_to_pixels(_width, _height, _channels, raw_pixel_byte_data);
        PImage::init(pixels, _width, _height);
    } else {
        error("failed to load image: ", filepath);
    }
    stbi_image_free(raw_pixel_byte_data);
}

PImage::~PImage() {
    if (pixels != nullptr && clean_up_pixel_buffer) {
        delete[] pixels;
        pixels = nullptr;
    }
    if (sdl_texture != nullptr) {
        SDL_DestroyTexture(sdl_texture);
        sdl_texture = nullptr;
    }
    texture_id = TEXTURE_NOT_GENERATED; // reset texture ID
}

uint32_t* PImage::convert_bytes_to_pixels(const int width, const int height, const int channels, const uint8_t* data) {
    if (channels != 4 && channels != 3) {
        error("unsupported image channels, defaulting to RGBA forcing 4 color channels. this might fail ...");
    }
    const auto pixels = new uint32_t[width * height];
    for (int i = 0; i < width * height; ++i) {
        const int j = i * channels;
        if (channels == 4) {
            pixels[i] = RGBA255(data[j + 0], data[j + 1], data[j + 2], data[j + 3]);
        } else if (channels == 3) {
            pixels[i] = RGBA255(data[j + 0], data[j + 1], data[j + 2], 0xFF);
        }
    }

    // if (channels == 3) {
    //     warning_in_function_once("RGB was converted to RGBA and number of channels is changed to 4");
    // }

    return pixels;
}

void PImage::init(uint32_t* pixels,
                  const int width,
                  const int height) {
    if (pixels == nullptr) {
        warning(umfeld::format_label("PImage::init()"), "pixel buffer is not initialized ( might be intentional )");
    }
    this->pixels = pixels;
    this->width  = static_cast<float>(width);
    this->height = static_cast<float>(height);
}

static float clamp(const float x) {
    return (x < 0 ? 0 : x) > 1 ? 1 : x;
}

void PImage::update(PGraphics*   graphics,
                    const float* pixel_data, // NOTE this is the float version of pixel data, i.e. [0.0, 1.0] range
                    const int    width,
                    const int    height,
                    const int    offset_x,
                    const int    offset_y) {
    /* NOTE pixel data must be 4 times the length of pixels */
    const int length = width * height;
    uint32_t  mPixels[length];
    for (int i = 0; i < width * height; ++i) {
        const int j = i * 4;
        mPixels[i]  = RGBA(clamp(pixel_data[j + 0]),
                           clamp(pixel_data[j + 1]),
                           clamp(pixel_data[j + 2]),
                           clamp(pixel_data[j + 3]));
    }
    update(graphics, mPixels, width, height, offset_x, offset_y);
}

void PImage::update_full_internal(PGraphics* graphics) {
    graphics->upload_texture(this,
                             pixels,
                             static_cast<int>(width), static_cast<int>(height),
                             0, 0);
}

void PImage::update(PGraphics* graphics, const uint32_t* pixel_data) {
    update(graphics, pixel_data, static_cast<int>(this->width), static_cast<int>(this->height), 0, 0);
}

void PImage::updatePixels(PGraphics* graphics) {
    update(graphics, pixels, static_cast<int>(this->width), static_cast<int>(this->height), 0, 0);
}

void PImage::updatePixels(PGraphics* graphics, const int x, const int y, const int w, const int h) {
    if (!pixels) {
        error("pixel array not initialized");
        return;
    }

    if (x < 0 || y < 0 || x + w > static_cast<int>(this->width) || y + h > static_cast<int>(this->height)) {
        return;
    }

    const int length  = w * h;
    auto*     mPixels = new uint32_t[length];
    for (int i = 0; i < length; ++i) {
        const int src_index = (y + i / w) * static_cast<int>(this->width) + (x + i % w);
        mPixels[i]          = pixels[src_index];
    }
    update(graphics, mPixels, w, h, x, y);
    delete[] mPixels;
}

void PImage::loadPixels(PGraphics* graphics) {
    if (graphics == nullptr) {
        return;
    }
    graphics->download_texture(this);
}

void PImage::update(PGraphics*      graphics,
                    const uint32_t* pixel_data,
                    const int       width,
                    const int       height,
                    const int       offset_x,
                    const int       offset_y) {
    if (!pixel_data) {
        error("invalid pixel data");
        return;
    }
    if (!pixels) {
        error("pixel array not initialized");
        return;
    }

    /* copy subregion `pixel_data` with offset into `pixels` */

    // Ensure the region is within bounds
    if (offset_x < 0 || offset_y < 0 ||
        offset_x + width > static_cast<int>(this->width) ||
        offset_y + height > static_cast<int>(this->height)) {
        error("subregion is out of bounds");
        return;
    }

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const int src_index  = y * width + x;
            const int dest_index = (offset_y + y) * static_cast<int>(this->width) + (offset_x + x);
            pixels[dest_index]   = pixel_data[src_index];
        }
    }

    graphics->upload_texture(this, pixel_data, width, height, offset_x, offset_y);
}