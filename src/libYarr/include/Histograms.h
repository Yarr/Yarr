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

/// Create general 2D histogram with LoopStatus
std::unique_ptr<Histo2d> createHisto2d(const std::string &name,
        const std::string &xname, size_t xcount, double xlow, double xhigh,
        const std::string &yname, size_t ycount, double ylow, double yhigh,
        const std::string &zname, const LoopStatus &stat);

/// Create 2D histogram matching FrontEnd geometry
std::unique_ptr<Histo2d> createHistoMap(const std::string &name, const std::string &zAxis, unsigned nCol, unsigned nRow);

/// Create 2D histogram matching FrontEnd geometry, including Loop position
std::unique_ptr<Histo2d> createHistoMap(const std::string &name, const std::string &zAxis, unsigned nCol, unsigned nRow, const LoopStatus &stat);

/// Create general 3D histogram (16-bit)
template<typename DataT = uint16_t>
std::unique_ptr<Histo3dT<DataT>> createHisto3d(const std::string &name,
        const std::string &xname, size_t xcount, double xlow, double xhigh,
        const std::string &yname, size_t ycount, double ylow, double yhigh,
        const std::string &zname, size_t zcount, double zlow, double zhigh)
{
    auto histo = std::make_unique<Histo3dT<DataT>>
            (name,
             xcount, xlow, xhigh,
             ycount, ylow, yhigh,
             zcount, zlow, zhigh);
    histo->setXaxisTitle(xname);
    histo->setYaxisTitle(yname);
    histo->setZaxisTitle(zname);
    return histo;
}

/// Create 3D histogram matching frontend geometry
template<typename DataT = uint16_t>
std::unique_ptr<Histo3dT<DataT>> createHistoMap3d(const std::string &name,
        size_t nCol, size_t nRow,
        const std::string &zname, size_t zcount, double zlow, double zhigh)
{
    // NB these are base 0 axes
    auto histo = std::make_unique<Histo3dT<DataT>>
            (name,
             nCol, -0.5, nCol-0.5,
             nRow, -0.5, nRow-0.5,
             zcount, zlow, zhigh);
    histo->setXaxisTitle("Column");
    histo->setYaxisTitle("Row");
    histo->setZaxisTitle(zname);
    return histo;
}

/// Create 3D histogram matching frontend geometry with LoopStatus
template<typename DataT = uint16_t>
std::unique_ptr<Histo3dT<DataT>> createHistoMap3d(const std::string &name,
        size_t nCol, size_t nRow,
        const std::string &zname, size_t zcount, double zlow, double zhigh,
        const LoopStatus &stat)
{
    // NB these are base 0 axes
    auto histo = std::make_unique<Histo3dT<DataT>>
            (name,
             nCol, -0.5, nCol-0.5,
             nRow, -0.5, nRow-0.5,
             zcount, zlow, zhigh, stat);
    histo->setXaxisTitle("Column");
    histo->setYaxisTitle("Row");
    histo->setZaxisTitle(zname);
    return histo;
}

#endif
