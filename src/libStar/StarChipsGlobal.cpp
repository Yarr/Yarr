#include "StarChipsGlobal.h"

#include "logging.h"

namespace {
  auto glog = logging::make_log("StarChipsGlobal");
}

#include "AllChips.h"

bool star_chips_global_default_registered =
StdDict::registerFrontEnd
  ("Star_Global", []() { return std::unique_ptr<FrontEnd>(new StarChipsGlobal(0, 0)); });
bool star_chips_global_v0_registered =
StdDict::registerFrontEnd
  ("Star_Global_vH0A0", []() { return std::unique_ptr<FrontEnd>(new StarChipsGlobal(0, 0)); });
// a.k.a PPA
bool star_chips_global_v1_registered =
StdDict::registerFrontEnd
  ("Star_Global_vH0A1", []() { return std::unique_ptr<FrontEnd>(new StarChipsGlobal(1, 0)); });
// a.k.a PPB
bool star_chips_global_v1_both_registered =
StdDict::registerFrontEnd
  ("Star_Global_vH1A1", []() { return std::unique_ptr<FrontEnd>(new StarChipsGlobal(1, 1)); });

StarChipsGlobal::StarChipsGlobal(int abc_version, int hcc_version) 
  : StarChips(abc_version, hcc_version)
{
  // Dummy configuration for globalFe in preScan routines
  setHCCChipId(0xf);
	addABCchipID(0xf, 0);
}

void StarChipsGlobal::resetAll() {
  StarChips::resetAll();
  // Move reset functions in StarChips here?
}

void StarChipsGlobal::configure() {
  // Chip configuration is done in the scanConsole by looping over individual FEs
  glog->error("configure() is called via the global FE");
}

void StarChipsGlobal::enableAll() {
  glog->error("enableAll() is called via the global FE");
}

void StarChipsGlobal::writeNamedRegister(std::string name, uint16_t value) {
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
bool StarChipsGlobal::isBroadcastable(const std::string& name) {
  // WIP
  return false;
}