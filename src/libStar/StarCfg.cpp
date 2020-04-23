// #################################
// # Project:
// # Description: Star Library
// # Comment: Star Config class
// ################################

#include "StarCfg.h"

#include <iomanip>

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

int StarCfg::hccChannelForABCchipID(unsigned int chipID) {
  auto itr = std::find_if(m_ABCchips.begin(), m_ABCchips.end(),
                        [this, chipID](auto &it) { return it.getABCchipID() == chipID; });
  return std::distance(m_ABCchips.begin(), itr);
}

//HCC register accessor functions
const uint32_t StarCfg::getHCCRegister(HCCStarRegister addr){
  return m_hcc.getRegisterValue(addr);
}
void StarCfg::setHCCRegister(HCCStarRegister addr, uint32_t val){
  m_hcc.setRegisterValue(addr, val);
}

//ABC register accessor functions, converts chipID into chip index
const uint32_t StarCfg::getABCRegister(ABCStarRegister addr, int32_t chipID){
  auto &abc = abcFromChipID(chipID);
  return abc.getRegisterValue(addr);
}
void StarCfg::setABCRegister(ABCStarRegister addr, uint32_t val, int32_t chipID){
  auto &abc = abcFromChipID(chipID);
  abc.setRegisterValue(addr, val);
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

    j["name"] = name;

    j["HCC"]["ID"] = getHCCchipID();

    auto &hccRegs = HccStarRegInfo::instance()->hccregisterMap;

    for(auto &reg: hccRegs) {
        auto &info = reg.second;
        int addr = info->addr();
        // Standard rw registers start from 32
        // Don't write status registers
        if(addr >= 32) {
          auto reg = HCCStarRegister::_from_integral(addr);
          std::string regKey = reg._to_string();
          uint32_t val = getHCCRegister(addr);
          std::stringstream ss;
          ss << std::hex << std::setw(8) << std::setfill('0') << val;
          j["HCC"]["regs"][regKey] = ss.str();
        }
    }

    auto &abcRegs = AbcStarRegInfo::instance()->abcregisterMap;

    std::map<std::string, std::string> common;
    // Store until we know which are not common
    std::vector<std::map<std::string, std::string>> regs(numABCs());

    for (int iABC = 0; iABC < numABCs(); iABC++) {
        auto &abc = abcFromIndex(iABC+1);
        j["ABCs"]["IDs"][iABC] = abc.getABCchipID();

        for(auto &reg_i: abcRegs) {
            auto &info = reg_i.second;
            int addr = info->addr();

            // Skip non-writeable, trim and mask registers
            if(addr==ABCStarRegister::SCReg) {
                continue;
            }
            if(addr>=ABCStarRegister::MaskInput(0) && addr<=ABCStarRegister::MaskInput(7)) {
                continue;
            }
            if(addr>=ABCStarRegister::CalREG0 && addr<=ABCStarRegister::CalREG7) {
                continue;
            }
            if(addr>=ABCStarRegister::STAT0 && addr<=ABCStarRegister::HPR) {
                continue;
            }
            if(addr>=ABCStarRegister::TrimLo(0) && addr<=ABCStarRegister::TrimHi(7)) {
                continue;
            }
            if(addr>=ABCStarRegister::HitCountREG0) {
                continue;
            }

            auto reg = ABCStarRegister::_from_integral(addr);
            uint32_t val = abc.getRegisterValue(reg);
            std::stringstream ss;
            ss << std::hex << std::setw(8) << std::setfill('0') << val;
            std::string regKey = reg._to_string();
            std::string regValue = ss.str();
            regs[iABC][regKey] = regValue;

            if(iABC == 0) {
                common[regKey] = regValue;
            } else {
                auto i = common.find(regKey);
                if(i != common.end() && i->second != regValue) {
                    // Not the same as others
                    common.erase(i);
                }
            }
        }

        std::array<uint8_t, 256> trims;
        bool sameTrims = true;
        for(int m=0; m<256; m++) {
            trims[m] = abc.getTrimDACRaw(m);
            if(m!=0 && (trims[m] != trims[m-1])) sameTrims = false;
            if(!abc.isMasked(m)) {
                continue;
            }

            j["ABCs"]["masked"][iABC].push_back(m);
        }
        if(sameTrims) {
            j["ABCs"]["trims"][iABC] = trims[0];
        } else {
            for(int m=0; m<256; m++) {
                j["ABCs"]["trims"][iABC][m] = trims[m];
            }
        }
    }

    for(size_t a=0; a<regs.size(); a++) {
        for(auto r: regs[a]) {
            if(common.find(r.first) != common.end()) {
                continue;
            }
            j["ABCs"]["regs"][a][r.first] = r.second;
        }
    }
    for(auto m: common) {
        j["ABCs"]["common"][m.first] = m.second;
    }
}

// No hex in json, so interpret a string
uint32_t valFromJson(const json &jValue) {
    if(jValue.is_string()) {
        // Interpret as hex string
        std::string regHex = jValue;
        std::size_t pos;
        try {
          uint32_t ret = std::stoul(regHex, &pos, 16);
          if(pos != regHex.size()) {
            logger->warn("Failed to read hex string {} (reached pos {})", regHex, pos);
            std::string msg = "Failed to read hex string (";
            msg += regHex + ")";
            throw std::runtime_error(msg);
          }
          return ret;
        } catch(std::invalid_argument &e) {
            logger->warn("Failed to parse hex string {}", regHex);
            std::string msg = "Failed to parse hex string (";
            msg += regHex + ")";
            throw std::runtime_error(msg);
        } catch(std::out_of_range &e) {
            logger->warn("Failed to read hex string {} (too big)", regHex);
            std::string msg = "Failed to read big hex string (";
            msg += regHex + ")";
            throw std::runtime_error(msg);
        }
    } else {
        // Interpret directly as integer (decimal in json)
        return jValue;
    }
}
 
void StarCfg::fromFileJson(json &j) {
    logger->debug("Read StarCfg from json");

    if (!j["name"].empty()) {
        name = j["name"];
    }

    if (j.find("HCC") == j.end()) {
        logger->error("No HCC config found in the config file!");
        throw std::runtime_error("Missing HCC in config file");
    }

    auto &hcc = j["HCC"];

    if (!hcc["ID"].empty()) {
        setHCCChipId(hcc["ID"]);
    } else {
        logger->error("No HCC ID found in the config file!");
        throw std::runtime_error("Missing ID in config file");
    }

    m_hcc.setDefaults();

    if (!hcc["regs"].empty()) {
        auto &regs = hcc["regs"];

        if(!regs.is_object()) {
          logger->error("HCC/regs is not an object!");
          throw std::runtime_error("HCC/regs should be an object");
        }

        // Iterate over object reg name: (hex strings|integer)
        auto b = regs.begin();
        auto e = regs.end();
        for (auto i=b; i!=e; i++) {
            std::string regName = i.key();
            auto &jregValue = i.value();

            uint32_t regValue = valFromJson(jregValue);

            logger->trace("Read HCC value {}", regValue);

            try {
                auto addr = HCCStarRegister::_from_string(regName.c_str());
                logger->trace("Set HCC value {} {}", addr, regValue);
                m_hcc.setRegisterValue(addr, regValue);
                auto value = m_hcc.getRegisterValue(addr);
                logger->trace("From JSON: Set HCC {} reg {} to {:08x}",
                              getHCCchipID(), regName, regValue);
            } catch(std::runtime_error &e) {
                logger->warn("Reg {} in JSON file does not exist as an HCC register.  It will be ignored!", regName);
            }
        }
    }

    // Clear list in case loading twice
    clearABCchipIDs();

    // Using find to avoid changing input
    if (j.find("ABCs") == j.end()) {
        logger->warn("No ABC chips found in the config file!");
        return;
    }

    auto &abcs = j["ABCs"];

    // Load the IDs
    if (!abcs["IDs"].empty()) {
        auto &ids = abcs["IDs"];

        for (int iABC = 0; iABC < ids.size(); iABC++) {
            addABCchipID(ids[iABC]);
        }
    }

    auto abc_count = numABCs();

    if( abc_count == 0 ){
        logger->warn("No ABC chipIDs were found in json file, continuing with HCC only");
        return; //No ABCs to load
    }

    // Initialize register maps for consistency
    for( int iABC = 0; iABC < abc_count; ++iABC) {
        // Make all registers and subregisters for the ABC
        abcFromIndex(iABC+1).setDefaults();
    }

    // First, commont register settings
    if(abcs.find("common") != abcs.end()) {
        auto &commonRegs = abcs["common"];

        if(!commonRegs.is_object()) {
            logger->error("common reg item not an object");
            return;
        }

        auto b = commonRegs.begin();
        auto e = commonRegs.end();
        for(auto i = b; i != e; i++) {
            std::string regName = i.key();
            uint32_t regValue = valFromJson(i.value());

            try {
                auto addr = ABCStarRegister::_from_string(regName.c_str());
                for (int iABC = 0; iABC < numABCs(); iABC++) {
                    auto &abc = abcFromIndex(iABC+1);
                    abc.setRegisterValue(addr, regValue);
                }
                logger->trace("All ABCs reg {} has been set to {:08x}", regName, regValue);
            } catch(std::runtime_error &e) {
                logger->warn("Reg {} in JSON file does not exist as an ABC register.  It will be ignored!", regName);
            }
        }
    }

    // First, read register settings
    if(abcs.find("regs") != abcs.end()) {
        auto &regArray = abcs["regs"];

        if(regArray.size() != numABCs()) {
            logger->error("ABCs/regs array size does not match number of ABCs");
            return;
        }

        for (int iABC = 0; iABC < numABCs(); iABC++) {
            auto &chipRegs = regArray[iABC];

            if(chipRegs.is_null()) continue;

            if(!chipRegs.is_object()) {
                logger->error("ABCs/regs array item not null or an object");
                return;
            }

            auto &abc = abcFromIndex(iABC+1);

            auto b = chipRegs.begin();
            auto e = chipRegs.end();
            for(auto i = b; i != e; i++) {
                std::string regName = i.key();
                uint32_t regValue = valFromJson(i.value());

                try {
                    auto addr = ABCStarRegister::_from_string(regName.c_str());
                    abc.setRegisterValue(addr, regValue);
                    logger->trace("For ABC index {}, reg {} has been set to {:08x}", iABC, regName, regValue);
                } catch(std::runtime_error &e) {
                  logger->warn("Reg {} in JSON file does not exist as an ABC register.  It will be ignored!", regName);
                }
            }
        } // Loop over ABCs
    }

    // Possible override by setting sub registers
    if(abcs.find("subregs") != abcs.end()) {
        auto &subregArray = abcs["subregs"];

        if(subregArray.size() != numABCs()) {
            logger->error("ABCs/subregs array size does not match number of ABCs");
            return;
        }

        auto abcSubRegs = AbcStarRegInfo::instance()->abcSubRegisterMap_all;

        for (int iABC = 0; iABC < numABCs(); iABC++) {
            auto &chipSubRegs = subregArray[iABC];

            if(chipSubRegs.is_null()) continue;

            if(!chipSubRegs.is_object()) {
                logger->error("ABCs/subregs array item not null or an object");
                return;
            }

            auto &abc = abcFromIndex(iABC+1);

            auto b = chipSubRegs.begin();
            auto e = chipSubRegs.end();
            for(auto i = b; i != e; i++) {
                std::string subRegName = i.key();
                uint32_t subRegValue = valFromJson(i.value());

                auto regPre = abc.getSubRegisterParentValue(subRegName);
                abc.setSubRegisterValue(subRegName, subRegValue);
                auto retrieved = abc.getSubRegisterValue(subRegName);
                auto regPost = abc.getSubRegisterParentValue(subRegName);
                logger->trace("Load from JSON: For ABC index {}, {} has been set to {} (check {}) {:08x} -> {:08x}", iABC, subRegName, subRegValue, retrieved, regPre, regPost);
            }
        } // Loop over ABCs
    }

    if(abcs.find("masked") != abcs.end()) {
        auto &maskArray = abcs["masked"];

        if(maskArray.size() != numABCs()) {
            logger->error("ABCs/masked array size does not match number of ABCs");
            return;
        }

        // Each chip has a list of strips
        for (int iABC = 0; iABC < numABCs(); iABC++) {
            auto &maskedStrips = maskArray[iABC];

            for(int strip: maskedStrips) {
                auto &abc = abcFromIndex(iABC+1);
                abc.setMask(strip, true);
            }
        }
    }

    if(abcs.find("trims") != abcs.end()) {
        auto &trimArray = abcs["trims"];

        if(trimArray.size() != numABCs()) {
            logger->error("ABCs/trims array size does not match number of ABCs");
            return;
        }

        // Each chip has either single integer (all the same), or array of value per strip
        for (int iABC = 0; iABC < numABCs(); iABC++) {
            auto &abc = abcFromIndex(iABC+1);

            auto &chipValue = trimArray[iABC];
            if(chipValue.is_number()) {
                int trim = chipValue;
                for(int m=0; m<256; m++) {
                    abc.setTrimDACRaw(m, trim);
                }
            } else {
                // Not the same
                for(int m=0; m<256; m++) {
                  int trim = chipValue[m];
                    abc.setTrimDACRaw(m, trim);
                }
            }
        }
    }
}
