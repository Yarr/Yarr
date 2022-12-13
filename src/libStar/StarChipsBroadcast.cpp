#include "StarChipsBroadcast.h"

#include "logging.h"

namespace {
  auto glog = logging::make_log("StarChipsBroadcast");
}

#include "AllChips.h"

bool star_chips_broadcast_default_registered =
StdDict::registerFrontEnd
  ("Star_Broadcast", []() { return std::unique_ptr<FrontEnd>(new StarChipsBroadcast(0, 0)); });
bool star_chips_broadcast_v0_registered =
StdDict::registerFrontEnd
  ("Star_Broadcast_vH0A0", []() { return std::unique_ptr<FrontEnd>(new StarChipsBroadcast(0, 0)); });
// a.k.a PPA
bool star_chips_broadcast_v1_registered =
StdDict::registerFrontEnd
  ("Star_Broadcast_vH0A1", []() { return std::unique_ptr<FrontEnd>(new StarChipsBroadcast(1, 0)); });
// a.k.a PPB
bool star_chips_broadcast_v1_both_registered =
StdDict::registerFrontEnd
  ("Star_Broadcast_vH1A1", []() { return std::unique_ptr<FrontEnd>(new StarChipsBroadcast(1, 1)); });

StarChipsBroadcast::StarChipsBroadcast(int abc_version, int hcc_version) 
  : StarChips(abc_version, hcc_version)
{
  // Dummy configuration for globalFe in preScan routines
  setHCCChipId(0xf);
  addABCchipID(0xf, 0);
}

void StarChipsBroadcast::resetAll() {
  StarChips::resetAll();
  // Move reset functions in StarChips here?
}

void StarChipsBroadcast::configure() {
  // Chip configuration is done in the scanConsole by looping over individual FEs
  glog->error("configure() is called via the global FE");
}

void StarChipsBroadcast::enableAll() {
  glog->error("enableAll() is called via the global FE");
}

void StarChipsBroadcast::writeNamedRegister(std::string name, uint16_t value) {
  if (isBroadcastable(name)) {
    this->writeNamedRegister(name, value);
  } else {
    assert(keeper);
    // Loop over individual front ends and write the register one at a time
    for (unsigned id = 0; id < keeper->getNumOfEntries(); id++) {
      auto& entry = keeper->getEntry(id);
      entry.fe->writeNamedRegister(name, value);
    }
  }
}

/// Check if a request to write a named register can be broadcasted  
bool StarChipsBroadcast::isBroadcastable(const std::string& name) {
  // for now
  return false;
}