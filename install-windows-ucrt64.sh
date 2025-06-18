#!/usr/bin/env bash

# Ensure the script is being run from MSYS2 UCRT64
if [[ "$MSYSTEM" != "UCRT64" ]]; then
  echo "Error: This script must be run from the MSYS2 UCRT64 environment."
  echo "Start MSYS2 using the 'MSYS2 UCRT64' shortcut and run this script again."
  exit 1
fi

echo "Updating MSYS2 package database and core system packages..."
pacman -Syu --noconfirm
# Repeat after first upgrade to ensure core tools are also updated
pacman -Syu --noconfirm

echo "Installing build dependencies and required libraries..."
pacman -S --noconfirm \
  git \
  curl \
  mingw-w64-ucrt-x86_64-toolchain \
  mingw-w64-ucrt-x86_64-cmake \
  mingw-w64-ucrt-x86_64-pkg-config \
  mingw-w64-ucrt-x86_64-ninja \
  mingw-w64-ucrt-x86_64-mesa \
  mingw-w64-ucrt-x86_64-ftgl \
  mingw-w64-ucrt-x86_64-sdl3 \
  mingw-w64-ucrt-x86_64-sdl3-image \
  mingw-w64-ucrt-x86_64-sdl3-ttf \
  mingw-w64-ucrt-x86_64-ffmpeg \
  mingw-w64-ucrt-x86_64-rtmidi \
  mingw-w64-ucrt-x86_64-glm \
  mingw-w64-ucrt-x86_64-portaudio \
  mingw-w64-ucrt-x86_64-curl \
  mingw-w64-ucrt-x86_64-cairo \
  mingw-w64-ucrt-x86_64-cairomm

echo "MSYS2 UCRT64 environment set up for 'umfeld'."
