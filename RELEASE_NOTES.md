# RELEASE NOTES

## v2.1.0 (2025-05-24)

- *Umfeld* now runs on macOS, Linux ( including RPI ), and Windows ( still a lot of glitches )
- OpenGL ES 3.0 now works ( as a subset of OpenGL 3.3 core ) ( + on macOS ( M3 ) ANGLE (Almost Native Graphics Layer Engine) works now to develop OpenGL ES )
- KMSDRM now works on RPIs ( i.e running applications without desktop environment )
- added support for gamepad
- replaced GLEW with GLAD ( OpenGL function loading library, prerequisite for OpenGL ES 3.0 )
- added support for serial communinication with `Serial`
- updated RPI image at http://dm-hb.de/umfeld-rpi
- updated documentation

## v2.0.1 (2025-04-10)

renamed project to *Umfeld*.

## v2.0.0 (2025-04-04)

this is the first official release of the totally rewritten library now based on [SDL3](https://wiki.libsdl.org/SDL3) with different graphic and audio backends and a flexible subsystem and library structure.
