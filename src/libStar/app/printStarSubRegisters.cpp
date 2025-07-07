#include "AbcNames.h"
#include "HccNames.h"
#include "StarCfg.h"

#include <fstream>
#include <getopt.h>
#include <iomanip>
#include <iostream>
#include <string>

using HCCSubRegisterMap =
    std::map<HCCStarRegister, std::vector<HCCStarSubRegister>>;
using ABCSubRegisterMap =
    std::map<ABCStarRegister, std::vector<ABCStarSubRegister>>;

struct Options {
  std::string configPath;
  int abcVersion = 1;
  int hccVersion = 1;
};

Options parseArgs(int argc, char *argv[]) {
  Options opts;
  int c;
  while ((c = getopt(argc, argv, "hc:a:r:")) != -1) {
    switch (c) {
      case 'h':
        std::cout << "usage: ./bin/printStarSubRegisters -c [CHIP_CONFIG] -r [HCC_VERSION] -a [ABC_VERSION]" << std::endl;
        exit(0);
      case 'c':
        opts.configPath = optarg;
        break;
      case 'a':
        opts.abcVersion = std::stoi(optarg);
        break;
      case 'r':
        opts.hccVersion = std::stoi(optarg);
        break;
      default:
        std::cerr << "Unknown option: " << c << std::endl;
        exit(1);
    }
  }

  return opts;
}

void loadJson(const std::string &path, json &j) {
  std::ifstream configFile(path);
  if (!configFile.is_open()) {
    std::cerr << "could not open " << path << std::endl;
    exit(1);
  }

  configFile >> j;
  configFile.close();

  if (j.empty()) {
    std::cerr << "json file " << path << " is empty" << std::endl;
    exit(1);
  }
}

HCCSubRegisterMap
fillHCCSubRegisterMap(std::vector<HCCStarSubRegister> &subRegs,
                      StarCfg &starCfg) {
  HCCSubRegisterMap subRegMap;
  for (auto &subReg : subRegs) {
    try {
      auto parentAddr = starCfg.getHCCSubRegisterParentAddr(subReg);
      HCCStarRegister parentReg = HCCStarRegister(parentAddr);

      auto it = subRegMap.find(parentReg);
      if (it == subRegMap.end()) {
        subRegMap[parentReg] = std::vector<HCCStarSubRegister>();
      }

      subRegMap[parentReg].push_back(subReg);
    } catch (std::runtime_error &e) {
      // subregister does not exist for this HCC version
    }
  }

  return subRegMap;
}

void printHCCSubRegisterMap(
    uint32_t fuseId,
    const std::map<HCCStarRegister, std::vector<HCCStarSubRegister>> &subRegMap,
    StarCfg &starCfg) {
  std::cout << "HCC fuseid " << std::hex << fuseId << std::endl;
  for (const auto &[reg, subRegs] : subRegMap) {
    std::cout << "  " << HccNames::regToString(reg) << " 0x" << std::hex << std::setw(8)
              << std::setfill('0') << starCfg.getHCCRegister(reg) << std::endl;
    for (const auto &subReg : subRegs) {
      std::cout << "    " << HccNames::subRegToString(subReg) << " 0x" << std::hex
                << std::setfill('0') << starCfg.getHCCSubRegisterValue(subReg)
                << std::endl;
    }
  }
}

ABCSubRegisterMap
fillABCSubRegisterMap(std::vector<ABCStarSubRegister> &subRegs,
                      StarCfg &starCfg) {
  ABCSubRegisterMap subRegMap;
  for (auto &subReg : subRegs) {
    try {
      auto parentAddr = starCfg.getABCSubRegisterParentAddr(subReg);
      ABCStarRegister parentReg = ABCStarRegister(parentAddr);

      auto it = subRegMap.find(parentReg);
      if (it == subRegMap.end()) {
        subRegMap[parentReg] = std::vector<ABCStarSubRegister>();
      }

      subRegMap[parentReg].push_back(subReg);
    } catch (std::out_of_range &e) {
      // subregister does not exist for this HCC version
    }
  }

  return subRegMap;
}

void printABCSubRegisterMap(
    unsigned abcId,
    const std::map<ABCStarRegister, std::vector<ABCStarSubRegister>> &subRegMap,
    StarCfg &starCfg) {
  std::cout << "ABC " << std::hex << abcId << std::endl;
  for (const auto &[reg, subRegs] : subRegMap) {
    std::cout << "  " << AbcNames::regToString(reg) << " 0x" << std::hex << std::setw(8)
              << std::setfill('0') << starCfg.getABCRegisterByID(reg, abcId) << std::endl;

    int inputChannel = starCfg.hccChannelForABCchipID(abcId);
    for (const auto &subReg : subRegs) {
      std::cout << "    " << AbcNames::subRegToString(subReg) << " 0x" << std::hex
                << std::setfill('0') << starCfg.getABCSubRegisterValue(inputChannel, subReg)
                << std::endl;
    }
  }
}

int main(int argc, char *argv[]) {

  Options opts = parseArgs(argc, argv);

  json j;
  loadJson(opts.configPath, j);

  StarCfg starCfg(opts.abcVersion, opts.hccVersion);
  try {
    starCfg.loadConfig(j);
  } catch (const std::exception &e) {
    std::cerr << "error loading config: " << e.what() << std::endl;
    std::cerr << "you may be specifying the wrong HCC and/or ABC versions" << std::endl;
    return 1;
  }

  std::vector<HCCStarSubRegister> subRegs = HccNames::listSubRegs();
  HCCSubRegisterMap hccSubRegMap = fillHCCSubRegisterMap(subRegs, starCfg);
  uint32_t hccFuseId = starCfg.getHCCfuseID();
  printHCCSubRegisterMap(hccFuseId, hccSubRegMap, starCfg);

  std::vector<ABCStarSubRegister> abcSubRegs = AbcNames::listSubRegs();
  ABCSubRegisterMap abcSubRegMap = fillABCSubRegisterMap(abcSubRegs, starCfg);
  starCfg.eachAbc([&](AbcCfg &abc) {
    unsigned abcId = abc.getABCchipID();
    printABCSubRegisterMap(abcId, abcSubRegMap, starCfg);
  });
}
