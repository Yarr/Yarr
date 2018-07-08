#ifndef GraphErrors_H
#define GraphErrors_H

// #################################
// # Author: Eunchong Kim
// # Email: eunchong.kim at cern.ch
// # Project: masspro(Yarr)
// # Description: Graph with Errors
// # Comment: 
// ################################

#include <iostream>
#include <fstream>
#include <string>
#include <typeinfo>
#include <typeindex>
#include <cmath>

#include "HistogramBase.h"

class GraphErrors : public HistogramBase {
    public:
        GraphErrors(std::string arg_name, int arg_n, const double *arg_x, const double *arg_y, const double *arg_x_err, const double *arg_y_err, std::type_index t);
        GraphErrors(std::string arg_name, int arg_n, const double *arg_x, const double *arg_y, const double *arg_x_err, const double *arg_y_err, std::type_index t, LoopStatus &stat);
        ~GraphErrors();
        
        unsigned size() const;
        //unsigned getEntries() const;
        //double getMean();
        //double getStdDev();

        //void fill(double x, double v=1);

        //void scale(const double s);
        //void add(const Histo1d &h);
        
        //void setBin(unsigned n, double v);
        //double getBin(unsigned n) const;
        //double* getData() { return data;};

        void setXaxisMinimum(double);
        void setXaxisMaximum(double);
        
        void toFile(std::string filename, std::string dir = "", bool header=true);
        void plot(std::string filename, std::string dir);

    private:
        int data_n;
        double *data_x;
        double *data_x_err;
        double *data_y;
        double *data_y_err;
        bool is_x_err;
        bool is_y_err;
        bool is_xmin_limit;
        bool is_xmax_limit;
        double xmin_limit;
        double xmax_limit;
        //double binWidth;

        //double underflow;
        //double overflow;
        //double max;
        //double min;
        //unsigned entries;
        //double sum;
};

#endif
