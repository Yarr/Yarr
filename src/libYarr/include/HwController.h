#ifndef HWCONTROLLER_H
#define HWCONTROLLER_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Abstract hardware controller
// # Date: Feb 2017
// ################################

#include <string>

#include "TxCore.h"
#include "RxCore.h"

#include "storage.hpp"

namespace HwControllerFeatures {
  /// Feature is present if the getStatus method is updated with stats during the run
  static const std::string STATS_IN_STATUS{"STATS_IN_STATUS"};
}

/**
 * Abstract hardware controller.
 *
 * Use TxCore and RxCore.
 */
class HwController : virtual public TxCore, virtual public RxCore {
    public:
        /** Configure controller */
        virtual void loadConfig(const json &j) = 0 ;

        size_t getPlotterThreads() const { return m_plotterThreads; }

        virtual void setupMode() {}
        virtual void runMode() {}
        virtual const json getStatus() { return json{}; };

        virtual bool hasFeature(const std::string &feature) { return false; }

        ~HwController() override = default;

    protected:
        size_t m_plotterThreads{4};
};

#endif
