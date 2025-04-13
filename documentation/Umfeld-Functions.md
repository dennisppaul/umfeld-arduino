below is a list of functions that have been added to *Umfeld* and are not available in the original *Processing* language:

## System

- `error` :: print a system log style error message
- `warning` :: print a system log style warning message 
- `console` :: print a system log style message 
- `timestamp` :: print a timestamp in ISO8601 format ( e.g `2025-04-13 22:30:33.983` )
- `time_function_ms` :: measures the time a function requires to finish in milliseconds
- `set_frame_rate` :: set frame rate
- `get_window_title` :: get the application windows title
- `register_library` :: register a library that needs to receive system events like update, events, setup, draw etcetera
- `unregister_library` :: unregister a library

## Audio

- `audio` :: initialize a default audio device ( accessible through `a` )
- `createAudio` :: creates an audio device
- `audio_start` :: start an audio device
- `audio_stop` :: stop an audio device 
- `loadSample` :: loads a sample from a WAV or MP3 file 

## Shape

- `loadOBJ` :: load a 3D model from an OBJ file and returns it as a `Vertex` list 

## String Functions

- `begins_with` :: checks if a `string` begins with a *prefix*
- `ends_with` :: checks if a `string` begins with a *suffix*
- `to_string` :: converts a single or a comma separated list of parameters to a `string` representation
- `get_int_from_argument` :: convert an argument ( formated e.g `"value=23"` ) string into a string
- `get_string_from_argument` :: convert an argument ( formated e.g `"value=twentythree"` ) into an integer value

## Color

- `color_pack` :: packs a color defined as RGBA float values ( 0...1 ) into a single 32-but integer `uint32_t` 
- `color_unpack` :: unpacks a color defined as a single 32-but integer `uint32_t` ( e.g `0xFF0000FF` ) into RGBA float values ( 0...1 )

## Files + Directories

- `file_exists` :: checks if a file exists and is accessible
- `directory_exists` :: checks if a folder ( or directory ) exists and is accessible
- `find_file_in_paths` :: tries to find a file in a list of paths
- `find_in_environment_path` :: tries to find a file in the system path ( e.g on linux and macOS that is `$PATH` )
- `get_executable_location` :: returns the location of the applications executable
- `get_files` :: collect a list of files from a directory optinally with a certain extension