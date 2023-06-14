#ifndef STAR_PRESET_HEADER
#define STAR_PRESET_HEADER

#include "StarCfg.h"

// For generating example configurations
namespace StarPreset {

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

  std::tuple<json, std::vector<json>> createConfigStarObject(
    StarCfg& feCfg, const std::array<HybridInfo, 14>& modules, bool isbarrel) {

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

} // close namespace StarPreset

#endif
