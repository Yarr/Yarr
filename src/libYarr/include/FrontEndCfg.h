#ifndef YARR_FRONTEND_CFG_H
#define YARR_FRONTEND_CFG_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Abstract FE class
// # Comment: Combined multiple FE 
// ################################

#include <string>
#include <utility>

#include "storage.hpp"

class FrontEndConnectivity {
    public:
    FrontEndConnectivity() {
        initFeConnectivity(99, 99, 99);
    }

    FrontEndConnectivity(unsigned arg_channel) {
        initFeConnectivity(arg_channel);
    }

    FrontEndConnectivity(unsigned arg_txChannel, unsigned arg_rxChannel) {
        initFeConnectivity(arg_txChannel, arg_rxChannel);
    }

    FrontEndConnectivity(unsigned arg_txChannel, unsigned arg_rxChannel, unsigned arg_regRxChannel) {
        initFeConnectivity(arg_txChannel, arg_rxChannel, arg_regRxChannel);
    }

    FrontEndConnectivity(const FrontEndConnectivity& cfg) {
        initFeConnectivity(cfg.getTxChannel(), cfg.getRxChannel());
    }

    virtual void initFeConnectivity(const FrontEndConnectivity& cfg) {
        initFeConnectivity(cfg.getTxChannel(), cfg.getRxChannel(), cfg.getRegRxChannel());
    }

    virtual void initFeConnectivity(unsigned arg_txChannel, unsigned arg_rxChannel, unsigned arg_regRxChannel){
        txChannel = arg_txChannel;
        rxChannel = arg_rxChannel;
        regRxChannel = arg_regRxChannel;
        lockCfg = false;
    }

    virtual void initFeConnectivity(unsigned arg_txChannel, unsigned arg_rxChannel){
        txChannel = arg_txChannel;
        rxChannel = arg_rxChannel;
        regRxChannel = arg_rxChannel;
        lockCfg = false;
    }

    virtual void initFeConnectivity(unsigned arg_channel){
        initFeConnectivity(arg_channel, arg_channel, arg_channel);
    }

    virtual ~FrontEndConnectivity()= default;

    unsigned getChannel() const {return rxChannel;}
    unsigned getTxChannel() const {return txChannel;}
    unsigned getRxChannel() const {return rxChannel;}
    unsigned getRegRxChannel() const {return regRxChannel;}

    void setChannel(unsigned channel) {txChannel = channel; rxChannel = channel;}
    void setChannel(unsigned arg_txChannel, unsigned arg_rxChannel) {txChannel = arg_txChannel; rxChannel = arg_rxChannel;}
    void setChannel(unsigned arg_txChannel, unsigned arg_rxChannel, unsigned arg_regRxChannel) {txChannel = arg_txChannel; rxChannel = arg_rxChannel; regRxChannel = arg_regRxChannel;}
    void setChannel(const FrontEndConnectivity &cfg) {setChannel(cfg.getTxChannel(), cfg.getRxChannel(), cfg.getRegRxChannel());}

    bool isLocked() const {return lockCfg;}
    void setLocked(bool v) {lockCfg = v;}

    protected:
    unsigned txChannel;
    unsigned rxChannel;
    unsigned regRxChannel;
    bool lockCfg;
};

class FrontEndCfg : public FrontEndConnectivity {
    public:
        FrontEndCfg() : FrontEndConnectivity() {
            name = "JohnDoe";
            enforceChipIdInName = false;
        }

	    FrontEndCfg(FrontEndCfg& cfg) : FrontEndConnectivity(cfg) {
	        name = cfg.getName();
	        enforceChipIdInName = cfg.checkChipIdInName();
	    }

        virtual ~FrontEndCfg()= default;
        
        virtual double toCharge(double)=0;
        virtual double toCharge(double, bool, bool)=0;
        virtual void writeConfig(json &) =0;
        virtual void loadConfig(const json &)=0;

        virtual unsigned getPixelEn(unsigned col, unsigned row, bool doAltMask = false) = 0;
        // col/row starting at 0,0
        virtual void maskPixel(unsigned col, unsigned row, bool doAltMask = false) = 0;
        /// Enable (disable mask) for all pixels
        virtual void enableAll() = 0;

        virtual std::tuple<json, std::vector<json>> getPreset(const std::string& systemType="SingleChip");

        std::string getName() {return name;}
        bool checkChipIdInName() { return enforceChipIdInName; }

        // Returns converted ADC counts (float) and unit (string)
	    virtual std::pair<float, std::string> convertAdc(uint16_t ADC, bool meas_curr) {return std::make_pair(0.0, "None");} 
        
        void setName(std::string arg_name) {name = std::move(arg_name);}
    
    protected:
        std::string name;
        bool enforceChipIdInName;
};

#endif
