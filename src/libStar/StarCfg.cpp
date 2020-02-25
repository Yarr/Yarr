// #################################
// # Project:
// # Description: Star Library
// # Comment: Star Config class
// ################################

#include "StarCfg.h"

#include "logging.h"

namespace {
  auto logger = logging::make_log("StarCfg");
}

StarCfg::StarCfg() {}

StarCfg::~StarCfg() {}

double StarCfg::toCharge(double vcal) {
    // Q = C*V
    // Linear is good enough
//    double V = (m_vcalPar[0]*Unit::Milli + m_vcalPar[1]*vcal*Unit::Milli)/Physics::ElectronCharge;
//    return V*m_injCap*Unit::Femto;
    return vcal;
}

double StarCfg::toCharge(double vcal, bool sCap, bool lCap) { return toCharge(vcal); }

void StarCfg::initRegisterMaps() {
  auto abc_count = m_ABCchips.size();

  //Make all registers and subregisters for the HCC
  m_hcc.configure_HCC_Registers();

  //Now make registers for each ABC
  logger->debug("Now have m_nABC as {}", abc_count);
  for( int iABC = 0; iABC < abc_count; ++iABC){ //Start at 1 b/c zero is HCC!
    //Make all registers and subregisters for the HCC
    abcFromIndex(iABC+1).configure_ABC_Registers();
  }
}

int StarCfg::indexForABCchipID(unsigned int chipID) {
  auto itr = std::find_if(m_ABCchips.begin(), m_ABCchips.end(),
                        [this, chipID](auto it) { return it.getABCchipID() == chipID; });
  return std::distance(m_ABCchips.begin(), itr) + 1;
}

//HCC register accessor functions
const uint32_t StarCfg::getHCCRegister(HCCStarRegister addr){
  return m_hcc.getRegisterValue(addr);
}
const uint32_t StarCfg::getHCCRegister(uint32_t addr){
    return m_hcc.getRegisterValue(HCCStarRegister::_from_integral(addr));
}
void StarCfg::setHCCRegister(HCCStarRegister addr, uint32_t val){
  m_hcc.setRegisterValue(addr, val);
}
void StarCfg::setHCCRegister(uint32_t addr, uint32_t val){
  m_hcc.setRegisterValue(HCCStarRegister::_from_integral(addr), val);
}

//ABC register accessor functions, converts chipID into chip index
const uint32_t StarCfg::getABCRegister(ABCStarRegister addr, int32_t chipID){
  auto &abc = abcFromChipID(chipID);
  return abc.getRegisterValue(addr);
}
const uint32_t StarCfg::getABCRegister(uint32_t addr, int32_t chipID){
  auto &abc = abcFromChipID(chipID);
  return abc.getRegisterValue(ABCStarRegister::_from_integral(addr));
}
void StarCfg::setABCRegister(ABCStarRegister addr, uint32_t val, int32_t chipID){
  auto &abc = abcFromChipID(chipID);
  abc.setRegisterValue(addr, val);
}
void StarCfg::setABCRegister(uint32_t addr, uint32_t val, int32_t chipID){
  auto &abc = abcFromChipID(chipID);
  abc.setRegisterValue(ABCStarRegister::_from_integral(addr), val);
}

void StarCfg::setTrimDAC(unsigned col, unsigned row, int value)  {
    ////NOTE: Each chip is divided in 2 row x 128 col. Histogram bins are adjusted based on number of activated chips. Does not have gap in between rows.
    ////      Let's say, of the 10 ABC in one hybrid, only chip 0, 4 and 6 are activated, the histogram has 6 rows x 128 cols.
    ////      i.e row 1&2 belong to chip_0; row 3&4 belong to chip_4;  row 5&6 belong to chip_6.
    ////      the trimDAC_4lsb_name for each chip is trimdac_4lsb_<nthRow[2:1]>_<nthCol[128:1]>
    ////      the trimDAC_1msb_name for each chip is trimdac_1msb_<nthRow[2:1]>_<nthCol[128:1]>

    ////NOTE: row and col pass from histogram starts from 1, while channel starts from 0

    int nthRow = row%2 ==0 ? 2 : 1;

    int channel=0;
    int chn_tmp = floor((col-1)/2);
    if(nthRow==1) channel = (col-1) + chn_tmp*2;
    else if(nthRow==2) channel = (col-1) + (chn_tmp+1)*2;

    SPDLOG_LOGGER_TRACE(logger,
                        "row:{} col:{} chn_tmp:{} channel:{}",
                        row-1, col-1, chn_tmp, channel);

    unsigned chipIndex = ceil(row/2.0);

    auto &abc = abcFromIndex(chipIndex);

    abc.setTrimDACRaw(channel, value);
}


int StarCfg::getTrimDAC(unsigned col, unsigned row) const {
    int nthRow = row%2 ==0 ? 2 : 1;

    int channel=0;
    int chn_tmp = floor((col-1)/2);
    if(nthRow==1) channel = (col-1) + chn_tmp*2;
    else if(nthRow==2) channel = (col-1) + (chn_tmp+1)*2;

    SPDLOG_LOGGER_TRACE(logger,
                        " row:{} col:{} chn_tmp:{} channel:{}",
                        row-1, col-1, chn_tmp, channel);

    unsigned chipIndex = ceil(row/2.0);

    const auto &abc = abcFromIndex(chipIndex);

    return abc.getTrimDACRaw(channel);
}

void StarCfg::toFileJson(json &j) {
    logger->debug("Send StarCfg to json");
}

void StarCfg::fromFileJson(json &j) {
    logger->debug("Read StarCfg from json");
}
