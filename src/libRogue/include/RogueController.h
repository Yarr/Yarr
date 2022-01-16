#ifndef __ROGUE_CONTROLLER_H__
#define __ROGUE_CONTROLLER_H__


#include "RogueTxCore.h"
#include "RogueRxCore.h"
#include "HwController.h"


#include "storage.hpp"

class RogueController : public HwController, public RogueTxCore, public RogueRxCore {
    public:
        RogueController() = default;
        void loadConfig(json const&j) override;
	void setupMode() override {
	}
	~RogueController() override;
	void GlobalMonitor(bool reset, int looptime){
		std::shared_ptr<RogueCom> com=RogueCom::getInstance();
		com->GlobalMonitor(reset, looptime);}
};

#endif
