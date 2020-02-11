#ifndef HCC_STAR_CFG_INCLUDE
#define HCC_STAR_CFG_INCLUDE

// #################################
// # Project:
// # Description: StarChips Library
// # Comment: HccStar configuration class
// ################################

#include <iostream>
#include <fstream>
#include <cmath>

#include "FrontEnd.h"

class HccStarCfg : public FrontEndCfg {
public:
    HccStarCfg();
    ~HccStarCfg();

    // Don't use for now
    double toCharge(double) override { return 0.0; };
    double toCharge(double, bool, bool) override { return 0.0; };

    void toFileJson(json &j) override;
    void fromFileJson(json &j) override;


};

#endif
