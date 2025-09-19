# Umfeld on Raspberry Pi (RPI)

*Umfeld* runs on Raspberry Pi devices with support for both *OpenGL ES 3.0* and *OpenGL 2.0* rendering backends. 

On the Raspberry Pi OS desktop, it uses X11 for window management, but it can also run directly from the command line without a graphical user interface using the *KMSDRM* system ( requires *OpenGL ES 3.0* ). This setup allows *Umfeld* to launch fullscreen graphics applications more efficiently—perfect for installations, performance setups, or headless operation over SSH.

## Quickstart

A ready-to-use Raspberry Pi OS ( and Raspberry Pi OS Lite ) (64-bit) ( *Debian Bookworm* ) image with *Umfeld* and all dependencies pre-installed is available at:

**Download:** http://dm-hb.de/umfeld-rpi or http://dm-hb.de/umfeld-rpi-lite ( no GUI )

You can install the image using tools such as the [Raspberry Pi Imager](https://www.raspberrypi.com/software/). Make sure the SD Card you are using is at least 8 GB in size.

**Default login:**

- Hostname: `umfeld.local`  
- Username: `umfeld`  
- Password: `umfeld123`

both images are based on Raspberry Pi OS (64-bit) *Debian Bookworm* ( v12, release date: 2025-05-13, Image size: 1152MB for desktop version and 423MB for lite version ). they have been tested on the **Raspberry Pi 4 Model B** and **Raspberry Pi 400**.

## Limitations and Issues

⚠️ WARNING ⚠️ there are still quite a few glitches on RPIs. for example, audio is still shaky.

RPI does not support antialiasing i.e make sure to set the value `antialiasing` to `0` in `settings()`:

```cpp
void settings(){
    antialiasing = 0;
}
```

## Preparing the Build Environment on Raspberry Pi OS

this guide helps set up a Raspberry Pi ( running *Raspberry Pi OS* ) for compiling and running SDL3-based applications with direct rendering ( KMSDRM ).

### 1. System Update and Base Development Packages

update package list and upgrade installed packages:

```sh
sudo apt-get update
sudo apt-get dist-upgrade -y
```

### 2. Dependencies

these packages are needed for media, audio, and graphics support in *Umfeld* applications:

```sh
sudo apt-get -y install \
  build-essential \
  git \
  cmake \
  curl \
  libx11-dev \
  libegl1-mesa-dev \
  libgles2-mesa-dev \
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
  libcurl4-openssl-dev \
  libncurses-dev
#sudo apt install libsdl3-dev # currently (2025-05-22) not available
```

**Note:** `gcc`, `git`, and `pkg-config` are typically pre-installed on Raspberry Pi OS with desktop, but are included above for completeness.

SDL3 is not yet available in the official Raspberry Pi OS repositories  ( e.g via `apt-get install libsdl3-dev` ). it must be built from source.

### 3. Building and Installing SDL3 from Source

these additional packages are required for building SDL3 with KMSDRM (direct rendering) support:

```sh
sudo apt-get install -y \
  libdrm-dev \
  libgbm-dev \
  libudev-dev \
  libegl-dev \
  libgl-dev
```

now build and install SDL3 from source:

```sh
sudo apt-get install -y \
  libdrm-dev \
  libgbm-dev \
  libudev-dev \
  libegl-dev \
  libgl-dev
git clone --depth=1 https://github.com/libsdl-org/SDL.git
cd SDL
USE_CONSOLE_BUILD=OFF # if running headless (no X11/Wayland) set to 'USE_CONSOLE_BUILD=ON'
cmake -S . -B build \
  -DSDL_KMSDRM=ON \
  -DSDL_OPENGL=ON \
  -DSDL_OPENGLES=ON \
  -DSDL_ALSA=ON \
  -DSDL_PULSEAUDIO=ON \
  -DSDL_PIPEWIRE=ON \
  -DSDL_JACK=ON \
  -DSDL_UNIX_CONSOLE_BUILD=$USE_CONSOLE_BUILD \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build
sudo cmake --install build --prefix /usr/local
```

note, if building for non-GUI systems ( e.g *Raspberry Pi OS Lite* ) add `-DSDL_UNIX_CONSOLE_BUILD=ON` to `cmake` command above.

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
git clone --recurse-submodules https://github.com/dennisppaul/umfeld-libraries.git # clone with submodules
```

now enter the example directory, to build and run an example e.g a basic minimal example:

```sh
cd umfeld-examples/Basics/minimal
cmake -B build
cmake --build build # `--parallel` might be too much for the RPI
./build/minimal
```

note, the first time might take a bit on a small machine. note, examples can not be run from `ssh` sessions ( without *X11 forwarding* ).

## Running Applications Outside the Windowing System

- leave the windowing system into a console (TTY) press `CTRL+ALT+F1` ( or any `+F[1–6]` )
- run application from console e.g: `./build/minimal` ( make sure the application was built with OpenGL ES 3.0 )
- to return to the desktop press `CTRL+ALT+F7`

## Running KMSDRM Applications from Remote SSH session

it is possible to run an application from a remote SSH session on a headless Raspberry PI ( i.e no windowing system like X11/Wayland ).

TL;DR log in via SSH and run `my_application`:

```sh
sudo systemd-run --tty --same-dir --uid=$USER ./my_application
```

if something goes wrong or if you prefer the verbose, *proper* way do this:

- make sure application runs *locally* in fullscreen ( with keyboard and HDMI )
    ```sh
    ./my_application
    ```
- grant SSH session user access to DRM device nodes ( `/dev/dri`, usually owned by `root:video` ) @optional
    ```sh
    sudo usermod -aG video $USER
    ```
- log out of SSH session and back in again @optional
- bind to current virtual terminal (VT) with `systemd-run`
    ```sh
    sudo systemd-run --tty --same-dir --uid=$USER ./my_application
    ```

on some setups SDL might complain about the absence of keyboard or audio driver. set these environment variables to rectify this:

```sh
export SDL_INPUT_LINUX_KEEPKBD=1
```

if audio and video drivers are not recognized you can also set these with environment variables:

```sh
export SDL_VIDEODRIVER=kmsdrm
export SDL_AUDIODRIVER=alsa   # or dummy
```

## X11 Forwarding

to run GUI applications from another machine via `ssh` the following steps must be taken:

- check that remote RPI has *X11 forwarding* enabled. in `/etc/ssh/sshd_config` confirm that `X11Forwarding yes`. if not change it an restart `ssh` with `sudo systemctl restart ssh`
- make sure local machine has an *X server* running ( e.g for macOS install XQuartz with `brew install xquartz` )
- start `ssh` session from local machine with `-Y` option e.g `ssh -Y umfeld@umfeld.local`
- run application with `DISPLAY` set to remote screen e.g `DISPLAY=:0 ./umgegbung-application`
