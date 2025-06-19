#!/bin/bash

# fail fast if any command fails and elevate privileges
set -e
echo "--- requesting sudo access once"
sudo -v
# keep-alive: update existing `sudo` time stamp until script is done
# (this runs in background and exits when this script finishes)
while true; do sudo -n true; sleep 60; done 2>/dev/null &
KEEP_SUDO_ALIVE_PID="$!"
# trap exit to clean up background sudo keeper
trap 'kill "$KEEP_SUDO_ALIVE_PID"' EXIT

echo "--- updating apt"
sudo apt-get update -y
sudo apt-get upgrade -y

echo "--- installing dependencies (Linux)"
sudo apt-get install -y \
  git \
  clang \
  cmake \
  curl \
  libx11-dev \
  libgl1-mesa-dev \
  libglu1-mesa-dev \
  mesa-utils \
  libxext-dev \
  libxrandr-dev \
  libxcursor-dev \
  libxi-dev \
  libxinerama-dev \
  libwayland-dev \
  libxkbcommon-dev \
  wayland-protocols
echo "--- installing umfeld dependencies"
sudo apt-get install -y \
  pkg-config \
  ffmpeg \
  libavcodec-dev \
  libavformat-dev \
  libavutil-dev \
  libswscale-dev \
  libavdevice-dev \
  libharfbuzz-dev \
  libfreetype6-dev \
  librtmidi-dev \
  libglm-dev \
  portaudio19-dev \
  libcairo2-dev \
  libcurl4-openssl-dev
# sudo apt-get install libsdl3-dev # currently (2025-06-19) not available

# install SDL3 from source into system
echo "--- building SDL3 from source..."

TMP_DIR=$(mktemp -d)
cd "$TMP_DIR"

git clone --depth=1 https://github.com/libsdl-org/SDL.git
cd SDL

cmake -S . -B build \
  -DSDL_KMSDRM=ON \
  -DSDL_OPENGL=ON \
  -DSDL_OPENGLES=ON \
  -DSDL_WAYLAND=ON \
  -DSDL_X11=ON \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
sudo cmake --install build

echo "--- testing SDL3 pkg-config:"
pkg-config --libs --cflags sdl3

cd ~
rm -rf "$TMP_DIR"
echo "--- installed SDL3 successfully"

echo "--- Linux environment set up for 'umfeld'."
echo
