// #################################
// # Author: Eunchong Kim
// # Email: eunchong.kim at cern.ch
// # Project: masspro(Yarr)
// # Description: Graph with Errors
// # Comment: 
// ################################

#include "GraphErrors.h"
#include <stdio.h>
#include <cstring>

GraphErrors::GraphErrors(std::string arg_name, int arg_n, const double *arg_x, const double *arg_y, const double *arg_x_err, const double *arg_y_err, std::type_index t) : HistogramBase(arg_name, t) {
    data_n = arg_n;
    data_x = new double[data_n];
    data_y = new double[data_n];
    data_x_err = new double[data_n]();
    data_y_err = new double[data_n]();
    std::memcpy(data_x, arg_x, data_n*sizeof(double));
    std::memcpy(data_y, arg_y, data_n*sizeof(double));
    if (arg_x_err) {
        is_x_err = true;
        std::memcpy(data_x_err, arg_x_err, data_n*sizeof(double));
    }
    else is_x_err = false;
    if (arg_y_err) {
        is_y_err = true;
        std::memcpy(data_y_err, arg_y_err, data_n*sizeof(double));
    }
    else is_y_err = false;

    is_xmin_limit = false;
    is_xmax_limit = false;
}

GraphErrors::GraphErrors(std::string arg_name, int arg_n, const double *arg_x, const double *arg_y, const double *arg_x_err, const double *arg_y_err, std::type_index t, LoopStatus &stat) : HistogramBase(arg_name, t, stat) {
    data_n = arg_n;
    data_x = new double[data_n];
    data_y = new double[data_n];
    data_x_err = new double[data_n]();
    data_y_err = new double[data_n]();
    std::memcpy(data_x, arg_x, data_n*sizeof(double));
    std::memcpy(data_y, arg_y, data_n*sizeof(double));
    if (arg_x_err) {
        is_x_err = true;
        std::memcpy(data_x_err, arg_x_err, data_n*sizeof(double));
    }
    else is_x_err = false;
    if (arg_y_err) {
        is_y_err = true;
        std::memcpy(data_y_err, arg_y_err, data_n*sizeof(double));
    }
    else is_y_err = false;

    is_xmin_limit = false;
    is_xmax_limit = false;
}

GraphErrors::~GraphErrors() {
    delete[] data_x;
    delete[] data_y;
    delete[] data_x_err;
    delete[] data_y_err;
}

unsigned GraphErrors::size() const {
    return data_n;
}

//unsigned GraphErrors::getEntries() const {
//    return entries;
//}
//
//double GraphErrors::getMean() {
//    if (sum == 0)
//        return 0;
//    double weighted_sum = 0;
//    for (unsigned i=0; i<bins; i++)
//        weighted_sum += data[i]*(((i+1)*binWidth)+xlow+(binWidth/2.0));
//    return weighted_sum/sum;
//}
//
//double GraphErrors::getStdDev() {
//    if (sum == 0)
//        return 0;
//    double mean = this->getMean();
//    double mu = 0;
//    for (unsigned i=0; i<bins; i++)
//        mu += data[i] * pow((((i+1)*binWidth)+xlow+(binWidth/2.0))-mean,2);
//    return sqrt(mu/(double)sum);
//}
//
//void GraphErrors::fill(double x, double v) {
//    if (x < xlow) {
//        underflow+=v;
//    } else if (xhigh <= x) {
//        overflow+=v;
//    } else {
//        unsigned bin = (x-xlow)/binWidth;
//        data[bin]+=v;
//        if (v > max)
//            max = v;
//        if (v < min)
//            min = v;
//    }
//    entries++;
//    sum+=v;
//}
//
//void GraphErrors::setBin(unsigned data_n, double v) {
//    if (data_n < bins) {
//        data[data_n] = v;
//        if (v > max)
//            max = v;
//        if (v < min)
//            min = v;
//    }
//    entries++;
//    sum+=v;
//}
//
//double GraphErrors::getBin(unsigned data_n) const {
//    if (data_n < bins) {
//        return data[data_n];
//    }
//    return 0;
//}
//
//void GraphErrors::scale(const double s) {
//    for (unsigned int i=0; i<bins; i++) {
//        data[i] = data[i] * s;
//    }
//    sum = sum*s;
//}
//
//void GraphErrors::add(const GraphErrors &h) {
//    if (h.size() != bins) {
//        return;
//    } else {
//        for (unsigned i=0; i<bins; i++) {
//            data[i] += h.getBin(i);
//            sum += h.getBin(i);
//        }
//        entries += h.getEntries();
//    }
//}

void GraphErrors::setXaxisMinimum(double arg_xmin) {
    is_xmin_limit = true;  
    xmin_limit = arg_xmin;
}

void GraphErrors::setXaxisMaximum(double arg_xmax) {
    is_xmax_limit = true;  
    xmax_limit = arg_xmax;
}

void GraphErrors::toFile(std::string prefix, std::string dir, bool header) {
    std::string filename = dir + prefix + "_" + HistogramBase::name + ".dat";
    std::fstream file(filename, std::fstream::out | std::fstream::trunc);
    // Header
    if (header) {
        file << "GraphErrors " << std::endl;
        file << name << std::endl;
        file << xAxisTitle << std::endl;
        file << yAxisTitle << std::endl;
        file << zAxisTitle << std::endl;
//        file << bins << " " << xlow << " " << xhigh << std::endl;
//        file << underflow << " " << overflow << std::endl;
    }
    // Data
    for (int i=0; i<data_n; i++) {
        file << data_x[i] << " " << data_y[i] << " " << data_x_err[i] << " " << data_y_err[i] << std::endl;
    }
    file.close();
}

void GraphErrors::plot(std::string prefix, std::string dir) {
    // Put raw histo data in tmp file
    std::string tmp_name = std::string(getenv("USER")) + "/tmp_yarr_histo1d_" + prefix;
    this->toFile(tmp_name, "/tmp/", false);
    std::string cmd = "gnuplot | epstopdf -f > " + dir + prefix + "_" + HistogramBase::name + ".pdf";

    // Open gnuplot as file and pipe commands
    FILE *gnu = popen(cmd.c_str(), "w");
    
    fprintf(gnu, "set terminal postscript enhanced color \"Helvetica\" 18 eps\n");
    fprintf(gnu, "unset key\n");
    fprintf(gnu, "set title \"%s\"\n" , HistogramBase::name.c_str());
    fprintf(gnu, "set xlabel \"%s\"\n" , HistogramBase::xAxisTitle.c_str());
    fprintf(gnu, "set ylabel \"%s\"\n" , HistogramBase::yAxisTitle.c_str());
    if (is_xmin_limit && is_xmax_limit) fprintf(gnu, "set xrange[%f:%f]\n", xmin_limit, xmax_limit);
    else if (is_xmin_limit) fprintf(gnu, "set xrange[%f:]\n", xmin_limit);
    else if (is_xmax_limit) fprintf(gnu, "set xrange[:%f]\n", xmax_limit);
//    fprintf(gnu, "set yrange[0:*]\n");
//    fprintf(gnu, "set grid\n");
//    fprintf(gnu, "set style line 1 lt 1 lc rgb '#A6CEE3'\n");
//    fprintf(gnu, "set style fill solid 0.5\n");
    fprintf(gnu, "plot \"%s\" using 1:2", ("/tmp/" + tmp_name + "_" + name + ".dat").c_str());
    if (is_x_err && is_y_err) fprintf(gnu, ":3:4 with xyerrorbars");
    else if (is_x_err) fprintf(gnu, ":3 with xerrorbars");
    else if (is_y_err) fprintf(gnu, ":4 with yerrorbars");
    fprintf(gnu, " pointtype 7 lc rgb \"black\" \n");
    pclose(gnu);
}
