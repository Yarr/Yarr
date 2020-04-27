// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: 2D Histogram
// # Comment: 
// ################################

#include "Histo2d.h"

#include <cmath>
#include <fstream>
#include <iostream>
#include <iomanip>

#include "storage.hpp"

#include "logging.h"

namespace {
    auto hlog = logging::make_log("Histo2d");
}

Histo2d::Histo2d(std::string arg_name, unsigned arg_xbins, double arg_xlow, double arg_xhigh, 
        unsigned arg_ybins, double arg_ylow, double arg_yhigh, std::type_index t) : HistogramBase(arg_name, t) {
    xbins = arg_xbins;
    xlow = arg_xlow;
    xhigh = arg_xhigh;
    xbinWidth = (xhigh - xlow)/xbins;
    
    ybins = arg_ybins;
    ylow = arg_ylow;
    yhigh = arg_yhigh;
    ybinWidth = (yhigh - ylow)/ybins;
    
    min = 0;
    max = 0;
    underflow = 0;
    overflow = 0;
    data = std::vector<double>(xbins*ybins,0);
    isFilled = std::vector<bool>(xbins*ybins,false);
    entries = 0;

}

Histo2d::Histo2d(std::string arg_name, unsigned arg_xbins, double arg_xlow, double arg_xhigh, 
        unsigned arg_ybins, double arg_ylow, double arg_yhigh, std::type_index t, LoopStatus &stat) : HistogramBase(arg_name, t, stat) {
    xbins = arg_xbins;
    xlow = arg_xlow;
    xhigh = arg_xhigh;
    xbinWidth = (xhigh - xlow)/xbins;
    
    ybins = arg_ybins;
    ylow = arg_ylow;
    yhigh = arg_yhigh;
    ybinWidth = (yhigh - ylow)/ybins;
    
    min = 0;
    max = 0;
    underflow = 0;
    overflow = 0;
    data = std::vector<double>(xbins*ybins,0);
    isFilled =  std::vector<bool>(xbins*ybins,false);
    entries = 0;
}

Histo2d::Histo2d(Histo2d *h) : HistogramBase(h->getName(), h->getType()) {
    xbins = h->getXbins();
    xlow = h->getXlow();
    xhigh = h->getXhigh();
    xbinWidth = h->getXbinWidth();

    ybins = h->getYbins();
    ylow = h->getYlow();
    yhigh = h->getYhigh();
    ybinWidth = h->getYbinWidth();

    min = h->getMin();
    max = h->getMax();
    underflow = h->getUnderflow();
    overflow = h->getOverflow();

    data = std::vector<double>(xbins*ybins,0);
    isFilled = std::vector<bool>(xbins*ybins,false);
    for(unsigned i=0; i<xbins*ybins; i++)
        data[i] = h->getBin(i);
    entries = h->getNumOfEntries();
    lStat = h->getStat();
}

Histo2d::~Histo2d() {
}

unsigned Histo2d::size() const {
    return xbins*ybins;
}

unsigned Histo2d::numOfEntries() const {
    return entries;
}

void Histo2d::fill(double x, double y, double v) {
    if (x < xlow || y < ylow) {
        //std::cout << "Underflow " << x << " " << y << std::endl;
        underflow += v;
    } else if (x > xhigh || y > yhigh) {
        //std::cout << "Overflow " << x << " " << y << std::endl;
        overflow += v;
    } else {
        unsigned xbin = (x-xlow)/xbinWidth;
        unsigned ybin = (y-ylow)/ybinWidth;
        data[ybin+(xbin*ybins)]+=v;
        if (v > max)
            max = v;
        if (v < min)
            min = v;
        isFilled[ybin+(xbin*ybins)] = true;
    }
    entries++;
}

void Histo2d::setAll(double v) {
    for (unsigned int i=0; i<ybins; i++) {
        for (unsigned int j=0; j<xbins; j++) {
            data[i+(j*ybins)] = v;
            entries++;
        }
    }
}

void Histo2d::add(const Histo2d &h) {
    if (this->size() != h.size())
        return;
    for (unsigned int i=0; i<(xbins*ybins); i++) {
        data[i] += h.getBin(i);
    }
    entries += h.numOfEntries();
}

void Histo2d::divide(const Histo2d &h) {
    if (this->size() != h.size())
        return;
    for (unsigned int i=0; i<(xbins*ybins); i++) {
        if (h.getBin(i) == 0) {
            data[i] = 0;
        } else {
            data[i] = data[i]/h.getBin(i);
        }
    }
    entries += h.numOfEntries();
}

void Histo2d::multiply(const Histo2d &h) {
    if (this->size() != h.size())
        return;
    for (unsigned int i=0; i<(xbins*ybins); i++) {
        data[i] = data[i]*h.getBin(i);
    }
    entries += h.numOfEntries();
}

void Histo2d::scale(const double s) {
    for (unsigned int i=0; i<(xbins*ybins); i++) {
        data[i] = data[i]*s;
    }
}

double Histo2d::getMean() {
    double sum = 0;
    double entries = 0;
    for (unsigned int i=0; i<(xbins*ybins); i++) {
        if (isFilled[i]) {
            sum += data[i];
            entries++;
        }
    }
    if (entries < 1) return 0;
    return sum/entries;
}

double Histo2d::getStdDev() {
    double mean = this->getMean();
    double mu = 0;
    double entries = 0;
    for (unsigned int i=0; i<(xbins*ybins); i++) {
        if (isFilled[i]) {
             mu += pow(data[i]-mean, 2);
             entries++;
        }
    }
    if (entries < 2) return 0;
    return sqrt(mu/(double)(entries-1));
}


double Histo2d::getBin(unsigned n) const {
    if (n < this->size()) {
        return data[n];
    } else {
        return 0;
    }
}

void Histo2d::setBin(unsigned n, double v) {
    if (n < this->size()) {
        data[n] = v;
        isFilled[n] = true;
    }
}


int Histo2d::binNum(double x, double y) {
    if (x < xlow || y < ylow) {
        //std::cout << "Underflow " << x << " " << y << std::endl;
        return -1;
    } else if (x > xhigh || y > yhigh) {
        //std::cout << "Overflow " << x << " " << y << std::endl;
        return -1;
    } else {
        unsigned xbin = (x-xlow)/xbinWidth;
        unsigned ybin = (y-ylow)/ybinWidth;
        return (ybin+(xbin*ybins));
    }
}


void Histo2d::toFile(std::string prefix, std::string dir, bool jsonType) {
    std::string filename = dir + prefix + "_" + HistogramBase::name;
    json j;

    if (jsonType) {
        filename += ".json";
    } else {
        filename += ".dat";
    }
    std::fstream file(filename, std::fstream::out | std::fstream::trunc);
    // jsonType
    if (jsonType) {
        j["Type"] = "Histo2d";
        j["Name"] = name;
        
        j["x"]["AxisTitle"] = xAxisTitle;
        j["x"]["Bins"] = xbins;
        j["x"]["Low"] = xlow;
        j["x"]["High"] = xhigh;
        
        j["y"]["AxisTitle"] = yAxisTitle;
        j["y"]["Bins"] = ybins;
        j["y"]["Low"] = ylow;
        j["y"]["High"] = yhigh;
        
        j["z"]["AxisTitle"] = zAxisTitle;
        
        j["Underflow"] = underflow;
        j["Overflow"] = overflow;
        
        for (unsigned int y=0; y<ybins; y++) {
            for (unsigned int x=0; x<xbins; x++) {
                j["Data"][x][y] = data[y+(x*ybins)] ;
            }
        }
        
        file << std::setw(4) << j;
    } else {
        // Raw Data
        for (unsigned int i=0; i<ybins; i++) {
            for (unsigned int j=0; j<xbins; j++) {
                file << data[i+(j*ybins)] << " ";
            }
            file << std::endl;
        }
    }
    file.close();
}

bool Histo2d::fromFile(std::string filename) {
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
        std::cerr << "#ERROR# opening histogram: " << e.what() << std::endl;
        return false;
    }
    // Check for type
    if (j["Type"].empty()) {
        std::cerr << "#ERROR# this does not seem to be a histogram file, could not parse." << std::endl;
        return false;
    } else {
        if (j["Type"] == "Histo2d") {
            std::cerr << "#ERROR# File contains the wrong type: " << j["Type"] <<  std::endl;
            return false;
        }

        name = j["Name"];
        xAxisTitle = j["x"]["AxisTitle"];
        yAxisTitle = j["y"]["AxisTitle"];
        zAxisTitle = j["z"]["AxisTitle"];

        xbins = j["x"]["Bins"];
        xlow = j["x"]["Low"];
        xhigh = j["x"]["High"];

        ybins = j["y"]["Bins"];
        xlow = j["y"]["Low"];
        xhigh = j["y"]["High"];
        
        underflow = j["underflow"];
        overflow = j["overflow"];

        data = std::vector<double>(xbins*ybins);
        for (unsigned int x=0; x<ybins; x++) {
            for (unsigned int y=0; y<xbins; y++) {
                data[x+(y*ybins)] = j["Data"][x][y];
            }
        }
    }
    file.close();
    return true;
}

void Histo2d::plot(std::string prefix, std::string dir) {
    hlog->info("Plotting {}", HistogramBase::name);
    // Put raw histo data in tmp file
    std::string tmp_name = std::string(getenv("USER")) + "/tmp_yarr_histo2d_" + prefix;
    this->toFile(tmp_name, "/tmp/", false);
    //std::string cmd = "gnuplot | epstopdf -f > " + dir + prefix + "_" + HistogramBase::name;
    std::string cmd = "gnuplot > " + dir + prefix + "_" + HistogramBase::name;
    for (unsigned i=0; i<lStat.size(); i++)
        cmd += "_" + std::to_string(lStat.get(i));
    //cmd += ".pdf";
    cmd += ".png";

    // Open gnuplot as file and pipe commands
    FILE *gnu = popen(cmd.c_str(), "w");
    
    //fprintf(gnu, "set terminal postscript enhanced color \"Helvetica\" 18 eps\n");
    fprintf(gnu, "set terminal png size 1280, 1024\n");
    fprintf(gnu, "set palette negative defined ( 0 '#D53E4F', 1 '#F46D43', 2 '#FDAE61', 3 '#FEE08B', 4 '#E6F598', 5 '#ABDDA4', 6 '#66C2A5', 7 '#3288BD')\n");
    //fprintf(gnu, "set pm3d map\n");
    fprintf(gnu, "unset key\n");
    fprintf(gnu, "set title \"%s\"\n" , HistogramBase::name.c_str());
    fprintf(gnu, "set xlabel \"%s\"\n" , HistogramBase::xAxisTitle.c_str());
    fprintf(gnu, "set ylabel \"%s\"\n" , HistogramBase::yAxisTitle.c_str());
    fprintf(gnu, "set cblabel \"%s\"\n" , HistogramBase::zAxisTitle.c_str());
    fprintf(gnu, "set xrange[%f:%f]\n", xlow, xhigh);
    fprintf(gnu, "set yrange[%f:%f]\n", ylow, yhigh);
    //fprintf(gnu, "set cbrange[0:120]\n");
    //fprintf(gnu, "splot \"/tmp/tmp_%s.dat\" matrix u (($1)*((%f-%f)/%d)):(($2)*((%f-%f)/%d)):3\n", HistogramBase::name.c_str(), xhigh, xlow, xbins, yhigh, ylow, ybins);
    fprintf(gnu, "plot \"%s\" matrix u (($1)*((%f-%f)/%d.0)+%f):(($2)*((%f-%f)/%d.0)+%f):3 with image\n", ("/tmp/" + tmp_name + "_" + name + ".dat").c_str(), xhigh, xlow, xbins, xlow +(xhigh-xlow)/(xbins*2.0), yhigh, ylow, ybins, ylow+(yhigh-ylow)/(ybins*2.0));
   // fprintf(gnu, "plot \"%s\" matrix u (($1)):(($2)):3 with image\n", ("/tmp/" + tmp_name + "_" + name + ".dat").c_str());
    pclose(gnu);
}

