#ifndef STAR_RRHISTOGRAMMER_H
#define STAR_RRHISTOGRAMMER_H

// #################################
// # Author: Jenna Chisholm
// # Email: jenna.lori.chisholm@cern.ch
// # Project: Yarr
// # Description: Star Histogrammer for RR
// ################################

#include "EventData.h"
#include "HistogramAlgorithm.h"
#include "HistogramBase.h"
#include "StdHistogrammer.h"


class StarHCCRRDist : public HistogramAlgorithm {
    public:
        StarHCCRRDist() : HistogramAlgorithm() {
            r = nullptr;
            h = nullptr;
        }
        ~StarHCCRRDist() override = default;
        
        void create(const LoopStatus &stat) override;

        void processEvent(FrontEndData *data) override;

        static const std::string outputName()  { return "StarHCCRRDist"; }
    private:
        Histo1d *h;
};

class StarABCRRDist : public HistogramAlgorithm {
    public:
        StarABCRRDist() : HistogramAlgorithm() {
            r = nullptr;
            h = nullptr;
        }
        ~StarABCRRDist() override = default;
        
        void create(const LoopStatus &stat) override;

        void processEvent(FrontEndData *data) override;

        static const std::string outputName()  { return "StarABCRRDist"; }
    private:
        Histo2d *h;
};


#endif