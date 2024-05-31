#ifndef STAR_PRESET_HEADER
#define STAR_PRESET_HEADER

#include "StarCfg.h"

#include "logging.h"

#include <functional>

// For generating example configurations
namespace StarPreset {
  auto logger = logging::make_log("StarPreset");

  struct HybridInfo {
    std::string name; unsigned hccID; unsigned numABCs;  unsigned offset; unsigned tx; unsigned rx;
  };

  // 14 HCCStars on one side of a long strip stave
  // An example configuration based on the ITSDAQ GLIB setup
  const std::array<HybridInfo, 14> lsstave {{
      {.name="LS0",  .hccID=0,  .numABCs=10, .offset=1, .tx=100, .rx=13 },
      {.name="LS1",  .hccID=1,  .numABCs=10, .offset=1, .tx=100, .rx=12 },
      {.name="LS2",  .hccID=2,  .numABCs=10, .offset=1, .tx=100, .rx=11 },
      {.name="LS3",  .hccID=3,  .numABCs=10, .offset=1, .tx=100, .rx=10 },
      {.name="LS4",  .hccID=4,  .numABCs=10, .offset=1, .tx=101, .rx=9  },
      {.name="LS5",  .hccID=5,  .numABCs=10, .offset=1, .tx=101, .rx=8  },
      {.name="LS6",  .hccID=6,  .numABCs=10, .offset=1, .tx=101, .rx=7  },
      {.name="LS7",  .hccID=7,  .numABCs=10, .offset=1, .tx=101, .rx=6  },
      {.name="LS8",  .hccID=8,  .numABCs=10, .offset=1, .tx=102, .rx=5  },
      {.name="LS9",  .hccID=9,  .numABCs=10, .offset=1, .tx=102, .rx=4  },
      {.name="LS10", .hccID=10, .numABCs=10, .offset=1, .tx=102, .rx=3  },
      {.name="LS11", .hccID=11, .numABCs=10, .offset=1, .tx=102, .rx=2  },
      {.name="LS12", .hccID=12, .numABCs=10, .offset=1, .tx=102, .rx=1  },
      {.name="LS13", .hccID=13, .numABCs=10, .offset=1, .tx=103, .rx=0  }
  }};

  // 28 HCCStars on one side of a short strip stave
  // An example configuration based on the PPB1C in SR1 Barrel System Test
  const std::array<HybridInfo, 28> ssstave {{
      {.name="SS0X",  .hccID=2,  .numABCs=10, .offset=1, .tx=16, .rx=64 },
      {.name="SS0Y",  .hccID=3,  .numABCs=10, .offset=1, .tx=16, .rx=66 },
      {.name="SS1X",  .hccID=4,  .numABCs=10, .offset=1, .tx=16, .rx=68 },
      {.name="SS1Y",  .hccID=5,  .numABCs=10, .offset=1, .tx=16, .rx=72 },
      {.name="SS2X",  .hccID=6,  .numABCs=10, .offset=1, .tx=16, .rx=0  },
      {.name="SS2Y",  .hccID=7,  .numABCs=10, .offset=1, .tx=16, .rx=2  },
      {.name="SS3X",  .hccID=8,  .numABCs=10, .offset=1, .tx=16, .rx=4  },
      {.name="SS3Y",  .hccID=9,  .numABCs=10, .offset=1, .tx=16, .rx=6  },
      {.name="SS4X",  .hccID=2,  .numABCs=10, .offset=1, .tx=6, .rx=88  },
      {.name="SS4Y",  .hccID=3,  .numABCs=10, .offset=1, .tx=6, .rx=90  },
      {.name="SS5X",  .hccID=4,  .numABCs=10, .offset=1, .tx=6, .rx=74  },
      {.name="SS5Y",  .hccID=5,  .numABCs=10, .offset=1, .tx=6, .rx=76  },
      {.name="SS6X",  .hccID=6,  .numABCs=10, .offset=1, .tx=6, .rx=8   },
      {.name="SS6Y",  .hccID=7,  .numABCs=10, .offset=1, .tx=6, .rx=24  },
      {.name="SS7X",  .hccID=8,  .numABCs=10, .offset=1, .tx=6, .rx=10  },
      {.name="SS7Y",  .hccID=9,  .numABCs=10, .offset=1, .tx=6, .rx=12  },
      {.name="SS8X",  .hccID=2,  .numABCs=10, .offset=1, .tx=11, .rx=78  },
      {.name="SS8Y",  .hccID=3,  .numABCs=10, .offset=1, .tx=11, .rx=80  },
      {.name="SS9X",  .hccID=4,  .numABCs=10, .offset=1, .tx=11, .rx=86  },
      {.name="SS9Y",  .hccID=5,  .numABCs=10, .offset=1, .tx=11, .rx=84  },
      {.name="SS10X", .hccID=6,  .numABCs=10, .offset=1, .tx=11, .rx=82  },
      {.name="SS10Y", .hccID=7,  .numABCs=10, .offset=1, .tx=11, .rx=14  },
      {.name="SS11X", .hccID=8,  .numABCs=10, .offset=1, .tx=11, .rx=16  },
      {.name="SS11Y", .hccID=9,  .numABCs=10, .offset=1, .tx=11, .rx=18  },
      {.name="SS12X", .hccID=10, .numABCs=10, .offset=1, .tx=11, .rx=20  },
      {.name="SS12Y", .hccID=11, .numABCs=10, .offset=1, .tx=11, .rx=22  },
      {.name="SS13X", .hccID=2,  .numABCs=10, .offset=1, .tx=1,  .rx=70  },
      {.name="SS13Y", .hccID=3,  .numABCs=10, .offset=1, .tx=1,  .rx=26  }
  }};

  // 14 HCCStars on one side of a petal
  // An example configuration based on the ITSDAQ GLIB setup
  const std::array<HybridInfo, 14> petal {{
      {.name="R0H0", .hccID=0,  .numABCs=8,  .offset=3, .tx=102, .rx=16 },
      {.name="R0H1", .hccID=1,  .numABCs=9,  .offset=0, .tx=102, .rx=24 },
      {.name="R1H0", .hccID=2,  .numABCs=10, .offset=1, .tx=102, .rx=20 },
      {.name="R1H1", .hccID=3,  .numABCs=11, .offset=0, .tx=102, .rx=26 },
      {.name="R2H0", .hccID=4,  .numABCs=6,  .offset=5, .tx=101, .rx=22 },
      {.name="R2H1", .hccID=5,  .numABCs=6,  .offset=5, .tx=101, .rx=12 },
      {.name="R3H0", .hccID=6,  .numABCs=7,  .offset=4, .tx=101, .rx=14 },
      {.name="R3H1", .hccID=7,  .numABCs=7,  .offset=4, .tx=101, .rx=18 },
      {.name="R3H2", .hccID=8,  .numABCs=7,  .offset=0, .tx=101, .rx=0  },
      {.name="R3H3", .hccID=9,  .numABCs=7,  .offset=0, .tx=101, .rx=2  },
      {.name="R4H0", .hccID=10, .numABCs=8,  .offset=3, .tx=103, .rx=4  },
      {.name="R4H1", .hccID=11, .numABCs=8,  .offset=3, .tx=103, .rx=8  },
      {.name="R5H0", .hccID=12, .numABCs=9,  .offset=2, .tx=103, .rx=6  },
      {.name="R5H1", .hccID=13, .numABCs=9,  .offset=2, .tx=103, .rx=10 }
  }};

  template <unsigned n_modules>
  std::tuple<json, std::vector<json>> createConfigStarObject(
    StarCfg& feCfg, const std::array<HybridInfo, n_modules>& modules, bool isbarrel) {

    std::tuple<json, std::vector<json>> preset;
    auto& [systemCfg, chips] = preset;

    const std::string chipName(feCfg.getName());

    // Connectivity config
    systemCfg["chipType"] = chipName;

    for (int i=0; i<modules.size(); i++) {
      std::string mName(chipName+"_"+modules[i].name);
      systemCfg["chips"][i]["config"] = mName+".json";
      systemCfg["chips"][i]["tx"] = modules[i].tx;
      systemCfg["chips"][i]["rx"] = modules[i].rx;
      systemCfg["chips"][i]["locked"] = 1;
      systemCfg["chips"][i]["enable"] = 1;

      // Chip config
      feCfg.setName(mName);
      feCfg.setHCCChipId(modules[i].hccID);

      // Add ABCStars
      feCfg.clearABCchipIDs();

      unsigned maskHccIn = 0;

      for (unsigned iABC=0; iABC<modules[i].numABCs; iABC++) {
        // HCC input channel
        unsigned hccIn = iABC + modules[i].offset;

        // ABC chip ID
        unsigned abcID = hccIn;

        // Flip the channel number if HCCv0
        if (feCfg.hccVersion() == 0) {
          hccIn = 10 - hccIn;
        }

        // ABC ID offset in case of HCCv0 for barrel modules
        if (feCfg.hccVersion() == 0 and isbarrel) {
          abcID -= 1;
        }

        feCfg.addABCchipID(abcID, hccIn);
        maskHccIn += (1<<hccIn);
      }

      // Set HCC's IC_Enable accordingly
      feCfg.hcc().setSubRegisterValue("ICENABLE", maskHccIn);

      json chipCfg;
      feCfg.writeConfig(chipCfg);

      // Add chip config to list
      chips.push_back(std::move(chipCfg));
    }

    return preset;
  }

  std::tuple<json, std::vector<json>> createConfigSingleStar(StarCfg& feCfg) {
    std::tuple<json, std::vector<json>> preset;
    auto& [connectivity, chips] = preset;

    // Add an ABCStar
    feCfg.clearABCchipIDs();
    feCfg.addABCchipID(0, 0);
    // And set HCC's IC_Enable accordingly
    feCfg.hcc().setSubRegisterValue("ICENABLE", 0x1);

    // Config for one HCCStar + one ABCStar
    json cfg;
    feCfg.writeConfig(cfg);
    chips.push_back(std::move(cfg));

    // Connectivity configuration
    connectivity["chipType"] = feCfg.getName();
    connectivity["chips"][0]["config"] = feCfg.getName()+".json";
    connectivity["chips"][0]["tx"] = 0;
    connectivity["chips"][0]["rx"] = 1;
    connectivity["chips"][0]["locked"] = 1;
    connectivity["chips"][0]["enable"] = 1;

    return preset;
  }

  using configFuncMap = std::unordered_map<std::string, std::function<std::tuple<json, std::vector<json>>(StarCfg&)>>;
  configFuncMap createConfigsMap = {
      {"SingleChip",   [](StarCfg& feCfg) { return createConfigSingleStar(feCfg); }},
      {"StripLSStave", [](StarCfg& feCfg) { return createConfigStarObject<14>(feCfg, lsstave, true); }},
      {"StripSSStave", [](StarCfg& feCfg) { return createConfigStarObject<28>(feCfg, ssstave, true); }},
      {"StripPetal",   [](StarCfg& feCfg) { return createConfigStarObject<14>(feCfg, petal, false); }}
  };

  std::tuple<json, std::vector<json>> createConfigStar(StarCfg& feCfg, const std::string& systemType) {
    try {
      auto preset = createConfigsMap[systemType](feCfg);
      return preset;
    }

    catch (std::out_of_range &oor) {
      logger->error("StarPreset::createConfigStar Unknown system type: {}", systemType);
      std::string knowntypes;
      for (const auto& f : createConfigsMap)
          knowntypes += f.first+" ";
      logger->info("StarPreset::createConfigStar Known system types are: {}", knowntypes);
      throw;
    }

    catch (...) {
        throw;
    }
  }

} // close namespace StarPreset

#endif
