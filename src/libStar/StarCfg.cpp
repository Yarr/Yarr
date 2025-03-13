// #################################
// # Project:
// # Description: Star Library
// # Comment: Star Config class
// ################################

#include "StarCfg.h"
#include "StarPreset.h"

#include <iomanip>

#include "logging.h"

namespace {
  auto logger = logging::make_log("StarCfg");
}

StarCfg::StarCfg(int abc_version, int hcc_version)
  : m_abc_info(AbcStarRegInfo::instance(abc_version)),
    m_hcc_info(HccStarRegInfo::instance(hcc_version)),
    m_abc_version(abc_version),
    m_hcc_version(hcc_version),
    m_hcc(hcc_version),
    m_ABCchips{}
{}

StarCfg::~StarCfg() = default;

double StarCfg::toCharge(double vcal) {
    // Q = C*V
    // Linear is good enough
//    double V = (m_vcalPar[0]*Unit::Milli + m_vcalPar[1]*vcal*Unit::Milli)/Physics::ElectronCharge;
//    return V*m_injCap*Unit::Femto;
    return vcal;
}

double StarCfg::toCharge(double vcal, bool sCap, bool lCap) { return toCharge(vcal); }

void StarCfg::enableAll() {
    eachAbc([&](auto &abc) {
        for(int m=0; m<8; m++) {
          abc.setRegisterValue(ABCStarRegister::MaskInput(m), 0);
        }
      });
}

int StarCfg::hccChannelForABCchipID(unsigned int chipID) const {
  auto itr = std::find_if(m_ABCchips.begin(), m_ABCchips.end(),
                        [this, chipID](auto &it) { return it.second.getABCchipID() == chipID; });
  return itr->first;
  //return std::distance(m_ABCchips.begin(), itr);
}

//HCC register accessor functions
uint32_t StarCfg::getHCCRegister(HCCStarRegister addr) const {
  return m_hcc.getRegisterValue(addr);
}
void StarCfg::setHCCRegister(HCCStarRegister addr, uint32_t val){
  m_hcc.setRegisterValue(addr, val);
}

//ABC register accessor functions, converts chipID into chip index
uint32_t StarCfg::getABCRegister(ABCStarRegister addr, int32_t chipID){
  auto &abc = abcFromChipID(chipID);
  return abc.getRegisterValue(addr);
}
void StarCfg::setABCRegister(ABCStarRegister addr, uint32_t val, int32_t chipID){
  auto &abc = abcFromChipID(chipID);
  abc.setRegisterValue(addr, val);
}

int StarCfg::inputChannelForHistoChip(int histo_abc) const
{
    auto chip_map = hcc().histoChipMap();
    for(int i=0; i<chip_map.size(); i++) {
        if(chip_map[i] == histo_abc) {
            return i;
        }
    }
    return -1;
}

bool StarCfg::isAbcForHistoChip(int histo_chip) const
{
    auto ic = inputChannelForHistoChip(histo_chip);
    return ic != -1;
}

AbcCfg &StarCfg::abcForHistoChip(int histo_chip)
{
    auto ic = inputChannelForHistoChip(histo_chip);
    return abcForInputChannel(ic);
}

const AbcCfg &StarCfg::abcForHistoChip(int histo_chip) const
{
    auto ic = inputChannelForHistoChip(histo_chip);
    return abcForInputChannel(ic);
}

void StarCfg::logMappings() const
{
    auto chip_map = hcc().histoChipMap();
    for(int i=0; i<chip_map.size(); i++) {
        logger->trace("IC map: {} {}", i, chip_map[i]);
    }
}

void StarCfg::writeConfig(json &j) {
    logger->debug("Send StarCfg to json");

    j["name"] = name;

    j["HCC"]["ID"] = getHCCchipID();

    auto &hccRegs = m_hcc_info->hccregisterMap;

    for(auto &reg: hccRegs) {
        auto &info = reg.second;
        int addr = info->addr();
        // Standard rw registers start from 32
        // Don't write status registers
        if(addr >= 32) {
          auto reg = HCCStarRegister::_from_integral(addr);
          std::string regKey = reg._to_string();
          uint32_t val = getHCCRegister(reg);
          std::stringstream ss;
          ss << std::hex << std::setw(8) << std::setfill('0') << val;
          j["HCC"]["regs"][regKey] = ss.str();
        }
    }

    auto &abcRegs = m_abc_info->abcregisterMap;

    std::map<std::string, std::string> common;
    // Store until we know which are not common
    std::vector<std::map<std::string, std::string>> regs(highestABC()+1);

    auto chip_map = m_hcc.histoChipMap();

    for (int iABC = 0; iABC <= highestABC(); iABC++) {
        if (!isAbcForInputChannel(iABC))
            continue;
        auto &abc = abcForInputChannel(iABC);
        int histo_index = chip_map[iABC];
        if(histo_index == HCC_INPUT_CHANNEL_BAD_SLOT) {
          continue;
        }
        j["ABCs"]["IDs"][histo_index] = abc.getABCchipID();

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
            regs[histo_index][regKey] = regValue;

            if(iABC == lowestABC()) {
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

            j["ABCs"]["masked"][histo_index].push_back(m);
        }
        if(sameTrims) {
            j["ABCs"]["trims"][histo_index] = trims[0];
        } else {
            for(int m=0; m<256; m++) {
                j["ABCs"]["trims"][histo_index][m] = trims[m];
            }
        }
    }

    for(size_t a=0; a<regs.size(); a++) {
        for(auto &r: regs[a]) {
            if(common.find(r.first) != common.end()) {
                continue;
            }
            j["ABCs"]["regs"][a][r.first] = r.second;
        }
    }
    for(auto &m: common) {
        j["ABCs"]["common"][m.first] = m.second;
    }

    auto &cfg_ct = j["ABCs"]["Parameters"] = json::object();
    m_ct.writeConfig(cfg_ct);
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
 
void StarCfg::loadConfig(const json &j) {
    logger->debug("Read StarCfg from json");

    if (j.contains("name")) {
        name = j["name"];
    }

    if (j.find("HCC") == j.end()) {
        logger->error("No HCC config found in the config file!");
        throw std::runtime_error("Missing HCC in config file");
    }

    auto &hcc = j["HCC"];

    if (hcc.contains("ID")) {
        setHCCChipId(hcc["ID"]);
    } else {
        logger->error("No HCC ID found in the config file!");
        throw std::runtime_error("Missing ID in config file");
    }

    if (hcc.contains("fuse_id")) {
      std::string str_fuse = hcc["fuse_id"];
      m_fuse_id = std::stol(str_fuse, nullptr, 16);
      logger->info("Reading configuration for chip with serial number=0x{:05x}", m_fuse_id);
    }

    m_hcc.setDefaults(m_hcc_version);

    if (hcc.contains("regs")) {
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
                logger->trace("From JSON: Set HCC {} reg {} to {:08x} check {:08x}",
                              getHCCchipID(), regName, regValue, value);
            } catch(std::runtime_error &e) {
                logger->warn("Reg {} in JSON file does not exist as an HCC register.  It will be ignored!", regName);
            }
        }
    }

    // Possible override by setting sub registers
    if(hcc.find("subregs") != hcc.end()) {
        auto &subregHCC = hcc["subregs"];

        if(!subregHCC.is_object()) {
          logger->error("HCC/subregs is not an object!");
          throw std::runtime_error("HCC/subregs should be an object");
        }

        auto b = subregHCC.begin();
        auto e = subregHCC.end();
        for(auto i = b; i != e; i++) {
            std::string subRegName = i.key();
            uint32_t subRegValue = valFromJson(i.value());

            auto regPre = m_hcc.getSubRegisterParentValue(subRegName);
            m_hcc.setSubRegisterValue(subRegName, subRegValue);
            auto retrieved = m_hcc.getSubRegisterValue(subRegName);
            auto regPost = m_hcc.getSubRegisterParentValue(subRegName);
            logger->trace("Load from JSON: For HCC, {} has been set to {} (check {}) {:08x} -> {:08x}", subRegName, subRegValue, retrieved, regPre, regPost);
        } 
    }

    // Map from input channels to histo location
    auto chip_map = m_hcc.histoChipMap();

    // Count enabled chips for later consistency check
    int enables_count = 0;
    for(auto &i: chip_map) {
      if(i==HCC_INPUT_CHANNEL_BAD_SLOT)
        continue;
      enables_count ++;
    }

    // Clear list in case loading twice
    clearABCchipIDs();

    // Using find to avoid changing input
    if (j.find("ABCs") == j.end()) {
        logger->warn("No ABC chips found in the config file!");
        return;
    }

    auto &abcs = j["ABCs"];

    unsigned abc_arr_length = 0;

    // Load the IDs (presented in histogram order)
    if (abcs.contains("IDs")) {
        auto &ids = abcs["IDs"];
        abc_arr_length = ids.size();
        for (int iABC = 0; iABC < ids.size(); iABC++) {
            auto &id = ids[iABC];
            if (id.is_null())
                continue;

            int ic_abc = -1;
            for(int i=0; i<chip_map.size(); i++) {
              if(chip_map[i] == iABC) {
                ic_abc = i;
              }
            }
            if(ic_abc == -1) {
              logger->warn("While loading no mapping for ID {} found in HCC map at {}", (int)id, iABC);
              continue;
            }

            addABCchipID(id, ic_abc);
        }
    }

    if (abcs.contains("fuse_ids")) {
        auto &ids = abcs["fuse_ids"];
        abc_arr_length = ids.size();
        for (int iABC = 0; iABC < ids.size(); iABC++) {
            auto &id = ids[iABC];
            if (id.is_null())
                continue;
            std::string str_fuse = id;
            uint32_t abc_fuse_id = std::stol(str_fuse, nullptr, 16);
            logger->info("Reading configuration for ABC chip with serial number=0x{:05x}", abc_fuse_id);

            // TODO: Use for checking against expectation
        }
    }

    auto abc_count = numABCs();

    if( abc_count == 0 ){
        logger->warn("No ABC chipIDs were found in json file, continuing with HCC only");
        return; //No ABCs to load
    }

    if(abc_arr_length != enables_count) {
      logger->warn("While loading, count from IDs {} doesn't match IC enables in HCC {}", abc_arr_length, enables_count);
    }

    //We need to null check these later. If it's empty, we already returned.
    auto &ids = abcs["IDs"];

    // Initialize register maps for consistency
    // Make all registers and subregisters for the ABC
    eachAbc( [&](auto &abc) {abc.setDefaults(m_abc_version);});

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
                for (int iABC = 0; iABC <= highestABC(); iABC++) {
                    if (isAbcForInputChannel(iABC))  {
                        // Doesn't need remapping as common
                        auto &abc = abcFromIndex(iABC+1);
                        abc.setRegisterValue(addr, regValue);
                    }
                }
                logger->trace("All ABCs reg {} has been set to {:08x}", regName, regValue);
            } catch(std::runtime_error &e) {
                logger->warn("Reg {} in JSON file does not exist as an ABC register.  It will be ignored!", regName);
            } catch(std::out_of_range &e) {
                logger->warn("Reg {} in JSON file is not valid ABC register (version {}).  It will be ignored!", regName, m_abc_version);
            }
        }
    }

    // First, read register settings
    if(abcs.find("regs") != abcs.end()) {
        auto &regArray = abcs["regs"];

        if(regArray.size() != abc_arr_length) {
            logger->error("ABCs/regs array size does not match number of ABCs");
            return;
        }

        for (size_t iABC = 0; iABC < abc_arr_length; iABC++) {
            if (ids[iABC].is_null())
                continue;

            auto &chipRegs = regArray[iABC];

            if(chipRegs.is_null()) continue;

            if(!chipRegs.is_object()) {
                logger->error("ABCs/regs array item not null or an object");
                return;
            }

            int ic_abc = -1;
            for(int i=0; i<chip_map.size(); i++) {
              if(chip_map[i] == iABC) {
                ic_abc = i;
              }
            }
            auto &abc = abcForInputChannel(ic_abc);

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

        if(subregArray.size() != abc_arr_length) {
            logger->error("ABCs/subregs array size does not match number of ABCs");
            return;
        }

        for (size_t iABC = 0; iABC < abc_arr_length; iABC++) {
            if (ids[iABC].is_null())
                continue;

            auto &chipSubRegs = subregArray[iABC];

            if(chipSubRegs.is_null()) continue;

            if(!chipSubRegs.is_object()) {
                logger->error("ABCs/subregs array item not null or an object");
                return;
            }

            int ic_abc = -1;
            for(int i=0; i<chip_map.size(); i++) {
              if(chip_map[i] == iABC) {
                ic_abc = i;
              }
            }
            auto &abc = abcForInputChannel(ic_abc);

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

        if(maskArray.size() != abc_arr_length) {
            logger->error("ABCs/masked array size does not match number of ABCs");
            return;
        }

        // Each chip has a list of strips
        for (size_t iABC = 0; iABC < abc_arr_length; iABC++) {
            if (ids[iABC].is_null())
                continue;
            auto &maskedStrips = maskArray[iABC];

            int ic_abc = -1;
            for(int i=0; i<chip_map.size(); i++) {
              if(chip_map[i] == iABC) {
                ic_abc = i;
              }
            }

            auto &abc = abcForInputChannel(ic_abc);
            for(int strip: maskedStrips) {
                abc.setMask(strip, true);
            }
        }
    }

    if(abcs.find("trims") != abcs.end()) {
        auto &trimArray = abcs["trims"];

        if(trimArray.size() != abc_arr_length) {
            logger->error("ABCs/trims array size {} does not match number of ABCs {}", trimArray.size(), abc_arr_length);
            return;
        }

        // Each chip has either single integer (all the same), or array of value per strip
        for (size_t iABC = 0; iABC < abc_arr_length; iABC++) {
            if (ids[iABC].is_null())
                continue;

            int ic_abc = -1;
            for(int i=0; i<chip_map.size(); i++) {
              if(chip_map[i] == iABC) {
                ic_abc = i;
              }
            }

            auto &abc = abcForInputChannel(ic_abc);

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

    // Load calibration config for converting BVT and BCAL
    if (abcs.contains("Parameters")) {
        m_ct.loadConfig(abcs["Parameters"]);
    }

    // FrontEnd stores the geometry, so have to update it there
    // Doing it here means it's correct even if configure is not
    // called, eg if analysis runs on it's own.
    auto fe = dynamic_cast<FrontEnd*>(this);
    if(fe) {
      // Make histo size match number of configured ABCs
      fe->geo.nCol = 128 * numABCs();
    }    
}

std::tuple<json, std::vector<json>> StarCfg::getPreset(const std::string& systemType) {
    // Return a json object for connectivity configuration and
    // a vector of json objects for chip configurations
    return StarPreset::createConfigStar(*this, systemType);
}
