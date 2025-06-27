# Umfeld Known Issues

- a LOT of functions + methods + strategies are not yet implemented (the goal is to implement these on demand). for a complete list of implented functions see [Processing-Function-Availability-Status](Processing-Function-Availability-Status.md)
- line drawing is very slow
- color mode is fixed to RGBA and the range is fixed `0.0 ... 1.0`
- tested on macOS(15.5) + Raspberry Pi OS + Windows (10+11) + Ubuntu (20.04+22.04+24.04). although theoretically the external libraries as well as the build system should be cross-platform ( i.e *macOS*, *Windows* and any *UNIX*-like system ) it may, however, require
  some tweaking. see [Tested-Setups](Tested-Setups.md) for some tested setups.
