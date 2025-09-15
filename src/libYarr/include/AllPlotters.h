#ifndef YARR_ALL_PLOTTERS_H
#define YARR_ALL_PLOTTERS_H

#include "Plotter.h"

#include <memory>
#include <string>
#include <vector>
#include <functional>

namespace StdDict {
    bool registerPlotter(std::string name,
                          std::function<std::unique_ptr<Plotter>()> f);
    std::unique_ptr<Plotter> getPlotter(std::string name);

    std::vector<std::string> listPlotters();
}

#endif
