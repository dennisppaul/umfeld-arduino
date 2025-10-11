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

/* deep copy constructor */
PImage::PImage(const PImage& other)
    : width(other.width),
      height(other.height),
      pixels(nullptr),
      flip_y_texcoords(other.flip_y_texcoords),
      texture_id(TEXTURE_NOT_GENERATED), // we allocate our own buffer
      auto_generate_mipmap(other.auto_generate_mipmap),
      clean_up_pixel_buffer(true),
      texture_wrap(other.texture_wrap),
      texture_wrap_dirty(other.texture_wrap_dirty),
      texture_filter(other.texture_filter),
      texture_filter_dirty(other.texture_filter_dirty) { // do not copy GPU handle
    const int len = static_cast<int>(width * height);
    if (other.pixels && len > 0) {
        pixels = new uint32_t[len];
        std::memcpy(pixels, other.pixels, len * sizeof(uint32_t));
    }
}

PImage::PImage(const PImage* other) : PImage(other ? *other : PImage()) {}

// Deep copy assignment
PImage& PImage::operator=(const PImage& other) {
    if (this == &other) {
        return *this;
    }
    // Release current
    if (pixels && clean_up_pixel_buffer) {
        delete[] pixels;
    }
    width  = other.width;
    height = other.height;

    const int len = static_cast<int>(width * height);
    pixels        = nullptr;
    if (other.pixels && len > 0) {
        pixels = new uint32_t[len];
        std::memcpy(pixels, other.pixels, len * sizeof(uint32_t));
    }

    auto_generate_mipmap  = other.auto_generate_mipmap;
    clean_up_pixel_buffer = true;
    texture_wrap          = other.texture_wrap;
    texture_wrap_dirty    = other.texture_wrap_dirty;
    texture_filter        = other.texture_filter;
    texture_filter_dirty  = other.texture_filter_dirty;
    flip_y_texcoords      = other.flip_y_texcoords;
    texture_id            = TEXTURE_NOT_GENERATED; // force re-upload if needed
    return *this;
}

// Move constructor
PImage::PImage(PImage&& other) noexcept
    : width(other.width),
      height(other.height),
      pixels(other.pixels),
      flip_y_texcoords(other.flip_y_texcoords),
      texture_id(other.texture_id),
      auto_generate_mipmap(other.auto_generate_mipmap),
      clean_up_pixel_buffer(other.clean_up_pixel_buffer),
      texture_wrap(other.texture_wrap),
      texture_wrap_dirty(other.texture_wrap_dirty),
      texture_filter(other.texture_filter),
      texture_filter_dirty(other.texture_filter_dirty) {
    other.width = other.height  = 0;
    other.pixels                = nullptr;
    other.clean_up_pixel_buffer = false;
    other.texture_id            = TEXTURE_NOT_GENERATED;
}

// Move assignment
PImage& PImage::operator=(PImage&& other) noexcept {
    if (this == &other) {
        return *this;
    }
    if (pixels && clean_up_pixel_buffer) {
        delete[] pixels;
    }
    width                 = other.width;
    height                = other.height;
    pixels                = other.pixels;
    auto_generate_mipmap  = other.auto_generate_mipmap;
    clean_up_pixel_buffer = other.clean_up_pixel_buffer;
    texture_wrap          = other.texture_wrap;
    texture_wrap_dirty    = other.texture_wrap_dirty;
    texture_filter        = other.texture_filter;
    texture_filter_dirty  = other.texture_filter_dirty;
    flip_y_texcoords      = other.flip_y_texcoords;
    texture_id            = other.texture_id;

    other.width = other.height  = 0;
    other.pixels                = nullptr;
    other.clean_up_pixel_buffer = false;
    other.texture_id            = TEXTURE_NOT_GENERATED;
    return *this;
}

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
    // if (sdl_texture != nullptr) {
    //     SDL_DestroyTexture(sdl_texture);
    //     sdl_texture = nullptr;
    // }
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
            pixels[i] = RGBAi(data[j + 0], data[j + 1], data[j + 2], data[j + 3]);
        } else if (channels == 3) {
            pixels[i] = RGBAi(data[j + 0], data[j + 1], data[j + 2], 0xFF);
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

void PImage::resize(int width, int height) {
    warning_in_function_once("not implemented yet");
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
    const int             length = width * height;
    std::vector<uint32_t> _pixels(length);
    for (int i = 0; i < width * height; ++i) {
        const int j = i * 4;
        _pixels[i]  = RGBAf(clamp(pixel_data[j + 0]),
                            clamp(pixel_data[j + 1]),
                            clamp(pixel_data[j + 2]),
                            clamp(pixel_data[j + 3]));
    }
    update(graphics, _pixels.data(), width, height, offset_x, offset_y);
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

PImage PImage::convert_SDL_Surface_to_PImage(SDL_Surface* surface) {
    if (!surface) {
        return PImage(); // Return empty image if surface is null
    }

    const int width  = surface->w;
    const int height = surface->h;
    // constexpr int format = 4; // TODO used to be 'SDL_PIXELFORMAT_RGBA8888;' but needs to be '4' // Assuming default RGBA channels

    PImage image(width, height);

    // Convert surface to the correct channels if necessary
    SDL_Surface* convertedSurface = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA8888);
    if (!convertedSurface) {
        return PImage(); // Conversion failed, return empty image
    }

    // Copy pixels
    image.pixels = new uint32_t[width * height];
    std::memcpy(image.pixels, convertedSurface->pixels, width * height * sizeof(uint32_t));

    SDL_DestroySurface(convertedSurface); // Clean up converted surface

    return image;
}

SDL_Surface* PImage::convert_PImage_to_SDL_Surface(const PImage& image) {
    if (!image.pixels || image.width <= 0 || image.height <= 0) {
        return nullptr;
    }

    // Create SDL surface with the correct channels
    SDL_Surface* surface = SDL_CreateSurface(image.width, image.height, SDL_PIXELFORMAT_RGBA8888);
    if (!surface) {
        return nullptr;
    }

    // Copy pixel data into the surface
    std::memcpy(surface->pixels, image.pixels, image.width * image.height * sizeof(uint32_t));

    return surface;
}