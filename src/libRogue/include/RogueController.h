#ifndef __ROGUE_CONTROLLER_H__
#define __ROGUE_CONTROLLER_H__


#include "RogueTxCore.h"
#include "RogueRxCore.h"
#include "HwController.h"


#include "storage.hpp"

class RogueController : public HwController, public RogueTxCore, public RogueRxCore {
    public:
        RogueController() {} 
        void loadConfig(json &j);
	void setupMode() {
	}
	~RogueController();
};

#endif
