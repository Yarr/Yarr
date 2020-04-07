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

#include "logging.h"

class StarChips : public StarCfg, public StarCmd, public FrontEnd {
static logging::LoggerStore tmplogger() {
  static logging::LoggerStore instance = logging::make_log("StarChipsTemp");
  return instance;
}

 public:
  StarChips();
  StarChips(HwController *arg_core);
  StarChips(HwController *arg_core, unsigned arg_channel);
  StarChips(HwController *arg_core, unsigned arg_txchannel, unsigned arg_rxchannel);

  ~StarChips() {}

    void init(HwController *arg_core, unsigned arg_txChannel, unsigned arg_rxChannel) override;

  //Will write value for setting name for the HCC if name starts with "HCC_" otherwise will write the setting for all ABCs if name starts with "ABCs_"
  void writeNamedRegister(std::string name, uint16_t value) override;

  // Pixel specific?
  void setInjCharge(double, bool, bool) override {}
  void maskPixel(unsigned col, unsigned row) override {}

    //! configure
    //! brief configure the chip (virtual)
    void configure() override final;

    //! toFileJson
    //! brief write configuration to json (virtual)
    //! param reference to json
    void toFileJson(json&) override;

    //! fromFileJson
    //! brief read configuration from json (virtual)
    //! param reference to json
    void fromFileJson(json&) override;

  void setHccId(unsigned);//Set the HCC ID to the argument, uses the chip serial number set by eFuse

  void makeGlobal() override final {
      setHccId(15);
  }

  void reset();
  void sendCmd(std::array<uint16_t, 9> cmd);
  void sendCmd(uint16_t cmd);

  bool writeRegisters();
  void readRegisters();

  void setAndWriteHCCRegister(int addr, int64_t  value=-1){
    if(value>=0){
//      std::cout << value << std::endl;
      m_hcc.setRegisterValue(HCCStarRegister::_from_integral(addr), value);
    }
    tmplogger()->debug("Doing HCC setAndWriteRegister with value 0x{:08x} from registerMap[addr={}]", value, addr);
    sendCmd(write_hcc_register(addr, value, getHCCchipID()));
  }
  void setAndWriteABCRegister(int addr, int64_t  value=-1, int32_t chipIndex = 1){
    //unsigned int chipIndex = indexForABCchipID(chipID);
    if(value>=0){
//      std::cout << value << std::endl;
      abcFromIndex(chipIndex).setRegisterValue(ABCStarRegs::_from_integral(addr), value);
    }
    tmplogger()->debug("Doing ABC {} setAndWriteRegister {} with value 0x{:08x}", chipIndex, addr, value);
    sendCmd(write_abc_register(addr, value, getHCCchipID(), getABCchipID(chipIndex)));

  }


  void readHCCRegister(int addr){
    sendCmd(read_hcc_register(addr, getHCCchipID()));
  }
  void readABCRegister(int addr, int32_t chipID = 0){
    sendCmd(read_abc_register(addr, getHCCchipID(), chipID));
  }


  void setAndWriteHCCSubRegister(std::string subRegName, uint32_t value){
    setSubRegisterValue(0, subRegName,value);
    sendCmd( write_hcc_register(getSubRegisterParentAddr(0, subRegName), getSubRegisterParentValue(0, subRegName), getHCCchipID()) );
  }
  void readHCCSubRegister(std::string subRegName){
    sendCmd(read_hcc_register(getSubRegisterParentAddr(0, subRegName), getHCCchipID()));
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

 public:

  //Uses chip index to set value on subregister called subRegName
  void setAndWriteABCSubRegisterForChipIndex(std::string subRegName, uint32_t value, unsigned int chipIndex){
    setAndWriteABCSubRegister(subRegName,
                              abcFromIndex(chipIndex), value);
  }

  //Uses chip ID to set value on subregister called subRegName
  void setAndWriteABCSubRegister(std::string subRegName, uint32_t value, int32_t chipID){
    setAndWriteABCSubRegister(subRegName,
                              abcFromChipID(chipID), value);
  }
  //Reads value of subregister subRegName for chip with index chipIndex
  void readABCSubRegisterForChipIndex(std::string subRegName, unsigned int chipIndex){
    readABCSubRegister(subRegName, abcFromIndex(chipIndex));
  }
  //Reads value of subregister subRegName for chip with ID chipID
  void readABCSubRegister(std::string subRegName, int32_t chipID){
    readABCSubRegister(subRegName, abcFromChipID(chipID));
  }

  private:
    TxCore * m_txcore;
};

#endif
