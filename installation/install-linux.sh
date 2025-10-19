#!/usr/bin/env bash

set -euo pipefail
IFS=$'\n\t'

log(){ printf "%s\n" "$*"; }

# --- Ensure we're on an apt-based distro -------------------------------------
if ! command -v apt-get >/dev/null 2>&1; then
  echo "ERROR: This installer targets Debian/Ubuntu (apt). For Arch use the Arch script, for RPi use the Pi script." >&2
  exit 1
fi

# --- Sudo warmup + keepalive (auto-cleans on exit) ---------------------------
log "--- requesting sudo access once"
sudo -v
KEEP_SUDO_ALIVE_PID=
{
  while true; do
    sudo -n true 2>/dev/null || exit
    sleep 60
  done
} &
KEEP_SUDO_ALIVE_PID=$!
cleanup_keepalive(){ [[ -n "${KEEP_SUDO_ALIVE_PID:-}" ]] && kill "$KEEP_SUDO_ALIVE_PID" 2>/dev/null || true; }
trap cleanup_keepalive EXIT

export DEBIAN_FRONTEND=noninteractive

# --- Update/upgrade with resilience ------------------------------------------
log "--- updating apt"
sudo apt-get update -y || sudo apt-get update -y

log "--- upgrading system (dist-upgrade)"
sudo apt-get dist-upgrade -y || sudo apt-get -f install -y

# --- Base toolchain & deps ----------------------------------------------------
log "--- installing dependencies (Debian/Ubuntu)"
# TODO consider dropping clang package to increase install speed?
# TODO removed 'clang' temporarily
sudo apt-get install -y --no-install-recommends \
  build-essential git cmake ninja-build curl pkg-config \
  libx11-dev libxext-dev libxrandr-dev libxcursor-dev libxi-dev libxinerama-dev \
  libgl1-mesa-dev libglu1-mesa-dev \
  libwayland-dev libxkbcommon-dev wayland-protocols \
  mesa-utils

log "--- installing 'umfeld' dependencies"
sudo apt-get install -y --no-install-recommends \
  ffmpeg libavcodec-dev libavformat-dev libavutil-dev libswscale-dev libavdevice-dev \
  libharfbuzz-dev libfreetype6-dev librtmidi-dev libglm-dev portaudio19-dev \
  libcairo2-dev libcurl4-openssl-dev libncurses-dev

hash -r

# --- SDL3: prefer package if available; otherwise build ----------------------
if apt-cache policy libsdl3-dev 2>/dev/null | grep -q 'Candidate:' && \
   ! apt-cache policy libsdl3-dev | grep -q '(none)'; then
  log "--- installing SDL3 from apt (libsdl3-dev)"
  sudo apt-get install -y --no-install-recommends libsdl3-dev
else
  log "--- SDL3 not available as package; building from source"
  SDL_TAG="${SDL_TAG:-main}"   # Set SDL_TAG=v3.2.0 to pin

  TMP_DIR="$(mktemp -d)"
  cleanup_tmp(){ rm -rf "$TMP_DIR"; }
  trap 'cleanup_keepalive; cleanup_tmp' EXIT

  git clone --depth=1 --branch "$SDL_TAG" https://github.com/libsdl-org/SDL.git "$TMP_DIR/SDL"

  # Conservatively cap parallel jobs to avoid lockups on smaller machines
  JOBS="$(nproc 2>/dev/null || echo 1)"
  if [[ "$JOBS" -gt 4 ]]; then JOBS=4; fi

  cmake -S "$TMP_DIR/SDL" -B "$TMP_DIR/build" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DSDL_KMSDRM=ON \
    -DSDL_OPENGL=ON \
    -DSDL_OPENGLES=ON \
    -DSDL_WAYLAND=ON \
    -DSDL_X11=ON

  cmake --build "$TMP_DIR/build" --parallel "$JOBS"
  sudo cmake --install "$TMP_DIR/build" --prefix /usr/local
fi

hash -r

# --- Verify SDL3 via pkg-config ----------------------------------------------
log "--- testing SDL3 pkg-config:"
if ! pkg-config --exists sdl3; then
  echo "ERROR: SDL3 pkg-config entry 'sdl3' not found after install." >&2
  exit 1
fi
pkg-config --cflags --libs sdl3 || true

log "--- Linux (Debian/Ubuntu) environment set up for 'umfeld'."
