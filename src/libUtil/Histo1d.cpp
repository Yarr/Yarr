// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: 1D Histogram
// # Comment: 
// ################################

#include "Histo1d.h"
#include <stdio.h>

Histo1d::Histo1d(std::string arg_name, unsigned arg_bins, double arg_xlow, double arg_xhigh, std::type_index t) : HistogramBase(arg_name, t) {
    bins = arg_bins;
    xlow = arg_xlow;
    xhigh = arg_xhigh;
    binWidth = (xhigh - xlow)/bins;
    data = new double[bins];
    for(unsigned i=0; i<bins; i++)
        data[i] = 0;
    min = 0;
    max = 0;

    underflow = 0;
    overflow = 0;
    entries = 0;
    sum = 0;
}

Histo1d::Histo1d(std::string arg_name, unsigned arg_bins, double arg_xlow, double arg_xhigh, std::type_index t, LoopStatus &stat) : HistogramBase(arg_name, t, stat) {
    bins = arg_bins;
    xlow = arg_xlow;
    xhigh = arg_xhigh;
    binWidth = (xhigh - xlow)/bins;
    data = new double[bins];
    for(unsigned i=0; i<bins; i++)
        data[i] = 0;
    min = 0;
    max = 0;

    underflow = 0;
    overflow = 0;
    entries = 0;
    sum = 0;
}

Histo1d::~Histo1d() {
    delete[] data;
}

unsigned Histo1d::size() const {
    return bins;
}

unsigned Histo1d::getEntries() const {
    return entries;
}

double Histo1d::getMean() {
    if (sum == 0 || entries == 0)
        return 0;
    double weighted_sum = 0;
    double n = 0;
    for (unsigned i=0; i<bins; i++) {
        weighted_sum += data[i]*(((i)*binWidth)+xlow+(binWidth/2.0));
        n += data[i];
    }
    if (n == 0) {
        return 0;
    }
    return weighted_sum/n;
}

double Histo1d::getStdDev() {
    if (sum == 0 || entries == 0)
        return 0;
    double mean = this->getMean();
    double mu = 0;
    for (unsigned i=0; i<bins; i++)
        mu += data[i] * pow((((i)*binWidth)+xlow+(binWidth/2.0))-mean,2);
    return sqrt(mu/(double)sum);
}

void Histo1d::fill(double x, double v) {
    if (x < xlow) {
        underflow+=v;
    } else if (xhigh <= x) {
        overflow+=v;
    } else {
        unsigned bin = (x-xlow)/binWidth;
        data[bin]+=v;
        if (v > max)
            max = v;
        if (v < min)
            min = v;
        entries++;
        sum+=v;
    }
}

void Histo1d::setBin(unsigned n, double v) {
    if (n < bins) {
        data[n] = v;
        if (v > max)
            max = v;
        if (v < min)
            min = v;
    }
    entries++;
    sum+=v;
}

double Histo1d::getBin(unsigned n) const {
    if (n < bins) {
        return data[n];
    }
    return 0;
}

void Histo1d::scale(const double s) {
    for (unsigned int i=0; i<bins; i++) {
        data[i] = data[i] * s;
    }
    sum = sum*s;
}

void Histo1d::add(const Histo1d &h) {
    if (h.size() != bins) {
        return;
    } else {
        for (unsigned i=0; i<bins; i++) {
            data[i] += h.getBin(i);
            sum += h.getBin(i);
        }
        entries += h.getEntries();
    }
}


void Histo1d::toFile(std::string prefix, std::string dir, bool header) {
    std::string filename = dir + prefix + "_" + HistogramBase::name + ".dat";
    std::fstream file(filename, std::fstream::out | std::fstream::trunc);
    // Header
    if (header) {
        file << "Histo1d " << std::endl;
        file << name << std::endl;
        file << xAxisTitle << std::endl;
        file << yAxisTitle << std::endl;
        file << zAxisTitle << std::endl;
        file << bins << " " << xlow << " " << xhigh << std::endl;
        file << underflow << " " << overflow << std::endl;
    }
    // Data
    for (unsigned int i=0; i<bins; i++) {
        file << data[i] << " ";
    }
    file << std::endl;
    file.close();
}

void Histo1d::plot(std::string prefix, std::string dir) {
    std::cout << "Plotting: " << HistogramBase::name << std::endl;
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
    fprintf(gnu, "set xrange[%f:%f]\n", xlow, xhigh);
    fprintf(gnu, "set yrange[0:*]\n");
    //fprintf(gnu, "set \n");
    fprintf(gnu, "set grid\n");
    fprintf(gnu, "set style line 1 lt 1 lc rgb '#A6CEE3'\n");
    fprintf(gnu, "set style fill solid 0.5\n");
    fprintf(gnu, "set boxwidth %f*0.9 absolute\n", binWidth);
    fprintf(gnu, "plot \"%s\" matrix u ((($1)*(%f))+%f+(%f/2)):3 with boxes\n", ("/tmp/" + tmp_name + "_" + name + ".dat").c_str(), binWidth, xlow, binWidth);
    pclose(gnu);
}
