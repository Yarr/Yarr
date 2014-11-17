#if 0
#ifndef HISTO_H
#define HISTO_H

template <class T>
class Histo {
    public:
        Histo(unsigned arg_nBins, double arg_xLow, double arg_xHigh);
        ~Histo();

        void fill(double value);
        void print();

    protected:
        void init();
        
        unsigned nBins;
        double gLow, gHigh;
        unsigned underflow;
        unsigned overflow;
        int entries;
        double binWidth;
        T *data;
    private:
};

extern template class Histo<int>;
extern template class Histo<double>;

template <class T>
class Histo2d : public Histo<T> {
    public:
        Histo2d(unsigned arg_nBinsX, double arg_xLow, double arg_xHigh, double arg_nBinsY, double arg_yLow, double arg_yHigh);
        ~Histo2d();

        void fill(double xValue, double yValue);
        void print();
    protected:
        unsigned nBinsX, nBinsY;
        double xLow, yLow;
        double xHigh, yHigh;
    private:
        double binWidthX;
        double binWidthY;

};

extern template class Histo2d<int>;
extern template class Histo2d<double>;

#endif
#endif
