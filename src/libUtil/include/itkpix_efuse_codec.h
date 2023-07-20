/*
itkpix-efuse-codec
version 0.3.2
https://gitlab.cern.ch/berkeleylab/itkpix-efuse-codec

Licensed under the MIT License <http://opensource.org/licenses/MIT>.
SPDX-License-Identifier: MIT
Copyright (c) 2020 Daniel Antrim

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef INCLUDE_ITKPIX_EFUSE_CODEC_H
#define INCLUDE_ITKPIX_EFUSE_CODEC_H


// std/stl
#include <string>
#include <sstream> // stringstream
#include <iomanip> // setw, setfill
#include <map>

// hamming-codec
#include "hamming_codec.h"

namespace itkpix_efuse_codec {

    namespace internal {
        union EfuseInput {
            EfuseInput() : raw(0){}
            uint32_t raw;
            struct {
                unsigned int chip_serial_number : 20;
                unsigned int probe_location_id : 4;
            } fields;
        };
        static const unsigned N_BITS_EFUSE_MESSAGE{24};
    } // namespace internal

    static const std::map<std::string, uint8_t> ProbeLocationMap {
        {"UNPROBED", 0}
       ,{"BONN", 1}
       ,{"GLASGOW", 2}
       ,{"PARIS", 3}
    };

    class EfuseData {
        public :
            EfuseData(const uint32_t& efuse_data) :
                _message(efuse_data) {}
            EfuseData(const std::string& binary_str) :
                _message(0)
            {
                try {
                    _message = std::stoul(binary_str, 0, 2);
                } catch(std::exception& e) {
                    throw std::runtime_error("Invalid binary string provided to EfuseData: \"" + binary_str + "\"");
                }
            }

            const uint32_t raw() { return _message._raw; }
            const uint32_t chip_sn() { return _message.fields.chip_serial_number; }
            const uint32_t probe_location_id() { return _message.fields.probe_location_id; }
            const std::string probe_location_name() {
                for(const auto& [k, v] : ProbeLocationMap) {
                    if(v == _message.fields.probe_location_id)
                        return k;
                }
                throw std::runtime_error("Could not get probe location name from stored data");
            }

        private :
            union _efuse_layout {
                _efuse_layout(uint32_t data) : _raw(data){}
                uint32_t _raw;
                struct {
                    unsigned int chip_serial_number : 20;
                    unsigned int probe_location_id : 4;
                } fields;
            };
            _efuse_layout _message;
    };

    // decode ITkPix e-fuse data readback from a chip
    static inline std::string decode(const uint32_t& efuse_data) {

        // expect LSB 8-bits to hold the 5 parity bits
        uint32_t parity = (0x1f & efuse_data);
        uint32_t without_parity = 0xffffff & (efuse_data >> 8);
        uint32_t efuse_data_formatted = (without_parity << 5) | parity;
        std::string decoded_binary_str = hamming_codec::decode(efuse_data_formatted
                                                            ,29
                                                            ,hamming_codec::ParityLocation::LSB
                                                            ,5);
        return decoded_binary_str;
    }

        static inline std::string decodeOldFormat(const uint32_t& efuse_data) {

        // expect LSB 8-bits to hold the 5 parity bits
        uint32_t parityR = (0x1f & efuse_data);
	uint32_t parity = ((parityR&0x10)>>4) | ((parityR&0x8)>>2) | (parityR&0x4) | ((parityR&0x2)<<2) | ((parityR&0x1)<<4);
        uint32_t without_parity = 0xffffff & (efuse_data >> 8);
        uint32_t efuse_data_formatted = (without_parity << 5) | parity;
        std::string decoded_binary_str = hamming_codec::decode(efuse_data_formatted
                                                            ,29
                                                            ,hamming_codec::ParityLocation::LSB
                                                            ,5);
        return decoded_binary_str;
    }

    static inline std::string encode(const std::string& probe_location_name, const uint32_t& chip_serial_number) {

        // check probe location
        if(ProbeLocationMap.count(probe_location_name) == 0) {
            throw std::runtime_error("Invalid probe location \"" + probe_location_name + "\"");
        }
        uint32_t probe_location_id = static_cast<uint32_t>(ProbeLocationMap.at(probe_location_name));

        uint32_t efuse_input{0x0};
        efuse_input |= (0xf & probe_location_id) << 20;
        efuse_input |= (0xfffff & chip_serial_number);

        std::string encoded_binary_string = hamming_codec::encode((0xffffff & efuse_input)
                                                                    ,internal::N_BITS_EFUSE_MESSAGE
                                                                    ,hamming_codec::ParityLocation::LSB);

        int n_parity_bits = encoded_binary_string.length() - internal::N_BITS_EFUSE_MESSAGE;
        if(n_parity_bits != 5) {
            throw std::runtime_error("Invalid number of parity bits (expect: 5, got: " + std::to_string(n_parity_bits) + ")");
        }

        std::string encoding_without_parity = encoded_binary_string.substr(0, encoded_binary_string.length() - n_parity_bits);
        std::string parity_bits_str = encoded_binary_string.substr(encoded_binary_string.length() - n_parity_bits, encoded_binary_string.length());

        std::stringstream expected;
        expected << std::hex << efuse_input;
        std::stringstream received;
        received << std::hex << std::stoul(encoding_without_parity, 0, 2);
        bool equal_as_expected = (expected.str() == received.str());

        // itkpix efuses store the parity bits as 8-bit block
        std::stringstream parity_bit_block;
        parity_bit_block << std::setw(8) << std::setfill('0') << parity_bits_str;

        std::stringstream itkpix_encoded_efuse;
        itkpix_encoded_efuse << encoding_without_parity << parity_bit_block.str();

        if(itkpix_encoded_efuse.str().length() != 32) {
            throw std::runtime_error("Encoded efuse data length != 32 bits");
        }
        return itkpix_encoded_efuse.str();
    }
} // namespace itkpix_efuse_codec


#endif
