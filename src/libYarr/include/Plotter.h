#ifndef YARR_PLOTTER_H
#define YARR_PLOTTER_H

#include "HistogramBase.h"
#include "ThreadPool.h"
#include "FrontEndCfg.h"
#include <memory>
#include <string>
#include <future>

class Plotter {
public:
    virtual ~Plotter() = default;

    virtual std::future<void> makePlots(bool doPlots, const std::string &outputDir,
                                        const std::string &fe_name,
                                        std::unique_ptr<HistogramBase> histo,
                                        ThreadPool &pool) = 0;

    virtual std::future<void> writeFeConfig(FrontEndCfg *feCfg, const std::string &filename,
                                            ThreadPool &pool) = 0;
};

#endif
