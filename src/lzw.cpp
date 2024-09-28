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
        m_dictSize = pow(2, m_minBitCount) + 2; // + 2 for Clear Code and End Code
        for (size_t i = 0; i < m_dictSize; i++) {
            // the first values are the same as their keys, as they represent the final decoded indices
            m_dict[i] = std::vector<uint32_t>(1, i); // Key: LZW code, Value: code pattern assigned to said code
            
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
        
        int32_t code = getNextValue();
        if (static_cast<uint32_t>(code) == m_clearCode) {
            initDictionary();
            code = getNextValue(); // Get next code, as a clear code shouldn't appear in the code stream or else things will break
            indexVector = m_dict[code]; // first code after start of clear is always a one-to-one match with its decoded index
        } else {
            indexVector = m_dict[code];
        }
        int32_t lastCode = code;
        if (verbose) { printf("Clear Code: %i, EOI Code: %i\n",m_clearCode,m_endCode); }

        while (true) {
            uint32_t dbg_bitOffset = m_reader.getBitOffset(), dbg_byteOffset = m_reader.getByteOffset();
            code = getNextValue();
            
            if (verbose) {
                printf("Code: %i, BitCount: %i, DictSize: %i, LastCode: %i, %i:%i\n",code, m_currBitCount, m_dictSize, lastCode, dbg_byteOffset, dbg_bitOffset); 
                printf("%#08x %#08x\n", m_reader.getBytePtr()[dbg_byteOffset], m_reader.getBytePtr()[dbg_byteOffset+1]);    
            }

            if (static_cast<uint32_t>(code) == m_endCode)
                break;
            if (static_cast<uint32_t>(code) == m_clearCode) {
                if (verbose) { printf("!!!! Reinitializing Dictionary !!!!\n"); }

                initDictionary();
                lastCode = -1; // -1 means NULL and states that a dictionary initialization has just happened and the next code is not encoded
                continue;
            }

            if (lastCode == -1) {
                indexVector.push_back(code);
                lastCode = code;
                continue;
            }


            if(m_dict.count(code) > 0) { // if code in keys of m_dict
                indexVector.insert(indexVector.end(), m_dict[code].begin(), m_dict[code].end());
                uint32_t k = m_dict[code][0];

                m_dict[m_dictSize] = m_dict[lastCode];
                m_dict[m_dictSize].push_back(k);
                m_dictSize++;

            } else {
                if (static_cast<uint32_t>(code) > m_dictSize) {
                    throw std::runtime_error("LZW code is bigger that the current dictionary size");
                }

                uint32_t k = m_dict[lastCode][0];
                indexVector.insert(indexVector.end(), m_dict[lastCode].begin(), m_dict[lastCode].end());
                indexVector.push_back(k);

                m_dict[m_dictSize] = m_dict[lastCode];
                m_dict[m_dictSize].push_back(k);
                m_dictSize++;
            }

            lastCode = code;

            if (m_dictSize == pow(2,m_currBitCount) && m_currBitCount < 12) // < 12 Not sure if standards compliant but it's a hacky way to fix a bug
                m_currBitCount++;
        }
        
        return indexVector;
    }


} // namespace GifLZW

