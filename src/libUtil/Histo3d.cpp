// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: 2D Histogram
// # Comment: 
// ################################

#include "Histo3d.h"

#include <iostream>
#include <iomanip>
#include <cmath>
#include <fstream>
#include <sstream>
#include "logging.h"

namespace {
    auto hlog = logging::make_log("Histo3d");
}
Histo3d::Histo3d(const std::string &arg_name, unsigned arg_xbins, double arg_xlow, double arg_xhigh,
                 unsigned arg_ybins, double arg_ylow, double arg_yhigh,
                 unsigned arg_zbins, double arg_zlow, double arg_zhigh)
  : HistogramBase(arg_name)
{
    xbins = arg_xbins;
    xlow = arg_xlow;
    xhigh = arg_xhigh;
    xbinWidth = (xhigh - xlow)/xbins;
    
    ybins = arg_ybins;
    ylow = arg_ylow;
    yhigh = arg_yhigh;
    ybinWidth = (yhigh - ylow)/ybins;
    
    zbins = arg_zbins;
    zlow = arg_zlow;
    zhigh = arg_zhigh;
    zbinWidth = (zhigh - zlow)/zbins;
 
    min = 0;
    max = 0;
    underflow = 0;
    overflow = 0;
    data = std::vector<uint16_t>(xbins*ybins*zbins,0);

    entries = 0;

}

Histo3d::Histo3d(std::string arg_name, unsigned arg_xbins, double arg_xlow, double arg_xhigh, 
        unsigned arg_ybins, double arg_ylow, double arg_yhigh, 
        unsigned arg_zbins, double arg_zlow, double arg_zhigh, 
        const LoopStatus &stat)
  : HistogramBase(arg_name, stat)
{
    xbins = arg_xbins;
    xlow = arg_xlow;
    xhigh = arg_xhigh;
    xbinWidth = (xhigh - xlow)/xbins;
    
    ybins = arg_ybins;
    ylow = arg_ylow;
    yhigh = arg_yhigh;
    ybinWidth = (yhigh - ylow)/ybins;

    zbins = arg_zbins;
    zlow = arg_zlow;
    zhigh = arg_zhigh;
    zbinWidth = (zhigh - zlow)/zbins;
 
    min = 0;
    max = 0;
    underflow = 0;
    overflow = 0;
    data = std::vector< uint16_t>(xbins*ybins*zbins,0);
    entries = 0;
}

Histo3d::Histo3d(Histo3d *h) : HistogramBase(h->getName()) {
    xbins = h->getXbins();
    xlow = h->getXlow();
    xhigh = h->getXhigh();
    xbinWidth = h->getXbinWidth();

    ybins = h->getYbins();
    ylow = h->getYlow();
    yhigh = h->getYhigh();
    ybinWidth = h->getYbinWidth();

    
    zbins = h->getZbins();
    zlow = h->getZbins();
    zhigh = h->getZbins();
    zbinWidth = h->getZbins();
 
    min = h->getMin();
    max = h->getMax();
    underflow = h->getUnderflow();
    overflow = h->getOverflow();

    data = std::vector<uint16_t>( xbins*ybins*zbins);
    for(unsigned i=0; i<xbins*ybins*zbins; i++)
        data[i] = h->getBin(i);
    entries = h->getNumOfEntries();
    lStat = h->getStat();
}

Histo3d::~Histo3d() = default;

unsigned Histo3d::size() const {
    return xbins*ybins*zbins;
}

unsigned Histo3d::numOfEntries() const {
    return entries;
}

void Histo3d::fill(double x, double y, double z, double v) {
    if (x < xlow || y < ylow || z < zlow) {
        //std::cout << "Underflow " << x << " " << y << std::endl;
        underflow += v;
    } else if (x > xhigh || y > yhigh || z > zhigh) {
        //std::cout << "Overflow " << x << " " << y << std::endl;
        overflow += v;
    } else {
        unsigned xbin = (x-xlow)/xbinWidth;
        unsigned ybin = (y-ylow)/ybinWidth;
        unsigned zbin = (z-zlow)/zbinWidth;
        data[((ybin+(xbin*ybins))*zbins)+zbin]+=v;
        if (v > max)
            max = v;
        if (v < min)
            min = v;
        m_isFilled[((ybin+(xbin*ybins))*zbins)+zbin] = true;
    }
    entries++;
}

void Histo3d::setAll(double v) {
    for (unsigned int i=0; i<ybins; i++) {
        for (unsigned int j=0; j<xbins; j++) {
            for (unsigned int k=0; k<zbins; k++) {
                data[(i+(j*ybins))*zbins+k] = v;
                entries++;
            }
        }
    }
}

void Histo3d::add(const Histo3d &h) {
    if (this->size() != h.size())
        return;
    for (unsigned int i=0; i<(xbins*ybins*zbins); i++) {
        data[i] += h.getBin(i);
        if (h.isFilled(i))
                m_isFilled[i] = true;
    }
    entries += h.numOfEntries();
}

void Histo3d::divide(const Histo3d &h) {
    if (this->size() != h.size())
        return;
    for (unsigned int i=0; i<(xbins*ybins*zbins); i++) {
        if (h.getBin(i) == 0) {
            data[i] = 0;
        } else {
            data[i] = data[i]/h.getBin(i);
        }
    }
    entries += h.numOfEntries();
}

void Histo3d::multiply(const Histo3d &h) {
    if (this->size() != h.size())
        return;
    for (unsigned int i=0; i<(xbins*ybins*zbins); i++) {
        data[i] = data[i]*h.getBin(i);
    }
    entries += h.numOfEntries();
}

void Histo3d::scale(const double s) {
    for (unsigned int i=0; i<(xbins*ybins*zbins); i++) {
        data[i] = data[i]*s;
    }
}

double Histo3d::getMean() {
    double sum = 0;
    double entries = 0;
    for (unsigned int i=0; i<(xbins*ybins*zbins); i++) {
        if (m_isFilled[i]) {
            sum += data[i];
            entries++;
        }
    }
    if (entries < 1) return 0;
    return sum/entries;
}

double Histo3d::getStdDev() {
    double mean = this->getMean();
    double mu = 0;
    double entries = 0;
    for (unsigned int i=0; i<(xbins*ybins*zbins); i++) {
        if (m_isFilled[i]) {
             mu += (data[i]-mean)*(data[i]-mean);
             entries++;
        }
    }
    if (entries < 2) return 0;
    return sqrt(mu/(double)(entries-1));
}


bool Histo3d::isFilled(unsigned n) const {
    return (m_isFilled.size()>=n && m_isFilled.at(n));
}


double Histo3d::getBin(unsigned n) const {
    if (n < this->size()) {
        return data[n];
    } else {
        return 0;
    }
}

void Histo3d::setBin(unsigned n, double v) {
    if (n < this->size()) {
        data[n] = v;
    }
}


int Histo3d::binNum(double x, double y, double z) const {
    if (x < xlow || y < ylow || z < zlow) {
        //std::cout << "Underflow " << x << " " << y << std::endl;
        return -1;
    } else if (x > xhigh || y > yhigh || z > zhigh) {
        //std::cout << "Overflow " << x << " " << y << std::endl;
        return -1;
    } else {
        unsigned xbin = (x-xlow)/xbinWidth;
        unsigned ybin = (y-ylow)/ybinWidth;
        unsigned zbin = (z-zlow)/zbinWidth;
        return ((((ybin+(xbin*ybins))*zbins)+zbin));
    }
}

void Histo3d::toStream(std::ostream &out) const{

}

void Histo3d::toJson(json &j) const {
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
    j["z"]["Bins"] = zbins;
    j["z"]["Low"] = zlow;
    j["z"]["High"] = zhigh;

    j["Underflow"] = underflow;
    j["Overflow"] = overflow;

    j["Entries"] = entries;

    for (unsigned i=0; i<lStat.size(); i++)
        j["loopStatus"][i] = (lStat.get(i));

    for (unsigned int z=0; z<zbins; z++) {
	for (unsigned int y=0; y<ybins; y++) {
        	for (unsigned int x=0; x<xbins; x++) {
            		j["Data"][x][y][z] = data[ (y+(x*ybins))*zbins + z ] ;
        	}
	}
    }
}

void Histo3d::toFile(const std::string &prefix, const std::string &dir, bool jsonType) const {
    std::string filename = dir + prefix + "_" + name + ".dat";
    std::fstream file(filename, std::fstream::out | std::fstream::trunc);

    if (jsonType) {
        filename += ".json";
    } else {
        filename += ".dat";
    }
    json j;
    // jsonType
    if (jsonType) {
             toJson(j);
             file << std::setw(4) << j;
    } else {
	// Header
	file << "Histo3d " <<  std::endl;
        file << name << std::endl;
        file << xAxisTitle << std::endl;
        file << yAxisTitle << std::endl; 
        file << zAxisTitle << std::endl;
        file << xbins << " " << xlow << " " << xhigh << std::endl;
        file << ybins << " " << ylow << " " << yhigh << std::endl;
        file << zbins << " " << zlow << " " << zhigh << std::endl;
        file << underflow << " " << overflow << std::endl;
    	// Data
	for (unsigned int i=0; i<ybins; i++) {
        	for (unsigned int j=0; j<xbins; j++) {
        	    for (unsigned int k=0; k<zbins; k++) {
                	file << data[(i+(j*ybins))*zbins+k] << " ";
        	    }
        	}
	}
        file << std::endl;
    }
    file.close();
}

bool Histo3d::fromFile(const std::string &filename) {
    std::fstream file(filename, std::fstream::in);
    // Check for header
    std::string line;
    std::getline(file, line);
    if (line.find("Histo3d") == std::string::npos) {
        std::cerr << "ERROR: Tried loading 3d Histogram from file " << filename << ", but file has non or incorrect header" << std::endl;
        file.close();
        return false;
    } else {
        file >> name;
        file >> xAxisTitle;
        file >> yAxisTitle;
        file >> zAxisTitle;
        file >> xbins >> xlow >> xhigh;
        file >> ybins >> ylow >> yhigh;
        file >> zbins >> zlow >> zhigh;
        file >> underflow >> overflow;
    }
    // Data

    data =  std::vector<uint16_t>(xbins*ybins*zbins);
    for (unsigned int i=0; i<ybins; i++) {
        for (unsigned int j=0; j<xbins; j++) {
            for (unsigned int k=0; k<zbins; k++) {
                file >> data[(i+(j*ybins))*zbins+k];
            }
        }
    }
    file.close();
    return true;
}

void Histo3d::plot(const std::string &prefix, const std::string &dir) const {
    hlog->info("Plotting {}", HistogramBase::name);
    // Put raw histo data in tmp file
    std::string tmp_name = std::string(getenv("USER")) + "/tmp_yarr_histo2d_" + prefix;
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
    input+="set ylabel '"  +HistogramBase::yAxisTitle+"';";
    input+="set cblabel '"  +HistogramBase::zAxisTitle+"';";
    input+="set xrange["+ std::to_string(xlow)+ ":"+std::to_string(xhigh)+ "];";
    input+="set yrange["+ std::to_string(ylow)+ ":"+std::to_string(yhigh)+ "];";
    input+="plot '-' matrix u ((\\$1)*(("+std::to_string(xhigh);
    input+="-"+std::to_string(xlow)+")/";
    input+=std::to_string(xbins)+".0)+"+std::to_string(xlow +(xhigh-xlow)/(xbins*2.0));
    input+= "):((\\$2)*(("+std::to_string(yhigh);
    input+="-"+std::to_string(ylow)+")/";
    input+=std::to_string(ybins)+".0)+"+std::to_string(ylow +(yhigh-ylow)/(ybins*2.0));
    input+="):3 with image\"";
    std::string cmd="gnuplot  -e "+input+" > "+output+"\n";
    FILE *gnu = popen(cmd.c_str(), "w");
    std::stringstream ss;
    toStream(ss);
    fprintf(gnu,"%s",ss.str().c_str());
    pclose(gnu);
}
