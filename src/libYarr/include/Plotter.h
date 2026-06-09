#ifndef YARR_PLOTTER_H
#define YARR_PLOTTER_H

#include "HistogramBase.h"
#include "ThreadPool.h"
#include "FrontEndCfg.h"
#include "storage.hpp"
#include <string>
#include <future>
#include <fstream>
#include <iomanip>
#include <stdexcept>

class Plotter {
public:
    virtual ~Plotter() = default;

    // One task per histogram (for individually independent plots, e.g. loop histograms).
    // Takes ownership of histo to avoid dangling reference across thread boundary.
    std::future<void> makePlots(bool doPlots, const std::string &outputDir,
                                const std::string &fe_name,
                                std::unique_ptr<HistogramBase> histo,
                                ThreadPool &pool)
    {
        return pool.enqueue([this, doPlots, outputDir, fe_name, h = std::move(histo)]() {
            this->implToFile(outputDir, fe_name, *h);
            if (doPlots) this->implPlot(outputDir, fe_name, *h);
        });
    }

    // One task per FE; takes ownership of histos and processes them serially in that task.
    std::future<void> makePlotsForFe(bool doPlots, const std::string &outputDir,
                                     const std::string &fe_name,
                                     std::vector<std::unique_ptr<HistogramBase>> histos,
                                     ThreadPool &pool)
    {
        if (histos.empty()) {
            std::promise<void> p;
            p.set_value();
            return p.get_future();
        }
        return pool.enqueue([this, doPlots, outputDir, fe_name, histos = std::move(histos)]() mutable {
            for (auto &histo : histos) {
                this->implToFile(outputDir, fe_name, *histo);
                if (doPlots) this->implPlot(outputDir, fe_name, *histo);
            }
        });
    }

    std::future<void> writeFeConfig(FrontEndCfg *feCfg, const std::string &filename, ThreadPool &pool) {
        return pool.enqueue([feCfg, filename]() {
            json backupCfg;
            feCfg->writeConfig(backupCfg);
            std::ofstream backupCfgFile(filename);
            if (!backupCfgFile.is_open())
                throw std::runtime_error("Failed to open config file for writing: " + filename);
            backupCfgFile << std::setw(4) << backupCfg;
        });
    }

protected:
    virtual void implToFile(const std::string &outputDir,
                            const std::string &fe_name,
                            const HistogramBase &histo) = 0;

    virtual void implPlot(const std::string &outputDir,
                          const std::string &fe_name,
                          const HistogramBase &histo) = 0;
};

#endif
