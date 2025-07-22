# Known Differences Between Processing and Umfeld

Java and C++ are similar in some aspects, but are very different in many others. a lot of the syntactic differences between *Processing* and *Umfeld* stem from these differences, while a few others are more or less developer decisions.

- elements in `println()` must be concatenated with `,` rather than `+` e.g `println("hello ", 23, " world");` @reason(raw strings cannot be concatenated with a `+` in C++ as they can be in Java)
- the properties `mousePressed` + `keyPressed` are renamed to `isMousePressed` and `isKeyPressed` @reason(in *Processing* the property `mousePressed` and the callback method `mousePressed` carry the same name. in C++ it is not allowed to have a field or property and a method with the same name)
- `fill()` and `stroke()` only accept `float` types in `Umfeld` ( e.g `stroke(float, float, float)` ). if you want to use a compound color ( i.e `color` in `Processing` ) you must call `fill_color(uint32_t)` or `stroke(uint32_t)` ( where `uint32_t` is an unsigned, 32bit integer ).
- `fill(float)` must be explicitly differentiated from `fill(uint32_t)` e.g `fill(1.0f)` or `fill(0xFF44FF88)` ( the same is true for `stroke(...)` ) @reason(in a different way from Java, C++ finds two methods that only differ in their parameter types i.e `float` and `uint32_t` ambiguous)
- Color mode is fixed to RGBA and the range is fixed to `0.0 ... 1.0`
- Subsystems ( audio + graphics )
- Libraries are handle differently in Umfeld
- CMake as the Build System