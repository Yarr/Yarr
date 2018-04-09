#include "AllChips.h"
#include "Fe65p2.h"

bool fe65p2_registered =
  StdDict::registerFrontEnd("FE65P2",
                                []() { return std::unique_ptr<FrontEnd>(new Fe65p2());});

Fe65p2::Fe65p2() : Fe65p2Cmd() {
    txChannel = 99;
    rxChannel = 99;
    geo.nRow = 64;
    geo.nCol = 64;
}

Fe65p2::Fe65p2(TxCore *arg_core) : Fe65p2Cmd(arg_core) {
    txChannel = 99;
    rxChannel = 99;
    geo.nRow = 64;
    geo.nCol = 64;
}

Fe65p2::Fe65p2(TxCore *arg_core, unsigned arg_channel) : Fe65p2Cmd(arg_core){
    txChannel = arg_channel;
    rxChannel = arg_channel;
    geo.nRow = 64;
    geo.nCol = 64;
}

Fe65p2::Fe65p2(TxCore *arg_core, unsigned arg_txChannel, unsigned arg_rxChannel) : Fe65p2Cmd(arg_core){
    txChannel = arg_txChannel;
    rxChannel = arg_rxChannel;
    geo.nRow = 64;
    geo.nCol = 64;
}

void Fe65p2::init(TxCore *arg_core, unsigned arg_txChannel, unsigned arg_rxChannel) {
    this->setCore(arg_core);
    txChannel = arg_txChannel;
    rxChannel = arg_rxChannel;
}

void Fe65p2::configure() {
    this->configureInit();
    this->configureGlobal();
    this->configurePixels();
}

void Fe65p2::configureInit() {
    clocksOn();
    reset();
}

void Fe65p2::configureGlobal() {
    writeGlobal(Fe65p2Cfg::cfg);
    setTrigCount(Fe65p2Cfg::trigCountReg);
    //setPlsrDac(Fe65p2Cfg::dacReg);
}

void Fe65p2::configDac() {
    setPlsrDac(Fe65p2Cfg::plsrDacReg);
}

void Fe65p2::configurePixels() {
    // Set threshold high
    uint16_t tmp1 = getValue(&Fe65p2::Vthin1Dac);
    uint16_t tmp2 = getValue(&Fe65p2::Vthin2Dac);
    uint16_t tmp3 = getValue(&Fe65p2::PreCompVbnDac);
    uint16_t tmp4 = getValue(&Fe65p2::CompVbnDac);
    uint16_t tmp5 = getValue(&Fe65p2::VffDac);
    uint16_t tmp6 = getValue(&Fe65p2::PrmpVbpDac);
    uint16_t tmp7 = getValue(&Fe65p2::PrmpVbnFolDac);
    setValue(&Fe65p2::Vthin1Dac, 255);
    setValue(&Fe65p2::Vthin2Dac, 0);
    //setValue(&Fe65p2::PreCompVbnDac, 0);
    //setValue(&Fe65p2::CompVbnDac, 0);
    setValue(&Fe65p2::VffDac, 10);
    //setValue(&Fe65p2::PrmpVbpDac, 50);
    setValue(&Fe65p2::PrmpVbnFolDac, 0);
    
   
    uint16_t colEn = getValue(&Fe65p2::ColEn);
    uint16_t colSrEn = getValue(&Fe65p2::ColSrEn);
    // Turn all off
    setValue(&Fe65p2::ColSrEn, 0xFFFF);
    setValue(&Fe65p2::ColEn, 0xFFFF);
    configureGlobal();
    writePixel((uint16_t) 0x0);
    setValue(&Fe65p2::PixConfLd, 0x3);
    configureGlobal();
    setValue(&Fe65p2::PixConfLd, 0x0);
    configureGlobal();
    
    // Configure global regs
    //configureGlobal();
    // Configure pixel regs
    for(unsigned i=0; i<Fe65p2Cfg::n_QC; i++) {
        setValue(&Fe65p2::ColSrEn, (0x1<<i));
        setValue(&Fe65p2::OneSr, 0);
        configureGlobal();
        for(unsigned bit=0; bit<Fe65p2Cfg::n_Bits; bit++) {
            writePixel(getCfg(bit, i));
            
            switch (bit) {
                case 0:
                    setValue(&Fe65p2::SignLd, 0x1);
                    break;
                case 1:
                    setValue(&Fe65p2::InjEnLd, 0x1);
                    break;
                case 2:
                    setValue(&Fe65p2::TDacLd, 0x1);
                    break;
                case 3:
                    setValue(&Fe65p2::TDacLd, 0x2);
                    break;
                case 4:
                    setValue(&Fe65p2::TDacLd, 0x4);
                    break;
                case 5:
                    setValue(&Fe65p2::TDacLd, 0x8);
                    break;
                case 6:
                    setValue(&Fe65p2::PixConfLd, 0x1);
                    break;
                case 7:
                    setValue(&Fe65p2::PixConfLd, 0x2);
                    break;
                default:
                    break;
            }

            configureGlobal();

            // Unset Shadow registers
            setValue(&Fe65p2::SignLd, 0x0);
            setValue(&Fe65p2::InjEnLd, 0x0);
            setValue(&Fe65p2::TDacLd, 0x0);
            setValue(&Fe65p2::PixConfLd, 0x0);
            configureGlobal();

            writePixel((uint16_t) 0x0);
        }
    }
    // Reset SR
    writePixel((uint16_t) 0x0);
    // Reset threshold
    setValue(&Fe65p2::Vthin1Dac, tmp1);
    setValue(&Fe65p2::Vthin2Dac, tmp2);
    setValue(&Fe65p2::PreCompVbnDac, tmp3);
    setValue(&Fe65p2::CompVbnDac, tmp4);
    setValue(&Fe65p2::VffDac, tmp5);
    setValue(&Fe65p2::PrmpVbpDac, tmp6);
    setValue(&Fe65p2::PrmpVbnFolDac, tmp7);

    setValue(&Fe65p2::ColSrEn, colSrEn);
    setValue(&Fe65p2::ColEn, colEn);
    configureGlobal();

}

void Fe65p2::writeNamedRegister(std::string name, uint16_t reg_value) {
    regMap[name]->write(reg_value);
}
