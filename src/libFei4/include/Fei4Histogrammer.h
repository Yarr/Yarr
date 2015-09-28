#ifndef FEI4HISTOGRAMMER_H
#define FEI4HISTOGRAMMER_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Histograms Fei4 data
// # Comment: 
// ################################

#include <iostream>
#include <deque>
#include <map>
#include <list>
#include <vector>
#include <typeinfo>

#include "DataProcessor.h"
#include "ClipBoard.h"
#include "Fei4EventData.h"
#include "HistogramBase.h"
#include "Histo1d.h"
#include "Histo2d.h"
#include "LoopStatus.h"

class HistogramAlgorithm {
    public:
        HistogramAlgorithm() {}
        virtual ~HistogramAlgorithm() {}

        virtual void create(LoopStatus &stat) {}
        
        HistogramBase* getHisto() {
            return r;
        }
        
        virtual void processEvent(Fei4Data *data) {}
    protected:
        HistogramBase *r;
};

class Fei4Histogrammer : public DataProcessor {
    public:
        Fei4Histogrammer();
        ~Fei4Histogrammer();

        void connect(ClipBoard<Fei4Data> *arg_input, ClipBoard<HistogramBase> *arg_output) {
            input = arg_input;
            output = arg_output;
        }

        void addHistogrammer(HistogramAlgorithm *a) {
            algorithms.push_back(a);
        }

        void init();
        void process();
        void publish();
        void toFile(std::string basename);
        void plot(std::string basename);

    private:
        ClipBoard<Fei4Data> *input;
        ClipBoard<HistogramBase> *output;

        std::vector<HistogramAlgorithm*> algorithms;
};

class OccupancyMap : public HistogramAlgorithm {
    public:
        OccupancyMap() : HistogramAlgorithm() {
            r = NULL;
            h = NULL;
        }
        ~OccupancyMap() {
        }
        
        void create(LoopStatus &stat) {
            h = new Histo2d("OccupancyMap", 80, 0.5, 80.5, 336, 0.5, 336.5, typeid(this), stat);
            h->setXaxisTitle("Column");
            h->setYaxisTitle("Row");
            h->setZaxisTitle("Hits");
            r = (HistogramBase*) h;
        }
        
        void processEvent(Fei4Data *data);
    private:
        Histo2d *h;
};

class TotMap : public HistogramAlgorithm {
    public:
        TotMap() : HistogramAlgorithm() {
            h = NULL;
            r = NULL;
        }
        ~TotMap() {
        }

        void create(LoopStatus &stat) {
            h = new Histo2d("TotMap", 80, 0.5, 80.5, 336, 0.5, 336.5, typeid(this), stat);
            h->setXaxisTitle("Column");
            h->setYaxisTitle("Row");
            h->setZaxisTitle("Total ToT");
            r = (HistogramBase*) h;
        }

        void processEvent(Fei4Data *data);
    private:
        Histo2d *h;
};

class Tot2Map : public HistogramAlgorithm {
    public:
        Tot2Map() : HistogramAlgorithm() {
        }
        ~Tot2Map() {
        }

        void create(LoopStatus &stat) {
            h = new Histo2d("Tot2Map", 80, 0.5, 80.5, 336, 0.5, 336.5, typeid(this), stat);
            h->setXaxisTitle("Column");
            h->setYaxisTitle("Row");
            h->setZaxisTitle("Total ToT2");
            r = (HistogramBase*) h;
        }

        void processEvent(Fei4Data *data);
    private:
        Histo2d *h;
};

class L1Dist : public HistogramAlgorithm {
    public:
        L1Dist() : HistogramAlgorithm() {
            h = NULL;
            r = NULL;
        }

        ~L1Dist() {
        }

        void create(LoopStatus &stat) {
            h = new Histo1d("L1Dist", 16, -0.5, 15.5, typeid(this), stat);
            h->setXaxisTitle("L1A");
            h->setYaxisTitle("Hits");
            r = (HistogramBase*) h;
            l1id = 33;
            bcid_offset = 0;
        }

        void processEvent(Fei4Data *data);
    private:
        Histo1d *h;
        unsigned l1id;
        unsigned bcid_offset;
};

class HitDist : public HistogramAlgorithm {
    public:
        HitDist() : HistogramAlgorithm() {
            h = NULL;
            r = NULL;
        }

        ~HitDist() {
        }

        void create(LoopStatus &stat) {
            h = new Histo1d("HitDist", 16, -0.5, 15.5, typeid(this), stat);
            h->setXaxisTitle("Number of Hits");
            h->setYaxisTitle("Events");
            r = (HistogramBase*) h;
        }

        void processEvent(Fei4Data *data);
    private:
        Histo1d *h;
};
#endif
