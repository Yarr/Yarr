#ifndef YARR_FRONTEND_CONNECTIVITY_H
#define YARR_FRONTEND_CONNECTIVITY_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Abstract FE class
// # Comment: Combined multiple FE 
// ################################

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

#endif
