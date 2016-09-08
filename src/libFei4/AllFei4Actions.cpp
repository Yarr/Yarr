#include "AllFei4Actions.h"

namespace Fei4Dict {
     LoopActionBase* getLoopAction(std::string name) {
        if (name == "Fei4DcLoop") {
            return (LoopActionBase*) new Fei4DcLoop;
        } else if (name == "Fei4MaskLoop") {
            return (LoopActionBase*) new Fei4MaskLoop;
        } else if (name == "Fei4TriggerLoop") {
            return (LoopActionBase*) new Fei4TriggerLoop;
        } else {
            return NULL;
        }
     }
}
