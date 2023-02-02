#include <vector>

#include "catch.hpp"
#include "StarConversionTools.h"

TEST_CASE("StarConversionTools", "[star][json]") {
  StarConversionTools ct;

  json cfg;

  SECTION("default") {
    ct.loadConfig(cfg);

    // Threshold
    REQUIRE_THAT(ct.convertBVTtomV(1), Catch::Matchers::WithinAbs(3.7674, 1e-5));

    // Charge injection
    REQUIRE_THAT(ct.convertBCALtofC(100), Catch::Matchers::WithinAbs(1.953125, 1e-7));

    // Response
    // No fit parameters are loaded. Expect an error message and an exception
    REQUIRE_THROWS_AS(ct.convertfCtomV(42, 10), std::out_of_range);
  }

  SECTION("BVTtoV") {
    SECTION("array") {
      // The array below is the simulated response sometimes used by ITSDAQ
      // The values are taken from: https://gitlab.cern.ch/atlas-itk-strips-daq/itsdaq-sw/-/blob/master/config/ABCStar_thrCal.txt
      // Unit: V
      cfg["BVTtoV"] = {
        -0.000423025, 0.002502479, 0.005427983, 0.008305862, 0.011183742, 0.01402607, 0.016868398, 0.019681878, 0.022495358, 0.025286024, 0.02807669, 0.03084766, 0.033618631, 0.036373231, 0.039127832, 0.041869107, 0.044610382, 0.047348878, 0.050087374, 0.052838029, 0.055588684, 0.058348824, 0.061108964, 0.063868934, 0.066628904, 0.069386677, 0.072144451, 0.074898614, 0.077652776, 0.08040351, 0.083154244, 0.08590056, 0.088646875, 0.091390676, 0.094134477, 0.096874878, 0.09961528, 0.102352784, 0.105090289, 0.107824446, 0.110558604, 0.113290469, 0.116022335, 0.11875144, 0.121480545, 0.124207358, 0.12693417, 0.129657635, 0.132381099, 0.135103368, 0.137825636, 0.140545635, 0.143265634, 0.145983792, 0.14870195, 0.151417753, 0.154133557, 0.156847979, 0.1595624, 0.162274913, 0.164987426, 0.167698432, 0.170409438, 0.173117984, 0.175826531, 0.178534399, 0.181242267, 0.183948496, 0.186654725, 0.189359695, 0.192064665, 0.194767828, 0.197470991, 0.200173271, 0.202875551, 0.205576393, 0.208277235, 0.210977005, 0.213676775, 0.216374353, 0.219071931, 0.221769346, 0.224466761, 0.227162889, 0.229859017, 0.232554209, 0.235249401, 0.237943092, 0.240636782, 0.243329863, 0.246022944, 0.248714845, 0.251406745, 0.254097806, 0.256788866, 0.259478102, 0.262167338, 0.264856448, 0.267545558, 0.270233552, 0.272921547, 0.275608753, 0.278295958, 0.280981793, 0.283667627, 0.28635295, 0.289038274, 0.291722496, 0.294406719, 0.29709015, 0.299773581, 0.302455073, 0.305136565, 0.307818055, 0.310499545, 0.313179873, 0.315860201, 0.318539647, 0.321219092, 0.323897031, 0.326574969, 0.329252194, 0.331929418, 0.334605276, 0.337281134, 0.33995585, 0.342630566, 0.34530305, 0.347975533, 0.350647291, 0.353319049, 0.355988928, 0.358658808, 0.361326901, 0.363994994, 0.366660454, 0.369325915, 0.371989234, 0.374652553, 0.377312724, 0.379972895, 0.382629675, 0.385286454, 0.387938045, 0.390589635, 0.393237048, 0.395884461, 0.398525476, 0.40116649, 0.403799997, 0.406433504, 0.409057184, 0.411680863, 0.414293352, 0.416905841, 0.41950396, 0.42210208, 0.42468304, 0.427264, 0.429823403, 0.432382806, 0.434918404, 0.437454003, 0.43996102, 0.442468038, 0.444943031, 0.447418023, 0.449857136, 0.452296248, 0.454697427, 0.457098607, 0.45945923, 0.461819852, 0.464138739, 0.466457626, 0.468733074, 0.471008522, 0.473241815, 0.475475109, 0.477665503, 0.479855898, 0.482004093, 0.484152288, 0.486258527, 0.488364765, 0.490430426, 0.492496088, 0.494521568, 0.496547048, 0.498533347, 0.500519646, 0.502466856, 0.504414065, 0.506323884, 0.508233704, 0.510106012, 0.51197832, 0.513813679, 0.515649038, 0.51744733, 0.519245622, 0.521007368, 0.522769113, 0.524493811, 0.526218509, 0.527905899, 0.52959329, 0.531242051, 0.532890813, 0.534500706, 0.536110599, 0.5376791, 0.539247601, 0.540771614, 0.542295627, 0.543769333, 0.545243038, 0.546657811, 0.548072583, 0.549418743, 0.550764903, 0.552040181, 0.55331546, 0.554515578, 0.555715697, 0.55686628, 0.558016864, 0.559122135, 0.560227406, 0.561286992, 0.562346578, 0.563359467, 0.564372357, 0.565337959, 0.56630356, 0.567221133, 0.568138706, 0.569008027, 0.569877348, 0.570698272, 0.571519197, 0.572292573, 0.573065949, 0.573792432, 0.574518914, 0.575199819, 0.575880723, 0.576517582, 0.577154441, 0.577749279, 0.578344116, 0.578898992, 0.579453869, 0.579971113, 0.580488357, 0.581005601
      };

      ct.loadConfig(cfg);

      REQUIRE_THAT(ct.convertBVTtomV(0), Catch::Matchers::WithinAbs(-0.423025, 1e-5));
      REQUIRE_THAT(ct.convertBVTtomV(255), Catch::Matchers::WithinAbs(581.005601, 0.001));
    }

    SECTION("points") {
      cfg["BVTtoV"]["DAC"] = {10, 50, 100};
      cfg["BVTtoV"]["values"] = {0.025, 0.150, 0.300};

      ct.loadConfig(cfg);

      REQUIRE_THAT(ct.convertBVTtomV(10), Catch::Matchers::WithinAbs(25., 0.001));
      REQUIRE_THAT(ct.convertBVTtomV(50), Catch::Matchers::WithinAbs(150., 0.001));
      REQUIRE_THAT(ct.convertBVTtomV(100), Catch::Matchers::WithinAbs(300., 0.001));

      // Extrapolation
      REQUIRE_THAT(ct.convertBVTtomV(0), Catch::Matchers::WithinAbs(-25./4, 0.001));
      REQUIRE_THAT(ct.convertBVTtomV(25), Catch::Matchers::WithinAbs(575./8, 0.001));
      REQUIRE_THAT(ct.convertBVTtomV(70), Catch::Matchers::WithinAbs(210., 0.001));
      REQUIRE_THAT(ct.convertBVTtomV(150), Catch::Matchers::WithinAbs(450., 0.001));
    }
  }

  SECTION("BCALtoV") {
    cfg["BCALtoV"]["DAC"] = {6, 22, 38, 48, 62, 74, 100, 152, 204, 308};
    cfg["BCALtoV"]["values"] = {0.0032, 0.00832167, 0.01343333, 0.01663333, 0.02111667, 0.02494833, 0.03326667, 0.04988333, 0.06651667, 0.09975};
    // fC: {0.192, 0.4993, 0.806, 0.998, 1.267, 1.4969, 1.996, 2.993, 3.991, 5.985};

    ct.loadConfig(cfg);

    REQUIRE_THAT(ct.convertBCALtofC(22), Catch::Matchers::WithinAbs(0.4993, 1e-4));
    REQUIRE_THAT(ct.convertBCALtofC(204), Catch::Matchers::WithinAbs(3.991, 1e-4));

    // Extrapolation
    REQUIRE_THAT(ct.convertBCALtofC(90), Catch::Matchers::WithinAbs(1.8040, 1e-4));
  }

  SECTION("Response") {
    SECTION("linear") {
      cfg["ResponseFitFunction"] = "linear";
      cfg["ResponseFitParams"] = json::array();
      // for 3 ABCs
      cfg["ResponseFitParams"][0] = {0.1,0.2};
      cfg["ResponseFitParams"][1] = {0.3,0.4};
      cfg["ResponseFitParams"][2] = {0.5,0.6};

      ct.loadConfig(cfg);

      REQUIRE_THAT(ct.convertfCtomV(66, 0), Catch::Matchers::WithinAbs(13.3, 0.001));
      REQUIRE_THAT(ct.convertfCtomV(66, 1), Catch::Matchers::WithinAbs(26.7, 0.001));
      REQUIRE_THAT(ct.convertfCtomV(66, 2), Catch::Matchers::WithinAbs(40.1, 0.001));

      // inverse
      REQUIRE_THAT(ct.convertmVtofC(13.3, 0), Catch::Matchers::WithinAbs(66, 0.001));
      REQUIRE_THAT(ct.convertmVtofC(26.7, 1), Catch::Matchers::WithinAbs(66, 0.001));
      REQUIRE_THAT(ct.convertmVtofC(40.1, 2), Catch::Matchers::WithinAbs(66, 0.001));
    }

    SECTION("polynomial") {
      cfg["ResponseFitFunction"] = "polynomial";
      cfg["ResponseFitParams"] = {{0.3,0.2,0.1}};

      ct.loadConfig(cfg);

      REQUIRE_THAT(ct.convertfCtomV(5, 0), Catch::Matchers::WithinAbs(3.8, 0.001));

      // inverse
      REQUIRE_THAT(ct.convertmVtofC(3.8, 0), Catch::Matchers::WithinAbs(5, 0.001));
    }

    SECTION("exponential") {
      cfg["ResponseFitFunction"] = "exponential";
      cfg["ResponseFitParams"] = {{1,2,3}};

      ct.loadConfig(cfg);

      REQUIRE_THAT(ct.convertfCtomV(5, 0), Catch::Matchers::WithinAbs(3.924, 0.001));

      // inverse
      REQUIRE_THAT(ct.convertmVtofC(3.924, 0), Catch::Matchers::WithinAbs(5, 0.01));
    }

    SECTION("wrong number of parameters") {
      cfg["ResponseFitFunction"] = "polynomial";
      cfg["ResponseFitParams"] = {{0.3,0.2}};

      ct.loadConfig(cfg);

      // Fit function is set but function parameters are not loaded.
      // Expect an error message and an exception
      REQUIRE_THROWS_AS(ct.convertfCtomV(5, 0), std::out_of_range);
    }
  }

  SECTION("Trim target") {
    cfg["TrimTarget"] = {
      { {"BCAL", 10}, {"BVT", {11, 22, 33}} }
      };

    // std::map of the above trim targets
    std::map<unsigned, std::map<unsigned, unsigned>> trim_targets {
      {10, {{0, 11}, {1, 22}, {2, 33}}}
      };

    ct.loadConfig(cfg);

    REQUIRE(ct.getTrimTarget(10, 0)==11);
    REQUIRE(ct.getTrimTarget(10) == trim_targets[10]);

    // Change a trim target
    ct.setTrimTarget(0, {{0, 111},{1,222},{2,333}});
    REQUIRE(ct.getTrimTarget(0, 1)==222);
  }

  //cfg.dump();
}