/*
 * Authors: T. Heim <timon.heim@cern.ch>,
 * Date: 2014-Oct-16
 */

#ifndef FEI4GLOBALFEEDBACK_H
#define FEI4GLOBALFEEDBACK_H

#include <queue>
#include <mutex>
#include "LoopActionBase.h"

class Fei4GlobalFeedbackBase : public LoopActionBase {
    public: 
        // TODO Feedback needs to handle multiple channels
        void feedback(unsigned channel, double sign, bool last = false) {
			//fbMutex.lock();
            if (sign != oldSign[channel]) {
                oldSign[channel] = 0;
                localStep[channel] = localStep[channel]/2;
            }
            int val = (values[channel]+(localStep[channel]*sign));
            if (val < 0) val = 0;
            values[channel] = val;
            std::cout << "--> Adding new value " << values[channel]+(localStep[channel]*sign) << std::endl;
            m_done = last;
            if (val < 50) m_done = true;
//            fbMutex.unlock();
			keeper->mutexMap[channel]->unlock();
			std::cout << "Used channel #" << channel << std::endl;		// Ingrid likes debugs
			//fbMutex.unlock();
        }
        
        void feedbackBinary(double sign, bool last = false) {
            int val = (cur+(step*sign));
            if (val < 0) val = 0;
            values[0] = val;
            std::cout << "--> Adding new value " << val << std::endl;
            step = step/2;
            m_done = last;
            if (step == 1) m_done = true;
//            fbMutex.unlock();
//			keeper->mutexMap[channel]->unlock();
        }
//		unsigned channel;
   protected:
        std::mutex fbMutex;
        std::map<unsigned, unsigned> values;
		std::map<unsigned, unsigned> localStep;
		std::map<unsigned, double> oldSign;
        unsigned cur;

};


template<typename T, unsigned mOffset, unsigned bOffset, unsigned mask, bool msbRight>
class Fei4GlobalFeedback : public Fei4GlobalFeedbackBase {
    public:
        Fei4GlobalFeedback(Field<T, mOffset, bOffset, mask, msbRight> Fei4GlobalCfg::*ref): parPtr(ref) { 
            loopType = typeid(this);
        };
        
        
    private:
        void init() {
            m_done = false;
            cur = 0;
			for(unsigned int k=0; k<keeper->feList.size(); k++) {
				if(keeper->feList[k]->getActive()) {	
					localStep[keeper->feList[k]->getChannel()] = step;
					values[keeper->feList[k]->getChannel()] = max;
					oldSign[keeper->feList[k]->getChannel()] = -1;
			    }
			}
            this->writePar();
        }

        void end() {}

        void execPart1() {
            g_stat->set(this, cur);
			for(unsigned int k=0; k<keeper->feList.size(); k++) {
				if(keeper->feList[k]->getActive()) {	
					keeper->mutexMap[keeper->feList[k]->getChannel()]->try_lock();
			    }
			}
		}

        void execPart2() {
			for(unsigned int k=0; k<keeper->feList.size(); k++) {
				if(keeper->feList[k]->getActive()) {
					std::cout << "waiting for channel " << 	keeper->feList[k]->getChannel() << std::endl;
					keeper->mutexMap[keeper->feList[k]->getChannel()]->lock();
			    }
			}

            cur++;//values[0];
            std::cout << "--> Received feedback: " << cur << std::endl;
            this->writePar();
        }

        void writePar() {
//			g_fe->writeRegister(parPtr, cur);
			for(unsigned int k=0; k<keeper->feList.size(); k++) {
				if(keeper->feList[k]->getActive()) {
					g_tx->setCmdEnable(1 << keeper->feList[k]->getChannel());
					keeper->feList[k]->writeRegister(parPtr, values[keeper->feList[k]->getChannel()]);
				}
			}
			g_tx->setCmdEnable(3);
        }


        Field<T, mOffset, bOffset, mask, msbRight> Fei4GlobalCfg::*parPtr;
};

template<typename T, unsigned mOffset, unsigned bOffset, unsigned mask, bool msbRight>
Fei4GlobalFeedbackBase* Fei4GlobalFeedbackBuilder(Field<T, mOffset, bOffset, mask, msbRight> Fei4GlobalCfg::*ref) {
        return new Fei4GlobalFeedback<T,mOffset,bOffset,mask,msbRight>(ref);
}

#endif
