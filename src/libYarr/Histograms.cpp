#include "Histograms.h"

/// Create general 1D histogram
std::unique_ptr<Histo1d> createHisto1d(const std::string &name,
        const std::string &xname, size_t xcount, double xlow, double xhigh,
        const std::string &yname)
{
    auto histo = std::make_unique<Histo1d>
            (name, xcount, xlow, xhigh);
    histo->setXaxisTitle(xname);
    histo->setYaxisTitle(yname);
    return histo;
}

/// Create general 1D histogram with LoopStatus
std::unique_ptr<Histo1d> createHisto1d(const std::string &name,
        const std::string &xname, size_t xcount, double xlow, double xhigh,
        const std::string &yname, const LoopStatus &stat)
{
    auto histo = std::make_unique<Histo1d>
            (name, xcount, xlow, xhigh, stat);
    histo->setXaxisTitle(xname);
    histo->setYaxisTitle(yname);
    return histo;
}

/// Create general 2D histogram
std::unique_ptr<Histo2d> createHisto2d(const std::string &name,
        const std::string &xname, size_t xcount, double xlow, double xhigh,
        const std::string &yname, size_t ycount, double ylow, double yhigh,
        const std::string &zname)
{
    auto histo = std::make_unique<Histo2d>
            (name,
             xcount, xlow, xhigh,
             ycount, ylow, yhigh);
    histo->setXaxisTitle(xname);
    histo->setYaxisTitle(yname);
    histo->setZaxisTitle(zname);
    return histo;
}

/// Create 2D histogram matching FrontEnd geometry
std::unique_ptr<Histo2d> createHistoMap(const std::string &name, const std::string &zAxis, unsigned nCol, unsigned nRow) {
    auto histo = std::make_unique<Histo2d>(name, nCol, 0.5, nCol + 0.5, nRow, 0.5, nRow + 0.5);
    histo->setXaxisTitle("Column");
    histo->setYaxisTitle("Row");
    histo->setZaxisTitle(zAxis);
    return histo;
}

/// Create 2D histogram matching FrontEnd geometry, including Loop position
std::unique_ptr<Histo2d> createHistoMap(const std::string &name, const std::string &zAxis, unsigned nCol, unsigned nRow, const LoopStatus &stat) {
    auto histo = std::make_unique<Histo2d>(name, nCol, 0.5, nCol + 0.5, nRow, 0.5, nRow + 0.5, stat);
    histo->setXaxisTitle("Column");
    histo->setYaxisTitle("Row");
    histo->setZaxisTitle(zAxis);
    return histo;
}

/// Create general 3D histogram (16-bit)
std::unique_ptr<Histo3d> createHisto3d(const std::string &name,
        const std::string &xname, size_t xcount, double xlow, double xhigh,
        const std::string &yname, size_t ycount, double ylow, double yhigh,
        const std::string &zname, size_t zcount, double zlow, double zhigh)
{
    auto histo = std::make_unique<Histo3d>
            (name,
             xcount, xlow, xhigh,
             ycount, ylow, yhigh,
             zcount, zlow, zhigh);
    histo->setXaxisTitle(xname);
    histo->setYaxisTitle(yname);
    histo->setZaxisTitle(zname);
    return histo;
}

/// Create 3D histogram (float) matching frontend geometry
std::unique_ptr<Histo3dT<float>> createHistoMap3d(const std::string &name,
        size_t nCol, size_t nRow,
        const std::string &zname, size_t zcount, double zlow, double zhigh)
{
    // NB these are base 0 axes
    auto histo = std::make_unique<Histo3dT<float>>
            (name,
             nCol, -0.5, nCol-0.5,
             nRow, -0.5, nRow-0.5,
             zcount, zlow, zhigh);
    histo->setXaxisTitle("Column");
    histo->setYaxisTitle("Row");
    histo->setZaxisTitle(zname);
    return histo;
}
