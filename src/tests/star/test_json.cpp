#include "catch.hpp"

#include <filesystem>

#include "AllChips.h"
#include "ScanHelper.h"

namespace fs = std::filesystem;

void bounce_check(json &j) {
  auto fe = StdDict::getFrontEnd("Star");
  auto fecfg = dynamic_cast<FrontEndCfg*>(&*fe);
  std::stringstream jj_pre;
  jj_pre << j;

  fecfg->fromFileJson(j);

  std::stringstream jj;
  jj << j;

  // Same before and after reading
  REQUIRE(jj_pre.str() == jj.str());

  json bounced;
  fecfg->toFileJson(bounced);

  std::stringstream outj;
  outj << bounced;

  REQUIRE(jj.str() == outj.str());
}

// Check that the default configuration is loadable
TEST_CASE("StarJsonDefault", "[star][json]") {
  // Look up in dir hierarchy so we can run from anywhere in YARR directory
  fs::path baseDir{fs::current_path()};
  while(!fs::exists(baseDir/".git")) {
    if(!baseDir.has_parent_path()) {
      // Can't find file
      std::cout << "Can't find default star config, no parent of " << baseDir << "\n";
      return;
    }
    if(baseDir == baseDir.parent_path()) {
      // Can't find file
      std::cout << "Can't find default star config from " << baseDir << "\n";
      return;
    }
    baseDir = baseDir.parent_path();
  }

  fs::path fileName = baseDir/"configs/defaults/default_star.json";

  // std::cout << "Loading json from " << fileName << "\n";

  json cfg = ScanHelper::openJsonFile(fileName);
  auto fe = StdDict::getFrontEnd("Star");
  auto fecfg = dynamic_cast<FrontEndCfg*>(&*fe);
  REQUIRE(fecfg);
  fecfg->fromFileJson(cfg);

  json output;
  fecfg->toFileJson(output);

  // debugging
  // output.dump(4);

  // Should be the same if read and write again
  bounce_check(output);
}

// Minimal configuration (no ABCs)
TEST_CASE("StarJsonMinimal", "[star][json]") {
  json cfg;

  cfg["name"] = "testname";
  cfg["HCC"]["ID"] = 12;

  auto fe = StdDict::getFrontEnd("Star");
  auto fecfg = dynamic_cast<FrontEndCfg*>(&*fe);
  REQUIRE(fecfg);
  fecfg->fromFileJson(cfg);

  json output;
  fecfg->toFileJson(output);

  REQUIRE(output["name"] == cfg["name"]);

  REQUIRE(output["HCC"]["ID"] == cfg["HCC"]["ID"]);

  // debugging
  // output.dump(4);

  // Should be the same if read and write again
  bounce_check(output);
}

// Minimal configuration (with some ABCs)
TEST_CASE("StarJsonMinimalABC", "[star][json]") {
  json cfg;

  cfg["name"] = "testname";
  cfg["HCC"]["ID"] = 12;

  for(int i=0; i<2; i++) {
    cfg["ABCs"]["IDs"][i] = i+3;
  }

  auto fe = StdDict::getFrontEnd("Star");
  auto fecfg = dynamic_cast<FrontEndCfg*>(&*fe);
  REQUIRE(fecfg);
  fecfg->fromFileJson(cfg);

  json output;
  fecfg->toFileJson(output);

  REQUIRE(output["name"] == cfg["name"]);

  REQUIRE(output["HCC"]["ID"] == cfg["HCC"]["ID"]);

  for(int i=0; i<2; i++) {
    CAPTURE(i);
    REQUIRE(output["ABCs"]["IDs"][i] == cfg["ABCs"]["IDs"][i]);
  }

  // debugging
  // output.dump(4);

  bounce_check(output);
}

// Configure some HCC registers
TEST_CASE("StarJsonHccRegs", "[star][json]") {
  json cfg;

  cfg["name"] = "testname";
  cfg["HCC"]["ID"] = 12;
  cfg["HCC"]["regs"]["Delay1"] = 0x12345678;

  // debugging
  // cfg.dump(4);

  // No ABC

  auto fe = StdDict::getFrontEnd("Star");
  auto fecfg = dynamic_cast<FrontEndCfg*>(&*fe);
  REQUIRE(fecfg);
  fecfg->fromFileJson(cfg);

  json output;
  fecfg->toFileJson(output);

  // debugging
  // output.dump(4);

  // REQUIRE(fecfg->numABCs() == 0);

  REQUIRE(output["name"] == cfg["name"]);

  REQUIRE(output["HCC"]["ID"] == cfg["HCC"]["ID"]);
  REQUIRE(output["HCC"]["regs"]["Delay1"] == "12345678");

  bounce_check(output);
}

// Configure some ABC registers
TEST_CASE("StarJsonAbcRegs", "[star][json]") {
  json cfg;

  cfg["name"] = "testname";

  cfg["HCC"]["ID"] = 12;

  cfg["ABCs"]["IDs"][0] = 4;
  cfg["ABCs"]["IDs"][1] = 6;

  cfg["ABCs"]["regs"][0]["ADCS1"] = 0x02040810;
  cfg["ABCs"]["regs"][1]["CREG0"] = "0ff00100";

  auto fe = StdDict::getFrontEnd("Star");
  auto fecfg = dynamic_cast<FrontEndCfg*>(&*fe);
  REQUIRE(fecfg);
  fecfg->fromFileJson(cfg);

  json output;
  fecfg->toFileJson(output);

  // debugging
  //  output.dump(4);

  REQUIRE(output["name"] == cfg["name"]);

  // Output is simply all registers in hex
  std::string outVal = output["ABCs"]["regs"][0]["ADCS1"];
  REQUIRE(outVal == "02040810");

  outVal = output["ABCs"]["regs"][1]["CREG0"];
  REQUIRE(outVal == "0ff00100");

  bounce_check(output);
}

// Configure ABC mask registers
TEST_CASE("StarJsonAbcMasks", "[star][json]") {
  json cfg;

  cfg["name"] = "testname";

  cfg["HCC"]["ID"] = 12;

  cfg["ABCs"]["IDs"][0] = 4;
  cfg["ABCs"]["IDs"][1] = 6;

  cfg["ABCs"]["masked"][0].push_back(0);
  cfg["ABCs"]["masked"][0].push_back(127);
  cfg["ABCs"]["masked"][0].push_back(128);
  cfg["ABCs"]["masked"][0].push_back(255);
  cfg["ABCs"]["masked"][1].push_back(0);
  cfg["ABCs"]["masked"][1].push_back(1);
  cfg["ABCs"]["masked"][1].push_back(2);
  cfg["ABCs"]["masked"][1].push_back(3);
  cfg["ABCs"]["masked"][1].push_back(4);

  auto fe = StdDict::getFrontEnd("Star");
  auto fecfg = dynamic_cast<FrontEndCfg*>(&*fe);
  REQUIRE(fecfg);
  fecfg->fromFileJson(cfg);

  json output;
  fecfg->toFileJson(output);

  // debugging
  // output.dump(4);

  REQUIRE(output["name"] == cfg["name"]);

  REQUIRE(output["ABCs"]["masked"] == cfg["ABCs"]["masked"]);

  bounce_check(output);
}

// Configure some ABC registers using subregs
TEST_CASE("StarJsonAbcSubRegs", "[star][json]") {
  json cfg;

  cfg["name"] = "testname";

  cfg["HCC"]["ID"] = 12;

  cfg["ABCs"]["IDs"][0] = 4;
  cfg["ABCs"]["IDs"][1] = 6;

  cfg["ABCs"]["subregs"][0]["LCB_ERRCOUNT_THR"] = 123;
  cfg["ABCs"]["subregs"][1] = nullptr;

  auto fe = StdDict::getFrontEnd("Star");
  auto fecfg = dynamic_cast<FrontEndCfg*>(&*fe);
  REQUIRE(fecfg);
  fecfg->fromFileJson(cfg);

  json output;
  fecfg->toFileJson(output);

  // debugging
  // output.dump(4);

  REQUIRE(output["name"] == cfg["name"]);

  // Output is simply all registers in hex
  std::string outVal = output["ABCs"]["regs"][0]["CREG6"];
  uint32_t regValue = std::stol(outVal, nullptr, 16);
  REQUIRE((regValue & 0xffff) == 123);

  bounce_check(output);
}

// Configure ABC trim registers
TEST_CASE("StarJsonAbcTrim", "[star][json]") {
  json cfg;

  cfg["name"] = "testname";

  cfg["HCC"]["ID"] = 12;

  cfg["ABCs"]["IDs"][0] = 4;
  cfg["ABCs"]["IDs"][1] = 6;

  cfg["ABCs"]["trims"][0] = 13;
  for(int i=0; i<256; i++) {
    cfg["ABCs"]["trims"][1][i] = (i*13)%32;
  }

  // cfg.dump(4);

  auto fe = StdDict::getFrontEnd("Star");
  auto fecfg = dynamic_cast<FrontEndCfg*>(&*fe);
  REQUIRE(fecfg);
  fecfg->fromFileJson(cfg);

  json output;
  fecfg->toFileJson(output);

  // debugging
  // output.dump(4);

  REQUIRE(output["name"] == cfg["name"]);

  REQUIRE(output["ABCs"]["trims"] == cfg["ABCs"]["trims"]);

  bounce_check(output);
}
