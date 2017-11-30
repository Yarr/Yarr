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


using json=nlohmann::basic_json<std::map, std::vector, std::string, bool, std::int32_t, std::uint32_t, float>;

class Fei4Analysis;
class Fei4Histogrammer;

class FrontEnd {
    public:
        FrontEnd() {}
        virtual ~FrontEnd() {}

        bool getActive();
		bool isActive();
		void setActive(bool active);
       
        virtual void configure() = 0;

        ClipBoard<Fei4Data> *clipDataFei4;
        ClipBoard<HistogramBase> *clipHisto;
        ClipBoard<HistogramBase> *clipResult;

        //Fei4Analysis *ana;
        //Fei4Histogrammer *histogrammer;

    protected:
        bool active;
};

class FrontEndCfg {
    public:
        FrontEndCfg() {
            name = "JohnDoe";
            txChannel = 99;
            rxChannel = 99;
        }

        virtual double toCharge(double)=0;
        virtual double toCharge(double, bool, bool)=0;
        virtual void toFileJson(json&)=0;
        virtual void fromFileJson(json&)=0;
        virtual void toFileBinary(std::string)=0;
        virtual void fromFileBinary(std::string)=0;
        virtual void toFileBinary()=0;
        virtual void fromFileBinary()=0;
		
        unsigned getChannel() {return rxChannel;}
		unsigned getTxChannel() {return txChannel;}
		unsigned getRxChannel() {return rxChannel;}
        std::string getName() {return name;}
        
        void setChannel(unsigned channel) {txChannel = channel; rxChannel = channel;}
		void setChannel(unsigned arg_txChannel, unsigned arg_rxChannel) {txChannel = arg_txChannel; rxChannel = arg_rxChannel;}
        void setName(std::string arg_name) {name = arg_name;}
    protected:
        std::string name;
        unsigned txChannel;
        unsigned rxChannel;
};

#endif
