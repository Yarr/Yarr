#ifndef OPTOUTILS_H
#define OPTOUTILS_H

#include <map>

#include "Utils.h"
#include "logging.h"

namespace OptoUtils {
    // I2C parameters, defined in: https://gitlab.cern.ch/bat/optoboard_felix/-/blob/main/src/optoboard_felix/driver/Hardware.py#L55 and lpGBTv1 manual ch. 12.2.1
    uint32_t m_i2c_write_cr{0x0};
    uint32_t m_i2c_write_msk{0x1};
    uint32_t m_i2c_1byte_write{0x2};
    uint32_t m_i2c_1byte_read{0x3};
    uint32_t m_i2c_1byte_write_ext{0x4};
    uint32_t m_i2c_1byte_read_ext{0x5};
    uint32_t m_i2c_1byte_rmw_or{0x6};
    uint32_t m_i2c_1byte_rmw_xor{0x7};
    uint32_t m_i2c_w_multi_4byte0{0x8};
    uint32_t m_i2c_w_multi_4byte1{0x9};
    uint32_t m_i2c_w_multi_4byte2{0xA};
    uint32_t m_i2c_w_multi_4byte3{0xB};
    uint32_t m_i2c_write_multi{0xC};
    uint32_t m_i2c_read_multi{0xD};
    uint32_t m_i2c_write_multi_ext{0xE};
    uint32_t m_i2c_read_multi_ext{0xF};
    uint32_t m_freq{2};
    uint32_t m_scldrive{0};

    // Default values, can be specified in connectivity file if different
    uint32_t m_lpgbt_primary_addr{116};
    uint32_t m_i2c_addr{0};
    unsigned int m_lpgbt_version{1};

    /*
        Registers
    */
    // Optoboard V0
    static const std::map<std::string, uint32_t> LPGBT_REGMAP {
    {"I2CM0ADDR_V0", 0x0F1},
    {"I2CM0DATA0_V0", 0x0F2},
    {"I2CM0DATA1_V0", 0x0F3},
    {"I2CM0DATA2_V0", 0x0F4},
    {"I2CM0CMD_V0", 0x0F6},
    {"I2CM0STATUS_V0", 0x161},
    {"I2CM0READ15_V0", 0x173},
    
    {"I2CM1ADDR_V0", 0x0F8},
    {"I2CM1DATA0_V0", 0x0F9},
    {"I2CM1DATA1_V0", 0x0FA},
    {"I2CM1DATA2_V0", 0x0FB},
    {"I2CM1CMD_V0", 0x0FD},
    {"I2CM1STATUS_V0", 0x176},   
    {"I2CM1READ15_V0", 0x188},

    {"I2CM2ADDR_V0", 0x0FF},
    {"I2CM2DATA0_V0", 0x100},
    {"I2CM2DATA1_V0", 0x101},
    {"I2CM2DATA2_V0", 0x102},
    {"I2CM2CMD_V0", 0x104},
    {"I2CM2STATUS_V0", 0x18B},
    {"I2CM2READ15_V0", 0x19D},
    {"I2CM0ADDR_V1", 0x101},
    {"I2CM0DATA0_V1", 0x102},
    {"I2CM0DATA1_V1", 0x103},
    {"I2CM0DATA2_V1", 0x104},
    {"I2CM0CMD_V1", 0x106},
    {"I2CM0STATUS_V1", 0x171},
    {"I2CM0READ15_V1", 0x183},
    
    {"I2CM1ADDR_V1", 0x108},
    {"I2CM1DATA0_V1", 0x109},
    {"I2CM1DATA1_V1", 0x10A},
    {"I2CM1DATA2_V1", 0x10C},
    {"I2CM1CMD_V1", 0x10D},
    {"I2CM1STATUS_V1", 0x186},
    {"I2CM1READ15_V1", 0x198},

    {"I2CM2ADDR_V1", 0x10F},
    {"I2CM2DATA0_V1", 0x110},
    {"I2CM2DATA1_V1", 0x111},
    {"I2CM2DATA2_V1", 0x112},
    {"I2CM2CMD_V1", 0x114},
    {"I2CM2STATUS_V1", 0x19B},
    {"I2CM2READ15_V1", 0x1AD}
    };

    /// @brief For a given device type and number (e.g. GBCR #2), return the address
    /// @param deviceType "GBCR", "LPGBT" accepted (string)
    /// @param rx From front-end device controller file
    /// @param lpgbt_primary_addr default value is 116 (uint32_t)
    /// @return Returns the device address (uint32_t)
    uint32_t getDeviceAddress(std::string device_type, int rx, uint32_t lpgbt_primary_addr = 116);

    int getFELIXLinkForLpGBT(int rx);

    int getDeviceNum(int rx);

    bool isPrimaryLpGBT(int rx);

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
std::vector<uint8_t> prepareICDataFrame(const bool write, const uint16_t reg_addr, const std::vector<uint8_t>& data, const uint8_t i2c_addr, const unsigned int device_version);
}

#endif

/*

give FID of front end connected to it, use the enable/find IC functions to figure out what the fid is for the lpgbt

*/