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
#include <utility>

#include "ClipBoard.h"
#include "HistogramBase.h"
#include "EventDataBase.h"
#include "HwController.h"
#include "FrontEndGeometry.h"

#include "storage.hpp"

//! extra int trigger tags to pass more feedback from the data processors
#define PROCESSING_FEEDBACK_TRIGGER_TAG_ERROR  -10
#define PROCESSING_FEEDBACK_TRIGGER_TAG_RR      -2
#define PROCESSING_FEEDBACK_TRIGGER_TAG_Control -3  //!< HPRs in Strips
#define PROCESSING_FEEDBACK_UNDEFINED_BCID -1

//! \brief RawData processing information from data processors
typedef struct FeedbackProcessingInfo
{
    unsigned packet_size = 0; //!< the size of the packet that the FE sent, i.e. RawData.getSize()
    int trigger_tag = PROCESSING_FEEDBACK_TRIGGER_TAG_ERROR; //!< l0id of the triggered data packets, and extra negative tags for RR etc
    int bcid = PROCESSING_FEEDBACK_UNDEFINED_BCID;
    unsigned n_clusters = 0;  //!< n_clusters in Strips & the number of hits in Pixels?
} FeedbackProcessingInfo;

class Bookkeeper;

class FrontEnd {
    public:
        FrontEnd() = default;
        virtual ~FrontEnd() = default;
        
        virtual void init(HwController *arg_core, unsigned arg_txChannel, unsigned arg_rxChannel)=0;

        bool getActive() const;
		bool isActive() const;
		void setActive(bool active);
        virtual void makeGlobal(){};
        virtual std::unique_ptr<FrontEnd> getGlobal() {return nullptr;}
        virtual void connectBookkeeper(Bookkeeper* k){};
       
        virtual void configure()=0;
        virtual int checkCom() {return 1;}
        virtual bool hasValidName() { return true; }

        virtual void resetAll() {}

        /// Reads the named register and writes it to the local object memory
        virtual void readUpdateWriteNamedReg(std::string name) {}
        /// Write to a register using a string name (most likely from json)
        virtual void writeNamedRegister(std::string name, uint16_t value) = 0;
        /// Reads a named register and returns the value of it
        virtual uint16_t readNamedRegister(std::string name) {return 0;}
        /// Write register value to the local object memory (does not write to the actual chip) 
        virtual void setRegisterValue(std::string name, uint16_t value) {};
        /// Reads a named register from the local object memory (does not write to the actual chip) 
        virtual uint16_t getRegisterValue(std::string name) {return 0;}

        /// Configures ADC
        virtual void confAdc(uint16_t MONMUX, bool doCur) {}

        virtual void setInjCharge(double, bool, bool) = 0;

        // Clipboards to buffer data
        ClipBoard<RawDataContainer> clipRawData;
        ClipBoard<EventDataBase> clipData;
        ClipBoard<HistogramBase> clipHisto;
        ClipBoard<FeedbackProcessingInfo> clipProcFeedback;
        std::vector<std::unique_ptr<ClipBoard<HistogramBase>> > clipResult;
        
        FrontEndGeometry geo;

    protected:
        bool active;
        RxCore *m_rxcore;
};

class FrontEndCfg {
    public:
        FrontEndCfg() {
            name = "JohnDoe";
            txChannel = 99;
            rxChannel = 99;
            lockCfg = false;
            enforceChipIdInName = false;
        }
        virtual ~FrontEndCfg()= default;
        

        virtual double toCharge(double)=0;
        virtual double toCharge(double, bool, bool)=0;
        virtual void writeConfig(json &) =0;
        virtual void loadConfig(const json &)=0;

        virtual unsigned getPixelEn(unsigned col, unsigned row) = 0;
        // col/row starting at 0,0
        virtual void maskPixel(unsigned col, unsigned row) = 0;
        /// Enable (disable mask) for all pixels
        virtual void enableAll() = 0;

        virtual std::tuple<json, std::vector<json>> getPreset(const std::string& systemType="SingleChip");

        unsigned getChannel() const {return rxChannel;}
		unsigned getTxChannel() const {return txChannel;}
		unsigned getRxChannel() const {return rxChannel;}
        std::string getName() {return name;}
        bool checkChipIdInName() { return enforceChipIdInName; }

        // Returns converted ADC counts (float) and unit (string)
	virtual std::pair<float, std::string> convertAdc(uint16_t ADC, bool meas_curr) {return std::make_pair(0.0, "None");} 
        
        void setChannel(unsigned channel) {txChannel = channel; rxChannel = channel;}
		void setChannel(unsigned arg_txChannel, unsigned arg_rxChannel) {txChannel = arg_txChannel; rxChannel = arg_rxChannel;}
        void setName(std::string arg_name) {name = std::move(arg_name);}
    
        bool isLocked() const {return lockCfg;}
        void setLocked(bool v) {lockCfg = v;}
    protected:
        std::string name;
        unsigned txChannel;
        unsigned rxChannel;
        bool lockCfg;
        bool enforceChipIdInName;
};

#endif
