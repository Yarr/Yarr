#ifndef __ROGUE_CONTROLLER_H__
#define __ROGUE_CONTROLLER_H__


#include "RogueTxCore.h"
#include "RogueRxCore.h"
#include "HwController.h"
#include "json.hpp"

using json=nlohmann::basic_json<std::map, std::vector, std::string, bool, std::int32_t, std::uint32_t, float>;

class RogueController : public HwController, public RogueTxCore, public RogueRxCore {
    public:
        RogueController() {} 
        void loadConfig(json &j);
	void setupMode() {
	}
	~RogueController();
};

#endif
