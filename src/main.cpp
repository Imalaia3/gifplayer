#include "gif.h"
#define SDL_MAIN_HANDLED
#include <SDL.h>

// Render methods will be implemented after main()
void renderNormally(GifFile::GifFileReader& reader, SDL_Surface* drawCanvas, GifFile::GifFrame& currentFrame);
void renderInterlaced(GifFile::GifFileReader& reader, SDL_Surface* drawCanvas, GifFile::GifFrame& currentFrame);



int main(int argc, const char *argv[]) {

    if (argc < 2) {
        printf("Syntax: reader.exe <file path> [verbose level 0-2 (1 - frames, 2 - LZW + frames)] [force interlace (i)]\n");
        return EXIT_FAILURE;
    }

    uint8_t verboseMode = 0;
    if (argc > 2) {
        if (memcmp(argv[2], "1", 1) == 0)
            verboseMode = 1;
        else if (memcmp(argv[2], "2", 1) == 0)
            verboseMode = 2;
        else if (memcmp(argv[2], "0", 1) == 0)
            verboseMode = false;
        else {
            printf("Unrecognised option '%c'\n", *argv[2]);
            return EXIT_FAILURE;
        }
    }

    uint8_t forceInterlace = false;
    if (argc > 3) {
        if (*argv[3] == 'i') {
            printf("Forcing interlace mode.\n");
            forceInterlace = true;
        } else {
            printf("Unrecognised option '%c'\n", *argv[3]);
        }
    }
    

    GifFile::GifFileReader reader(argv[1], verboseMode);
    bool retVal = reader.readFile();
    if (retVal != 0) {
        printf("Reader failed!\n");
    }

    printf("Frame Info :\n");
    for (size_t i = 0; i < reader.frames.size(); i++) {
        auto &frame = reader.frames[i];
        printf("Frame %i:\n", i);
        printf("\tTime on Screen: %i\n", frame.delayTime);
        printf("\tClear Screen: %i\n", frame.clearBuffer);
        printf("\tWidth: %i Height: %i\n", frame.width, frame.height);
        printf("\tLeft: %i Top: %i\n", frame.left, frame.top);
    }

    SDL_Init(SDL_INIT_EVERYTHING);
    SDL_Window *win = SDL_CreateWindow("GIF File Player",
                                       SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                       reader.gifHeader.scrWidth, reader.gifHeader.scrHeight,
                                       SDL_WINDOW_SHOWN);

    SDL_Surface *winSurface = SDL_GetWindowSurface(win);

    /* A surface is used here in order to speed up rendering by directly drawing onto the surface.
       Locking and Unlocking are expensive operations but still cheaper that SDL_RenderDrawPoint */
    SDL_Surface *drawCanvas = SDL_CreateRGBSurfaceWithFormat(
        0, reader.gifHeader.scrWidth, reader.gifHeader.scrHeight,
        32, SDL_PIXELFORMAT_RGB888);

    auto backgroundColor = reader.globalColorTable[reader.backgroundColorIndex];

    // NOTE: the structure of this "gameloop" has a bug. No event can be handled when an animation has started playing.
    bool running = true;
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT)
                running = false;
        }

        // Equivilant to SDL_RenderClear but for surfaces
        SDL_FillRect(drawCanvas, NULL, SDL_MapRGB(drawCanvas->format, backgroundColor.r, backgroundColor.g, backgroundColor.b));

        for (size_t i = 0; i < reader.frames.size(); i++) {
            auto &currentFrame = reader.frames[i];
            if (currentFrame.clearBuffer) {
                // must clear old loop
                if (i == 0) {
                    SDL_FillRect(drawCanvas, NULL, SDL_MapRGB(drawCanvas->format, backgroundColor.r, backgroundColor.g, backgroundColor.b));
                }

                // Small optimization to spend less time rendering, excuse the weird indentation
                else if (currentFrame.width != reader.frames[i - 1].width && currentFrame.height != reader.frames[i - 1].height &&
                         currentFrame.left != reader.frames[i - 1].left && currentFrame.top != reader.frames[i - 1].top) {
                        
                    SDL_FillRect(drawCanvas, NULL, SDL_MapRGB(drawCanvas->format, backgroundColor.r, backgroundColor.g, backgroundColor.b));
                }
            }

            uint64_t frameStart = SDL_GetTicks64();
            SDL_LockSurface(drawCanvas); // Take control of the canvas' buffer

            if (currentFrame.isInterlaced || forceInterlace)
                renderInterlaced(reader, drawCanvas, currentFrame);
            else
                renderNormally(reader, drawCanvas, currentFrame);


            SDL_UnlockSurface(drawCanvas);
            SDL_BlitSurface(drawCanvas, NULL, winSurface, NULL);
            SDL_UpdateWindowSurface(win);

            // account for frametime. SDL_Delay hangs when a negative value is given due to an underflow so a ternary expression is used.
            uint64_t frameTime = SDL_GetTicks64() - frameStart;
            SDL_Delay(currentFrame.delayTime - ((frameTime <= currentFrame.delayTime) ? frameTime : 0));
        }
    }

    SDL_DestroyWindow(win);
    SDL_Quit();
    return EXIT_SUCCESS;
}

void renderNormally(GifFile::GifFileReader& reader, SDL_Surface* drawCanvas, GifFile::GifFrame& currentFrame) {
    uint32_t *pixelBuffer = (uint32_t *)(drawCanvas->pixels);
    auto &indices = currentFrame.indices;
    for (size_t y = 0; y < currentFrame.height; y++) {
        for (size_t x = 0; x < currentFrame.width; x++) {
            if (indices[y * currentFrame.width + x] == currentFrame.transparencyIndex && currentFrame.hasTransparency)
                continue;

            auto pixel = reader.globalColorTable[indices[y * currentFrame.width + x]];
            pixelBuffer[(currentFrame.top + y) * reader.gifHeader.scrWidth + (x + currentFrame.left)] = SDL_MapRGB(
                drawCanvas->format,
                pixel.r, pixel.g, pixel.b);
        }
    }
}
void renderInterlaced(GifFile::GifFileReader& reader, SDL_Surface* drawCanvas, GifFile::GifFrame& currentFrame) {
    uint32_t *pixelBuffer = (uint32_t *)(drawCanvas->pixels);
    auto &indices = currentFrame.indices;
    uint32_t currentLine = 0;
    // Pass 0
    for (size_t y = 0; y < currentFrame.height / 8; y++) {
        for (size_t x = 0; x < currentFrame.width; x++) {
            auto pixel = reader.globalColorTable[indices[(currentLine) * currentFrame.width + x]];
            pixelBuffer[(currentFrame.top + y*8) * reader.gifHeader.scrWidth + (x + currentFrame.left)] = SDL_MapRGB(
                drawCanvas->format,
                pixel.r, pixel.g, pixel.b);
        }
        currentLine++;
    }
    currentLine+=3; // 3 is the magic number. No idea what it means, but it workded for me.

    // Pass 1
    for (size_t y = 4; y < currentFrame.height / 8; y++) {
        for (size_t x = 0; x < currentFrame.width; x++) {
            auto pixel = reader.globalColorTable[indices[(currentLine) * currentFrame.width + x]];
            pixelBuffer[(currentFrame.top + y*8) * reader.gifHeader.scrWidth + (x + currentFrame.left)] = SDL_MapRGB(
                drawCanvas->format,
                pixel.r, pixel.g, pixel.b);
        }
        currentLine++;  
    }
    currentLine+=3;
    
    // Pass 2
    for (size_t y = 2; y < currentFrame.height / 4; y++) {
        for (size_t x = 0; x < currentFrame.width; x++) {
            auto pixel = reader.globalColorTable[indices[(currentLine) * currentFrame.width + x]];
            pixelBuffer[(currentFrame.top + y*4) * reader.gifHeader.scrWidth + (x + currentFrame.left)] = SDL_MapRGB(
                drawCanvas->format,
                pixel.r, pixel.g, pixel.b);
        }
        currentLine++;
    }
    currentLine+=3;
    
    // Pass 3
    for (size_t y = 1; y < currentFrame.height / 2; y++) {
        for (size_t x = 0; x < currentFrame.width; x++) {
            auto pixel = reader.globalColorTable[indices[(currentLine) * currentFrame.width + x]];
            pixelBuffer[(currentFrame.top + y*2) * reader.gifHeader.scrWidth + (x + currentFrame.left)] = SDL_MapRGB(
                drawCanvas->format,
                pixel.r, pixel.g, pixel.b);
        }
        currentLine++;
    }
    currentLine+=3;
}