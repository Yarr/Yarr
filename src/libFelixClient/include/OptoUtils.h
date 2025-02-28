#ifndef OPTOUTILS_H
#define OPTOUTILS_H

#include "FelixController.h"
#include "Utils.h"
#include "logging.h"

namespace OptoUtils {

    /*
    Optoboard device communication
    */
    /// @brief Read a value from an LpGBT device register
    /// @param reg_addr Register address (uint32_t)
    /// @param reg_data Register data we're read back (set by reference) (uint8_t&)
    /// @return True if operation is successful
    bool readLpGBTRegister(uint_32t reg_addr, uint8_t& reg_data);

    /// @brief Write a value to an LpGBT device register
    /// @param reg_addr Register address (uint32_t)
    /// @param reg_data Register data we want to write (set by reference) (uint8_t&)
    /// @return True if operation is successful
    bool writeLpGBTRegister(uint32_t reg_addr, uint8_t& reg_data);

    /// @brief Read a value from a GBCR device register
    /// @param reg_addr Register address (uint32_t)
    /// @param reg_data Register data we're read back (set by reference) (uint8_t&)
    /// @return True if operation is successful
    uint8_t readGBCRRegister(uint32_t reg_addr, uint8_t& reg_data);

    /// @brief Write a value to a GBCR device register
    /// @param reg_addr Register address (uint32_t)
    /// @param reg_data Register data we want to write (set by reference) (uint8_t&)
    /// @return True if operation is successful
    bool writeGBCRRegister(uint32_t reg_addr, uint8_t& reg_data);

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

    uint32_t m_i2c_addr{0};

    /*
        Registers
    */
    // Optoboard V0
    static const uint32_t I2CM0ADDR_V0 {0x0F1};
    static const uint32_t I2CM0DATA0_V0 {0x0F2};
    static const uint32_t I2CM0DATA1_V0 {0x0F3};
    static const uint32_t I2CM0CMD_V0 {0x0F6};
    static const uint32_t I2CM0STATUS_V0 {0x161};
    static const uint32_t I2CM0READ15_V0 {0x173};
    
    static const uint32_t I2CM1ADDR_V0 {0x0F8};
    static const uint32_t I2CM1DATA0_V0 {0x0F9};
    static const uint32_t I2CM1DATA1_V0 {0x0FA};
    static const uint32_t I2CM1CMD_V0 {0x0FD};
    static const uint32_t I2CM1STATUS_V0 {0x176};   
    static const uint32_t I2CM1READ15_V0 {0x188};

    static const uint32_t I2CM2ADDR_V0 {0x0FF};
    static const uint32_t I2CM2DATA0_V0 {0x100};
    static const uint32_t I2CM2DATA1_V0 {0x101};
    static const uint32_t I2CM2CMD_V0 {0x104};
    static const uint32_t I2CM2STATUS_V0 {0x18B};
    static const uint32_t I2CM2READ15_V0 {0x19D};

    // Optoboard V1
    static const uint32_t I2CM0ADDR_V1 {0x101}    
    static const uint32_t I2CM0DATA0_V1 {0x102};
    static const uint32_t I2CM0DATA1_V1 {0x103};
    static const uint32_t I2CM0CMD_V1 {0x106};
    static const uint32_t I2CM0STATUS_V1 {0x171};
    static const uint32_t I2CM0READ15_V1 {0x183};
    
    static const uint32_t I2CM1ADDR_V1 {0x108};
    static const uint32_t I2CM1DATA0_V1 {0x109};
    static const uint32_t I2CM1DATA1_V1 {0x10A};
    static const uint32_t I2CM1CMD_V1 {0x10D};
    static const uint32_t I2CM1STATUS_V1 {0x186};   
    static const uint32_t I2CM1READ15_V1 {0x198};

    static const uint32_t I2CM2ADDR_V1 {0x10F};
    static const uint32_t I2CM2DATA0_V1 {0x110};
    static const uint32_t I2CM2DATA1_V1 {0x111};
    static const uint32_t I2CM2CMD_V1 {0x114};
    static const uint32_t I2CM2STATUS_V1 {0x19B};
    static const uint32_t I2CM2READ15_V1 {0x1AD};

    /// @brief For a given LpGBT number and FELIX link, finds the LpGBT address
    /// @param LpGBT_num 0,1,2,3 (unsigned int)
    /// @param link_num number of associated FELIX link (unsigned int)
    /// @return Returns the LpGBT address (uint32_t)
    uint32_t getLpGBTAddress(unsigned int LpGBT_num, unsigned int link_num);

    /*
    IC connection send/receive utilities
    */

    /// @brief Constructs the dataframe to send over an IC channel (used for example in LpGBT register reads)
    /// @param read Whether we will be reading or writing data (const bool, true for read, false for write)
    /// @param regAddr Address of the register to read (const uint16_t)
    /// @param i2cAddr I2C address to send data along, for optoboard communication, address of primary LpGBT (const uint8_t)
    /// @param deviceVersion LpGBT version (either 0 or 1), affects how the data frame is prepared (const unsigned int)
    /// @param data Data to send (const std::vector <uint8_t>&)
    /// @return Returns the dataframe to send through the IC channel (std::vector<uint8_t>)
    std::vector<uint8_t> prepareICDataFrame(const bool read, const uint16_t regAddr, const uint8_t i2cAddr, const unsigned int deviceVersion, const std::vector<uint8_t>& data);

}

#endif