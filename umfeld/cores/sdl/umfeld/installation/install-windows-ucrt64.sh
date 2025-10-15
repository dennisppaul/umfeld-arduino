#!/usr/bin/env bash

set -euo pipefail
IFS=$'\n\t'

log(){ printf "%s\n" "$*"; }

# --- Environment check --------------------------------------------------------
if [[ "${MSYSTEM:-}" != "UCRT64" ]]; then
  echo "ERROR: Run this from the MSYS2 UCRT64 shell." >&2
  echo "Open 'MSYS2 UCRT64' from the Start menu and re-run." >&2
  exit 1
fi

# Helpful: show key paths
log "--- environment"
log "MSYSTEM=${MSYSTEM}"
log "PATH=${PATH}"

# --- Full system update (often needs two passes) ------------------------------
log "--- updating MSYS2 package database and core system packages"
# First pass
pacman -Syu --noconfirm || true
# Second pass (handles updated pacman/msys2-runtime)
pacman -Syu --noconfirm || true

# Note: If the shell exits due to a runtime update, just reopen the UCRT64 shell
# and run this script again; it's safe and idempotent.

# --- Base toolchain & core deps ----------------------------------------------
log "--- installing base toolchain and core dependencies"
pacman -S --noconfirm --needed \
  ucrt64/mingw-w64-ucrt-x86_64-toolchain \
  git \
  ucrt64/mingw-w64-ucrt-x86_64-cmake \
  ucrt64/mingw-w64-ucrt-x86_64-ninja \
  ucrt64/mingw-w64-ucrt-x86_64-curl \
  ucrt64/mingw-w64-ucrt-x86_64-mesa \
  ucrt64/mingw-w64-ucrt-x86_64-pkgconf

# --- Umfeld deps --------------------------------------------------------------
log "--- installing Umfeld dependencies"
pacman -S --noconfirm --needed \
  ucrt64/mingw-w64-ucrt-x86_64-ffmpeg \
  ucrt64/mingw-w64-ucrt-x86_64-freetype \
  ucrt64/mingw-w64-ucrt-x86_64-harfbuzz \
  ucrt64/mingw-w64-ucrt-x86_64-rtmidi \
  ucrt64/mingw-w64-ucrt-x86_64-glm \
  ucrt64/mingw-w64-ucrt-x86_64-portaudio \
  ucrt64/mingw-w64-ucrt-x86_64-cairo \
  ucrt64/mingw-w64-ucrt-x86_64-pdcurses

# (removed duplicate plain `pkgconf` token and unnecessary `cairomm` unless you need C++ bindings)

# --- SDL3 (prefer official packages) ------------------------------------------
log "--- installing SDL3 packages"
pacman -S --noconfirm --needed \
  ucrt64/mingw-w64-ucrt-x86_64-sdl3 \
  ucrt64/mingw-w64-ucrt-x86_64-sdl3-image \
  ucrt64/mingw-w64-ucrt-x86_64-sdl3-ttf

# --- Make newly installed commands visible ------------------------------------
hash -r

# --- Sanity checks ------------------------------------------------------------
log "--- verifying toolchain and libraries"
command -v git   >/dev/null || { echo "ERROR: git missing"; exit 1; }
command -v cmake >/dev/null || { echo "ERROR: cmake missing"; exit 1; }
command -v ninja >/dev/null || { echo "ERROR: ninja missing"; exit 1; }
command -v pkg-config >/dev/null || { echo "ERROR: pkg-config (pkgconf) missing"; exit 1; }

# pkg-config names in MSYS2 UCRT64:
#   sdl3, SDL3_image, SDL3_ttf
pkg-config --exists sdl3 || { echo "ERROR: pkg-config 'sdl3' not found"; exit 1; }
pkg-config --exists sdl3-image || { echo "ERROR: pkg-config 'SDL3_image' not found"; exit 1; }
pkg-config --exists sdl3-ttf   || { echo "ERROR: pkg-config 'SDL3_ttf' not found"; exit 1; }

log "--- printing SDL3 cflags/libs for visibility"
pkg-config --cflags --libs sdl3 || true

log "--- Windows (MSYS2 UCRT64) environment set up for 'umfeld'."