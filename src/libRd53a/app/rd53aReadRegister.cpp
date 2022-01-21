#include <iostream>
#include <chrono>
#include <unistd.h>
#include "SpecController.h"
#include "Rd53a.h"
#include <bitset>

#define EN_RX2 0x1
#define EN_RX1 0x2
#define EN_RX4 0x4
#define EN_RX3 0x8
#define EN_RX6 0x10
#define EN_RX5 0x20
#define EN_RX8 0x40
#define EN_RX7 0x80

#define EN_RX10 0x100
#define EN_RX9 0x200
#define EN_RX12 0x400
#define EN_RX11 0x800
#define EN_RX14 0x1000
#define EN_RX13 0x2000
#define EN_RX16 0x4000
#define EN_RX15 0x8000

#define EN_RX18 0x10000
#define EN_RX17 0x20000
#define EN_RX20 0x40000
#define EN_RX19 0x80000
#define EN_RX22 0x100000
#define EN_RX21 0x200000
#define EN_RX24 0x400000
#define EN_RX23 0x800000

void decode(RawData *data) {
  if (data != NULL) {
    int frameNum = 0;
    unsigned zz = 0;
    unsigned stat = 0;
    unsigned addr1 = 0;
    unsigned val1 = 0;
    unsigned val1_1 = 0;
    unsigned val1_2 = 0;
    unsigned addr2 = 0;
    unsigned val2 = 0;

    for (unsigned i=0; i<data->words; i++) {
      if (data->buf[i] != 0xFFFFFFFF) {
	// std::cout << "[RawBuf]" << std::bitset<32>(data->buf[i]) << std::endl;
	if(frameNum%2==0) {
	  zz = 0;
	  stat = 0;
	  addr1 = 0;
	  val1 = 0;
	  val1_1 = 0;
	  val1_2 = 0;
	  addr2 = 0;
	  val2 = 0;

	  zz = 0xFF & (data->buf[i] >> 24);

	  if(!(zz == 0x55 || zz == 0x99 || zz == 0xd2)) {
	    std::cout << "wrong Aurora code" << std::endl;
	    std::cout << "[zz]" << std::hex << zz << std::dec << std::endl;
	    return;
	  }

	  stat = 0xF & (data->buf[i] >> 20);
	  addr1 = 0x3FF & (data->buf[i] >> 10);
	  val1_1 = (0x3FF & (data->buf[i] >> 0)) << 6;
	}
	else {

	  val1_2 = 0x3F & (data->buf[i] >> 26);
	  val1 = val1_1 + val1_2;

	  addr2 = 0x3FF & (data->buf[i] >> 16);
	  val2 = 0xFFFF & (data->buf[i] >> 0);

	  // display results
	  // std::cout << "[zz]" << std::hex << zz << std::dec << std::endl;
	  if(stat != 0) {
	    std::cout << "not Ready status!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl;
	    std::cout << "[stat]" << stat << std::endl;
	  }
	  if(zz == 0x99 || zz == 0xd2) {
	    std::cout << " [addr1]" << addr1 << std::endl;
	    std::cout << "[val1]" << val1 << std::endl;
	  }
	  if(zz == 0x55 || zz == 0xd2) {
	    std::cout << " [addr2]" << addr2 << std::endl;
	    std::cout << "[val2]" << val2 << std::endl;
	  }

	}
      }
      frameNum++;
    }
  }
}

int main(void) {

    SpecController spec;
    spec.init(0);

    //Send IO config to active FMC
    spec.writeSingle(0x6<<14 | 0x0, EN_RX1 | EN_RX3 | EN_RX4 | EN_RX5);
    spec.writeSingle(0x6<<14 | 0x1, 0xF);
    spec.setCmdEnable(0x1);
    spec.setRxEnable(0x0);

    Rd53a fe(&spec);
    fe.setChipId(0);
    // std::cout << ">>> Configuring chip with default config ..." << std::endl;
    // fe.configure();
    // std::cout << " ... done." << std::endl;
    // std::this_thread::sleep_for(std::chrono::milliseconds(1));


    fe.configureInit();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // following registers have to be set to different values from the default values
    fe.writeRegister(&Rd53a::OutputActiveLanes, 0xF);
    fe.writeRegister(&Rd53a::CdrSelSerClk, 3);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // TODO check link sync
    spec.setRxEnable(0x1);


        // 0
        // fe.readRegister(&Rd53a::PixPortal);

        //1
        fe.readRegister(&Rd53a::PixRegionCol);
        //2
        fe.readRegister(&Rd53a::PixRegionRow);
        //3
        fe.readRegister(&Rd53a::PixBroadcastEn);
        fe.readRegister(&Rd53a::PixAutoCol);
        fe.readRegister(&Rd53a::PixAutoRow);
        fe.readRegister(&Rd53a::PixBroadcastMask);
        //4
        fe.readRegister(&Rd53a::PixDefaultConfig);

        // Sync FE
        //5
        fe.readRegister(&Rd53a::SyncIbiasp1);
        //6
        fe.readRegister(&Rd53a::SyncIbiasp2);
        //7
        fe.readRegister(&Rd53a::SyncIbiasSf);
        //8
        fe.readRegister(&Rd53a::SyncIbiasKrum);
        //9
        fe.readRegister(&Rd53a::SyncIbiasDisc);
        //10
        fe.readRegister(&Rd53a::SyncIctrlSynct);
        //11
        fe.readRegister(&Rd53a::SyncVbl);
        //12
        fe.readRegister(&Rd53a::SyncVth);
        //13
        fe.readRegister(&Rd53a::SyncVrefKrum);
        //30 ***ooo
        fe.readRegister(&Rd53a::SyncAutoZero);
        fe.readRegister(&Rd53a::SyncSelC2F);
        fe.readRegister(&Rd53a::SyncSelC4F);
        fe.readRegister(&Rd53a::SyncFastTot);

        // Linear FE
        //14
        fe.readRegister(&Rd53a::LinPaInBias);
        //15
        fe.readRegister(&Rd53a::LinFcBias);
        //16
        fe.readRegister(&Rd53a::LinKrumCurr);
        //17
        fe.readRegister(&Rd53a::LinLdac);
        //18
        fe.readRegister(&Rd53a::LinComp);
        //19
        fe.readRegister(&Rd53a::LinRefKrum);
        //20
        fe.readRegister(&Rd53a::LinVth);

        // Diff FE
        //21
        fe.readRegister(&Rd53a::DiffPrmp);
        //22
        fe.readRegister(&Rd53a::DiffFol);
        //23
        fe.readRegister(&Rd53a::DiffPrecomp);
        //24
        fe.readRegister(&Rd53a::DiffComp);
        //25
        fe.readRegister(&Rd53a::DiffVff);
        //26
        fe.readRegister(&Rd53a::DiffVth1);
        //27
        fe.readRegister(&Rd53a::DiffVth2);
        //28
        fe.readRegister(&Rd53a::DiffLcc);
        //29
        fe.readRegister(&Rd53a::DiffLccEn);
        fe.readRegister(&Rd53a::DiffFbCapEn);

        // Power
        //31
        fe.readRegister(&Rd53a::SldoAnalogTrim);
        fe.readRegister(&Rd53a::SldoDigitalTrim);

        // Digital Matrix
        //32
        fe.readRegister(&Rd53a::EnCoreColSync);
        //33
        fe.readRegister(&Rd53a::EnCoreColLin1);
        //34
        fe.readRegister(&Rd53a::EnCoreColLin2);
        //35
        fe.readRegister(&Rd53a::EnCoreColDiff1);
        //36
        fe.readRegister(&Rd53a::EnCoreColDiff2);
        //37
        fe.readRegister(&Rd53a::LatencyConfig);
        //38
        fe.readRegister(&Rd53a::WrSyncDelaySync);

        // Injection
        //39
        fe.readRegister(&Rd53a::InjEnDig);
        fe.readRegister(&Rd53a::InjAnaMode);
        fe.readRegister(&Rd53a::InjDelay);
        //41
        fe.readRegister(&Rd53a::InjVcalHigh);
        //42
        fe.readRegister(&Rd53a::InjVcalMed);
        //46
        fe.readRegister(&Rd53a::CalColprSync1);
        //47
        fe.readRegister(&Rd53a::CalColprSync2);
        //48
        fe.readRegister(&Rd53a::CalColprSync3);
        //49
        fe.readRegister(&Rd53a::CalColprSync4);
        //50
        fe.readRegister(&Rd53a::CalColprLin1);
        //51
        fe.readRegister(&Rd53a::CalColprLin2);
        //52
        fe.readRegister(&Rd53a::CalColprLin3);
        //53
        fe.readRegister(&Rd53a::CalColprLin4);
        //54
        fe.readRegister(&Rd53a::CalColprLin5);
        //55
        fe.readRegister(&Rd53a::CalColprDiff1);
        //56
        fe.readRegister(&Rd53a::CalColprDiff2);
        //57
        fe.readRegister(&Rd53a::CalColprDiff3);
        //58
        fe.readRegister(&Rd53a::CalColprDiff4);
        //59
        fe.readRegister(&Rd53a::CalColprDiff5);

        // Digital Functions
        //40 ***ooo
        fe.readRegister(&Rd53a::ClkDelaySel);
        fe.readRegister(&Rd53a::ClkDelay);
        fe.readRegister(&Rd53a::CmdDelay);
        //43
        fe.readRegister(&Rd53a::ChSyncPhase);
        fe.readRegister(&Rd53a::ChSyncLock);
        fe.readRegister(&Rd53a::ChSyncUnlock);
        //44
        fe.readRegister(&Rd53a::GlobalPulseRt);

        // I/O
        //60
        fe.readRegister(&Rd53a::DebugConfig);
        //61
        fe.readRegister(&Rd53a::OutputDataReadDelay);
        fe.readRegister(&Rd53a::OutputSerType);
        fe.readRegister(&Rd53a::OutputActiveLanes);
        fe.readRegister(&Rd53a::OutputFmt);
        //62
        fe.readRegister(&Rd53a::OutPadConfig);
        //63
        fe.readRegister(&Rd53a::GpLvdsRoute);
        //64
        fe.readRegister(&Rd53a::CdrSelDelClk);
        fe.readRegister(&Rd53a::CdrPdSel);
        fe.readRegister(&Rd53a::CdrPdDel);
        fe.readRegister(&Rd53a::CdrEnGck);
        fe.readRegister(&Rd53a::CdrVcoGain);
        fe.readRegister(&Rd53a::CdrSelSerClk);
        //65
        fe.readRegister(&Rd53a::VcoBuffBias);
        //66
        fe.readRegister(&Rd53a::CdrCpIbias);
        //67
        fe.readRegister(&Rd53a::VcoIbias);
        //68
        fe.readRegister(&Rd53a::SerSelOut0);
        fe.readRegister(&Rd53a::SerSelOut1);
        fe.readRegister(&Rd53a::SerSelOut2);
        fe.readRegister(&Rd53a::SerSelOut3);
        //69
        fe.readRegister(&Rd53a::CmlInvTap);
        fe.readRegister(&Rd53a::CmlEnTap);
        fe.readRegister(&Rd53a::CmlEn);
        //70
        fe.readRegister(&Rd53a::CmlTapBias0);
        //71
        fe.readRegister(&Rd53a::CmlTapBias1);
        //72
        fe.readRegister(&Rd53a::CmlTapBias2);
        //73
        fe.readRegister(&Rd53a::AuroraCcWait);
        fe.readRegister(&Rd53a::AuroraCcSend);
        //74
        fe.readRegister(&Rd53a::AuroraCbWaitLow);
        fe.readRegister(&Rd53a::AuroraCbSend);
        //75
        fe.readRegister(&Rd53a::AuroraCbWaitHigh);
        //76
        fe.readRegister(&Rd53a::AuroraInitWait);
        //45
        fe.readRegister(&Rd53a::MonFrameSkip);
        //101
        fe.readRegister(&Rd53a::AutoReadA0);
        //102
        fe.readRegister(&Rd53a::AutoReadB0);
        //103
        fe.readRegister(&Rd53a::AutoReadA1);
        //104
        fe.readRegister(&Rd53a::AutoReadB1);
        //105
        fe.readRegister(&Rd53a::AutoReadA2);
        //106
        fe.readRegister(&Rd53a::AutoReadB2);
        //107
        fe.readRegister(&Rd53a::AutoReadA3);
        //108
        fe.readRegister(&Rd53a::AutoReadB3);

        // Test & Monitoring
        //77
        fe.readRegister(&Rd53a::MonitorEnable);
        fe.readRegister(&Rd53a::MonitorImonMux);
        fe.readRegister(&Rd53a::MonitorVmonMux);
        //78-81
        fe.readRegister(&Rd53a::HitOr0MaskSync);
        fe.readRegister(&Rd53a::HitOr1MaskSync);
        fe.readRegister(&Rd53a::HitOr2MaskSync);
        fe.readRegister(&Rd53a::HitOr3MaskSync);
        //82-89
        fe.readRegister(&Rd53a::HitOr0MaskLin0);
        fe.readRegister(&Rd53a::HitOr0MaskLin1);
        fe.readRegister(&Rd53a::HitOr1MaskLin0);
        fe.readRegister(&Rd53a::HitOr1MaskLin1);
        fe.readRegister(&Rd53a::HitOr2MaskLin0);
        fe.readRegister(&Rd53a::HitOr2MaskLin1);
        fe.readRegister(&Rd53a::HitOr3MaskLin0);
        fe.readRegister(&Rd53a::HitOr3MaskLin1);
        //90-97
        fe.readRegister(&Rd53a::HitOr0MaskDiff0);
        fe.readRegister(&Rd53a::HitOr0MaskDiff1);
        fe.readRegister(&Rd53a::HitOr1MaskDiff0);
        fe.readRegister(&Rd53a::HitOr1MaskDiff1);
        fe.readRegister(&Rd53a::HitOr2MaskDiff0);
        fe.readRegister(&Rd53a::HitOr2MaskDiff1);
        fe.readRegister(&Rd53a::HitOr3MaskDiff0);
        fe.readRegister(&Rd53a::HitOr3MaskDiff1);
        //98
        fe.readRegister(&Rd53a::AdcRefTrim);
        fe.readRegister(&Rd53a::AdcTrim);
        //99
        fe.readRegister(&Rd53a::SensorCfg0);
        fe.readRegister(&Rd53a::SensorCfg1);
        //109
        fe.readRegister(&Rd53a::RingOscEn);
        //110-117
        fe.readRegister(&Rd53a::RingOsc0);
        fe.readRegister(&Rd53a::RingOsc1);
        fe.readRegister(&Rd53a::RingOsc2);
        fe.readRegister(&Rd53a::RingOsc3);
        fe.readRegister(&Rd53a::RingOsc4);
        fe.readRegister(&Rd53a::RingOsc5);
        fe.readRegister(&Rd53a::RingOsc6);
        fe.readRegister(&Rd53a::RingOsc7);
        //118
        fe.readRegister(&Rd53a::BcCounter);
        //119
        fe.readRegister(&Rd53a::TrigCounter);
        //120
        fe.readRegister(&Rd53a::LockLossCounter);
        //121
        fe.readRegister(&Rd53a::BflipWarnCounter);
        //122
        fe.readRegister(&Rd53a::BflipErrCounter);
        //123
        fe.readRegister(&Rd53a::CmdErrCounter);
        //124-127
        fe.readRegister(&Rd53a::FifoFullCounter0);
        fe.readRegister(&Rd53a::FifoFullCounter1);
        fe.readRegister(&Rd53a::FifoFullCounter2);
        fe.readRegister(&Rd53a::FifoFullCounter3);
        //128
        fe.readRegister(&Rd53a::AiPixCol);
        //129
        fe.readRegister(&Rd53a::AiPixRow);
        //130-133
        fe.readRegister(&Rd53a::HitOrCounter0);
        fe.readRegister(&Rd53a::HitOrCounter1);
        fe.readRegister(&Rd53a::HitOrCounter2);
        fe.readRegister(&Rd53a::HitOrCounter3);
        //134
        fe.readRegister(&Rd53a::SkipTriggerCounter);
        //135
        fe.readRegister(&Rd53a::ErrMask);
        //136
        fe.readRegister(&Rd53a::AdcRead);
        //137
        fe.readRegister(&Rd53a::SelfTrigEn);


    RawData *data = NULL;
    do {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
      if (data != NULL)
	delete data;
      data = spec.readData();
      decode(data);
    } while (data != NULL);

    spec.setRxEnable(0x0);
    return 0;
}
