#include "Fei4DcLoop.h"
#include "Fei4TriggerLoop.h"
#include "Fei4MaskLoop.h"
#include "Fei4ParameterLoop.h"
#include "Fei4GlobalFeedback.h"
#include "Fei4PixelFeedback.h"
#include "Fei4NoiseScan.h"

#ifndef ALLFEI4ACTIONS_H
#define ALLFEI4ACTIONS_H

#include "LoopActionBase.h"
#include <string>

namespace Fei4Dict {
    LoopActionBase* getLoopAction(std::string name);
}

#endif
