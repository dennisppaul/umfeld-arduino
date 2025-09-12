#!/usr/bin/env bash
set -euo pipefail
IFS=$'\n\t'

export DEBIAN_FRONTEND=noninteractive

log(){ printf "%s\n" "$*"; }
# retry(){ local n=0; local max=${2:-3}; until "$1"; do ((n++>=max)) && return 1; sleep 2; done; }
apt_run(){ sudo apt-get -y "$@" ; }

# --- Detect headless vs GUI ---------------------------------------------------
USE_CONSOLE_BUILD=OFF
if [[ -z "${DISPLAY:-}" && -z "${WAYLAND_DISPLAY:-}" ]]; then
  USE_CONSOLE_BUILD=ON
fi

# --- Update/upgrade with a small retry ---------------------------------------
log "--- updating apt"
# TODO retry is broken ATM
# retry "apt_run update" 3 || apt_run update
# retry "apt_run dist-upgrade" 3 || apt_run -f install
apt_run update
apt_run -f install

# --- Base toolchain & deps ----------------------------------------------------
log "--- installing base dependencies"
apt_run install --no-install-recommends \
  build-essential git cmake ninja-build curl pkg-config \
  libx11-dev libegl1-mesa-dev libgles2-mesa-dev \
  ffmpeg libavcodec-dev libavformat-dev libavutil-dev \
  libswscale-dev libavdevice-dev \
  libharfbuzz-dev libfreetype6-dev librtmidi-dev \
  libglm-dev portaudio19-dev libcairo2-dev libcurl4-openssl-dev libncurses-dev \
  libdrm-dev libgbm-dev libudev-dev libegl-dev libgl-dev

hash -r

# --- SDL3: install only if missing -------------------------------------------
if ! pkg-config --exists 'sdl3 >= 3.0.0' 2>/dev/null; then
  log "--- SDL3 not found via pkg-config, building from source"

  SDL_TAG="${SDL_TAG:-main}"   # allow override: SDL_TAG=v3.2.0
  TMP_DIR=$(mktemp -d)
  trap 'rm -rf "$TMP_DIR"' EXIT

  git clone --depth 1 --branch "$SDL_TAG" https://github.com/libsdl-org/SDL.git "$TMP_DIR/SDL"
  cmake -S "$TMP_DIR/SDL" -B "$TMP_DIR/build" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DSDL_KMSDRM=ON \
    -DSDL_OPENGL=ON \
    -DSDL_OPENGLES=ON \
    -DSDL_ALSA=ON \
    -DSDL_PULSEAUDIO=ON \
    -DSDL_PIPEWIRE=ON \
    -DSDL_JACK=ON \
    -DSDL_UNIX_CONSOLE_BUILD="${USE_CONSOLE_BUILD}"

  JOBS=$(nproc 2>/dev/null || echo 1)
  if grep -qi raspberry /proc/device-tree/model 2>/dev/null; then
    # conservative: max 2 jobs on Raspberry Pi
    [[ $JOBS -gt 2 ]] && JOBS=2
  fi
  cmake --build "$TMP_DIR/build" --parallel "$JOBS"

  sudo cmake --install "$TMP_DIR/build" --prefix /usr/local

  hash -r

  log "--- testing SDL3 pkg-config:"
  if ! pkg-config --libs --cflags sdl3; then
    echo "ERROR: SDL3 pkg-config not found after install" >&2
    exit 1
  fi
else
  log "--- SDL3 already available via pkg-config"
fi

log "--- Raspberry Pi environment ready for 'umfeld'"