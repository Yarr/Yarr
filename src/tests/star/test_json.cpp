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

TEST_CASE("StarJsonMinimal", "[star][json]") {
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
