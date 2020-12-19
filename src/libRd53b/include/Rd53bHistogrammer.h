#ifndef RD53B_HISTOGRAMMER_H
#define RD53B_HISTOGRAMMER_H


#include <thread>
#include <vector>

#include "ClipBoard.h"
#include "EventData.h"
#include "HistogramAlgorithm.h"
#include "HistogramBase.h"
#include "Histo1d.h"
#include "Histo2d.h"
#include "LoopStatus.h"

class ToaMap : public HistogramAlgorithm {
    public :
        ToaMap() : HistogramAlgorithm() {
            h = nullptr;
            r = nullptr;
        }
        ~ToaMap() {}

        void create(const LoopStatus& stat) override {
            h = new Histo2d(outputName(), nCol, 0.5, nCol+0.5, nRow, 0.5, nRow+0.5, stat);
            h->setXaxisTitle("Column");
            h->setYaxisTitle("Row");
            h->setZaxisTitle("Total ToA");
            r.reset(h);
        }
        void processEvent(FrontEndData* data) override;
        static std::string outputName() { return "ToaMap"; }

    private :
        Histo2d *h;
};

class Toa2Map : public HistogramAlgorithm {
    public :
        Toa2Map() : HistogramAlgorithm() {
            h = nullptr;
            r = nullptr;
        }
        ~Toa2Map() {}

        void create(const LoopStatus& stat) override {
            h = new Histo2d(outputName(), nCol, 0.5, nCol+0.5, nRow, 0.5, nRow+0.5, stat);
            h->setXaxisTitle("Column");
            h->setYaxisTitle("Row");
            h->setZaxisTitle("Total ToA2");
            r.reset(h);
        }
        void processEvent(FrontEndData* data) override;
        static std::string outputName() { return "Toa2Map"; }

    private :
        Histo2d *h;
};

class PToTDist : public HistogramAlgorithm {
    public :
        PToTDist() : HistogramAlgorithm() {
            h = nullptr;
            r = nullptr;
        }
        ~PToTDist() {}

        void create(const LoopStatus& stat) override {
            h = new Histo1d(outputName(), 2049, -0.5, 2048+0.5, stat);
            h->setXaxisTitle("PToT [1.5625ns]");
            h->setYaxisTitle("# of hits");
            r.reset(h);
        }
        void processEvent(FrontEndData *data) override;
        static std::string outputName() { return "PToTDist"; }

    private :
        Histo1d*  h;
};

class PToADist : public HistogramAlgorithm {
    public :
        PToADist() : HistogramAlgorithm() {
            h = nullptr;
            r = nullptr;
        }
        ~PToADist() {}

        void create(const LoopStatus& stat) override {
            h = new Histo1d(outputName(), 513, -0.5, (32*16)+0.5, stat);
            h->setXaxisTitle("PToA [1.5625ns]");
            h->setYaxisTitle("# of hits");
            r.reset(h);
        }
        void processEvent(FrontEndData *data) override;
        static std::string outputName() { return "PToADist"; }

    private :
        Histo1d*  h;
};

#endif
