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

#include <string>

namespace umfeld {
    static constexpr int   VERSION_MAJOR  = 2; // SemVer: increase when introducing breaking changes
    static constexpr int   VERSION_MINOR  = 5; // SemVer: increase when introducing new features
    static constexpr int   VERSION_PATCH  = 0; // SemVer: increase when introducing bug fixes
    static constexpr auto  VERSION_STRING = "2.5.0";

    // Helper functions for version comparison
    static constexpr int version_number() {
        return VERSION_MAJOR * 10000 + VERSION_MINOR * 100 + VERSION_PATCH;
    }

    static constexpr bool version_at_least(int major, int minor, int patch = 0) {
        return version_number() >= (major * 10000 + minor * 100 + patch);
    }
} // namespace umfeld
