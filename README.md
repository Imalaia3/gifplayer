# GIF Player
A simple, straight forward, GIF video and image viewer based on SDL2 without any extra libraries required. It doesn't focus on performance but on code readability.

## Prerequisites

 - **Python 3.x** Interpreter for the [pybuild](https://github.com/Imalaia3/pybuild) build script.
 - **SDL2** for video playback / image rendering.
 - **g++** for compilation.
 
 ## Building & Execution
 To **build** the project, run the following command at the root directory and follow the instructions that appear on the terminal.  `python build.py all` (**Note**: On Windows systems the SDL2 Library/Include paths must be specified when building.)

To run the project, navigate to the `bin/` directory and execute `reader.exe`. The command line syntax is: 
` reader.exe <file path> [verbose level 0-2 (1-frames, 2-LZW+frames)] [force interlace(i)]`  

 - To display any common GIF file, run `reader.exe <file path>`.
 - To debug the GIF file or the player, a debug log level can be specified: `reader.exe <file path> [verbose level]`. Level 0 = No Debug Messages, 1 = Frame Header Info, 2 = LZW decompression logs + Level 1 messages.
 - Sometimes, a video or image may be interlaced. This may not get detected so the interlace mode should be enabled from the command line by appending an `i` argument after the debug level. **A debug level must be specified when using interlace mode.**


## Acknowledgments

 - https://giflib.sourceforge.net/whatsinagif/index.html
 - https://www.w3.org/Graphics/GIF/spec-gif89a.txt
 - https://www.fileformat.info/format/gif/egff.htm
 - https://github.com/qalle2/pygif (testing against a working LZW decoder)
 
