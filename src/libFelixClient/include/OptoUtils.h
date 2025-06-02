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
    constexpr static uint32_t m_freq{2};
    constexpr static uint32_t m_scldrive{0};

    constexpr static uint8_t DEFAULT_LPGBT_VERSION{1};
    constexpr static uint16_t DEFAULT_I2C_ADDR{0};
    constexpr static uint16_t DEFAULT_LPGBT_PRIMARY_ADDR{116};

    /// @brief Returns the device address from a provided LpGBT number and primary LpGBT address
    /// @param number The LpGBT number (0, 1, 2, 3) (uint8_t)
    /// @param primary_address The address of the primary LpGBT (default 116) (uint16_t)
    /// @return The LpGBT address (uint16_t)
    const uint16_t getLpGBTAddress(uint8_t number, uint16_t primary_address);
    
    /// @brief Returns a pointer to a member of the lpgbt_item_t class which defines lpgbt register properties
    /// @param name Name of the register (const char*)
    /// @param version LpGBT version (either 0 or 1) (uint8_t)
    /// @return Returns a pointer to a member of the lpgbt_item_t class
    const lpgbt_item_t* getLpGBTRegisterByName(const char* name, uint8_t version);


    /// @brief Returns a pointer to a member of the lpgbt_item_t class which defines lpgbt register properties
    /// @param regAddr Address of the register (uint16_t)
    /// @param version LpGBT version (either 0 or 1) (uint8_t)
    /// @return Returns a pointer to a member of the lpgbt_item_t class
    const lpgbt_item_t* getLpGBTRegisterByAddr(uint16_t reg_addr, uint8_t version);

    /// @brief Returns teh bitmask corresponding to a particular register field
    /// @param item lpgbt item (register) (const lpgbt_item_t*)
    /// @param version version of the lpgbt (uint8_t)
    /// @return the bitmask (uint8_t)
    uint8_t getRegBitmask(const lpgbt_item_t* item, uint8_t version);

    /// @brief Determines if a provided register is a regfield (sub-field of a total register in memory)
    /// @param item lpgbt item (register) (const lpgbt_item_t*)
    /// @return True if a regfield, false if not (bool)
    bool regField(const lpgbt_item_t* item);

    /// @brief Returns a value we've read after applying a regfield mask
    /// @param item lpgbt item (register) (const lpgbt_item_t*)
    /// @param reg_data the data we need to apply a mask to (uint8_t)
    /// @return The masked data (uint8_t)
    uint8_t applyRegfieldReadMask(const lpgbt_item_t* item, uint8_t reg_data);

    /// @brief Returns a value to write after applying a regfield mask
    /// @param item lpgbt item (register) (const lpgbt_item_t*)
    /// @param data the data we need to apply a mask to (uint8_t)
    /// @param current_data The current readout of the register (uint8_t)
    /// @param version The lpgbt version (uint8_t)
    /// @return The masked value (uint8_t)
    uint8_t applyRegfieldWriteMask(const lpgbt_item_t* item, uint8_t data, uint8_t current_data, uint8_t version);

    /// @brief Constructs the dataframe to send over an IC channel (used for example in LpGBT register reads)
    /// @param write Whether we will be reading or writing data (const bool, true for write, false for read)
    /// @param regAddr Address of the register to read (const uint16_t)
    /// @param data The data we want to send (uint8_t)
    /// @param version LpGBT version (either 0 or 1), affects how the data frame is prepared (uint8_t)
    /// @param devAddr Address of LpGBT device (uint16_t)
    /// @return Returns the dataframe to send through the IC channel (std::vector<uint8_t>)
    std::vector<uint8_t> prepareICDataFrame(const bool write, const uint16_t reg_addr, const uint8_t data, uint8_t version, uint16_t dev_addr);
}
#endif

