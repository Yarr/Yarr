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
        // Step down feedback algorithm
        void feedback(unsigned channel, double sign, bool last = false) {
            // Calculate new step and val
            if (sign != oldSign[channel]) {
                oldSign[channel] = 0;
                localStep[channel] = localStep[channel]/2;
            }
            int val = (values[channel]+(localStep[channel]*sign));
            if (val > (int)max) val = max;
            if (val < 0) val = 0;
	        values[channel] = val;
            doneMap[channel] |= last;

            if (localStep[channel] == 1) {
				 doneMap[channel] = true;
			}
            
            // Abort if we are getting to low
            if (val < 50) {
				doneMap[channel] = true;
			}
            // Unlock the mutex to let the scan proceed
			keeper->mutexMap[channel].unlock();
        }

        // Binary search feedback algorithm
        void feedbackBinary(unsigned channel, double sign, bool last = false) {
            // Calculate new step and value
            int val = (values[channel]+(localStep[channel]*sign));
            if (val < 0) val = 0;
            values[channel] = val;
            localStep[channel]  = localStep[channel]/2;
			doneMap[channel] |= last;
             
            if (localStep[channel] == 1) {
				 doneMap[channel] = true;
			}
            
            // Unlock the mutex to let the scan proceed
			keeper->mutexMap[channel].unlock();
        }



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
			// Init all maps:
            for(unsigned int k=0; k<keeper->feList.size(); k++) {
				if(keeper->feList[k]->getActive()) {
			        unsigned ch = keeper->feList[k]->getRxChannel();
					localStep[ch] = step;
					values[ch] = max;
					oldSign[ch] = -1;
					doneMap[ch] = false;
			    }
			}
            this->writePar();
        }

        void end() {
			for(unsigned int k=0; k<keeper->feList.size(); k++) {
				if(keeper->feList[k]->getActive()) {	
                    unsigned ch = keeper->feList[k]->getRxChannel();
                    std::cout << " --> Final parameter of Fe " << ch << " is " << values[ch] << std::endl;
			    }
			}
        }

        void execPart1() {
            g_stat->set(this, cur);
            // Lock all mutexes if open
			for(unsigned int k=0; k<keeper->feList.size(); k++) {
				if(keeper->feList[k]->getActive()) {	
					keeper->mutexMap[keeper->feList[k]->getRxChannel()].try_lock();
			    }
			}
			m_done = allDone();
		}

        void execPart2() {
            // Wait for mutexes to be unlocked by feedback
			for(unsigned int k=0; k<keeper->feList.size(); k++) {
				if(keeper->feList[k]->getActive()) {
					keeper->mutexMap[keeper->feList[k]->getRxChannel()].lock();
                    if (verbose)
                        std::cout << " --> Received Feedback on Channel " 
                            << keeper->feList[k]->getRxChannel() << " with value: " 
                            << values[keeper->feList[k]->getRxChannel()] << std::endl;
			    }
			}
            cur++;
            this->writePar();
        }

        void writePar() {
			for(unsigned int k=0; k<keeper->feList.size(); k++) {
				if(keeper->feList[k]->getActive()) {
					g_tx->setCmdEnable(1 << keeper->feList[k]->getTxChannel());
					keeper->feList[k]->writeRegister(parPtr, values[keeper->feList[k]->getRxChannel()]);
                    while(!g_tx->isCmdEmpty());
				}
			}
			g_tx->setCmdEnable(keeper->getTxMask());
        }
        
        bool allDone() {
            for(unsigned int k=0; k<keeper->feList.size(); k++) {
                if(keeper->feList[k]->getActive()) {
                    unsigned ch = keeper->feList[k]->getRxChannel();
                    if (!doneMap[ch])
                        return false;
                }
            }
            return true;
        }

        Field<T, mOffset, bOffset, mask, msbRight> Fei4GlobalCfg::*parPtr;
};

template<typename T, unsigned mOffset, unsigned bOffset, unsigned mask, bool msbRight>
Fei4GlobalFeedbackBase* Fei4GlobalFeedbackBuilder(Field<T, mOffset, bOffset, mask, msbRight> Fei4GlobalCfg::*ref) {
        return new Fei4GlobalFeedback<T,mOffset,bOffset,mask,msbRight>(ref);
}

#endif
