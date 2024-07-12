#ifndef STAR_CHIPS_BROADCAST_HEADER_
#define STAR_CHIPS_BROADCAST_HEADER_

#include "Bookkeeper.h"
#include "StarChips.h"

class StarChipsBroadcast : public StarChips {
  public:

    StarChipsBroadcast(int abc_version, int hcc_version);

    void resetAllHard() override;

    void configure() override;

    void enableAll() override;

    yarrStatus writeNamedRegister(std::string name, const uint16_t value) override;

    void connectBookkeeper(Bookkeeper* k) override {keeper = k;}

  private:

    bool isBroadcastable(const std::string& name);

    Bookkeeper* keeper;
};

#endif
