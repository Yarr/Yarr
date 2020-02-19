#ifndef ABC_STAR_CFG_INCLUDE
#define ABC_STAR_CFG_INCLUDE

// #################################
// # Project:
// # Description: StarChips Library
// # Comment: ABC Star configuration class
// ################################

#include "enum.h"

// Name different ABC registers that can be used
BETTER_ENUM(ABCStarRegs, int,
            SCReg=0, ADCS1=1, ADCS2=2, ADCS3=3, ADCS4=4, ADCS5=5, ADCS6=6, ADCS7=7,
            MaskInput0=16, MaskInput1=17, MaskInput2=18, MaskInput3=19, MaskInput4=20, MaskInput5=21, MaskInput6=22, MaskInput7=23,
            CREG0=32, CREG1=33, CREG2=34, CREG3=35, CREG4=36, CREG5=37, CREG6=38,
            TrimDAC0=64, TrimDAC1=65, TrimDAC2=66, TrimDAC3=67, TrimDAC4=68, TrimDAC5=69, TrimDAC6=70, TrimDAC7=71, TrimDAC8=72, TrimDAC9=73, TrimDAC10=74, TrimDAC11=75, TrimDAC12=76, TrimDAC13=77, TrimDAC14=78, TrimDAC15=79, TrimDAC16=80, TrimDAC17=81, TrimDAC18=82, TrimDAC19=83, TrimDAC20=84, TrimDAC21=85, TrimDAC22=86, TrimDAC23=87, TrimDAC24=88, TrimDAC25=89, TrimDAC26=90, TrimDAC27=91, TrimDAC28=92, TrimDAC29=93, TrimDAC30=94, TrimDAC31=95, TrimDAC32=96, TrimDAC33=97, TrimDAC34=98, TrimDAC35=99, TrimDAC36=100, TrimDAC37=101, TrimDAC38=102, TrimDAC39=103,
            CalREG0=104, CalREG1=105, CalREG2=106, CalREG3=107, CalREG4=108, CalREG5=109, CalREG6=110, CalREG7=111)

//Name different ABC subregisters that can be used for configuration, scans, etc.

BETTER_ENUM(ABCStarSubRegister, int,
            RRFORCE=1, WRITEDISABLE, STOPHPR, TESTHPR, EFUSEL, LCBERRCNTCLR,
            BVREF, BIREF, B8BREF, BTRANGE,
            BVT, COMBIAS, BIFEED, BIPRE,
            STR_DEL_R, STR_DEL, BCAL, BCAL_RANGE,
            ADC_BIAS, ADC_CH, ADC_ENABLE,
            D_S, D_LOW, D_EN_CTRL,
            BTMUX, BTMUXD, A_S, A_EN_CTRL,
            TEST_PULSE_ENABLE, ENCOUNT, MASKHPR, PR_ENABLE, LP_ENABLE, RRMODE, TM, TESTPATT_ENABLE, TESTPATT1, TESTPATT2,
            CURRDRIV, CALPULSE_ENABLE, CALPULSE_POLARITY,
            LATENCY, BCFLAG_ENABLE, BCOFFSET,
            DETMODE, MAX_CLUSTER, MAX_CLUSTER_ENABLE,
            EN_CLUSTER_EMPTY, EN_CLUSTER_FULL, EN_CLUSTER_OVFL, EN_REGFIFO_EMPTY, EN_REGFIFO_FULL, EN_REGFIFO_OVFL, EN_LPFIFO_EMPTY, EN_LPFIFO_FULL, EN_PRFIFO_EMPTY, EN_PRFIFO_FULL, EN_LCB_LOCKED, EN_LCB_DECODE_ERR, EN_LCB_ERRCNT_OVFL, EN_LCB_SCMD_ERR,
            DOFUSE,
            LCB_ERRCOUNT_THR)

class ABCStarRegister : public ABCStarRegs {
 public:
 ABCStarRegister(const ABCStarRegs & other) : ABCStarRegs::ABCStarRegs(other) {};
  static  ABCStarRegister MaskInput(int i) { return ABCStarRegs::_from_integral((int)(ABCStarRegs::MaskInput0) + i);};
};

#endif
