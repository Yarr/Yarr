#ifndef ITKPIXV2PARAMETERLOOP_H
#define ITKPIXV2PARAMETERLOOP_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Parameter Loop for ITkPixV2
// # Date: 07/2023
// ################################

#include "LoopActionBase.h"
#include "Itkpixv2.h"

class Itkpixv2ParameterLoop : public LoopActionBase {
    public:
        Itkpixv2ParameterLoop();
        Itkpixv2ParameterLoop(Itkpixv2RegDefault Itkpixv2GlobalCfg::*ref);

        void writeConfig(json &j) override;
        void loadConfig(const json &j) override;

    private:
        Itkpixv2RegDefault Itkpixv2GlobalCfg::*parPtr;
        std::string parName;
        void writePar();
        unsigned m_cur;

        void init() override;
        void end() override;
        void execPart1() override;
        void execPart2() override;
};

#endif
