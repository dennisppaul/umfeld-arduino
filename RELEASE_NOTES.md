# RELEASE NOTES

## v2.2.0 (2025-06-27)

- `size()` can now interpret `DISPLAY_WIDTH, DISPLAY_HEIGHT` to set window to full display width and height
- audio devices can now run in their own thread ( e.g by setting `audio_threaded = true;` in `settings()` )
- shapes can now be exported as PDF or OBJ
- lights are now working
- window location and window title can now be set by application
- `loadBytes()`, `loadStrings()`, and `loadImage()` now support loading from local files as well as URLs and URIs (with base64 + percent + html encoding)
- added `data` folder concept. folder will be automatically copied upon building
- `load...` functions now look for files in `data` folder @breakingchange
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
