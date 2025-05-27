# Umfeld on Raspberry Pi (RPI)

*Umfeld* runs on Raspberry Pi devices with support for both *OpenGL ES 3.0* and *OpenGL 2.0* rendering backends. 

On the Raspberry Pi OS desktop, it uses X11 for window management, but it can also run directly from the command line without a graphical user interface using the *KMSDRM* system ( requires *OpenGL ES 3.0* ). This setup allows *Umfeld* to launch fullscreen graphics applications more efficiently—perfect for installations, performance setups, or headless operation over SSH.

## Quickstart

A ready-to-use Raspberry Pi OS (64-bit) ( *Debian Bookworm* ) image with *Umfeld* and all dependencies pre-installed is available at:

**Download:** http://dm-hb.de/umfeld-rpi or http://dm-hb.de/umfeld-rpi-lite ( no GUI )

You can install the image using tools such as the [Raspberry Pi Imager](https://www.raspberrypi.com/software/).

**Default login:**

- Hostname: `umfeld.local`  
- Username: `umfeld`  
- Password: `umfeld123`

this image is based on Raspberry Pi OS (64-bit) based on *Debian Bookworm* (Release date: 2025-05-13, Image size: 1152 MB) and has been tested on the **Raspberry Pi 4 Model B** and **Raspberry Pi 400**.

## Limitations and Issues

⚠️ WARNING ⚠️ there are still quite a few glitches on RPIs. for example, audio is still shaky.

RPI does not support antialiasing i.e make sure to set the value `antialiasing` to `0` in `settings()`:

```cpp
void settings(){
    antialiasing = 0;
}
```

this step by step guide has been tested on a *Raspberry Pi 4 Model B* and *Raspberry Pi 400* with Raspberry Pi OS (64-bit), *Debian Bookworm* ( Released: 2025-05-13, Size: 1152MB ) installed.

## Preparing the Build Environment on Raspberry Pi OS (with desktop)

This guide helps set up a Raspberry Pi (running *Raspberry Pi OS with desktop*) for compiling and running SDL3-based applications with direct rendering (KMSDRM).

### 1. System Update and Base Development Packages

update package list and upgrade installed packages:

```sh
sudo apt update -y
sudo apt upgrade -y
```

### 2. Dependencies

these packages are needed for media, audio, and graphics support in *Umfeld* applications:

```sh
sudo apt install -y \
  build-essential \
  cmake \
  git \
  pkg-config \
  ffmpeg \
  libharfbuzz-dev \
  libfreetype6-dev \
  libavcodec-dev \
  libavformat-dev \
  libavutil-dev \
  libswscale-dev \
  libavdevice-dev \
  librtmidi-dev \
  libglm-dev \
  portaudio19-dev \
  libegl1-mesa-dev \
  libgles2-mesa-dev
#sudo apt install libsdl3-dev # currently (2025-05-22) not available
```

**Note:** `gcc`, `git`, and `pkg-config` are typically pre-installed on Raspberry Pi OS with desktop, but are included above for completeness.

SDL3 is not yet available in the official Raspberry Pi OS repositories  ( e.g via `apt install libsdl3-dev` ). It must be built from source.

### 3. Building and Installing SDL3 from Source

these additional packages are required for building SDL3 with KMSDRM (direct rendering) support:

```sh
sudo apt install -y \
  libdrm-dev \
  libgbm-dev \
  libudev-dev \
  libegl-dev \
  libgl-dev
```

now build and install SDL3 from source:

```sh
git clone https://github.com/libsdl-org/SDL.git
cd SDL
cmake -S . -B build \
  -DSDL_KMSDRM=ON \
  -DSDL_OPENGL=ON \
  -DSDL_OPENGLES=ON \
  -DSDL_ALSA=ON \
  -DSDL_PULSEAUDIO=ON \
  -DSDL_PIPEWIRE=ON \
  -DSDL_JACK=ON \
  -DCMAKE_BUILD_TYPE=Release 
cmake --build build # Avoid using -j$(nproc) or `--parallel` for now, it may overwhelm the RPi
sudo cmake --install build --prefix /usr/local
```

note, if building for non-GUI systems ( e.g *Raspberry Pi OS Lite* ) add `-DSDL_UNIX_CONSOLE_BUILD=ON` to `cmake -S . -B…` command above.

once installed, confirm SDL3 is available to projects:

```sh
pkg-config --libs --cflags sdl3
```

this should return the necessary compiler and linker flags for SDL3.

now the system is fully set up to build SDL3-based applications with KMSDRM support.

## Setting up Umfeld

first clone [umfeld](https://github.com/dennisppaul/umfeld) and the [umfeld-examples](https://github.com/dennisppaul/umfeld-examples) repositories with submodules from GitHub:

```sh
git clone https://github.com/dennisppaul/umfeld
git clone https://github.com/dennisppaul/umfeld-examples
```

now enter the example directory, to build and run an example e.g a basic minimal example:

```sh
cd umfeld-examples/Basics/minimal
cmake -B build
cmake --build build # `--parallel` might be too much for the RPI
./build/minimal
```

note, the first time might take a bit on a small machine. note, examples can not be run from `ssh` sessions ( without *X11 forwarding* ).

## Running Applications Outside Windowing System

- to switch to a console (TTY) press `CTRL+ALT+F3` ( or any `+F[3–6]` )
- run application ( e.g `./build/minimal` )
- to return to the desktop press `CTRL+ALT+F1` or `CTRL+ALT+F2`.

## X11 Forwarding

to run GUI applications from another machine via `ssh` the following steps must be taken:

- check that remote RPI has *X11 forwarding* enabled. in `/etc/ssh/sshd_config` confirm that `X11Forwarding yes`. if not change it an restart `ssh` with `sudo systemctl restart ssh`
- make sure local machine has an *X server* running ( e.g for macOS install XQuartz with `brew install xquartz` )
- start `ssh` session from local machine with `-Y` option e.g `ssh -Y umfeld@umfeld.local`
- run application with `DISPLAY` set to remote screen e.g `DISPLAY=:0 ./umgegbung-application`
