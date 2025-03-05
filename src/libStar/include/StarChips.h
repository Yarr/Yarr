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
  void setAndWriteHCCSubRegister(std::string subRegName, uint32_t value){
    m_hcc.setSubRegisterValue(subRegName, value);
    sendCmd( write_hcc_register(hcc().getSubRegisterParentAddr(subRegName),
                                hcc().getSubRegisterParentValue(subRegName),
                                getHCCchipID()) );
  }

  /// Send command to read named HCC register field
  void readHCCSubRegister(std::string subRegName){
    sendCmd(read_hcc_register(hcc().getSubRegisterParentAddr(subRegName),
                              getHCCchipID()));
  }

  /**
     Set ABC register field and write to front end.

     @param subRegName Name of register field.
     @param value Value to write to field.
     @param chipID Communications ID of ABC to write to.
  */
  void setAndWriteABCSubRegister(std::string subRegName, uint32_t value, int32_t chipID){
                if (chipID != 15) { //User specified a chipID, no broadcast
    setAndWriteABCSubRegister(subRegName,
                              abcFromChipID(chipID), value);
                }
                else {  //User wants to broadcast, but we want to set the cfg. Iterate through ABCs.
                        eachAbc([&] (auto &abc)->void{
                                        setAndWriteABCSubRegister(subRegName, abc, value); });
                }
  }

  /// Reads value of subregister subRegName for chip with ID chipID
  void readABCSubRegister(std::string subRegName, int32_t chipID){
    readABCSubRegister(subRegName, abcFromChipID(chipID));
  }

 private:
  void setAndWriteABCSubRegister(std::string subRegName, AbcCfg &cfg, uint32_t value) {
    cfg.setSubRegisterValue(subRegName, value);
    sendCmd( write_abc_register(cfg.getSubRegisterParentAddr(subRegName),
                                cfg.getSubRegisterParentValue(subRegName),
                                getHCCchipID(), cfg.getABCchipID()) );
  }

  void readABCSubRegister(std::string subRegName, AbcCfg &cfg) {
    sendCmd(read_abc_register(cfg.getSubRegisterParentAddr(subRegName),
                              getHCCchipID(), cfg.getABCchipID()));
  }

  void writeABCRegister(int addr, AbcCfg &cfg);

    TxCore * m_txcore;
};

#endif
