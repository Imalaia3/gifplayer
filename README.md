# GIF Player
**A simple and easy to understand GIF image and video player in C++.**
GIF Player is a relatively bare-bones [GIF](https://en.wikipedia.org/wiki/GIF) reader.  Its main purpose is to educate and to be used as a framework for similar projects. GIF decoder references are relatively scarce, so I have tried to comment the most important parts of the code. Moreover ,at the end of this README, I have put some useful documents and articles that aided in the development of this program.

## Compilation & Execution
To **compile** GIF player the supplied [pybuild](https://github.com/Imalaia3/pybuild) script can be used. To run this script a Python 3.5+ interpreter is required. The **[SDL2](https://www.libsdl.org/)** library must also be installed if you want to compile the example program, but the main GIF Player does not rely on any libraries.

### Compilation for Windows
If you wish to compile on Windows you must first install SDL2  from their [GitHub repository](https://github.com/libsdl-org/SDL/releases/).
Moreover you should also have g++ installed via [MinGW](https://sourceforge.net/projects/mingw/) or any other similar method.
Now run `python build.py`.  The script should ask you for the SDL2 include path and the lib path.
```
SDL2 Include Path> C:\your-sdl-path-here\include
SDL2 Library Path> C:\your-sdl-path-here\lib\xYourArchitecture
```
The project should then build the example program and output `player.exe`. If it doesn't compile try changing the library architecture from **x64** to **x86**. 

### Compilation for Linux
Compilation on Linux is significantly easier:
Install the SDL2 Development Packages using your favorite package manager (apt example `sudo apt-get install libsdl2-2.0-0`. For more info visit [this page on the SDL Wiki](https://wiki.libsdl.org/SDL2/Installation).
Then, just run `python3 build.py` and the program should output a `player` executable.

### Execution
The command syntax is as follows `player.exe <gif file path> [debug level (0-2)]`
`<gif file path>` is required, however the `debug level` can be omitted and will default  to 0.

**Debug Levels Explained**
*0 - No Debug info
1 - GIF Header / Frame info
2 - Level 1 & LZW Decode info (a lot of messages!)*

## Capabilities
GIF Player currently **supports**:
 - GIF Image Viewing
 - GIF Video Playback
 - Global Color Table support
 - GIF89a and partial GIF87a support
 
 GIF Player **does not yet support**:
 
 - Local Color Tables
 - Interlaced Video

## References & Acknowledgments

 - pygif - https://github.com/qalle2/pygif/tree/main (Used for testing)
- What's In A GIF - https://giflib.sourceforge.net/whatsinagif/lzw_image_data.html (LZW algorithm)
- GIF89a Specification - https://www.w3.org/Graphics/GIF/spec-gif89a.txt (General Reference)
- GIF File Format Summary - https://www.fileformat.info/format/gif/egff.htm (Headers + other Data Types)
