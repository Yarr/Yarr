#ifndef ITKPIXV2_H
#define ITKPIXV2_H
// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: ITkPixV2 Library
// # Date: Jul 2023
// ################################

#include "FrontEnd.h"
#include "TxCore.h"
#include "RxCore.h"

#include "Itkpixv2Cmd.h"
#include "Itkpixv2Cfg.h"
#include "itkpix_efuse_codec.h" // EfuseData

class Itkpixv2 : public FrontEnd, public Itkpixv2Cfg, public Itkpixv2Cmd{
    public:

        Itkpixv2();
        Itkpixv2(HwController *arg_core);
        Itkpixv2(HwController *arg_core, unsigned arg_channel);
        Itkpixv2(HwController *arg_core, unsigned arg_txchannel, unsigned arg_rxchannel);
        
        void init(HwController *arg_core, unsigned arg_txChannel, unsigned arg_rxChannel) override;
        void makeGlobal() override {m_chipId = 16;}
        std::unique_ptr<FrontEnd> getGlobal() override {
            return std::make_unique<Itkpixv2>();
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

        void maskPixel(unsigned col, unsigned row) override {
            this->setEn(col, row, 0);
            this->setHitbus(col, row, 0);
        }
        
        unsigned getPixelEn(unsigned col, unsigned row) override {
            return this->getEn(col, row);
        }


        void enableAll() override;

        void writeRegister(Itkpixv2RegDefault Itkpixv2GlobalCfg::*ref, uint16_t value);
        void readRegister(Itkpixv2RegDefault Itkpixv2GlobalCfg::*ref);
        void writeNamedRegister(std::string name, uint16_t value) override;
        uint16_t readNamedRegister(std::string name) override;
        
        Itkpixv2RegDefault Itkpixv2GlobalCfg::* getNamedRegister(std::string name);

        void setInjCharge(double charge, bool sCap=true, bool lCap=true) override {
            this->writeRegister((Itkpixv2RegDefault Itkpixv2GlobalCfg::*)&Itkpixv2GlobalCfg::InjVcalDiff, this->toVcal(charge));
        }
        
        static std::pair<uint32_t, uint32_t> decodeSingleRegRead(uint32_t higher, uint32_t lower);
        static std::tuple<uint8_t, uint32_t, uint32_t> decodeSingleRegReadID(uint32_t higher, uint32_t lower);

	void readUpdateWriteNamedReg(std::string name) override;
	void readUpdateWriteReg(Itkpixv2RegDefault Itkpixv2GlobalCfg::*ref);
        uint32_t readSingleRegister(Itkpixv2RegDefault Itkpixv2GlobalCfg::*ref);
        
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
