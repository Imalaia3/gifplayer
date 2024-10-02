#pragma once

#include <unordered_map>
#include <stdint.h>
#include <cstdio>
#include <cmath>
#include <vector>


namespace GifLZW {
    class BitStreamReader {
    public:
        BitStreamReader() {}
        BitStreamReader(uint8_t* bytes) : m_bytes(bytes) {}

        // returns a single bit as a uint32_t (0/1)
        uint32_t readSingleBit() {
            if (m_bitOffset > 7) {
                m_bitOffset = 0;
                m_byteOffset++;
            }
            auto ret = (m_bytes[m_byteOffset] & (1 << m_bitOffset)) >> m_bitOffset;
            m_bitOffset++;
            return ret;
        }

        // read n bits (maximum 32 bits)
        uint32_t readBits(uint32_t nBits) {
            //assert(nBits <= sizeof(uint32_t)*8)
            uint32_t ret = 0;
            for (size_t i = 0; i < nBits; ++i) {
                ret |= readSingleBit() << i;
            }
            return ret;
        }

        uint32_t getByteOffset() { return m_byteOffset; }
        uint32_t getBitOffset() { return m_bitOffset; }
        uint8_t* getBytePtr() { return m_bytes; }

    private:
        uint8_t* m_bytes;
        uint32_t m_byteOffset = 0;
        uint32_t m_bitOffset = 0;
    };



    class LzwDecoder {
    public:
        LzwDecoder(uint8_t* data, uint32_t minimumBitCount);
        /*
         * Decode GIF LZW compressed data
         * Returns: vector of the decoded color table indices
         */
        std::vector<uint32_t> decode(bool verbose = false);
    private:
        uint32_t getNextValue();
        void initDictionary();

        BitStreamReader m_reader;
        std::unordered_map<uint32_t, std::vector<uint32_t>> m_dict;
        // clear code: when this code appears in the stream, reset the dictionary to its original values
        // end code: when this code appears, no more bytes need to be decoded
        uint32_t m_clearCode, m_endCode;
        uint32_t m_dictSize;
        // minBitCount: minimum bits required to represent a color index
        // currBitCount: since LZW codes can have a variable size, the currBitCount specifies that size
        uint32_t m_minBitCount, m_currBitCount;
    };
} // namespace GifLZW