#include "AllStdActions.h"

namespace StdDict {
     LoopActionBase* getLoopAction(std::string name) {
        if (name == "StdDataLoop") {
            return (LoopActionBase*) new StdDataLoop;
        } else {
            return NULL;
        }
     }
}
