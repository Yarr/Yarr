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

#include "FrontEndConnectivity.h"

#include "storage.hpp"

/**
 * Representation of configuration for a FrontEnd chip.
 *
 * Each is attached to a rx/tx channel via FrontEndConnectivity.
 */
class FrontEndCfg : public FrontEndConnectivity {
    public:
        /** Default config */
        FrontEndCfg() : FrontEndConnectivity() {
            name = "JohnDoe";
            enforceChipIdInName = false;
        }

        /** Copy constructor */
        FrontEndCfg(FrontEndCfg& cfg) : FrontEndConnectivity(cfg) {
            name = cfg.getName();
            enforceChipIdInName = cfg.checkChipIdInName();
        }

        virtual ~FrontEndCfg()= default;
        
        /** Convert voltage to charge based on response stored in configuration */
        virtual double toCharge(double)=0;
        /** Convert voltage to charge, with some flags */
        virtual double toCharge(double, bool, bool)=0;
        /** Store in json object */
        virtual void writeConfig(json &) =0;
        /** Load from json object */
        virtual void loadConfig(const json &)=0;

        virtual unsigned getPixelEn(unsigned col, unsigned row, bool doAltMask = false) = 0;
        // col/row starting at 0,0
        virtual void maskPixel(unsigned col, unsigned row, bool doAltMask = false) = 0;
        /// Enable (disable mask) for all pixels
        virtual void enableAll() = 0;

        virtual std::tuple<json, std::vector<json>> getPreset(const std::string& systemType="SingleChip");

        /** Return name of this FE */
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
