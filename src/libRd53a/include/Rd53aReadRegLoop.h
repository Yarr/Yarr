#ifndef RD53AREADREGLOOP_H
#define RD53AREADREGLOOP_H

// #################################
// # Author: Ismet Siral
// # Email: ismet.siral at cern.ch
// # Project: Yarr
// # Description: Loop for Reading Registers for RD53A
// # Date: 02/2018
// ################################

#include <array>
#include <chrono>
#include <thread>
#include "LoopActionBase.h"
#include "Rd53a.h"
#include "Rd53aCmd.h"

class Rd53aReadRegLoop : public LoopActionBase {

    public:
        Rd53aReadRegLoop();


        void writeConfig(json &config) override;
        void loadConfig(const json &config) override;


    private:

        std::vector<unsigned short> m_VoltMux;
        std::vector<std::string> m_STDReg;
        std::vector<unsigned short> m_CurMux;
        std::vector<unsigned short> m_TempMux;

        uint16_t ReadRegister(Rd53Reg Rd53aGlobalCfg::*ref, Rd53a *tmpFE);
        uint16_t ReadADC(unsigned short Reg, bool doCur,  Rd53a *tmpFE );
        std::pair<uint16_t,uint16_t> ReadTemp(unsigned short Reg, Rd53a *tmpFE);

        uint16_t m_EnblRingOsc,m_RingOscDur;
        uint32_t m_RingOscRep;

        void init() override;
        void execPart1() override;
        void execPart2() override;
        void end() override;

        Rd53Reg Rd53aGlobalCfg::* OscRegisters[8] = {
                (Rd53Reg Rd53aGlobalCfg::*) &Rd53a::RingOsc0,
                (Rd53Reg Rd53aGlobalCfg::*) &Rd53a::RingOsc1,
                (Rd53Reg Rd53aGlobalCfg::*) &Rd53a::RingOsc2,
                (Rd53Reg Rd53aGlobalCfg::*) &Rd53a::RingOsc3,
                (Rd53Reg Rd53aGlobalCfg::*) &Rd53a::RingOsc4,
                (Rd53Reg Rd53aGlobalCfg::*) &Rd53a::RingOsc5,
                (Rd53Reg Rd53aGlobalCfg::*) &Rd53a::RingOsc6,
                (Rd53Reg Rd53aGlobalCfg::*) &Rd53a::RingOsc7
        };

};

#endif
