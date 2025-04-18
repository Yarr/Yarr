#ifndef OPTOUTILS_H
#define OPTOUTILS_H

#include <map>
#include <iostream>

#include "Utils.h"
#include "logging.h"
#include "lpgbt-items-v0.h"
#include "lpgbt-items-v1.h"

namespace OptoUtils {
    constexpr static std::size_t NUM_BYTES_IC_HEADER_V0{7};          ///< First 7 bytes of IC reply are header
    constexpr static std::size_t NUM_BYTES_IC_HEADER_V1{6};          ///< First 7 bytes of IC reply are header
    constexpr static std::size_t NUM_PARITY_BYTES_IC_TRAILER{1};  ///< Last byte of IC reply is parity word
    constexpr static std::size_t FIRST_IC_PAYLOAD_BYTE_V0{NUM_BYTES_IC_HEADER_V0};
    constexpr static std::size_t FIRST_IC_PAYLOAD_BYTE_V1{NUM_BYTES_IC_HEADER_V1};

    // I2C parameters, defined in: https://gitlab.cern.ch/bat/optoboard_felix/-/blob/main/src/optoboard_felix/driver/Hardware.py#L55 and lpGBTv1 manual ch. 12.2.1
    constexpr static uint32_t m_i2c_write_cr{0x0};
    constexpr static uint32_t m_i2c_write_msk{0x1};
    constexpr static uint32_t m_i2c_1byte_write{0x2};
    constexpr static uint32_t m_i2c_1byte_read{0x3};
    constexpr static  uint32_t m_i2c_1byte_write_ext{0x4};
    constexpr static  uint32_t m_i2c_1byte_read_ext{0x5};
    constexpr static uint32_t m_i2c_1byte_rmw_or{0x6};
    constexpr static uint32_t m_i2c_1byte_rmw_xor{0x7};
    constexpr static uint32_t m_i2c_w_multi_4byte0{0x8};
    constexpr static uint32_t m_i2c_w_multi_4byte1{0x9};
    constexpr static uint32_t m_i2c_w_multi_4byte2{0xA};
    constexpr static  uint32_t m_i2c_w_multi_4byte3{0xB};
    constexpr static uint32_t m_i2c_write_multi{0xC};
    constexpr static uint32_t m_i2c_read_multi{0xD};
    constexpr static  uint32_t m_i2c_write_multi_ext{0xE};
    constexpr static  uint32_t m_i2c_read_multi_ext{0xF};
    constexpr static  uint32_t m_freq{2};
    constexpr static uint32_t m_scldrive{0};

    /*
        Registers
    */

    /// @brief For a given device type and number (e.g. GBCR #2), return the address
    /// @param deviceType "GBCR", "LPGBT" accepted (string)
    /// @param rx From front-end device controller file
    /// @param lpgbt_primary_addr default value is 116 (uint32_t)
    /// @return Returns the device address (uint32_t)
   //uint32_t getDeviceAddress(std::string device_type, int rx, uint32_t lpgbt_primary_addr);

    int getFELIXLinkForLpGBT(int rx);

    int getDeviceNum(int rx);

    bool isPrimaryLpGBT(int rx);

    const lpgbt_item_t* getLpGBTRegisterByName(const char* name, uint8_t version);

    const lpgbt_item_t* getLpGBTRegisterByAddr(uint16_t reg_addr, uint8_t version);

    /*
    IC connection send/receive utilities
    */

    /// @brief Constructs the dataframe to send over an IC channel (used for example in LpGBT register reads)
    /// @param write Whether we will be reading or writing data (const bool, true for write, false for read)
    /// @param regAddr Address of the register to read (const uint16_t)
    /// @param data The data we want to send (const std::vector<uint8_t>&)
    /// @param i2cAddr I2C address to send data along, for optoboard communication, address of primary LpGBT (const uint8_t)
    /// @param deviceVersion LpGBT version (either 0 or 1), affects how the data frame is prepared (const unsigned int)
    /// @return Returns the dataframe to send through the IC channel (std::vector<uint8_t>)

    std::vector<uint8_t> prepareICDataFrame(const bool write, const uint16_t reg_addr, const uint8_t data, const uint16_t data_size);
}
#endif

/*

give FID of front end connected to it, use the enable/find IC functions to figure out what the fid is for the lpgbt

*/