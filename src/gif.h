#pragma once

#include "lzw.h"
#include <vector>
#include <string.h>
#include <string>


namespace GifFile {
    typedef uint8_t byte;
    typedef uint16_t word;
    typedef uint32_t dword;

    // Base Gif Header
    struct GifHeader {
        byte magic[3];
        byte version[3];
        word scrWidth;
        word scrHeight;
        byte packedByte;
        byte bgColorIdx;
        byte ratio; // aspect ratio
    } __attribute__((packed));
    
    // Contents of GifHeader::packedByte
    struct GifHeaderPacked {
        dword gctEntryCount;
        dword colorRes;
        bool sortFlag;
        bool gctFlag; // Global Color Table Flag
    };


#pragma pack(push, 1) // packed + alignment to avoid any padding
    // Preceeds every image in a GIF file
    struct GifLocalImageDescriptor {
        byte id; // not used
        word left;
        word top;
        word width;
        word height;
        byte packedByte;
    };
#pragma pack(pop)

    // Contents of GifLocalImageDescriptor::packedByte
    struct GifLocalImageDescriptorPacked {
        bool lctFlag;
        bool interlaceFlag;
        bool sortFlag;
        dword lctEntrySize;
    };

#pragma pack(push, 1)
    // Specifies animation data. This struct omits the size byte and the extension identifier 
    struct GifGraphicControlExtension {
        byte packedByte;
        word delayTime; // time on screen in increments of 10 milliseconds
        byte transparentIndex; // color index that is used as a transparent color
    } __attribute__((packed));
#pragma pack(pop)

    // Contents of GifGraphicControlExtension::packedByte
    struct GifGraphicControlExtensionPacked {
        byte disposalMethod; // 1 do not clear buffer, 2 clear buffer to bg color, 3 rarely used
        bool userInputFlag; // Wait for user input. Rarely used
        bool transparencyFlag;
    };

    struct GifGctColorEntry {
        byte r, g, b;
    };

    // Container for a frame
    struct GifFrame {
        word delayTime; // time on screen in milliseconds
        word width, height;
        word left, top; // x, y position of image rectangle on image canvas
        bool clearBuffer; // should the screen buffer be cleared to its background color when drawing
        bool hasTransparency;
        word transparencyIndex;
        std::vector<uint32_t> indices; // raw decompressed GCT indices
        bool isInterlaced;
        
        std::vector<GifGctColorEntry> asPixels(GifGctColorEntry* colorTable) {
            std::vector<GifGctColorEntry> ret;
            for (auto&& index : indices) {
                ret.push_back(colorTable[index]);
            }
            return ret;
        }
    };

    class GifFileReader {
    public:
        GifFileReader(const char* fileName, uint8_t verbose = 0) : filename(fileName), m_verbose(verbose) {}
        ~GifFileReader() {
            delete[] globalColorTable;
        }
        /* 
         * reads the GIF file and stores the results in GifFileReader::frames
         * Returns: 0 on success, 1 on failure
         */
        bool readFile();

        GifHeaderPacked unpackGifHeader(GifHeader& header);
        GifLocalImageDescriptorPacked unpackGifLocalImageDescriptor(GifLocalImageDescriptor& descriptor);
        GifGraphicControlExtensionPacked unpackGifGraphicControlExtension(GifGraphicControlExtension& extension);
        



        std::string filename;
        std::vector<GifFrame> frames;
        GifHeader gifHeader;
        GifHeaderPacked gifHeaderPacked;

        GifGctColorEntry* globalColorTable;
        uint32_t backgroundColorIndex;

    private:
        bool checkHeader(GifHeader& header) {
            if (memcmp(header.magic,"GIF",3) != 0) {
                printf("File does not contain a GIF signature!\n");
                return 1;
            }
            if (memcmp(header.version,"89a",3) != 0) {
                printf("GIF File is not version 89a! Program will run, but some features may not work!\n");
                return 0;
            }
            return 0;
        }

        FILE* m_file;
        size_t m_filesize;
        uint8_t m_verbose;
    };

} // namespace GifFile