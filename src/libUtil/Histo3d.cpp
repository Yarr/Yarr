// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: 2D Histogram
// # Comment: 
// ################################

#include "Histo3d.h"

Histo3d::Histo3d(std::string arg_name, unsigned arg_xbins, double arg_xlow, double arg_xhigh, 
        unsigned arg_ybins, double arg_ylow, double arg_yhigh, 
        unsigned arg_zbins, double arg_zlow, double arg_zhigh, 
        std::type_index t) : HistogramBase(arg_name, t) {
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
    data = new uint16_t[xbins*ybins*zbins];
    this->setAll(0);
    entries = 0;

}

Histo3d::Histo3d(std::string arg_name, unsigned arg_xbins, double arg_xlow, double arg_xhigh, 
        unsigned arg_ybins, double arg_ylow, double arg_yhigh, 
        unsigned arg_zbins, double arg_zlow, double arg_zhigh, 
        std::type_index t, LoopStatus &stat) : HistogramBase(arg_name, t, stat) {
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
    data = new uint16_t[xbins*ybins*zbins];
    this->setAll(0);
    entries = 0;
}

Histo3d::Histo3d(Histo3d *h) : HistogramBase(h->getName(), h->getType()) {
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

    data = new uint16_t[xbins*ybins*zbins];
    for(unsigned i=0; i<xbins*ybins*zbins; i++)
        data[i] = h->getBin(i);
    entries = h->getNumOfEntries();
    lStat = h->getStat();
}

Histo3d::~Histo3d() {
    delete[] data;
}

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
        isFilled[((ybin+(xbin*ybins))*zbins)+zbin] = true;
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
        if (isFilled[i]) {
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
        if (isFilled[i]) {
             mu += pow(data[i]-mean, 2);
             entries++;
        }
    }
    if (entries < 2) return 0;
    return sqrt(mu/(double)(entries-1));
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


int Histo3d::binNum(double x, double y, double z) {
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


void Histo3d::toFile(std::string prefix, std::string dir, bool header) {
    std::string filename = dir + prefix + "_" + name + ".dat";
    std::fstream file(filename, std::fstream::out | std::fstream::trunc);
    // Header
    if (header) {
        file << "Histo3d " <<  std::endl;
        file << name << std::endl;
        file << xAxisTitle << std::endl;
        file << yAxisTitle << std::endl; 
        file << zAxisTitle << std::endl;
        file << xbins << " " << xlow << " " << xhigh << std::endl;
        file << ybins << " " << ylow << " " << yhigh << std::endl;
        file << zbins << " " << zlow << " " << zhigh << std::endl;
        file << underflow << " " << overflow << std::endl;
    }
    // Data
    for (unsigned int i=0; i<ybins; i++) {
        for (unsigned int j=0; j<xbins; j++) {
            for (unsigned int k=0; k<zbins; k++) {
                file << data[(i+(j*ybins))*zbins+k] << " ";
            }
        }
        file << std::endl;
    }
    file.close();
}

void Histo3d::plot(std::string prefix, std::string dir) {
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
