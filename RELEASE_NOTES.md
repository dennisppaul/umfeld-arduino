# RELEASE NOTES

## v2.5.0 (2025-10-13)

- added per sample `audioEvent` ( enable with `enable_audio_per_sample_processing = true;` )
- window resize now works ( including pixel density changes )
- fixed reverb
- sampler can now resample data to other sample rates
- color functions now exist in float ( e.g `stroke(0.5)` ) and 8-bit versions ( e.g `stroke_8(127)` ). the 8-bit versions are suffixed with a `_8`.
- fixed library registration

## v2.4.2 (2025-09-22)

- `update` can run in own thread now with `run_update_in_thread = true`
- fixed *Windows UCRT64* installer

## v2.4.0 (2025-09-10)

- translated examples from Processing ( thank you [haram choi @oudeis01](https://github.com/oudeis01) )
- major ( under the hood ) rework of the OpenGL renderer. a few core points are:
    - renderer is now MUCH faster than before ( faster than Processing in some cases )
    - has two different strategies: render shapes by *order of submission* and by *depth-order*
    - can handle transparency properly ( depth sorting )
    - optimized rendering for shapes without or with similar textures 
- `size()` can now handle `P2D` and `P3D` options
- added optional profiler ( see [tracy](https://github.com/wolfpld/tracy) )
- `umfeld-arduino` has matured a LOT. it works now with `arduino-cli` and *Arduino IDE* and surprisingly, it even works on Windows (UCRT64)
- callback functions ( e.g `settings()`, `setup()`, `draw()`, … ) can now be configured with `umfeld_set_callbacks()` and `#define UMFELD_SET_DEFAULT_CALLBACK FALSE`

## v2.3.0 (2025-07-24)

- textures now have filtering and wrapping options with `texture_filter` and `texture_wrap`
- application now set automatically window title to project title
- added option `flip_y_texcoords` to `PImage` to flip y-axis when drawn ( addressing the bottom-left vs bottom-right dilema )
- terminal renderer has been improved, can draw images now, handles colors, and has been added officially
- pixel operations `loadPixels()`, `pixels[]`, and `updatePixel()` now work
- improved install scripts
- Arch Linux now works again and is detected by installation script
- improved random functions; mode can be set with `set_random_mode()`
- added additional console and warning function that only emit a message once: `console_once()` and `warning_in_function_once()`
- default window name is now deduced from CMake script
- made package and library finding in CMake script more robust 
- improved terminal ( CLI ) renderer
- fixed threaded audio

## v2.2.0 (2025-06-27)

- `size()` can now interpret `DISPLAY_WIDTH, DISPLAY_HEIGHT` to set window to full display width and height
- audio devices can now run in their own thread ( e.g by setting `audio_threaded = true;` in `settings()` )
- shapes can now be exported as PDF or OBJ
- lights are now working
- window location and window title can now be set by application
- `loadBytes()`, `loadStrings()`, and `loadImage()` now support loading from local files as well as URLs and URIs (with base64 + percent + html encoding)
- added `data` folder concept. folder will be automatically copied upon building
- `load...` functions now look for files in `data` folder #breakingchange
- fixed vertex buffer on Windows and Linux
- added dedicated point and line shaders
- added a CLI-based renderer ( WIP )
- added `noLoop()` and `redraw()`
- updated documentation
- improved build scripts. *Umfeld* can now be installed via scripts from repository ( see [Quickstart](README.md#Quickstart) ) .

... and many more bug fixes and minor improvements.

## v2.1.0 (2025-05-24)

- *Umfeld* now runs on macOS, Linux ( including RPI ), and Windows ( still a lot of glitches )
- OpenGL ES 3.0 now works ( as a subset of OpenGL 3.3 core ) ( + on macOS ( M3 ) ANGLE (Almost Native Graphics Layer Engine) works now to develop OpenGL ES )
- KMSDRM now works on RPIs ( i.e running applications without desktop environment )
- added support for gamepad
- replaced GLEW with GLAD ( OpenGL function loading library, prerequisite for OpenGL ES 3.0 )
- added support for serial communinication with `Serial`
- updated RPI image at http://dm-hb.de/umfeld-rpi
- added a *lighter* RPI image without GUI ( i.e X11 or wayland ) at http://dm-hb.de/umfeld-rpi-lite
- updated documentation

## v2.0.1 (2025-04-10)

renamed project to *Umfeld*.

## v2.0.0 (2025-04-04)

this is the first official release of the totally rewritten library now based on [SDL3](https://wiki.libsdl.org/SDL3) with different graphic and audio backends and a flexible subsystem and library structure.
