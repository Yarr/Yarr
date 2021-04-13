#ifndef STD_HISTOGRAMMER_H
#define STD_HISTOGRAMMER_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Histograms FrontEnd data
// ################################

#include "EventData.h"
#include "HistogramAlgorithm.h"
#include "HistogramBase.h"

class Histo1d;
class Histo2d;
class Histo3d;
class LoopStatus;

class DataArchiver : public HistogramAlgorithm {
    public :
        DataArchiver() : HistogramAlgorithm() {
            r = nullptr;
        }
        ~DataArchiver() { if(fileHandle.is_open()) fileHandle.close(); }

        bool open(std::string filename);
        void create(const LoopStatus &stat) override {}
        void processEvent(FrontEndData *data) override;

    private :
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
        
        void create(const LoopStatus &stat) override;

        void processEvent(FrontEndData *data) override;

        static const std::string outputName()  { return "OccupancyMap"; }
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

        void create(const LoopStatus &stat) override;

        void processEvent(FrontEndData *data) override;

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

        void create(const LoopStatus &stat) override;

        void processEvent(FrontEndData *data) override;

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

        void create(const LoopStatus &stat) override;

        void processEvent(FrontEndData *data) override;

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

        void create(const LoopStatus &stat) override;

        void processEvent(FrontEndData *data) override;
    private:
        Histo3d *h;
};

class TagDist : public HistogramAlgorithm {
    public:
        TagDist() : HistogramAlgorithm() {
            h = nullptr;
            r = nullptr;
        }

        ~TagDist() {
        }

        void create(const LoopStatus &stat) override;

        void processEvent(FrontEndData *data) override;

        static std::string outputName() { return "TagDist"; }
    private:
        Histo1d *h;
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

        void create(const LoopStatus &stat) override;

        void processEvent(FrontEndData *data) override;

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

        void create(const LoopStatus &stat) override;

        void processEvent(FrontEndData *data) override;

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

        void create(const LoopStatus &stat) override;

        void processEvent(FrontEndData *data) override;

        static std::string outputName() { return "HitDist"; }
    private:
        Histo1d *h;
};
#endif
