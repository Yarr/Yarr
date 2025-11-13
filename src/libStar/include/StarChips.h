#ifndef STAR_CHIPS_HEADER_
#define STAR_CHIPS_HEADER_

// #################################
// # Project:
// # Description: StarChips Library
// # Comment: StarChips FrontEnd class
// ################################

#include <string>

#include "FrontEnd.h"

class TxCore;
class RxCore;

#include "StarCmd.h"
#include "StarCfg.h"

/// FrontEnd class for Star chips
class StarChips : public StarCfg, public StarCmd, public FrontEnd {
 public:
  /**
     Create Star of particular version.

     These are registered to created via StdDict::getDataProcessor with 
     the following names:

     * 'Star' HCC and ABC version 0
     * 'Star_vH0A0' HCC and ABC version 0
     * 'Star_vH0A1' HCC version 0 and ABC version 0 (aka PPA)
     * 'Star_vH1A1' HCC and ABC version 1 (aka PPB)
  */
  StarChips(int abc_version, int hcc_version);

  ~StarChips() override = default;

  void init(HwController *arg_core, const FrontEndConnectivity& fe_cfg) override;

  //Will write value for setting name for the HCC if name starts with "HCC_" otherwise will write the setting for all ABCs if name starts with "ABCs_"
  yarrStatus writeNamedRegister(std::string name, const uint16_t value) override;
  yarrStatus writeNamedRegister(std::string name, const uint16_t value, bool bcr) override;

  // Pixel specific?
  void setInjCharge(double, bool, bool) override {}

    /// Configure the chip.
    void configure() override;

    /**
       Write HCC address register.

       Set the communications ID as specified, using the FUSE ID.
       Called as part of configure if FUSE ID is known.

       @param hccID The required communications ID.
    */
  void setHccId(unsigned hccID);

  /// Give this object the broadcast address
  void makeGlobal() override {}

  /// Retrieve broadcast object
  std::unique_ptr<FrontEnd> getGlobal() override;

  /// Send HCC reset command
  void resetHCCStars();

  /// Send ABC reset command (reg reset + slow reset)
  void resetABCStars();

  /// Reset HCC, set up minimal registers and reset ABCs
  void resetAllHard() override;

  /// Send slow command to associated TxCore
  void sendCmd(std::array<uint16_t, 9> cmd);

  /// Send fast command to associated TxCore
  void sendCmd(uint16_t cmd);

  /// Write all registers (HCC then ABCs)
  bool writeRegisters();

  /// Write all trim registers
  bool writeTrims();

  /// Send commands to read all registers
  void readRegisters();

  /// Send command to write configured value for HCC register
  void writeHCCRegister(int addr);

  /// Send command to write configured value for ABC register
  void writeABCRegister(int addr) {
    eachAbc([&](auto &abc) { writeABCRegister(addr, abc); });
  }

  /**
     Send command to read HCC register.

      @param addr HCC register address.
  */
  void readHCCRegister(int addr);

  /***
      Send command to read ABC register.

      @param addr ABC register address.
      @param chipID ABC communications ID.
  */
  void readABCRegister(int addr, int32_t chipID);

  /// Set HCC register field and write to front end
  void setAndWriteHCCSubRegister(HCCStarSubRegister subRegEnum, uint32_t value){
    m_hcc.setSubRegisterValue(subRegEnum, value);
    sendCmd( write_hcc_register(hcc().getSubRegisterParentAddr(subRegEnum),
                                hcc().getSubRegisterParentValue(subRegEnum),
                                getHCCchipID()) );
  }

  /// Send command to read HCC register field
  void readHCCSubRegister(HCCStarSubRegister subRegEnum){
    sendCmd(read_hcc_register(hcc().getSubRegisterParentAddr(subRegEnum),
                              getHCCchipID()));
  }

  /**
     Set ABC register field and write to front end.

     @param subReg Register field (enum).
     @param value Value to write to field.
     @param chipID Communications ID of ABC to write to (15 for broadcast).
  */
  void setAndWriteABCSubRegister(ABCStarSubRegister subReg, uint32_t value, int32_t chipID){
      if (chipID != 15) {
          //User specified a chipID, no broadcast
          setAndWriteABCSubRegister(subReg,
                                    abcFromChipID(chipID), value);
      } else {
          //User wants to broadcast, but we want to set the cfg. Iterate through ABCs.
          eachAbc([&] (auto &abc)->void
              {
                  setAndWriteABCSubRegister(subReg, abc, value);
              });
      }
  }

  /// Reads value of subregister for chip with ID chipID
  void readABCSubRegister(ABCStarSubRegister subReg, int32_t chipID){
     readABCSubRegister(subReg, abcFromChipID(chipID));
  }

 private:
  void setAndWriteABCSubRegister(ABCStarSubRegister subReg, AbcCfg &cfg, uint32_t value) {
    cfg.setSubRegisterValue(subReg, value);
    sendCmd( write_abc_register(cfg.getSubRegisterParentAddr(subReg),
                                cfg.getSubRegisterParentValue(subReg),
                              getHCCchipID(), cfg.getABCchipID()));
  }

  void readABCSubRegister(ABCStarSubRegister subReg, AbcCfg &cfg) {
    sendCmd(read_abc_register(cfg.getSubRegisterParentAddr(subReg),
                              getHCCchipID(), cfg.getABCchipID()));
  }

  void writeABCRegister(int addr, AbcCfg &cfg);

    TxCore * m_txcore;
};

#endif
