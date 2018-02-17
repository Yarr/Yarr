#ifndef ALL_HW_CONTROLLERS_H
#define ALL_HW_CONTROLLERS_H

#include "HwController.h"

#include <string>

namespace StdDict {
    bool registerHwController(std::string name,
                              std::function<std::unique_ptr<HwController>()> f);
    std::unique_ptr<HwController> getHwController(std::string name);

    std::vector<std::string> listHwControllers();
}

#endif
