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

private:

  std::shared_ptr<FelixClientThread> client;

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
};

#endif
