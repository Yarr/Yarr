// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: 2D Histogram
// # Comment: 
// ################################

#include "Histo2d.h"
#include "Histo1d.h"

#include <cmath>
#include <fstream>
#include <iostream>
#include <iomanip>

#include "storage.hpp"

#include "logging.h"

namespace {
    auto hlog = logging::make_log("Histo2d");
}

Histo2d::Histo2d(const std::string &arg_name, unsigned arg_xbins, double arg_xlow, double arg_xhigh,
                 unsigned arg_ybins, double arg_ylow, double arg_yhigh) : HistogramBase(arg_name) {
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
    data = std::vector<float>(xbins*ybins,0);
    m_isFilled = std::vector<bool>(xbins*ybins,false);
    entries = 0;

}

Histo2d::Histo2d(const std::string &arg_name, unsigned arg_xbins, double arg_xlow, double arg_xhigh,
                 unsigned arg_ybins, double arg_ylow, double arg_yhigh, const LoopStatus &stat) : HistogramBase(arg_name, stat) {
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
    data = std::vector<float>(xbins*ybins,0);
    m_isFilled =  std::vector<bool>(xbins*ybins,false);
    entries = 0;
}

Histo2d::Histo2d(Histo2d *h) : HistogramBase(h->getName()) {
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

    data = std::vector<float>(xbins*ybins,0);
    m_isFilled = h->m_isFilled;
    for(unsigned i=0; i<xbins*ybins; i++)
        data[i] = h->getBin(i);
    entries = h->getNumOfEntries();
    lStat = h->getStat();
}

Histo2d::~Histo2d() = default;

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
        auto index = xbin+(ybin*xbins);
        data[index]+=v;
        if (v > max)
            max = v;
        if (v < min)
            min = v;
        m_isFilled[index] = true;
    }
    entries++;
}

void Histo2d::setAll(double v) {
    for (unsigned int i=0; i<ybins; i++) {
        for (unsigned int j=0; j<xbins; j++) {
            data[j+(i*xbins)] = v;
            entries++;
        }
    }
}

void Histo2d::add(const Histo2d &h) {
    if (this->size() != h.size())
        return;
    for (unsigned int i=0; i<(xbins*ybins); i++) {
      if (h.isFilled(i)){
        double d = h.getBin(i);
        data[i] += d;
        max = std::max(d, max);
	m_isFilled[i] = true;
      }
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

double Histo2d::getMean() const {
    double sum = 0;
    double entries = 0;
    for (unsigned int i=0; i<(xbins*ybins); i++) {
        if (m_isFilled[i]) {
            sum += data[i];
            entries++;
        }
    }
    if (entries < 1) return 0;
    return sum/entries;
}

double Histo2d::getStdDev() const {
    double mean = this->getMean();
    double mu = 0;
    double entries = 0;
    for (unsigned int i=0; i<(xbins*ybins); i++) {
        if (m_isFilled[i]) {
             mu += pow(data[i]-mean, 2);
             entries++;
        }
    }
    if (entries < 2) return 0;
    return sqrt(mu/(double)(entries-1));
}


bool Histo2d::isFilled(unsigned n) const {
    return (m_isFilled.size()>=n && m_isFilled.at(n));
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
        m_isFilled[n] = true;
    }
}


int Histo2d::binNum(double x, double y) const {
    if (x < xlow || y < ylow) {
        //std::cout << "Underflow " << x << " " << y << std::endl;
        return -1;
    } else if (x > xhigh || y > yhigh) {
        //std::cout << "Overflow " << x << " " << y << std::endl;
        return -1;
    } else {
        unsigned xbin = (x-xlow)/xbinWidth;
        unsigned ybin = (y-ylow)/ybinWidth;
        return (xbin+(ybin*xbins));
    }
}

void Histo2d::toStream(std::ostream &out) const{
    // Raw Data
    for (unsigned int i=0; i<ybins; i++) {
        for (unsigned int j=0; j<xbins; j++) {
            out << data[j+(i*xbins)] << " ";
        }
        out << std::endl;
    }
}

void Histo2d::toJson(json &j) const{
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

    j["Entries"] = entries;

    for (unsigned i=0; i<lStat.size(); i++)
        j["loopStatus"][i] = (lStat.get(i));

    for (unsigned int y=0; y<ybins; y++) {
        for (unsigned int x=0; x<xbins; x++) {
            j["Data"][x][y] = data[x+(y*xbins)] ;
        }
    }
}

void Histo2d::toFile(const std::string &prefix, const std::string &dir, bool jsonType) const{
    std::string filename = dir + prefix + "_" + HistogramBase::name;
    json j;
    for (unsigned i=0; i<lStat.size(); i++)
        filename += "_" + std::to_string(lStat.get(i));

    if (jsonType) {
        filename += ".json";
    } else {
        filename += ".dat";
    }
    std::fstream file(filename, std::fstream::out | std::fstream::trunc);
    // jsonType
    if (jsonType) {
       toJson(j);
       file << std::setw(4) << j;
    } else {
       toStream(file);
    }
    file.close();
}

bool Histo2d::fromFile(const std::string &filename) {
    std::ifstream file(filename, std::fstream::in);
    json j;
    try {
        if (!file) {
            throw std::runtime_error("could not open file");
        }
        try {
            j = json::parse(file);
            file.close();
        } catch (json::parse_error &e) {
            throw std::runtime_error(e.what());
        }
    } catch (std::runtime_error &e) {
        hlog->error("Error opening histogram: {}", e.what());
        return false;
    }

    try {
        auto isOk = fromJson(j);
        if(!isOk) {
          hlog->error("Reading file: {}", filename);
        }
        return isOk;
    } catch (std::runtime_error &e) {
        hlog->error("Exception while loading {}", filename);
        throw;
    }

    return true;
}

bool Histo2d::fromJson(const json &j) {
    // Check for type
    if (!j.contains("Type")) {
        hlog->error("ERROR this does not seem to be a histogram file, could not parse.");
        return false;
    } else {
        if (j["Type"] != "Histo2d") {
            hlog->error("ERROR File contains the wrong type: {}", std::string(j["Type"]));
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
        ylow = j["y"]["Low"];
        yhigh = j["y"]["High"];
        
        underflow = j["Underflow"];
        overflow = j["Overflow"];

        entries = j["Entries"];

        data = std::vector<float>(xbins*ybins);
        m_isFilled.resize(xbins*ybins);
        m_isFilled.assign(m_isFilled.size(), false);
        for (unsigned int y=0; y<ybins; y++) {
            for (unsigned int x=0; x<xbins; x++) {
                auto index = x+(y*xbins);
                double d = j["Data"][x][y];;
                data[index] = d;
                if(d > 0.0) {
                    m_isFilled[index] = true;
                    max = std::max(d, max);
                }
            }
        }
    }
    return true;
}

void Histo2d::plot(const std::string &prefix, const std::string &dir) const{
    hlog->info("Plotting {}", HistogramBase::name);

    std::string output = dir + prefix + "_" + HistogramBase::name;
    for (unsigned i=0; i<lStat.size(); i++)
        output += "_" + std::to_string(lStat.get(i));
    output += ".png";

    // Open gnuplot as file and pipe commands
    std::string input;

    input+="\"set terminal png size 1280, 1024;";
    input+="set palette negative defined ( 0 '#D53E4F', 1 '#F46D43', 2 '#FDAE61', 3 '#FEE08B', 4 '#E6F598', 5 '#ABDDA4', 6 '#66C2A5', 7 '#3288BD');";
    input+="unset key;";
    input+="set title '"  +HistogramBase::name+"';";
    input+="set xlabel '"  +HistogramBase::xAxisTitle+"';";
    input+="set xlabel '"  +HistogramBase::xAxisTitle+"';";
    input+="set ylabel '"  +HistogramBase::yAxisTitle+"';";
    input+="set cblabel '"  +HistogramBase::zAxisTitle+"';";
    input+="set xrange["+ std::to_string(xlow)+ ":"+std::to_string(xhigh)+ "];";
    input+="set yrange["+ std::to_string(ylow)+ ":"+std::to_string(yhigh)+ "];";
    input+="plot '-' binary array=(" + std::to_string(xbins) + "," + std::to_string(ybins) + ") ";// format=\"\%float\" ";
    input+="dx=" +std::to_string((xhigh-xlow)/((double)xbins)) + " ";
    input+="dy=" +std::to_string((yhigh-ylow)/((double)ybins)) + " ";
    input+="origin=(" + std::to_string(xlow+(xhigh-xlow)/(xbins*2.0)) + "," + std::to_string(ylow+(yhigh-ylow)/(ybins*2.0)) + ") ";
    input+="with image\"";

    std::string cmd="gnuplot  -e "+input+" > "+output+"\n";
    FILE *gnu = popen(cmd.c_str(), "w");
    fwrite(&data[0], sizeof(float), data.size(), gnu); 
    pclose(gnu);
}

std::unique_ptr<Histo1d> Histo2d::profileY() const {
  // Create the profile histogram
  auto outH = std::make_unique<Histo1d>(getName() + "_pfy", getYbins(), getYlow(), getYhigh());
  outH->setXaxisTitle(getYaxisTitle());
 
  // Fill the profile histogram
  for (int ybin = 0; ybin < getYbins(); ybin++) {
    double bin_y = getYlow() + ybin * getYbinWidth();
    for (int xbin = 0; xbin < getXbins(); xbin++) {
      auto bin = xbin+(ybin*xbins);
      double cxy = getBin(bin);
      if (cxy)
	outH->fill( bin_y, cxy );
    }
    //Let's divide by the number of bins we profiled
    unsigned binOnOutH = outH->binNum(bin_y);
    outH->setBin(binOnOutH, outH->getBin(binOnOutH)/(double)getXbins());
  }
 
  return outH;
}
