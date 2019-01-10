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
#include <unistd.h>
#include <fcntl.h>

#include "DataProcessor.h"
#include "ClipBoard.h"
#include "Fei4EventData.h"
#include "HistogramBase.h"
#include "Histo1d.h"
#include "Histo2d.h"
#include "Histo3d.h"
#include "LoopStatus.h"

class HistogramAlgorithm {
    public:
        HistogramAlgorithm() {
            nCol = 80;
            nRow = 336;
        
        }
        virtual ~HistogramAlgorithm() {}

        virtual void create(LoopStatus &stat) {}
        
        std::unique_ptr<HistogramBase> getHisto() {
            return std::move(r);
        }
        
        virtual void processEvent(Fei4Data *data) {}
        void setMapSize(unsigned col, unsigned row) {
            nCol = col;
            nRow = row;
        }
    protected:
        std::unique_ptr<HistogramBase> r;
        unsigned nCol;
        unsigned nRow;
};

class Fei4Histogrammer : public DataProcessor {
    public:
        Fei4Histogrammer();
        ~Fei4Histogrammer();

        void connect(ClipBoard<EventDataBase> *arg_input, ClipBoard<HistogramBase> *arg_output) {
            input = arg_input;
            output = arg_output;
        }

        void addHistogrammer(HistogramAlgorithm *a) {
            algorithms.push_back(a);
        }

        void setMapSize(unsigned col, unsigned row) {
            for (unsigned i=0; i<algorithms.size(); i++) {
                algorithms[i]->setMapSize(col, row);
            }
        }
        
        void clearHistogrammers();

        void init();
        void run();
        void join();
        void process();
        void process_core();
        void publish();

        ClipBoard<EventDataBase>& getInput() { return *input; }

        static bool processorDone;
        
    private:
        ClipBoard<EventDataBase> *input;
        ClipBoard<HistogramBase> *output;
        std::unique_ptr<std::thread> thread_ptr;

        std::vector<HistogramAlgorithm*> algorithms;
};

class DataArchiver : public HistogramAlgorithm {
    public:
        DataArchiver(std::string filename) : HistogramAlgorithm() {
            r = NULL;
            fileHandle.open(filename.c_str(), std::fstream::out | std::fstream::binary | std::fstream::trunc);
        }
        ~DataArchiver() {
            fileHandle.close();
        }

        void create(LoopStatus &stat) {}
        void processEvent(Fei4Data *data);
    private:
        std::fstream fileHandle;
};

class OccupancyMap : public HistogramAlgorithm {
    public:
        OccupancyMap() : HistogramAlgorithm() {
            r = nullptr;
            h = nullptr;
        }
        ~OccupancyMap() {
        }
        
        void create(LoopStatus &stat) {
            h = new Histo2d("OccupancyMap", nCol, 0.5, nCol+0.5, nRow, 0.5, nRow+0.5, typeid(this), stat);
            h->setXaxisTitle("Column");
            h->setYaxisTitle("Row");
            h->setZaxisTitle("Hits");
            r.reset(h);
        }
        
        void processEvent(Fei4Data *data);
    private:
        Histo2d *h;
};

class TotMap : public HistogramAlgorithm {
    public:
        TotMap() : HistogramAlgorithm() {
            h = nullptr;
            r = nullptr;
        }
        ~TotMap() {
        }

        void create(LoopStatus &stat) {
            h = new Histo2d("TotMap", nCol, 0.5, nCol+0.5, nRow, 0.5, nRow+0.5, typeid(this), stat);
            h->setXaxisTitle("Column");
            h->setYaxisTitle("Row");
            h->setZaxisTitle("Total ToT");
            r.reset(h);
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
            h = new Histo2d("Tot2Map", nCol, 0.5, nCol+0.5, nRow, 0.5, nRow+0.5, typeid(this), stat);
            h->setXaxisTitle("Column");
            h->setYaxisTitle("Row");
            h->setZaxisTitle("Total ToT2");
            r.reset(h);
        }

        void processEvent(Fei4Data *data);
    private:
        Histo2d *h;
};

class TotDist : public HistogramAlgorithm {
    public:
        TotDist() : HistogramAlgorithm() {
        }
        ~TotDist() {
        }

        void create(LoopStatus &stat) {
            h = new Histo1d("TotDist", 16, 0.5, 16.5, typeid(this), stat);
            h->setXaxisTitle("ToT [bc]");
            h->setYaxisTitle("# of Hits");
            r.reset(h);
        }

        void processEvent(Fei4Data *data);
    private:
        Histo1d *h;
};

class Tot3d : public HistogramAlgorithm {
    public:
        Tot3d() : HistogramAlgorithm() {
            h = NULL;
            r = NULL;
        }
        ~Tot3d() {
        }

        void create(LoopStatus &stat) {
            h = new Histo3d("Tot3d", nCol, 0.5, nCol+0.5, nRow, 0.5, nRow+0.5, 16, 0.5, 16.5, typeid(this), stat);
            h->setXaxisTitle("Column");
            h->setYaxisTitle("Row");
            h->setZaxisTitle("ToT");
            r.reset(h);
        }

        void processEvent(Fei4Data *data);
    private:
        Histo3d *h;
};

class L1Dist : public HistogramAlgorithm {
    public:
        L1Dist() : HistogramAlgorithm() {
            h = nullptr;
            r = nullptr;
            current_tag = 0;
        }

        ~L1Dist() {
        }

        void create(LoopStatus &stat) {
            h = new Histo1d("L1Dist", 16, -0.5, 15.5, typeid(this), stat);
            h->setXaxisTitle("L1A");
            h->setYaxisTitle("Hits");
            r.reset(h);
            l1id = 33;
            bcid_offset = 0;
        }

        void processEvent(Fei4Data *data);
    private:
        Histo1d *h;
        unsigned l1id;
        unsigned bcid_offset;
        unsigned current_tag;
};

class L13d : public HistogramAlgorithm {
    public:
        L13d() : HistogramAlgorithm() {
            h = NULL;
            r = NULL;
            current_tag = 0;
        }
        ~L13d() {
        }

        void create(LoopStatus &stat) {
            h = new Histo3d("L13d", nCol, 0.5, nCol+0.5, nRow, 0.5, nRow+0.5, 16, -0.5, 15.5, typeid(this), stat);
            h->setXaxisTitle("Column");
            h->setYaxisTitle("Row");
            h->setZaxisTitle("L1A");
            r.reset(h);
            l1id = 33;
            bcid_offset = 0;
        }

        void processEvent(Fei4Data *data);
    private:
        Histo3d *h;
        unsigned l1id;
        unsigned bcid_offset;
        unsigned current_tag;
};

class HitsPerEvent : public HistogramAlgorithm {
    public:
        HitsPerEvent() : HistogramAlgorithm() {
            h = nullptr;
            r = nullptr;
        }

        ~HitsPerEvent() {
        }

        void create(LoopStatus &stat) {
            h = new Histo1d("HitDist", 16, -0.5, 15.5, typeid(this), stat);
            h->setXaxisTitle("Number of Hits");
            h->setYaxisTitle("Events");
            r.reset(h);
        }

        void processEvent(Fei4Data *data);
    private:
        Histo1d *h;
};
#endif
