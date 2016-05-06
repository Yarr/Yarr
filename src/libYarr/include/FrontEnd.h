#ifndef FRONTEND_H
#define FRONTEND_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Abstract FE class
// # Comment: Combined multiple FE 
// ################################

#include <string>

#include "TxCore.h"

class FrontEnd {
    public:
        FrontEnd() {}
        ~FrontEnd() {}
		
        unsigned getChannel();
		unsigned getTxChannel();
		unsigned getRxChannel();
        std::string getName();
     
		void setChannel(unsigned channel);
		void setChannel(unsigned arg_txChannel, unsigned arg_rxChannel);
        void setName(std::string arg_name);

        bool getActive();
		bool isActive();
		void setActive(bool active);

    protected:
        std::string name;
        bool active;
        unsigned txChannel;
        unsigned rxChannel;
};


#endif
