#ifndef YARR_PLOTTER_H
#define YARR_PLOTTER_H

#include "HistogramBase.h"

class Plotter {
 public:
    virtual ~Plotter() = default;

    virtual void makePlots(bool doPlots, const std::string &outputDir,
                           const std::string &fe_name,
                           const HistogramBase &histo) = 0;

};

#endif
