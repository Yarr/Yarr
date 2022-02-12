#ifndef RD53ACORECOLLOOP_H
#define RD53ACORECOLLOOP_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Loop over core columns for RD53A
// # Date: 03/2018
// ################################

#include <iostream>

#include "LoopActionBase.h"

class Rd53aCoreColLoop : public LoopActionBase {
    public:
        Rd53aCoreColLoop();
        
        void writeConfig(json &j) override;
        void loadConfig(const json &j)  override;
    
    private:
        /**
         * Encapsulating details to class Impl
         * Only forward declaration here
         * Class definition is given in the cpp file
         */
        class Impl;
        std::unique_ptr<Impl> m_impl;
        std::vector<unsigned> m_delayArray;
        
        void init()      override;
        void end()       override;
        void execPart1() override;
        void execPart2() override;
};


#endif

