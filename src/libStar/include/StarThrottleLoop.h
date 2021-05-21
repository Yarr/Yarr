#ifndef STARGLOBALFEEDBACK_H
#define STARGLOBALFEEDBACK_H

// #################################
// # Author:
// # Email:
// # Project: Yarr
// # Description: Star Global Feedback Loop action
// # Comment: 
// # Date: Oct 2018
// ################################


#include <iostream>
#include <queue>
#include <mutex>

#include "LoopActionBase.h"
#include "FeedbackBase.h"
#include "StarChips.h"

class StarGlobalFeedback : public LoopActionBase, public GlobalFeedbackReceiver {
    public:
        StarGlobalFeedback();
        StarGlobalFeedback(std::string subRegName);
        StarGlobalFeedback(Register StarCfg::*ref);

        void writeConfig(json &j);
        void loadConfig(json &j);

        // TODO should probably register a single function
        void feedback(unsigned channel, double sign, bool last = false);
        void feedbackBinary(unsigned channel, double sign, bool last = false);
        void feedback(unsigned channel, bool stop = false); //@@ for noise occ

        std::string getScannedParameterName(){return m_subRegName;}

    protected:
    private:
        Register StarCfg::*parPtr;
        SubRegister* StarCfg::*subRegPtr;
        std::string m_subRegName;
        int m_cur;

        std::mutex m_fbMutex;
        std::map<unsigned, int> m_values;
        std::map<unsigned, int> m_localStep;
        std::map<unsigned, int> m_oldSign;
        std::map<unsigned, bool> m_doneMap;

        void writePar();
        bool allDone();
        
        void init();
        void end();
        void execPart1();
        void execPart2();
};


#endif
