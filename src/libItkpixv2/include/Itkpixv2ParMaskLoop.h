#ifndef ITKPIXV2PARMASKLOOP_H
#define ITKPIXV2PARMASKLOOP_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Parallel Mask Loop for ITkPixV2
// # Date: 07/2023
// ################################

#include <vector>
#include <array>
#include <map>

#include "LoopActionBase.h"
#include "Itkpixv2.h"
#include "FrontEnd.h"

class Itkpixv2ParMaskLoop: public LoopActionBase {
    public:
        Itkpixv2ParMaskLoop();

        void writeConfig(json &j) override;
        void loadConfig(const json &j) override;

    private:
        unsigned m_cur;
        std::map<FrontEnd*, std::array<std::array<uint16_t, Itkpixv2::n_Row>, Itkpixv2::n_DC> > m_pixRegs;
        int m_maskType;
        bool m_applyEnMask;
        
        void init() override;
        void end() override;
        void execPart1() override;
        void execPart2() override;

        bool applyMask(unsigned col, unsigned row);

};
#endif
