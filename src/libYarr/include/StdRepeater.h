/*
 * Authors: T. Heim <timon.heim@cern.ch>,
 * Date: 2018-Aug
 */

#ifndef STDREPEATER_H
#define STDREPEATER_H

#include "LoopActionBase.h"

class StdRepeater: public LoopActionBase {
    public:
        StdRepeater();
        
        void writeConfig(json &j) override final;
        void loadConfig(json &j)  override final;
    
    private:
        int m_cur;
        void init() override;
        void end() override;
        void execPart1() override;
        void execPart2() override; 
};

#endif


