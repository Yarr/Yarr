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

#include "DataProcessor.h"
#include "ClipBoard.h"
#include "Fei4EventData.h"
#include "ResultBase.h"
#include "Histo1d.h"
#include "Histo2d.h"

class HistogramAlgorithm {
    public:
        HistogramAlgorithm() {}
        virtual ~HistogramAlgorithm() {}
        
        ResultBase* getResult() {
            return r;
        }
        
        virtual void processEvent(Fei4Data *data) {}
    protected:
        ResultBase *r;
};

class Fei4Histogrammer : public DataProcessor {
    public:
        Fei4Histogrammer();
        ~Fei4Histogrammer();

        void connect(ClipBoard<Fei4Data> *arg_input, ClipBoard<ResultBase> *arg_output) {
            input = arg_input;
            output = arg_output;
        }

        void addHistogrammer(HistogramAlgorithm *a) {
            algorithms.push_back(a);
        }

        void init();
        void process();
        void publish();

    private:
        ClipBoard<Fei4Data> *input;
        ClipBoard<ResultBase> *output;

        std::vector<HistogramAlgorithm*> algorithms;
};

class OccupancyHistogram : public HistogramAlgorithm {
    public:
        OccupancyHistogram() : HistogramAlgorithm() {
            h = new Histo2d("Occupancy", 80, 0.5, 80.5, 336, 0.5, 336.5);
            r = (ResultBase*) h;
        }
        ~OccupancyHistogram() {
            delete h;
        }
        
        void processEvent(Fei4Data *data);
    private:
        Histo2d *h;
};  

#endif
