#include "gif.h"
#include <assert.h>

namespace GifFile {

    GifHeaderPacked GifFileReader::unpackGifHeader(GifHeader& header) {
        byte packed = header.packedByte;
        uint32_t gctLowSize = (packed & 0b00000111);
        return GifHeaderPacked {
            ((dword)1 << (gctLowSize+1)       ), // 2^(gctLowSize+1) for actual GCT entry count
            (dword)((packed & 0b01110000) >> 4),
            (bool)((packed & 0b00001000)  >> 3),
            (bool)((packed & 0b10000000)  >> 7)
        };
    }

    GifLocalImageDescriptorPacked GifFileReader::unpackGifLocalImageDescriptor(GifLocalImageDescriptor& descriptor) {
        byte packed = descriptor.packedByte;
        return GifLocalImageDescriptorPacked {
            (bool)((packed & 0b00000001)     ),
            (bool)((packed & 0b00000010)  >> 1),
            (bool)((packed & 0b00000100)  >> 2),
            (dword)((packed & 0b11100000) >> 5)
        };
    }

    GifGraphicControlExtensionPacked GifFileReader::unpackGifGraphicControlExtension(GifGraphicControlExtension& extension) {
        byte packed = extension.packedByte;
        return GifGraphicControlExtensionPacked {
            (byte)((packed & 0b00011100) >> 2),
            (bool)((packed & 0b00000010) >> 1),
            (bool)((packed & 0b00000001) >> 0),
        };
    }

    bool GifFileReader::readFile() {
        m_file = fopen(filename.c_str(), "rb");
        fseek(m_file, 0, SEEK_END);
        m_filesize = ftell(m_file);
        fseek(m_file, 0, SEEK_SET);

        fread(&gifHeader, sizeof(GifHeader), 1, m_file);
        if (checkHeader(gifHeader)) // Magic or Version are invalid
            return 1;
        gifHeaderPacked = unpackGifHeader(gifHeader);
        
        if(m_verbose) {
            printf("%s: %ix%i px, %i bpp, %i GCT Entries, Sorted: %i, GCT Flag: %i\n",filename.c_str(), gifHeader.scrWidth,
                gifHeader.scrHeight,
                (gifHeaderPacked.colorRes+1)*3,
                gifHeaderPacked.gctEntryCount,
                gifHeaderPacked.sortFlag,
                gifHeaderPacked.gctFlag);
        }

        // Set The index of the color to be used when clearing the screen
        backgroundColorIndex = gifHeader.bgColorIdx;

        // Populate the Global Color Table
        globalColorTable = new GifGctColorEntry[gifHeaderPacked.gctEntryCount];
        fread(globalColorTable, 3*gifHeaderPacked.gctEntryCount, 1, m_file);



        // loop until all frames have been read
        while (true) {
            GifFrame thisFrame{};

            byte identifier = 0x00;
            while (true) {
                fread(&identifier, 1, 1, m_file);
                // found an image, 0x2C = LocalImageDescriptor
                if (identifier == 0x2C) {
                    if (m_verbose) { printf("Local Image Descriptor (offset %li)\n", ftell(m_file)); }
                    fseek(m_file, -1, SEEK_CUR); // Seek back so Local Image Descriptor has the ID
                    break; // jump straight to reading the LocalImageDescriptor and the compressed data
                
                // 0x21 = Extension
                } else if (identifier == 0x21) {
                    byte extensionType;
                    fread(&extensionType, 1, 1, m_file);

                    if (extensionType == 0xF9) { // 0xF9 = Graphic Control Extension
                        if(m_verbose) { printf("Graphic Control Extension (offset %li):\n", ftell(m_file)); }

                        byte extensionSize;
                        fread(&extensionSize, 1, 1, m_file);
                        
                        // confirm program is reading the correct thing
                        assert(extensionSize == sizeof(GifGraphicControlExtension));

                        GifGraphicControlExtension extension;
                        fread(&extension, sizeof(GifGraphicControlExtension), 1, m_file);
                        auto extPacked = unpackGifGraphicControlExtension(extension);
                    
                        if(m_verbose) {
                            printf("\tTime on screen: %i*10 ms\n\tDisposal Method: %i\n\tTransparency Flag: %i\n",
                                extension.delayTime,
                                extPacked.disposalMethod,
                                extPacked.transparencyFlag
                            );
                        }

                        // Set the current frame metadata
                        thisFrame.clearBuffer = (extPacked.disposalMethod == 2);
                        thisFrame.delayTime = extension.delayTime * 10; // GifGraphicControlExtension::delayTime hundreths of a second not milliseconds 
                        thisFrame.hasTransparency = extPacked.transparencyFlag;
                        thisFrame.transparencyIndex = extension.transparentIndex;
                    }
                } 
            } // end of extension and descriptor search

            // Store Local Image Descriptor Data
            GifLocalImageDescriptor localImageDescriptor;
            fread(&localImageDescriptor, sizeof(GifLocalImageDescriptor), 1, m_file);
            GifLocalImageDescriptorPacked localImageDescriptorPacked = unpackGifLocalImageDescriptor(localImageDescriptor);
            if (m_verbose && localImageDescriptorPacked.lctFlag) {
                printf("Local Image Descriptor has LCT flag set.\n");
            }
            
            
            // Set frame metadata again
            thisFrame.width = localImageDescriptor.width;
            thisFrame.height = localImageDescriptor.height;
            thisFrame.left = localImageDescriptor.left;
            thisFrame.top = localImageDescriptor.top;
            thisFrame.isInterlaced = localImageDescriptorPacked.interlaceFlag;

            // After the Local Image Descriptor, the image data should exist

            /* the base amount of bits required for LZW to work. They decide the dictionary size.
               This byte is always the first in the compressed data. */
            byte lzwMinCodeSize;
            fread(&lzwMinCodeSize, 1, 1, m_file);
            if(m_verbose) { printf("LZW minimum code size: %i\n",lzwMinCodeSize); }

            // Read the image data
            if(m_verbose) { printf("Reading image data...\n"); }
            
            
            std::vector<byte> compressedData;

            // Every block of an image begins with a one byte size, meaning that any block has the size of 0-255
            byte blockSize;
            fread(&blockSize,1,1,m_file);

            // if a block size of 0 appears, it means that there are no more blocks to be read
            uint32_t offset = 0;
            while (blockSize > 0) {
                std::vector<byte> tmp;
                tmp.reserve(blockSize);
                fread(tmp.data(), blockSize,1,m_file);

                for (size_t i = 0; i < blockSize; i++) {
                    compressedData.push_back(tmp[i]);
                }
                

                offset += blockSize;
                fread(&blockSize,1,1,m_file);
            }

            if(m_verbose) { printf("Decoding LZW compressed data...\n"); }
            
            GifLZW::LzwDecoder decoder(compressedData.data(), lzwMinCodeSize);
            thisFrame.indices = decoder.decode(m_verbose >= 2); // 2 - LZW log level
            
            if(m_verbose) { printf("Done.\n"); }

            if(m_verbose) { printf("Storing frame...\n"); }
            frames.push_back(thisFrame);
            if(m_verbose) { printf("Done!\n"); }


            byte stopByte;
            fread(&stopByte, 1, 1, m_file);
            // 3B is a unique byte that should not appear anywhere else after an image except for the end of the file
            if (stopByte == 0x3B)
                break; // EOF has been reached
            fseek(m_file, -1, SEEK_CUR); // If, not EOF seek back so the other functions can handle this byte
        }
        if(m_verbose) { printf("Stored all %li frames!\n",frames.size()); }
        

        fclose(m_file);
        return 0;
    }
    
} // namespace GifFile
