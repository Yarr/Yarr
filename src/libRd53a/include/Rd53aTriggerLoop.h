#ifndef RD53ATRIGGERLOOP_H
#define RD53ATRIGGERLOOP_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Trigger Loop for RD53A
// # Date: 02/2018
// ################################

#include "LoopActionBase.h"

class Rd53aTriggerLoop final: public LoopActionBase {
    public:
        Rd53aTriggerLoop();
 
       /**
        * Get the specified trigger count number
        */
        uint32_t getTrigCnt() const;
    
       /**
        * Set the trigger count number
        */
        void setTrigCnt(uint32_t /*cnt*/);
    
       /**
        * Set the trigger time in [s]? ( default = 10. )
        */
        void setTrigTime(double /*time*/);

       /**
        * Set the frequency of the trigger rate in [Hz] ( default = 1000. )
        */
        void setTrigFreq(double /*freq*/);
    
       /**
        * This function sets not only the internal trigger delay value in the class,
        * but also resets trigWords to be consistent with the specified size of the delay.
        */
        void setTrigDelay(uint32_t /*delay*/);
    
       /**
        * Set the edge mode
        * Note that the argument duration is not really used
        * right at the moment and hard-coded. (HO: 2018-MAY-27)
        */
        void setEdgeMode(uint32_t /*duration*/);
    
       /**
        * Reset the trigger words to the no-injection mode
        */
        void setNoInject();
        
        void writeConfig(json & /*config*/) override final;
        void loadConfig(json & /*config*/)  override final;

    private:
    
       /**
        * Concrete implementations are encapsulated in this Impl class
        * Only forward declaration is given in the header file.
        */
        class Impl;    
        std::unique_ptr<Impl> m_impl;
        
        void init()       override final;
        void execPart1()  override final;
        void execPart2()  override final;
        void end()        override final;
};

#endif
