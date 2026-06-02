#ifndef FRONTEND_H
#define FRONTEND_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Abstract FE class
// # Comment: Combined multiple FE 
// ################################

#include <memory>
#include <string>
#include <utility>

#include "FrontEndGeometry.h"

class HwController;
class RxCore;

// Status enum
enum yarrStatus {
    yarrSuccess = 0,
    yarrFailure = -1,
};

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
struct FrontEndClipBoards;
class FrontEndConnectivity;

/**
 * Representation of a FrontEnd ASIC.
 */
class FrontEnd {
    public:
        FrontEnd();
        virtual ~FrontEnd() = default;
        
        virtual void init(HwController *arg_core, const FrontEndConnectivity& fe_cfg)=0;

        /** Synonym for isActive */
        bool getActive() const;

        /** Is this chip active */
        bool isActive() const;
        /** Change active state */
        void setActive(bool active);

        /** Is this chip active (for parameter loops) */
        bool isActiveLoop() const;
        /** Change active state (for parameter loops) */
        void setActiveLoop(bool activeLoop);

        /** Make this represent a global ASIC */
        virtual void makeGlobal(){};
        virtual std::unique_ptr<FrontEnd> getGlobal();
        virtual void connectBookkeeper(Bookkeeper* k){};
       
        /** Send configuration to FrontEnd ASIC(s). */
        virtual void configure()=0;
        virtual yarrStatus checkCom() {return yarrSuccess;}
        virtual yarrStatus hasValidName() { return yarrSuccess; }

        // A parallel reset that undos any configuration
        virtual void resetAllHard() {}
        // A parallel reset that keeps configuration but reset counters/datapath
        virtual void resetAllSoft() {}

        // Set/Get Register in memory only
        virtual yarrStatus setNamedRegister(std::string name, const uint16_t value) {return yarrFailure;};
        virtual yarrStatus getNamedRegister(std::string name, uint16_t &value) {return yarrFailure;};
    
        // Write register to memory and chip
        virtual yarrStatus writeNamedRegister(std::string name, const uint16_t value) = 0;
        // Read register from chip to memory and return value through reference
        virtual yarrStatus readNamedRegister(std::string name, uint16_t &value) {return yarrFailure;};
        // Read register from chip to memory, write register to memory, and then to chip
        virtual yarrStatus readUpdateWriteNamedRegister(std::string name, const uint16_t value) {return yarrFailure;};

        /// Configures ADC
        virtual yarrStatus confAdc(uint16_t MONMUX, bool doCur) {return yarrFailure;};

        virtual void setInjCharge(double, bool, bool) = 0;

        FrontEndClipBoards &clipboards() { return *m_clipboards; }

        FrontEndGeometry geo = {};

    protected:
        std::unique_ptr<FrontEndClipBoards> m_clipboards;
        bool active = false;
        bool activeLoop = false;
        RxCore *m_rxcore = nullptr;
};

#endif
