#include "lzw.h"
#include <stdexcept>

namespace GifLZW
{
    LzwDecoder::LzwDecoder(uint8_t* data, uint32_t minimumBitCount) : m_minBitCount(minimumBitCount) {
        m_reader = BitStreamReader(data);
        initDictionary();
    }

    void LzwDecoder::initDictionary() {
        m_dict.clear();

        m_currBitCount = m_minBitCount + 1; // add one since the Clear Code and End Code also need to be taken into account
        m_dictSize = (1L << m_minBitCount) + 2; // + 2 for Clear Code and End Code
        for (size_t i = 0; i < m_dictSize; i++) {
            // the first values are the same as their keys, as they represent the final decoded indices
            m_dict.push_back(std::vector<uint32_t>(1, i)); // Key: LZW code, Value: code pattern assigned to said code      
        }
        m_clearCode = m_dictSize - 2;
        m_endCode   = m_dictSize - 1;
        
    }


    uint32_t LzwDecoder::getNextValue() {
        return m_reader.readBits(m_currBitCount);
    }


    // For more info, refer to https://giflib.sourceforge.net/whatsinagif/lzw_image_data.html
    std::vector<uint32_t> LzwDecoder::decode(bool verbose) {
        std::vector<uint32_t> indexVector;

        uint32_t code = getNextValue();
        if (code == m_clearCode) {
            initDictionary();
            code = getNextValue(); // Get next code, as a clear code shouldn't appear in the code stream or else things will break
        }

        indexVector.push_back(code);
        uint32_t lastCode = code;
        if (verbose) { printf("Clear Code: %i, EOI Code: %i\n",m_clearCode,m_endCode); }

        while (true) {
            uint32_t dbg_bitOffset = m_reader.getBitOffset(), dbg_byteOffset = m_reader.getByteOffset();
            code = getNextValue();
            
            if (verbose) {
                printf("Code: %i, BitCount: %i, DictSize: %i, LastCode: %i, %i:%i\n",code, m_currBitCount, m_dictSize, lastCode, dbg_byteOffset, dbg_bitOffset); 
                printf("%#08x %#08x\n", m_reader.getBytePtr()[dbg_byteOffset], m_reader.getBytePtr()[dbg_byteOffset+1]);    
            }

            if (code == m_endCode)
                break;
            if (code == m_clearCode) {
                if (verbose) { printf("!!!! Reinitializing Dictionary !!!!\n"); }

                initDictionary();
                
                /* code is a clear code so invalidate it
                 * and since every code after a clearCode
                 * is a uncompressed code just output it
                 * to the index stream
                 */ 
                code = getNextValue();
                indexVector.push_back(code);
                lastCode = code;
                continue;
            }


            if(code < m_dictSize) { // if code in keys of m_dict
                indexVector.insert(indexVector.end(), m_dict[code].begin(), m_dict[code].end());
                uint32_t k = m_dict[code][0];

                m_dict.push_back(m_dict[lastCode]);
                m_dict[m_dictSize].push_back(k);
                m_dictSize++;

            } else {
                if (code > m_dictSize) {
                    throw std::runtime_error("LZW code is bigger that the current dictionary size");
                }

                uint32_t k = m_dict[lastCode][0];
                m_dict.push_back(m_dict[lastCode]);
                m_dict[m_dictSize].push_back(k);
                indexVector.insert(indexVector.end(), m_dict[m_dictSize].begin(), m_dict[m_dictSize].end());
                m_dictSize++;
            }


            if (m_dictSize == (1L << m_currBitCount) && m_currBitCount < 12) // < 12 Not sure if standards compliant but it's a hacky way to fix a bug
                m_currBitCount++;
   
            lastCode = code;
        }
        
        return indexVector;
    }
} // namespace GifLZW

