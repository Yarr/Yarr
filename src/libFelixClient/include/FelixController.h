#ifndef FELIXCONTROLLER_H
#define FELIXCONTROLLER_H

#include "HwController.h"
#include "FelixRxCore.h"
#include "FelixTxCore.h"


#include "felix/felix_client_thread.hpp"

#include "storage.hpp"

class FelixController
  : public HwController, public FelixTxCore, public FelixRxCore
{
public:
  FelixController() = default;

  void loadConfig(json const &j) override;
  const json getStatus() override;
  /*
  E-link control
  */

  /* IC */
  /// @brief Check if or not the IC channel on the same link as the FELIX ID is enabled
  /// @param fid 64-bit FELIX ID
  /// @return True if the IC associated with the fid is enable, false otherwise
  bool getICEnable(uint64_t fid);

  /// @brief Check if all IC channels on the same links as the list of FELIX IDs are enabled
  /// @param fids A vector of FELIX IDs
  /// @return True if the IC channels associated with the fids are all enabled, false otherwise
  bool getICEnable(const std::vector<uint64_t>& fids);

  /// @brief Check if all IC channels on the same links as the list of FELIX IDs are enabled AND if all other IC channels on the same FELIX device are disabled
  /// @param fids A vector of FELIX IDs
  /// @return True if the IC channels associated with the fids are all enabled and other IC channels on the same FELIX device are all disabled, false otherwise
  bool getICEnableExclusive(const std::vector<uint64_t>& fids);

  /// @brief Enable or disable the IC channel on the same link as the FELIX ID
  /// @param fid 64-bit FELIX ID
  /// @param enable Set to true to enable the channel, false to disable the channel. Default: true
  /// @return True if the operation is successful, false otherwise
  bool setICEnable(uint64_t fid, bool enable=true);

  /// @brief Enable or disable the IC channels on the same links as the list of FELIX IDs
  /// @param fids A vector of FELIX IDs
  /// @param enable Set to true to enable the channels, false to disable the channels. Default: true
  /// @return True if the operation is successful, false otherwise
  bool setICEnable(const std::vector<uint64_t>& fids, bool enable=true);

  /// @brief Enable the IC channels on the same links as the list of FELIX IDs AND also disable all other IC channels on the same FELIX device
  /// @param fids A vector of FELIX IDs
  /// @return True if the operation is successful, false otherwise
  bool setICEnableExclusive(const std::vector<uint64_t>& fids);

  /// @brief Disable all IC channels on a FELIX device
  /// @param toflx Link direction
  /// @return True if the operation is successful, false otherwise
  bool disableAllICs(bool toflx);

  /* EC */
  /// @brief Check if or not the EC channel on the same link as the FELIX ID is enabled
  /// @param fid 64-bit FELIX ID
  /// @return True if the EC associated with the fid is enable, false otherwise
  bool getECEnable(uint64_t fid);

  /// @brief Check if all EC channels on the same links as the list of FELIX IDs are enabled
  /// @param fids A vector of FELIX IDs
  /// @return True if the EC channels associated with the fids are all enabled, false otherwise
  bool getECEnable(const std::vector<uint64_t>& fids);

  /// @brief Check if all EC channels on the same links as the list of FELIX IDs are enabled AND if all other EC channels on the same FELIX device are disabled
  /// @param fids A vector of FELIX IDs
  /// @return True if the EC channels associated with the fids are all enabled and other EC channels on the same FELIX device are all disabled, false otherwise
  bool getECEnableExclusive(const std::vector<uint64_t>& fids);

  /// @brief Enable or disable the EC channel on the same link as the FELIX ID
  /// @param fid 64-bit FELIX ID
  /// @param enable Set to true to enable the channel, false to disable the channel. Default: true
  /// @return True if the operation is successful, false otherwise
  bool setECEnable(uint64_t fid, bool enable=true);

  /// @brief Enable or disable the EC channels on the same links as the list of FELIX IDs
  /// @param fids A vector of FELIX IDs
  /// @param enable Set to true to enable the channels, false to disable the channels. Default: true
  /// @return True if the operation is successful, false otherwise
  bool setECEnable(const std::vector<uint64_t>& fids, bool enable=true);

  /// @brief Enable the EC channels on the same links as the list of FELIX IDs AND also disable all other EC channels on the same FELIX device
  /// @param fids A vector of FELIX IDs
  /// @return True if the operation is successful, false otherwise
  bool setECEnableExclusive(const std::vector<uint64_t>& fids);

  /// @brief Disable all EC channels on a FELIX device
  /// @param toflx Link direction
  /// @return True if the operation is successful, false otherwise
  bool disableAllECs(bool toflx);

  /* Data E-links */
  /// @brief Check if or not the elink is enabled
  /// @param fid 64-bit FELIX ID
  /// @return True if the elink is enabled, false otherwise
  bool getELinkEnable(uint64_t fid);

  /// @brief Check if all the elinks as specified by the list of FELIX IDs are enabled
  /// @param fids A vector of FELIX IDs
  /// @return True of all the elinks are enabled, false otherwise
  bool getELinkEnable(const std::vector<uint64_t>& fids);

  /// @brief Check if all the elinks as specified by the list of FELIX IDs are enabled AND if all other elinks on the same FELIX device are disabled
  /// @param fids A vector of FELIX IDs
  /// @return True if the elinks as specified by the fids are all enabled and other elinks on the same FELIX device are disabled
  bool getELinkEnableExclusive(const std::vector<uint64_t>& fids);

  /// @brief Get the elink width in the unit of number of bits
  /// @param fid 64-bit FELIX ID
  /// @return Elink width in number of bits
  unsigned getELinkWidthNBits(uint64_t fid);

  /// @brief Get the elink width in the unit of Mbps
  /// @param fid 64-bit FELIX ID
  /// @return Elink width in Mbps
  unsigned getELinkWidthMbps(uint64_t fid);

  /// @brief Enable or disable an elink
  /// @param fid 64-bit FELIX ID
  /// @param enable Set to true to enable the elink, false to disable it. Default: true
  /// @return True if the operation is successful, false otherwise
  bool setELinkEnable(uint64_t fid, bool enable=true);

  /// @brief Enable or disable multiple elinks as specified by the list of FELIX IDs
  /// @param fids A vector of FELIX IDs to configure
  /// @param enable Set to true to enable the elinks, false to disable them. Default: true
  /// @return True if the operation is successful, false otherwise
  bool setELinkEnable(const std::vector<uint64_t>& fids, bool enable=true);

  /// @brief Enable the elinks as specified by the list of FELIX IDs AND disable all other elinks on the same FELIX device
  /// @param fids A vector of FELIX IDs to enable
  /// @return True if the operation is successful, false otherwise
  bool setELinkEnableExclusive(const std::vector<uint64_t>& fids);

  /// @brief Disable all elinks on a FELIX device
  /// @param toflx Link direction
  /// @return True if the operation is successful, false otherwise
  bool disableAllELinks(bool toflx);

  /// @brief Set the elink width
  /// @param fid 64-bit FELIX ID
  /// @param nbits Elink width in number of bits
  /// @return True if the operation is successful, false otherwise
  bool setELinkWidthNBits(uint64_t fid, unsigned nbits);

  /// @brief Set the width of multiple elinks
  /// @param fids A vector of FELIX IDs
  /// @param nbits Elink width in number of bits
  /// @return True if the operation is successful, false otherwise
  bool setELinkWidthNBits(const std::vector<uint64_t>& fids, unsigned nbits);

  /// @brief Set the elink width
  /// @param fid 64-bit FELIX ID
  /// @param bandwidth Elink bandwidth in Mbps
  /// @return True if the operation is successful
  bool setELinkWidthMbps(uint64_t fid, unsigned bandwidth);

  /// @brief Set the width of multiple elinks
  /// @param fids A vector of FELIX IDs
  /// @param bandwidth Elink bandwidth in Mbps
  /// @return True if the operation is successful
  bool setELinkWidthMbps(const std::vector<uint64_t>& fids, unsigned bandwidth);

  /*
  Optoboard device communication
  */

    /// @brief Read a value from an LpGBT device register (providing address)
    /// @param reg_addr Register address (int)
    /// @param reg_data Register data we are reading back (set by reference) (uint8_t&)
    /// @param dev_addr Address of lpgbt (uint16_t)
    /// @param rx_ic_fid The fid of the ic channel for rx (uint64_t)
    /// @param tx_ic_fid The fid of the ic channel for tx (uint64_t)
    /// @return True if successful, false if not (bool)
    bool readLpGBTRegister(int reg_addr, uint8_t& reg_data, uint16_t dev_addr, uint64_t rx_ic_fid, uint64_t tx_ic_fid);

    /// @brief Read a value from an LpGBT device register (providing name)
    /// @param reg_name Register name (std::string)
    /// @param reg_data Register data we are reading back (set by reference) (uint8_t&)
    /// @param dev_addr Address of lpgbt (uint16_t)
    /// @param rx_ic_fid The fid of the ic channel for rx (uint64_t)
    /// @param tx_ic_fid The fid of the ic channel for tx (uint64_t)
    /// @return True if successful, false if not (bool)
    bool readLpGBTRegister(const char* reg_name, uint8_t& reg_data, uint16_t dev_addr, uint64_t rx_ic_fid, uint64_t tx_ic_fid);

    /// @brief Write a value to an LpGBT device register (providing address)
    /// @param reg_addr Register address (int)
    /// @param reg_data Register data we are writing (uint8_t&)
    /// @param dev_addr Address of lpgbt (uint16_t)
    /// @param rx_ic_fid The fid of the ic channel for rx (uint64_t)
    /// @param tx_ic_fid The fid of the ic channel for tx (uint64_t)
    /// @return True if successful, false if not (bool)
    bool writeLpGBTRegister(int reg_addr, uint8_t& reg_data, uint16_t dev_addr, uint64_t rx_ic_fid, uint64_t tx_ic_fid);

    /// @brief Write a value to an LpGBT device register (providing name)
    /// @param reg_name Register name (std::string)
    /// @param reg_data Register data we are writing (uint8_t&)
    /// @param dev_addr Address of lpgbt (uint16_t)
    /// @param rx_ic_fid The fid of the ic channel for rx (uint64_t)
    /// @param tx_ic_fid The fid of the ic channel for tx (uint64_t)
    /// @return True if successful, false if not (bool)
    bool writeLpGBTRegister(const char* reg_name, uint8_t& reg_data, uint16_t dev_addr, uint64_t rx_ic_fid, uint64_t tx_ic_fid);

protected:
    class OptoDevice{
      public:
        OptoDevice(uint8_t arg_version, uint16_t arg_i2c_addr, uint16_t arg_dev_addr, uint16_t arg_dev_primary_addr, std::string arg_dev_type, uint64_t arg_tx_fid, uint64_t arg_rx_fid){
          version = arg_version;
          i2c_addr = arg_i2c_addr;
          dev_addr = arg_dev_addr;
          dev_primary_addr = arg_dev_primary_addr;
          dev_type = arg_dev_type;
          tx_fid = arg_tx_fid;
          rx_fid = arg_rx_fid;

          if (arg_dev_type != "lpgbt" && arg_dev_type != "gbcr"){
            std::cerr << "Invalid device type, accepted options are lpgbt or gbcr" << std::endl;
          }
        };
  
        virtual ~OptoDevice() = default;
  
        /*
          Accessor Functions
        */

        virtual inline uint8_t getVersion(){
          return version;
        }
        virtual inline uint16_t getI2CAddr(){
          return i2c_addr;
        }
        virtual inline uint16_t getDevAddr(){
          return dev_addr;
        }
        virtual inline uint16_t getPrimaryAddr(){
          return dev_primary_addr;
        }
        virtual inline uint64_t getTxFid(){
          return tx_fid;
        }
        virtual inline uint64_t getRxFid(){
          return rx_fid;
        }
        virtual inline std::string getDevType(){
          return dev_type;
        }
        virtual inline bool isPrimary(){
          if (dev_addr == dev_primary_addr){
            return true;
          }
          else
            return false;
        }
        /*
          Mutator Functions
        */
        virtual inline void setVersion(uint8_t arg_version){
          version = arg_version;
        }
        virtual inline void setI2CAddr(uint16_t arg_i2c_addr){
          i2c_addr = arg_i2c_addr;
        }
        virtual inline void setDevAddr(uint16_t arg_dev_addr){
          dev_addr = arg_dev_addr;
        }
        virtual inline void setPrimaryAddr(uint16_t arg_dev_primary_addr){
          dev_primary_addr = arg_dev_primary_addr;
        }
        virtual inline void setTxFid(uint64_t arg_tx_fid){
          tx_fid = arg_tx_fid;
        }

        virtual inline void setRxFid(uint64_t arg_rx_fid){
          rx_fid = arg_rx_fid;
        }

        virtual inline void setDevType(std::string arg_dev_type){
          dev_type = arg_dev_type;
        }

      private:
        uint8_t version;
        uint16_t i2c_addr;
        uint16_t dev_addr;
        uint16_t dev_primary_addr;
        uint64_t tx_fid;
        uint64_t rx_fid;
        std::string dev_type;
    };

private:
  std::shared_ptr<FelixClientThread> client;
  std::vector<std::unique_ptr<OptoDevice>> m_opto_dev_list;

  // Felix client callbacks
  void on_init() {}

  void on_connect(uint64_t fid) {
    FelixRxCore::on_connect(fid);
  }

  void on_disconnect(uint64_t fid) {
    FelixRxCore::on_disconnect(fid);
  }

  void on_data(uint64_t fid, const uint8_t* data, size_t size, uint8_t status) {
    FelixRxCore::on_data(fid, data, size, status);
  }

  /*
  E-Link control utilities
  */
  /// Update the value of a FELIX register in a register value map
  void updateRegMap(std::map<std::string, unsigned>& regMap, const std::string& regName, unsigned value, bool overwrite);

  /// Read a FELIX register "regName" and check if its value equals to "value". If "mask" is nonzero, only compare the bits that are masked by "mask".
  bool checkRegValue(const std::string& regName, unsigned value, unsigned mask=0);

  /// Read all FELIX registers in the "regValueMap" and check if they all equal to the values stored in the map. If there are nonzero masks provided, only compare the bits that are masked.
  bool checkRegValuesAll(const std::map<std::string, unsigned>& regValueMap, const std::map<std::string, unsigned>& regMaskMap);
  bool checkRegValuesAll(const std::map<std::string, unsigned>& regValueMap, unsigned mask=0);

  /// Update the value of a FELIX register "regName" to "value". If "mask" is nonzero, only update the bits that are masked by "mask"
  bool setRegValue(const std::string& regName, unsigned value, unsigned mask=0);

  /// Update the values of all FELIX registers in "regValueMap". If nonzero masks are provided, only update the bits that are masked.
  bool setRegValueAll(const std::map<std::string, unsigned>& regValueMap, const std::map<std::string, unsigned>& regMaskMap);
  bool setRegValueAll(const std::map<std::string, unsigned>& regValueMap, unsigned mask=0);

  /// Initiate a map with all IC enable register names on a FELIX device and set their value to 0
  void initAllICEnableRegMap(std::map<std::string, unsigned>& regMap, bool toflx, bool tohost);

  /// Initiate a map with all EC enable register names on a FELIX device and set their value to 0
  void initAllECEnableRegMap(std::map<std::string, unsigned>& regMap, bool toflx, bool tohost);

  /// Initiate a map with all elink enable register names on a FELIX device and set their value to 0
  void initAllELinkEnableRegMap(std::map<std::string, unsigned>& regMap, bool toflx, bool tohost);

  /*
  Optoboard device communication
  */

  /// @brief Checks if a member of the OptoDevice class exists in the m_opto_dev_list
  /// @param rx_ic_fid The fid of the ic channel for rx (uint64_t)
  /// @param dev_addr Address of the device (uint16_t)
  /// @return True if in list, false if not (bool)
  bool optoDeviceInList(uint64_t rx_ic_fid, uint16_t dev_addr);

  /// @brief Checks if a member of the OptoDevice class exists in the m_opto_dev_list
  /// @param rx_ic_fid The fid of the ic channel for rx (uint64_t)
  /// @param tx_ic_fid The fid of the ic channel for tx (uint64_t)
  /// @param type The type of the device (lpgbt or gbcr) (std::string)
  /// @return True if in list, false if not (bool)
  bool optoDeviceInList(uint64_t rx_ic_fid, uint64_t tx_ic_fid, std::string type);

  /// @brief Returns a pointer to a member of the OptoDevice class found in the m_opto_dev_list
  /// @param rx_ic_fid The fid of the ic channel for rx (uint64_t)
  /// @param dev_addr Address of the device (uint16_t)
  /// @return Raw pointer to optical device object (OptoDevice* )
  OptoDevice* getOptoDeviceInList(uint64_t rx_ic_fid, uint16_t dev_addr);

  /// @brief Returns a pointer to a member of the OptoDevice class found in the m_opto_dev_list
  /// @param rx_ic_fid The fid of the ic channel for rx (uint64_t)
  /// @param tx_ic_fid The fid of the ic channel for tx (uint64_t)
  /// @param type The type of the device (lpgbt or gbcr) (std::string)
  /// @return Raw pointer to optical device object (OptoDevice* )
  OptoDevice* getOptoDeviceInList(uint64_t rx_ic_fid, uint32_t tx_ic_fid, std::string type);

  /// @brief Defines and returns a pointer to a new member of the OptoDevice class, adds object to m_opto_dev_list variable
  /// @param dev_addr Address of the device (uint16_t)
  /// @param type Type of device: lpgbt or gbcr accepted (std::string)
  /// @param rx_ic_fid The fid of the ic channel for rx (uint64_t)
  /// @param tx_ic_fid The fid of the ic channel for tx (uint64_t)
  /// @return Raw pointer to new optical device object (OptoDevice*)
  OptoDevice* newDefaultOptoDevice(uint16_t dev_addr, std::string type, uint64_t rx_ic_fid, uint64_t tx_ic_fid);

  /// @brief Generic function to handle read and write operations to either LpGBT or GBCR devices
  /// @param reg The register item (lpgbt_item_t*)
  /// @param data The data we want to send or receive (uint8_t&)
  /// @param write True if we want to write, false if we want to read (bool)
  /// @param lpgbt OptoDevice (OptoDevice*)
  void communicateLpGBT(const lpgbt_item_t* reg, uint8_t& data, const bool write, OptoDevice* lpgbt);

  /// @brief Handles reads/writes of LpGBTs or GBCRs
  /// @param reg The register item (lpgbt_item_t*)
  /// @param reg_data The data we want to write or read back (uint8_t&)
  /// @param write True if we want to write, false if we want to read (bool)
  /// @param lpgbt Opto device (OptoDevice*)
  void readWriteOptoReg(const lpgbt_item_t* reg, uint8_t& reg_data, bool write, OptoDevice* lpgbt);
  
};


/// namespace and then static const variables for the different registers 

#endif
