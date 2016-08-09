#ifndef FRONTEND_H
#define FRONTEND_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Abstract FE class
// # Comment: Combined multiple FE 
// ################################

#include <string>
#include "json.hpp"
#include "ClipBoard.h"
#include "HistogramBase.h"
#include "Fei4EventData.h"

using json = nlohmann::json;

class Fei4Analysis;
class Fei4Histogrammer;

class FrontEnd {
    public:
        FrontEnd() {}
        virtual ~FrontEnd() {}
		
        unsigned getChannel();
		unsigned getTxChannel();
		unsigned getRxChannel();
        std::string getName();
     
		void setChannel(unsigned channel);
		void setChannel(unsigned arg_txChannel, unsigned arg_rxChannel);
        void setName(std::string arg_name);

        bool getActive();
		bool isActive();
		void setActive(bool active);
       
        virtual void configure() = 0;

        ClipBoard<Fei4Data> *clipDataFei4;
        ClipBoard<HistogramBase> *clipHisto;
        ClipBoard<HistogramBase> *clipResult;

        Fei4Analysis *ana;
        Fei4Histogrammer *histogrammer;

    protected:
        std::string name;
        bool active;
        unsigned txChannel;
        unsigned rxChannel;
};

class FrontEndCfg {
    public:
        virtual double toCharge(double)=0;
        virtual void toFileJson(json&)=0;
        virtual void fromFileJson(json&)=0;
        virtual void toFileBinary(std::string)=0;
        virtual void fromFileBinary(std::string)=0;
        virtual void toFileBinary()=0;
        virtual void fromFileBinary()=0;
};

#endif
