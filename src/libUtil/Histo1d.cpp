// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: 1D Histogram
// # Comment: 
// ################################

#include "Histo1d.h"

#include <iostream>
#include <fstream>
#include <cmath>
#include <iomanip>

#include "storage.hpp"

#include "logging.h"

namespace {
    auto hlog = logging::make_log("Histo1d");
}

Histo1d::Histo1d(const std::string &arg_name, unsigned arg_bins, double arg_xlow, double arg_xhigh) : HistogramBase(arg_name) {
    bins = arg_bins;
    xlow = arg_xlow;
    xhigh = arg_xhigh;
    binWidth = (xhigh - xlow)/bins;
    data =  std::vector<double>(bins,0);
    min = 0;
    max = 0;

    underflow = 0;
    overflow = 0;
    entries = 0;
    sum = 0;
}

Histo1d::Histo1d(const std::string &arg_name, unsigned arg_bins, double arg_xlow, double arg_xhigh, const LoopStatus &stat) : HistogramBase(arg_name, stat) {
    bins = arg_bins;
    xlow = arg_xlow;
    xhigh = arg_xhigh;
    binWidth = (xhigh - xlow)/bins;
    data = std::vector<double>(bins,0);

    min = 0;
    max = 0;

    underflow = 0;
    overflow = 0;
    entries = 0;
    sum = 0;
}

Histo1d::~Histo1d() {

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
    overflow = overflow*s;
    underflow = underflow*s;
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
        overflow += h.getOverflow();
        underflow += h.getUnderflow();
    }
}

void Histo1d::toStream(std::ostream &out) const {
    //  Only Data
    for (unsigned int i=0; i<bins; i++) {
        out << data[i] << " ";
    }
    out << std::endl;
}

void Histo1d::toJson(json &j) const {
    j["Type"] = "Histo1d";
    j["Name"] = name;

    j["x"]["AxisTitle"] = xAxisTitle;
    j["x"]["Bins"] = bins;
    j["x"]["Low"] = xlow;
    j["x"]["High"] = xhigh;

    j["y"]["AxisTitle"] = yAxisTitle;

    j["z"]["AxisTitle"] = zAxisTitle;

    j["Underflow"] = underflow;
    j["Overflow"] = overflow;

    for (unsigned int i=0; i<bins; i++)
        j["Data"][i] = data[i];
}

void Histo1d::toFile(const std::string &prefix, const std::string &dir, bool jsonType) const{
    std::string filename = dir + prefix + "_" + HistogramBase::name;
    if (jsonType) {
        filename += ".json";
    } else {
        filename += ".dat";
    }
    std::fstream file(filename, std::fstream::out | std::fstream::trunc);
    json j;
    // jsonType
    if (jsonType) {
        toJson(j);
        file << std::setw(4) << j;
    } else {
       toStream(file);
    }
    file.close();
}

bool Histo1d::fromFile(const std::string &filename) {
    std::ifstream file(filename, std::fstream::in);
    json j;
    try {
        if (!file) {
            throw std::runtime_error("could not open file");
        }
        try {
            j = json::parse(file);
        } catch (json::parse_error &e) {
            throw std::runtime_error(e.what());
        }
    } catch (std::runtime_error &e) {
        hlog->error("Error opening histogram: {}", e.what());
        return false;
    }
    // Check for type
    if (j["Type"].empty()) {
        hlog->error("Tried loading 1d Histogram from file {}, but file has no header: {}", filename);
        return false;
    } else {
        if (j["Type"] == "Histo1d") {
            hlog->error("Tried loading 1d Histogram from file {}, but file has incorrect header: {}", filename, std::string(j["Type"]));
            return false;
        }

        name = j["Name"];
        xAxisTitle = j["x"]["AxisTitle"];
        yAxisTitle = j["y"]["AxisTitle"];
        zAxisTitle = j["z"]["AxisTitle"];

        bins = j["x"]["Bins"];
        xlow = j["x"]["Low"];
        xhigh = j["x"]["High"];

        underflow = j["underflow"];
        overflow = j["overflow"];

        data.resize(bins);
        for (unsigned i=0; i<bins; i++)
            data[i] = j["data"][i];
    }
    file.close();
    return true;
}

void Histo1d::plot(const std::string &prefix, const std::string &dir) const {
    hlog->info("Plotting {}", HistogramBase::name);
    // Put raw histo data in tmp file
    std::string tmp_name = std::string(getenv("USER")) + "/tmp_yarr_histo2d_" + prefix;
    std::string output = dir + prefix + "_" + HistogramBase::name;
    for (unsigned i=0; i<lStat.size(); i++)
        output += "_" + std::to_string(lStat.get(i));
    output += ".png";
    std::string input;
    input+="$'set terminal png size 1280, 1024;";
    input+="unset key;";
    input+="set xlabel \""  +HistogramBase::xAxisTitle+"\";";
    input+="set ylabel \""  +HistogramBase::yAxisTitle+"\";";
    input+="set xrange["+ std::to_string(xlow)+ ":"+std::to_string(xhigh)+ "];";
    input+="set yrange[0:*];";
    input+="set grid;";
    input+="set style line 1 lt 1 lc rgb \\'#A6CEE3\\';";
    input+="set style fill solid 0.5;";
    input+="set boxwidth "+std::to_string(binWidth)+"*0.9 absolute;";
    input+="plot \\'-\\' matrix u ((($1)*("+std::to_string(binWidth);
    input+="))+"+std::to_string(xlow)+"+(" + std::to_string(binWidth)+"/2)):3 with boxes'";
    std::string cmd="gnuplot  -e "+input+" > "+output+"\n";
    FILE *gnu = popen(cmd.c_str(), "w");
    std::stringstream ss;
    toStream(ss);
    fprintf(gnu,"%s",ss.str().c_str());
    pclose(gnu);
}
