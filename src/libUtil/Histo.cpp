#if 0
#include "Histo.h"

#include <iostream>
#include <iomanip>

template <class T>
Histo<T>::Histo(unsigned arg_nBins, double arg_xLow, double arg_xHigh) {
    nBins = arg_nBins;
    gLow = arg_xLow;
    gHigh = arg_xHigh;
    binWidth = (gHigh - gLow)/(double)nBins;
    entries = 0;
    this->init();
}

template <class T>
Histo<T>::~Histo() {
    delete data;
}

template <class T>
void Histo<T>::init() {
    data = new T[nBins];
    for (unsigned i=0; i<nBins; i++) {
        data[i] = 0;
    }
}

template <class T>
void Histo<T>::fill(double value) {
    entries++;
    if (value < gLow) {
        underflow++;
    } else if (value >= gHigh) {
        overflow++;
    } else {
        unsigned bin = (value - gLow)/binWidth;
        data[bin]++;
    }
}

template <class T>
void Histo<T>::print() {
    for (unsigned int i=0; i<nBins; i++)
        std::cout << "# " << std::setw(5) << i << " # ";
    std::cout << std::endl;
    for (unsigned int i=0; i<nBins; i++)
        std::cout << "  " << std::setw(5) << data[i] << "   "; 
    std::cout << std::endl;
    for (unsigned int i=0; i<nBins; i++)
        std::cout << "# " <<std::setw(5) << i << " # ";
    std::cout << std::endl;
}

template class Histo<int>;
template class Histo<double>;

template <class T>
Histo2d<T>::Histo2d(unsigned arg_nBinsX, double arg_xLow, double arg_xHigh,double arg_nBinsY, double arg_yLow, double arg_yHigh) : Histo<T>(arg_nBinsX*arg_nBinsY, arg_xLow, arg_xHigh) {
    nBinsX = arg_nBinsX;
    nBinsY = arg_nBinsY;
    xLow = arg_xLow;
    xHigh = arg_xHigh;
    yLow = arg_yLow;
    yHigh = arg_yHigh;
    binWidthX = (xHigh - xLow)/(double)nBinsX;
    binWidthY = (yHigh - yLow)/(double)nBinsY;
}

template <class T>
Histo2d<T>::~Histo2d() {

}

template <class T>
void Histo2d<T>::fill(double xValue, double yValue) {
    Histo<T>::entries++;
    if (xValue < xLow || yValue < yLow) {
        Histo<T>::underflow++;
    } else if (xValue >= xHigh || yValue >= yHigh) {
        Histo<T>::overflow++;
    } else {
        unsigned xbin = (xValue - xLow)/binWidthX;
        unsigned ybin = (yValue - yLow)/binWidthY;
        Histo<T>::data[xbin+(ybin*nBinsY)]++;
    }
}

template <class T>
void Histo2d<T>::print() {
    std::cout << "         ";
    for (unsigned int i=0; i<nBinsX; i++)
        std::cout << " # " << std::setw(5) << i << " # ";
    std::cout << std::endl;
    for (unsigned int y=0; y<nBinsY; y++) {
        std::cout << std::setw(6) << y << " # ";
        for (unsigned int x=0; x<nBinsX; x++)
            std::cout << "   " << std::setw(5) << Histo<T>::data[x+(y*nBinsX)] << "   "; 
        std::cout << std::endl;
    }
    std::cout << "         ";
    for (unsigned int i=0; i<nBinsX; i++)
        std::cout << " # " <<std::setw(5) << i << " # ";
    std::cout << std::endl;
}

template class Histo2d<int>;
template class Histo2d<double>;

#endif
