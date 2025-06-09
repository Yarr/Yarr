#ifndef YARR_HISTO_BUILDER
#define YARR_HISTO_BUILDER

#include <memory>

#include "Histo1d.h"
#include "Histo2d.h"
#include "Histo3d.h"

/// Create general 1D histogram
std::unique_ptr<Histo1d> createHisto1d(const std::string &name,
        const std::string &xname, size_t xcount, double xlow, double xhigh,
        const std::string &yname);

/// Create general 1D histogram with LoopStatus
std::unique_ptr<Histo1d> createHisto1d(const std::string &name,
        const std::string &xname, size_t xcount, double xlow, double xhigh,
        const std::string &yname, const LoopStatus &stat);

/// Create general 2D histogram
std::unique_ptr<Histo2d> createHisto2d(const std::string &name,
        const std::string &xname, size_t xcount, double xlow, double xhigh,
        const std::string &yname, size_t ycount, double ylow, double yhigh,
        const std::string &zname);

/// Create 2D histogram matching FrontEnd geometry
std::unique_ptr<Histo2d> createHistoMap(const std::string &name, const std::string &zAxis, unsigned nCol, unsigned nRow);

/// Create 2D histogram matching FrontEnd geometry, including Loop position
std::unique_ptr<Histo2d> createHistoMap(const std::string &name, const std::string &zAxis, unsigned nCol, unsigned nRow, const LoopStatus &stat);

/// Create general 3D histogram (16-bit)
std::unique_ptr<Histo3d> createHisto3d(const std::string &name,
        const std::string &xname, size_t xcount, double xlow, double xhigh,
        const std::string &yname, size_t ycount, double ylow, double yhigh,
        const std::string &zname, size_t zcount, double zlow, double zhigh);

/// Create 3D histogram (float) matching frontend geometry
std::unique_ptr<Histo3dT<float>> createHistoMap3d(const std::string &name,
        size_t nCol, size_t nRow,
        const std::string &zname, size_t zcount, double zlow, double zhigh);

#endif
