#ifndef EMPTY_FRONTEND_H
#define EMPTY_FRONTEND_H

// #################################
// # Project: Yarr
// # Description: Test FE class
// ################################

#include "FrontEnd.h"

class EmptyFrontEndCfg : public FrontEndCfg {
    public:
        EmptyFrontEndCfg() = default;
        virtual ~EmptyFrontEndCfg()= default;
        
        double toCharge(double) override { return 0.0; }
        double toCharge(double, bool, bool) override { return 0.0; }

        void writeConfig(json &) override {}
        void loadConfig(const json &) override {}

        unsigned getPixelEn(unsigned col, unsigned row) override { return 0; }
        void maskPixel(unsigned col, unsigned row) override {}
        void enableAll() override {}
};

class EmptyFrontEnd : public FrontEnd, public EmptyFrontEndCfg {
    public:
        EmptyFrontEnd() = default;
        EmptyFrontEnd(FrontEndGeometry g) { geo = g; }

        virtual ~EmptyFrontEnd() = default;
        
        void init(HwController *arg_core, const FrontEndConnectivity& fe_cfg) override { m_rxcore = arg_core; }

        void configure() override {}

        void writeNamedRegister(std::string name, uint16_t value) override {}
        void setInjCharge(double, bool, bool) override {}
};

#endif
