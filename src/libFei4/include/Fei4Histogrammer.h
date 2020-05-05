#ifndef FEI4HISTOGRAMMER_H
#define FEI4HISTOGRAMMER_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Histograms Fei4 data
// # Comment: 
// ################################

#include <fstream>
#include <vector>
#include <typeinfo>
#include <thread>

#include "DataProcessor.h"
#include "ClipBoard.h"
#include "Fei4EventData.h"
#include "HistogramAlgorithm.h"
#include "HistogramBase.h"
#include "Histo1d.h"
#include "Histo2d.h"
#include "Histo3d.h"
#include "LoopStatus.h"

class DataArchiver : public HistogramAlgorithm {
    public:
        DataArchiver(std::string filename) : HistogramAlgorithm() {
            r = NULL;
            fileHandle.open(filename.c_str(), std::fstream::out | std::fstream::binary | std::fstream::trunc);
        }
        ~DataArchiver() {
            fileHandle.close();
        }

        void create(LoopStatus &stat) override {}
        void processEvent(Fei4Data *data) override;
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
        
        void create(LoopStatus &stat) override {
            h = new Histo2d("OccupancyMap", nCol, 0.5, nCol+0.5, nRow, 0.5, nRow+0.5, typeid(this), stat);
            h->setXaxisTitle("Column");
            h->setYaxisTitle("Row");
            h->setZaxisTitle("Hits");
            r.reset(h);
        }
        
        void processEvent(Fei4Data *data) override;

        static std::string outputName() { return "OccupancyMap"; }
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

        void create(LoopStatus &stat) override {
            h = new Histo2d(outputName(), nCol, 0.5, nCol+0.5, nRow, 0.5, nRow+0.5, typeid(this), stat);
            h->setXaxisTitle("Column");
            h->setYaxisTitle("Row");
            h->setZaxisTitle("Total ToT");
            r.reset(h);
        }

        void processEvent(Fei4Data *data) override;

        static std::string outputName() { return "TotMap"; }
    private:
        Histo2d *h;
};

class Tot2Map : public HistogramAlgorithm {
    public:
        Tot2Map() : HistogramAlgorithm() {
        }
        ~Tot2Map() {
        }

        void create(LoopStatus &stat) override {
            h = new Histo2d(outputName(), nCol, 0.5, nCol+0.5, nRow, 0.5, nRow+0.5, typeid(this), stat);
            h->setXaxisTitle("Column");
            h->setYaxisTitle("Row");
            h->setZaxisTitle("Total ToT2");
            r.reset(h);
        }

        void processEvent(Fei4Data *data) override;

        static std::string outputName() { return "Tot2Map"; }
    private:
        Histo2d *h;
};

class TotDist : public HistogramAlgorithm {
    public:
        TotDist() : HistogramAlgorithm() {
        }
        ~TotDist() {
        }

        void create(LoopStatus &stat) override {
            h = new Histo1d(outputName(), 16, 0.5, 16.5, typeid(this), stat);
            h->setXaxisTitle("ToT [bc]");
            h->setYaxisTitle("# of Hits");
            r.reset(h);
        }

        void processEvent(Fei4Data *data) override;

        static std::string outputName() { return "TotDist"; }
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

        void create(LoopStatus &stat) override {
            h = new Histo3d("Tot3d", nCol, 0.5, nCol+0.5, nRow, 0.5, nRow+0.5, 16, 0.5, 16.5, typeid(this), stat);
            h->setXaxisTitle("Column");
            h->setYaxisTitle("Row");
            h->setZaxisTitle("ToT");
            r.reset(h);
        }

        void processEvent(Fei4Data *data) override;
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

        void create(LoopStatus &stat) override {
            h = new Histo1d(outputName(), 16, -0.5, 15.5, typeid(this), stat);
            h->setXaxisTitle("L1A");
            h->setYaxisTitle("Hits");
            r.reset(h);
            l1id = 33;
            bcid_offset = 0;
        }

        void processEvent(Fei4Data *data) override;

        static std::string outputName() { return "L1Dist"; }
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

        void create(LoopStatus &stat) override {
            h = new Histo3d(outputName(), nCol, 0.5, nCol+0.5, nRow, 0.5, nRow+0.5, 16, -0.5, 15.5, typeid(this), stat);
            h->setXaxisTitle("Column");
            h->setYaxisTitle("Row");
            h->setZaxisTitle("L1A");
            r.reset(h);
            l1id = 33;
            bcid_offset = 0;
        }

        void processEvent(Fei4Data *data) override;

        static std::string outputName() { return "L13d"; }
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

        void create(LoopStatus &stat) override {
            h = new Histo1d(outputName(), 16, -0.5, 15.5, typeid(this), stat);
            h->setXaxisTitle("Number of Hits");
            h->setYaxisTitle("Events");
            r.reset(h);
        }

        void processEvent(Fei4Data *data) override;

        static std::string outputName() { return "HitDist"; }
    private:
        Histo1d *h;
};
#endif
