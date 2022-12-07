#ifndef STAR_CHIPS_GLOBAL_HEADER_
#define STAR_CHIPS_GLOBAL_HEADER_

#include "Bookkeeper.h"
#include "StarChips.h"

class StarChipsGlobal : public StarChips {
  public:

    StarChipsGlobal(int abc_version, int hcc_version);

    void resetAll() override;

    void configure() override;

    void enableAll() override;

    void writeNamedRegister(std::string name, uint16_t value) override;

    void connectBookkeeper(Bookkeeper* k) override {keeper = k;}

  private:

    bool isBroadcastable(const std::string& name);

    Bookkeeper* keeper;
};

#endif