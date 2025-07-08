# Additional Umfeld Functions

below is a list of functions that have been added to *Umfeld* and are not available in the original *Processing* language:

## System

- `arguments` :: callback containing command-line arguments, called right before `settings()`
- `error` :: print a system log style error message @note(can be deactivated with compile defintion `UMFELD_PRINT_ERRORS` set to `0`)
- `warning` :: print a system log style warning message @note(can be deactivated with compile defintion `UMFELD_PRINT_WARNINGS` set to `0`)
- `console` :: print a system log style message @note(can be deactivated with compile defintion `UMFELD_PRINT_CONSOLE` set to `0`)
- `console_n` :: print a system log style message without newline
- `console_c` :: print a system log style message without timestamp and timestamp
- `timestamp` :: print a timestamp in ISO8601 format ( e.g `2025-04-13 22:30:33.983` )
- `time_function_ms` :: measures the time a function requires to finish in milliseconds
- `set_frame_rate` :: set frame rate
- `get_window_title` :: get the application window's title
- `register_library` :: register a library that needs to receive system events like update, events, setup, draw etcetera
- `unregister_library` :: unregister a library
- `format_label` :: formats a label with a fixed width ( useful for console output alignment )
- `separator` :: returns a separator line ( optional equal sign and width can be configured )

## Audio

- `audio` :: initialize a default or custom audio device ( accessible through `a` ), supports various overloads:
    - by channel count, sample rate, buffer size
    - by device name ( input/output )
    - by `AudioUnitInfo` struct
- `createAudio` :: creates and returns a new audio device with the given configuration
- `audio_start` :: start an audio device ( or the default one if none is specified )
- `audio_stop` :: stop an audio device ( or the default one if none is specified )
- `is_initialized` :: checks if the audio system is initialized
- `loadSample` :: loads a sample from a WAV or MP3 file

## Image

### Textures

- `texture_filter` :: sets the texture filter mode for the current texture ( `NEAREST`, `LINEAR` or `MIPMAP` )

## Shape

- `loadOBJ` :: load a 3D model from an OBJ file and return it as a `Vertex` list ( optional material loading )

## String Functions

- `begins_with` :: checks if a `string` begins with a *prefix*
- `ends_with` :: checks if a `string` ends with a *suffix*
- `to_string` :: converts a single value or a comma-separated list of parameters into a `string` representation
- `get_int_from_argument` :: extracts an integer value from an argument string ( formatted e.g `"value=23"` )
- `get_string_from_argument` :: extracts a string value from an argument string ( formatted e.g `"value=text"` )

## Color

- `color_pack` :: packs RGBA float values ( 0...1 ) into a single 32-bit integer `uint32_t`
- `color_unpack` :: unpacks a 32-bit integer `uint32_t` ( e.g `0xFF0000FF` ) into RGBA float values ( 0...1 )

## Files + Directories

- `file_exists` :: checks if a file exists and is accessible
- `directory_exists` :: checks if a folder ( or directory ) exists and is accessible
- `find_file_in_paths` :: tries to find a file in a given list of paths
- `find_in_environment_path` :: tries to find a file in the system's environment path ( e.g `$PATH` on Unix )
- `get_executable_location` :: returns the file path of the current application's executable
- `get_files` :: collects a list of files from a directory, optionally filtered by file extension