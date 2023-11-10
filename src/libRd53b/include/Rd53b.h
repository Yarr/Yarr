#ifndef RD53B_H
#define RD53B_H
// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: RD53B Library
// # Comment: Combines ITkPixV1 and CROCv1
// # Date: May 2020
// ################################

#include "FrontEnd.h"
#include "TxCore.h"
#include "RxCore.h"

#include "Rd53bCmd.h"
#include "Rd53bCfg.h"
#include "itkpix_efuse_codec.h" // EfuseData

class Rd53b : public FrontEnd, public Rd53bCfg, public Rd53bCmd{
    public:

        Rd53b();
        Rd53b(HwController *arg_core);
        Rd53b(HwController *arg_core, unsigned arg_channel);
        Rd53b(HwController *arg_core, unsigned arg_txchannel, unsigned arg_rxchannel);
        
        void init(HwController *arg_core, unsigned arg_txChannel, unsigned arg_rxChannel) override;
        void makeGlobal() override {m_chipId = 16;}
        std::unique_ptr<FrontEnd> getGlobal() override {
            return std::make_unique<Rd53b>();
        }

        void resetAll() override;
        void configure() override;
        void configureInit();
        void configureGlobal();
        void configurePixels();
        void configurePixels(std::vector<std::pair<unsigned, unsigned>> &pixels);
        void configurePixelMaskParallel();
        
        int checkCom() override;
        bool hasValidName() override;

        void writeRegister(Rd53bRegDefault Rd53bGlobalCfg::*ref, uint16_t value);
        void readRegister(Rd53bRegDefault Rd53bGlobalCfg::*ref);
        void writeNamedRegister(std::string name, uint16_t value) override;
        uint16_t readNamedRegister(std::string name) override;
        void setRegisterValue(std::string name, uint16_t value) override;
        uint16_t getRegisterValue(std::string name) override;

        Rd53bRegDefault Rd53bGlobalCfg::* getNamedRegister(std::string name);

        void setInjCharge(double charge, bool sCap=true, bool lCap=true) override {
            this->writeRegister((Rd53bRegDefault Rd53bGlobalCfg::*)&Rd53bGlobalCfg::InjVcalDiff, this->toVcal(charge));
        }
        
        static std::pair<uint32_t, uint32_t> decodeSingleRegRead(uint32_t higher, uint32_t lower);
        static std::tuple<uint8_t, uint32_t, uint32_t> decodeSingleRegReadID(uint32_t higher, uint32_t lower);

	void readUpdateWriteNamedReg(std::string name) override;
	void readUpdateWriteReg(Rd53bRegDefault Rd53bGlobalCfg::*ref);
        uint32_t readSingleRegister(Rd53bRegDefault Rd53bGlobalCfg::*ref);
        
        // perform the necessary steps to program the E-fuse circuitry and perform
        // the readback of the E-fuse data
        itkpix_efuse_codec::EfuseData readEfuses();
        uint32_t readEfusesRaw();

        void runRingOsc(uint16_t duration, bool isBankB);
        void confAdc(uint16_t MONMUX, bool doCur = false) override;
    protected:
    private:
};

#endif
