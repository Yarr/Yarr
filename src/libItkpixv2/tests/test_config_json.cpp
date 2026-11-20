#include "catch.hpp"
#include <sstream>

#include "AllChips.h"
#include "FrontEndCfg.h"
#include "Itkpixv2Cfg.h"

// Check that loadConfig(writeConfig(x)) reproduces the same json, i.e.
// that a freshly constructed config round-trips through json without loss.
TEST_CASE("Itkpixv2JsonBounce", "[itkpixv2][json]") {
  auto fe = StdDict::getFrontEnd("ITKPIXV2");
  auto fecfg = dynamic_cast<FrontEndCfg*>(&*fe);
  REQUIRE(fecfg);

  json written;
  fecfg->writeConfig(written);

  std::stringstream writtenj;
  writtenj << written;

  auto fe2 = StdDict::getFrontEnd("ITKPIXV2");
  auto fecfg2 = dynamic_cast<FrontEndCfg*>(&*fe2);
  REQUIRE(fecfg2);

  fecfg2->loadConfig(written);

  json bounced;
  fecfg2->writeConfig(bounced);

  std::stringstream bouncedj;
  bouncedj << bounced;

  REQUIRE(writtenj.str() == bouncedj.str());
}

// Currents read through the ADC are shifted by ImuxGndOffset (mV): the ADC is
// referenced to the Vmux ground, the current mux to the Imux ground.
TEST_CASE("Itkpixv2ImuxGndOffset", "[itkpixv2][json][adc]") {
  auto fe = StdDict::getFrontEnd("ITKPIXV2");
  auto cfg = dynamic_cast<Itkpixv2Cfg*>(&*fe);
  REQUIRE(cfg);

  json j;
  cfg->writeConfig(j);
  // Default when the chip config doesn't set it
  REQUIRE(j["ITKPIXV2"]["Parameter"]["ImuxGndOffset"] == Catch::Approx(2.4));

  // 1000 counts -> 0.2 V with a 10 kOhm R_Imux
  j["ITKPIXV2"]["Parameter"]["ADCcalPar"] = {0.0, 0.2, 10000.0};

  SECTION("default offset") {
    j["ITKPIXV2"]["Parameter"].erase("ImuxGndOffset");
    cfg->loadConfig(j);
    REQUIRE(cfg->adcToV(1000) == Catch::Approx(0.2));
    REQUIRE(cfg->adcToI(1000) == Catch::Approx((0.2 + 2.4e-3) / 10000.0));
    REQUIRE(cfg->convertAdc(1000, true).first == Catch::Approx((0.2 + 2.4e-3) / 10000.0 / 1e-6));
  }

  SECTION("offset from the chip config") {
    j["ITKPIXV2"]["Parameter"]["ImuxGndOffset"] = 3.85;
    cfg->loadConfig(j);
    REQUIRE(cfg->adcToV(1000) == Catch::Approx(0.2));
    REQUIRE(cfg->adcToI(1000) == Catch::Approx((0.2 + 3.85e-3) / 10000.0));
  }

  SECTION("zero offset") {
    j["ITKPIXV2"]["Parameter"]["ImuxGndOffset"] = 0.0;
    cfg->loadConfig(j);
    REQUIRE(cfg->adcToI(1000) == Catch::Approx(0.2 / 10000.0));
  }
}
