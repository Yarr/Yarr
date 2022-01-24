#ifndef STD_HISTOGRAMMER_H
#define STD_HISTOGRAMMER_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Histograms FrontEnd data
// ################################

#include <fstream>

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
        ~DataArchiver() override { if(fileHandle.is_open()) fileHandle.close(); }

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
        ~OccupancyMap() override = default;
        
        void create(const LoopStatus &stat) override;

        void processEvent(FrontEndData *data) override;

        static const std::string outputName()  { return "OccupancyMap"; }
    private:
        Histo2d *h;
};

class HistoFromDisk : public HistogramAlgorithm {
    public:
        HistoFromDisk(pointerJson config) : HistogramAlgorithm() {
            r = nullptr;
            h = nullptr;
	    m_config = *config;

	    //Deal with config, in particular check that the input file is readable
	    std::ifstream file(m_config["inputFileName"], std::fstream::in);
	    try {
	      if (!file) {
		throw std::runtime_error("could not open file");
	      }
	      try {
		json inputFile(json::parse(file));
		file.close();
	      } catch (json::parse_error &e) {
		throw std::runtime_error(e.what());
	      }
	    } catch (std::runtime_error &e) {
	      std::cout << "HistoFromDisk: Error opening histogram: " << e.what() << std::endl;;
	    }
        }
        ~HistoFromDisk() {        }
        
        void create(const LoopStatus &stat) override;

        void processEvent(FrontEndData *data) override {};

        static const std::string outputName()  { return "HistoFromDisk"; }
    private:
        HistogramBase *h;
        json m_config;
};

class TotMap : public HistogramAlgorithm {
    public:
        TotMap() : HistogramAlgorithm() {
            h = nullptr;
            r = nullptr;
        }
        ~TotMap() override = default;

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
        ~Tot2Map() override = default;

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
        ~TotDist() override = default;

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
        ~Tot3d() override = default;

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

        ~TagDist() override = default;

        void create(const LoopStatus &stat) override;

        void processEvent(FrontEndData *data) override;

        static std::string outputName() { return "TagDist"; }
    private:
        Histo1d *h;
};

class TagMap : public HistogramAlgorithm {
    public:
        TagMap() : HistogramAlgorithm() {
            h = nullptr;
            r = nullptr;
        }

        ~TagMap() override = default;

        void create(const LoopStatus &stat) override;

        void processEvent(FrontEndData *data) override;

        static std::string outputName() { return "TagMap"; }
    private:
        Histo2d *h;
};

class L1Dist : public HistogramAlgorithm {
    public:
        L1Dist() : HistogramAlgorithm() {
            h = nullptr;
            r = nullptr;
            current_tag = 0;
        }

        ~L1Dist() override = default;

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
        ~L13d() override = default;

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

        ~HitsPerEvent() override = default;

        void create(const LoopStatus &stat) override;

        void processEvent(FrontEndData *data) override;

        static std::string outputName() { return "HitDist"; }
    private:
        Histo1d *h;
};
#endif
