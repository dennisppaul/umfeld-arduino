#!/usr/bin/env bash

set -euo pipefail
IFS=$'\n\t'

log(){ printf "%s\n" "$*"; }

# --- Sudo warmup + keepalive (auto-cleans on exit)
log "--- requesting sudo access once"
sudo -v
# refresh sudo timestamp every 60s in background
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

# --- System update
log "--- updating pacman"
sudo pacman -Syu --noconfirm

# --- Base toolchain & deps (idempotent)
log "--- installing dependencies (Arch Linux)"
sudo pacman -S --noconfirm --needed \
  base-devel git clang cmake ninja pkgconf curl \
  mesa libxext libxrandr libxcursor libxi libxinerama \
  wayland libxkbcommon wayland-protocols

log "--- installing umfeld dependencies"
sudo pacman -S --noconfirm --needed \
  ffmpeg harfbuzz freetype2 rtmidi glm portaudio cairo \
  libcurl-gnutls ncurses

hash -r

# --- SDL3: prefer repo, else build from source
if pacman -Si sdl3 >/dev/null 2>&1; then
  log "--- installing SDL3 from Arch repo"
  sudo pacman -S --noconfirm --needed sdl3
else
  log "--- SDL3 not in repo; building from source"
  SDL_TAG="${SDL_TAG:-main}"   # e.g. SDL_TAG=v3.2.0 to pin

  TMP_DIR="$(mktemp -d)"
  cleanup_tmp(){ rm -rf "$TMP_DIR"; }
  trap 'cleanup_keepalive; cleanup_tmp' EXIT

  git clone --depth=1 --branch "$SDL_TAG" https://github.com/libsdl-org/SDL.git "$TMP_DIR/SDL"
  # Build with Ninja (faster) and modest parallelism
  JOBS="$(nproc 2>/dev/null || echo 1)"
  if [ "$JOBS" -gt 4 ]; then JOBS=4; fi # cap default to 4; adjust if you like

  cmake -S "$TMP_DIR/SDL" -B "$TMP_DIR/build" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DSDL_KMSDRM=ON \
    -DSDL_OPENGL=ON \
    -DSDL_OPENGLES=ON \
    -DSDL_WAYLAND=ON \
    -DSDL_X11=ON

  cmake --build "$TMP_DIR/build" --parallel "$JOBS"
  sudo cmake --install "$TMP_DIR/build"
fi

# --- Verify SDL3 visibility via pkg-config
if ! pkg-config --exists sdl3; then
  echo "ERROR: SDL3 pkg-config not found after install" >&2
  exit 1
fi

log "--- testing SDL3 pkg-config:"
pkg-config --libs --cflags sdl3 || true

log "--- Arch Linux environment set up for 'umfeld'."