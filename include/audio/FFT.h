
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

#include <cmath>
#include <memory>
#include <vector>
#include "pffft.h"

namespace umfeld {

    // TODO add `fast_sqrt`

inline void aligned_free_wrapper(void* ptr) {
#ifdef _WIN32
    _aligned_free(ptr);
#else
    free(ptr);
#endif
}

struct FFTContext {
    std::unique_ptr<float, void(*)(void*)> input_aligned  = { nullptr, aligned_free_wrapper };
    std::unique_ptr<float, void(*)(void*)> output_aligned = { nullptr, aligned_free_wrapper };
    std::vector<float> window;
    int fft_size = 0;
    float sample_rate = 0.f;
    PFFFT_Setup* setup = nullptr;
};

    static FFTContext ctx;

    inline float bin_width_Hz() { return ctx.sample_rate / ctx.fft_size; }

    inline float fft_power(float real, float imag);
    inline float fft_amplitude(float real, float imag);
    inline float fft_db(float power, float floor);
    inline std::vector<float> fft_make_hann_window(int fft_size);

    inline std::vector<std::pair<float, float>> fft_extract_db_with_freq(const float *out, int fft_size,
                                                                         float sample_rate, float min_freq,
                                                                         float max_freq, float floor);
    inline std::vector<std::pair<float, float>>
    fft_extract_power_with_freq(const float *out, int fft_size, float sample_rate, float min_freq, float max_freq);
    inline std::vector<std::pair<float, float>> fft_extract_power_with_freq_DC(const float *out, int fft_size,
                                                                               float sample_rate, float min_freq,
                                                                               float max_freq, bool include_dc);
    inline std::vector<std::pair<float, float>>
    fft_extract_amplitude_with_freq(const float *out, int fft_size, float sample_rate, float min_freq, float max_freq);

    // ---------------------------------------------------------------------
    // API
    // ---------------------------------------------------------------------

    inline void fft_start(const int fft_size, const float sample_rate) {
        ctx.fft_size = fft_size;
        ctx.sample_rate = sample_rate;
        ctx.window = fft_make_hann_window(fft_size);

#ifdef _WIN32
    void* in_ptr  = _aligned_malloc(sizeof(float) * fft_size, 16);
    void* out_ptr = _aligned_malloc(sizeof(float) * fft_size, 16);
#else
    void* in_ptr  = aligned_alloc(16, sizeof(float) * fft_size);
    void* out_ptr = aligned_alloc(16, sizeof(float) * fft_size);
#endif

ctx.input_aligned  = std::unique_ptr<float, void(*)(void*)>(static_cast<float*>(in_ptr), aligned_free_wrapper);
ctx.output_aligned = std::unique_ptr<float, void(*)(void*)>(static_cast<float*>(out_ptr), aligned_free_wrapper);

        ctx.setup = pffft_new_setup(fft_size, PFFFT_REAL);
    }

    // Compute sum(window[i]^2) once per window
    inline float compute_window_energy(const std::vector<float> &window) {
        float sum = 0.0f;
        for (float v: window) {
            sum += v * v;
        }
        return sum;
    }

    // Normalize a single power value
    inline float normalize_power(float power, int fft_size, float window_energy) {
        if (window_energy <= 0.0f)
            return 0.0f;
        return power / (fft_size * window_energy);
    }

    // Normalize a vector of power values
    inline void normalize_power_vector(std::vector<std::pair<float, float>> &power_bins, int fft_size,
                                       float window_energy) {
        for (auto &[freq, power]: power_bins) {
            power = normalize_power(power, fft_size, window_energy);
        }
    }

    // return values are in DB
    inline std::vector<std::pair<float, float>> fft_process_db(const float *input, const float min_freq = 20.0f,
                                                               const float max_freq = 20000.0f,
                                                               const bool include_dc = true) {
        const int N = ctx.fft_size;

        // Apply window to input
        for (int i = 0; i < N; ++i) {
            ctx.input_aligned.get()[i] = input[i] * ctx.window[i];
        }

        // Perform FFT
        pffft_transform_ordered(ctx.setup, ctx.input_aligned.get(), ctx.output_aligned.get(), nullptr, PFFFT_FORWARD);

        // Return frequency-dB pairs
        return fft_extract_db_with_freq(ctx.output_aligned.get(), N, ctx.sample_rate, min_freq, max_freq, include_dc);
    }

    inline std::vector<std::pair<float, float>> fft_process_db_binned(const float *input, const int num_bins,
                                                                      const float min_freq = 20.0f,
                                                                      const float max_freq = 20000.0f,
                                                                      const bool include_dc = false) {
        const int N = ctx.fft_size;
        const float bin_step = ctx.sample_rate / N;

        // Step 1: Full-resolution FFT dB spectrum
        const std::vector<std::pair<float, float>> full_spectrum =
                fft_process_db(input, min_freq, max_freq, include_dc);

        // Step 2: Divide into user-defined output bins
        std::vector<std::pair<float, float>> result;
        result.reserve(num_bins);

        const float freq_range = max_freq - min_freq;

        for (int b = 0; b < num_bins; ++b) {
            const float bin_freq_start = min_freq + (freq_range * b) / num_bins;
            const float bin_freq_end = min_freq + (freq_range * (b + 1)) / num_bins;
            const float bin_center = 0.5f * (bin_freq_start + bin_freq_end);

            // Accumulate dB values inside this frequency range
            float sum_db = 0.0f;
            int count = 0;

            for (const auto &[freq, db]: full_spectrum) {
                if (freq >= bin_freq_start && freq < bin_freq_end) {
                    sum_db += db;
                    ++count;
                }
            }

            float avg_db = (count > 0) ? sum_db / count : -100.0f; // fallback for empty bins
            result.emplace_back(bin_center, avg_db);
        }

        return result;
    }

    inline std::vector<std::pair<float, float>> fft_process_power(const float *input, const float min_freq = 20.0f,
                                                                  const float max_freq = 20000.0f,
                                                                  const bool include_dc = true) {
        const int N = ctx.fft_size;

        // Apply window to input
        for (int i = 0; i < N; ++i) {
            ctx.input_aligned.get()[i] = input[i] * ctx.window[i];
        }

        // Perform FFT
        pffft_transform_ordered(ctx.setup, ctx.input_aligned.get(), ctx.output_aligned.get(), nullptr, PFFFT_FORWARD);

        // Return frequency-power pairs
        return fft_extract_power_with_freq_DC(ctx.output_aligned.get(), N, ctx.sample_rate, min_freq, max_freq,
                                              include_dc);
    }

    inline std::vector<std::pair<float, float>> fft_process_power_binned(const float *input, const int num_bins,
                                                                         const float min_freq = 20.0f,
                                                                         const float max_freq = 20000.0f,
                                                                         const bool include_dc = false) {
        const std::vector<std::pair<float, float>> full_spectrum =
                fft_process_power(input, min_freq, max_freq, include_dc);

        std::vector<std::pair<float, float>> result;
        result.reserve(num_bins);

        const float freq_range = max_freq - min_freq;

        for (int b = 0; b < num_bins; ++b) {
            const float bin_freq_start = min_freq + (freq_range * b) / num_bins;
            const float bin_freq_end = min_freq + (freq_range * (b + 1)) / num_bins;
            const float bin_center = 0.5f * (bin_freq_start + bin_freq_end);

            float sum = 0.0f;
            int count = 0;

            for (const auto &[freq, power]: full_spectrum) {
                if (freq >= bin_freq_start && freq < bin_freq_end) {
                    sum += power;
                    ++count;
                }
            }

            float avg_power = (count > 0) ? sum / count : 0.0f;
            result.emplace_back(bin_center, avg_power);
        }

        return result;
    }

    inline std::vector<std::pair<float, float>> fft_process_amplitude(const float *input, const float min_freq = 20.0f,
                                                                      const float max_freq = 20000.0f,
                                                                      const bool include_dc = true) {
        const int N = ctx.fft_size;

        // Apply window to input
        for (int i = 0; i < N; ++i) {
            ctx.input_aligned.get()[i] = input[i] * ctx.window[i];
        }

        // Perform FFT
        pffft_transform_ordered(ctx.setup, ctx.input_aligned.get(), ctx.output_aligned.get(), nullptr, PFFFT_FORWARD);

        // Extract frequency–amplitude pairs
        return fft_extract_amplitude_with_freq(ctx.output_aligned.get(), N, ctx.sample_rate, min_freq, max_freq);
    }

    inline std::vector<std::pair<float, float>> fft_process_amplitude_binned(const float *input, const int num_bins,
                                                                             const float min_freq = 20.0f,
                                                                             const float max_freq = 20000.0f,
                                                                             const bool include_dc = false) {
        const std::vector<std::pair<float, float>> full_spectrum =
                fft_process_amplitude(input, min_freq, max_freq, include_dc);

        std::vector<std::pair<float, float>> result;
        result.reserve(num_bins);

        const float freq_range = max_freq - min_freq;

        for (int b = 0; b < num_bins; ++b) {
            const float bin_freq_start = min_freq + (freq_range * b) / num_bins;
            const float bin_freq_end = min_freq + (freq_range * (b + 1)) / num_bins;
            const float bin_center = 0.5f * (bin_freq_start + bin_freq_end);

            float sum = 0.0f;
            int count = 0;

            for (const auto &[freq, amp]: full_spectrum) {
                if (freq >= bin_freq_start && freq < bin_freq_end) {
                    sum += amp;
                    ++count;
                }
            }

            float avg_amp = (count > 0) ? sum / count : 0.0f;
            result.emplace_back(bin_center, avg_amp);
        }

        return result;
    }

    inline std::vector<std::pair<float, float>> fft_process(const float *input, const float min_freq = 20.0f,
                                                            const float max_freq = 20000.0f) {
        return fft_process_db(input, min_freq, max_freq, true);
    }

    inline void fft_stop() {
        if (ctx.setup) {
            pffft_destroy_setup(ctx.setup);
            ctx.setup = nullptr;
        }
        ctx.input_aligned.reset();
        ctx.output_aligned.reset();
        ctx.window.clear();
        ctx.fft_size = 0;
        ctx.sample_rate = 0;
    }

    // ---------------------------------------------------------------------
    // LOW_LEVEL FUNCTIONS
    // ---------------------------------------------------------------------

    // NOTE example usage: `auto db_bands = fft_extract_db_with_freq(out, fft_size, 44100.0f, 20.0f, 20000.0f);`

    // NOTE bin_width_Hz = sample_rate_Hz / fft_size
    //      e.g `bin_width = 44100 Hz / 2048 ≈ 21.53 Hz`

    // NOTE DC offset: float dc_amplitude = std::abs(out[0]); // for PFFFT_REAL
    //                 float dc_power     = out[0] * out[0];
    //                 float dc_db        = 10.0f * std::log10(std::max(dc_power, 1e-10f));

    inline float fft_power(const float real, const float imag) { return real * real + imag * imag; }

    inline float fft_amplitude(const float real, const float imag) { return std::sqrt(fft_power(real, imag)); }

    inline float fft_db(const float power, const float floor = 1e-10f) {
        return 10.0f * std::log10(std::max(power, floor));
    }

    // Extract power values from FFT output, limited to a frequency range
    inline std::vector<float> fft_extract_power(const float *out, const int fft_size, const float sample_rate,
                                                const float min_freq, const float max_freq) {
        const int min_bin = std::max(1, static_cast<int>(min_freq * fft_size / sample_rate));
        const int max_bin = std::min(fft_size / 2 - 1, static_cast<int>(max_freq * fft_size / sample_rate));

        std::vector<float> result;
        result.reserve(max_bin - min_bin + 1);

        for (int k = min_bin; k <= max_bin; ++k) {
            const float real = out[2 * k];
            const float imag = out[2 * k + 1];
            result.push_back(fft_power(real, imag));
        }

        return result;
    }

    // Extract amplitude values from FFT output, limited to a frequency range
    inline std::vector<float> fft_extract_amplitude(const float *out, const int fft_size, const float sample_rate,
                                                    const float min_freq, const float max_freq) {
        std::vector<float> result;
        const auto powers = fft_extract_power(out, fft_size, sample_rate, min_freq, max_freq);
        result.reserve(powers.size());
        for (const float power: powers) {
            result.push_back(std::sqrt(power));
        }
        return result;
    }

    // Extract decibel values from FFT output, limited to a frequency range
    inline std::vector<float> fft_extract_db(const float *out, const int fft_size, const float sample_rate,
                                             const float min_freq, const float max_freq, const float floor = 1e-10f) {
        std::vector<float> result;
        const auto powers = fft_extract_power(out, fft_size, sample_rate, min_freq, max_freq);
        result.reserve(powers.size());
        for (float power: powers) {
            result.push_back(10.0f * std::log10(std::max(power, floor)));
        }
        return result;
    }

    // Power (frequency, power) pairs
    inline std::vector<std::pair<float, float>> fft_extract_power_with_freq(const float *out, const int fft_size,
                                                                            const float sample_rate,
                                                                            const float min_freq,
                                                                            const float max_freq) {
        const int min_bin = std::max(1, static_cast<int>(min_freq * fft_size / sample_rate));
        const int max_bin = std::min(fft_size / 2 - 1, static_cast<int>(max_freq * fft_size / sample_rate));

        std::vector<std::pair<float, float>> result;
        result.reserve(max_bin - min_bin + 1);

        for (int k = min_bin; k <= max_bin; ++k) {
            const float real = out[2 * k];
            const float imag = out[2 * k + 1];
            float freq = k * sample_rate / fft_size;
            result.emplace_back(freq, fft_power(real, imag));
        }

        return result;
    }

    // Amplitude (frequency, amplitude) pairs
    inline std::vector<std::pair<float, float>> fft_extract_amplitude_with_freq(const float *out, const int fft_size,
                                                                                const float sample_rate,
                                                                                const float min_freq,
                                                                                const float max_freq) {
        auto powers = fft_extract_power_with_freq(out, fft_size, sample_rate, min_freq, max_freq);
        for (auto &pair: powers) {
            pair.second = std::sqrt(pair.second);
        }
        return powers;
    }

    // dB (frequency, dB) pairs
    inline std::vector<std::pair<float, float>> fft_extract_db_with_freq(const float *out, const int fft_size,
                                                                         const float sample_rate, const float min_freq,
                                                                         const float max_freq,
                                                                         const float floor = 1e-10f) {
        auto powers = fft_extract_power_with_freq(out, fft_size, sample_rate, min_freq, max_freq);
        for (auto &pair: powers) {
            pair.second = 10.0f * std::log10(std::max(pair.second, floor));
        }
        return powers;
    }

    // Power (frequency, power) pairs
    inline std::vector<std::pair<float, float>>
    fft_extract_power_with_freq_DC(const float *out, const int fft_size, const float sample_rate, const float min_freq,
                                   const float max_freq, const bool include_dc = true) {
        int min_bin = include_dc ? 0 : 1;
        const int max_bin = std::min(fft_size / 2 - 1, static_cast<int>(max_freq * fft_size / sample_rate));
        min_bin = std::max(min_bin, static_cast<int>(min_freq * fft_size / sample_rate));

        std::vector<std::pair<float, float>> result;
        result.reserve(max_bin - min_bin + 1);

        for (int k = min_bin; k <= max_bin; ++k) {
            float freq;
            float value;
            if (k == 0) {
                freq = 0.0f;
                value = out[0] * out[0]; // DC is real-only in PFFFT_REAL
            } else {
                const float real = out[2 * k];
                const float imag = out[2 * k + 1];
                freq = k * sample_rate / fft_size;
                value = fft_power(real, imag);
            }
            result.emplace_back(freq, value);
        }

        return result;
    }

    // Amplitude (frequency, amplitude) pairs
    inline std::vector<std::pair<float, float>>
    fft_extract_amplitude_with_freq_DC(const float *out, const int fft_size, const float sample_rate,
                                       const float min_freq, const float max_freq, const bool include_dc = true) {
        auto powers = fft_extract_power_with_freq_DC(out, fft_size, sample_rate, min_freq, max_freq, include_dc);
        for (auto &pair: powers) {
            pair.second = std::sqrt(pair.second);
        }
        return powers;
    }

    // dB (frequency, dB) pairs
    inline std::vector<std::pair<float, float>>
    fft_extract_db_with_freq_DC(const float *out, const int fft_size, const float sample_rate, const float min_freq,
                                const float max_freq, const bool include_dc = true, const float floor = 1e-10f) {
        auto powers = fft_extract_power_with_freq_DC(out, fft_size, sample_rate, min_freq, max_freq, include_dc);
        for (auto &pair: powers) {
            pair.second = 10.0f * std::log10(std::max(pair.second, floor));
        }
        return powers;
    }

    // Generate a Hann window (length = fft_size)
    inline std::vector<float> fft_make_hann_window(const int fft_size) {
        std::vector<float> window(fft_size);
        for (int i = 0; i < fft_size; ++i) {
            window[i] = 0.5f * (1.0f - std::cos(2.0f * M_PI * i / (fft_size - 1)));
        }
        return window;
    }

    // Apply a window to your input signal (in-place)
    inline void fft_apply_window(float *buffer, const std::vector<float> &window, const int size) {
        for (int i = 0; i < size; ++i) {
            buffer[i] *= window[i];
        }
    }

    // Generate a Hamming window
    inline std::vector<float> fft_make_hamming_window(const int fft_size) {
        std::vector<float> window(fft_size);
        for (int i = 0; i < fft_size; ++i) {
            window[i] = 0.54f - 0.46f * std::cos(2.0f * M_PI * i / (fft_size - 1));
        }
        return window;
    }
} // namespace umfeld
