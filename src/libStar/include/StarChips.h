#ifndef STAR_CHIPS_HEADER_
#define STAR_CHIPS_HEADER_

// #################################
// # Project:
// # Description: StarChips Library
// # Comment: StarChips FrontEnd class
// ################################

#include <iostream>
#include <string>

#include "FrontEnd.h"

class TxCore;
class RxCore;

#include "StarCmd.h"
#include "HccStarCfg.h"

class StarChips : public HccStarCfg, public StarCmd, public FrontEnd {
 public:
    StarChips();
    StarChips(TxCore *arg_core);
    StarChips(TxCore *arg_core, unsigned arg_channel);
    StarChips(TxCore *arg_core, unsigned arg_txchannel, unsigned arg_rxchannel);

    ~StarChips() {}

    void init(TxCore *arg_core, unsigned arg_txChannel, unsigned arg_rxChannel);

    void writeNamedRegister(std::string name, uint16_t value) override;

    // Pixel specific?
    void setInjCharge(double, bool, bool) override {}
    void maskPixel(unsigned col, unsigned row) override {}

    //! configure
    //! brief configure the chip (virtual)
    void configure() override;

    void toFileBinary(std::string) override {}
    void fromFileBinary(std::string) override {}

    //! toFileJson
    //! brief write configuration to json (virtual)
    //! param reference to json
    void toFileJson(json&) override;

    //! fromFileJson
    //! brief read configuration from json (virtual)
    //! param reference to json
    void fromFileJson(json&) override;

  private:
    TxCore * m_txcore;
    RxCore * m_rxcore;
    bool m_debug;
};

#endif
