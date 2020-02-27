/*
 * Authors: T. Heim <timon.heim@cern.ch>,
 * Date: 2014-Oct-16
 */

#ifndef FEI4PARAMETERLOOP_H
#define FEI4PARAMETERLOOP_H

#include "Fei4.h"
#include "LoopActionBase.h"

#include "logging.h"

class Fei4ParameterLoop : public LoopActionBase{
        static logging::Logger &logger() {
          static logging::LoggerStore instance = logging::make_log("Fei4ParameterLoop");
          return *instance;
        }

    public:
        Fei4ParameterLoop() {
	  min=0;
	  max=100;
	  step=1;
            loopType = typeid(this);
        }

        Fei4ParameterLoop(Fei4Register Fei4GlobalCfg::*ref) : parPtr(ref){ 
	  min=0;
	  max=100;
	  step=1;
            loopType = typeid(this);
        };

        void setRange(unsigned arg_min, unsigned arg_max, unsigned arg_step) {
            min = arg_min;
            max = arg_max;
            step = arg_step;
        }
	void writeConfig(json &j);
	void loadConfig(json &j);


    private:
	std::string parName = "";
        void init() {
            m_done = false;
            cur = min;
	    if(parName!=""){
	      parPtr = keeper->globalFe<Fei4>()->regMap[parName];
	    }
            this->writePar();
        }

        void end() {}
        void execPart1() {
            logger().info("Parameter Loop at -> {}", cur);
            g_stat->set(this, cur);
        }

        void execPart2() {
            cur += step;
            if ((int)cur > max) m_done = true;
            this->writePar();
        }

        void writePar() {
            keeper->globalFe<Fei4>()->writeRegister(parPtr, cur);
            while(!g_tx->isCmdEmpty());
        }

        unsigned cur;

        Fei4Register Fei4GlobalCfg::*parPtr;

        // Somehow we need to register logger at static init time
        friend void logger_static_init_fei4();
};

#endif
