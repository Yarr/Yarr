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

class StarChips : public StarCfg, public StarCmd, public FrontEnd {
 public:
  StarChips(int abc_version, int hcc_version);

  ~StarChips() override = default;

  void init(HwController *arg_core, const FrontEndConnectivity& fe_cfg) override;

  //Will write value for setting name for the HCC if name starts with "HCC_" otherwise will write the setting for all ABCs if name starts with "ABCs_"
  void writeNamedRegister(std::string name, uint16_t value) override;

  // Pixel specific?
  void setInjCharge(double, bool, bool) override {}

    //! configure
    //! brief configure the chip (virtual)
    void configure() override;

  void setHccId(unsigned);//Set the HCC ID to the argument, uses the chip serial number set by eFuse

  void makeGlobal() override {}

  std::unique_ptr<FrontEnd> getGlobal() override;

  void resetHCCStars();
  void resetABCStars();
  void resetAllHard() override;
  void sendCmd(std::array<uint16_t, 9> cmd);
  void sendCmd(uint16_t cmd);

  bool writeRegisters();
  bool writeTrims();
  void readRegisters();

  void writeHCCRegister(int addr);

  void writeABCRegister(int addr) {
    eachAbc([&](auto &abc) { writeABCRegister(addr, abc); });
  }

  void readHCCRegister(int addr);

  void readABCRegister(int addr, int32_t chipID);


  void setAndWriteHCCSubRegister(std::string subRegName, uint32_t value){
    m_hcc.setSubRegisterValue(subRegName, value);
    sendCmd( write_hcc_register(hcc().getSubRegisterParentAddr(subRegName),
                                hcc().getSubRegisterParentValue(subRegName),
                                getHCCchipID()) );
  }
  void readHCCSubRegister(std::string subRegName){
    sendCmd(read_hcc_register(hcc().getSubRegisterParentAddr(subRegName),
                              getHCCchipID()));
  }

 public:
  //Uses chip ID to set value on subregister called subRegName
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

  //Reads value of subregister subRegName for chip with ID chipID
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
                              0xf, cfg.getABCchipID()));
  }

  void writeABCRegister(int addr, AbcCfg &cfg);

    TxCore * m_txcore;
};

#endif
