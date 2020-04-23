#include "AbcCfg.h"

#include "logging.h"

namespace {
  auto logger = logging::make_log("StarCfgABC");
}

std::shared_ptr<AbcStarRegInfo> AbcStarRegInfo::m_instance;

//Register enums definitions
typedef std::tuple<ABCStarSubRegister, unsigned int, unsigned int, unsigned int> abcsubregdef;
const std::vector<abcsubregdef> s_abcsubregdefs = {
  {ABCStarSubRegister::RRFORCE			,0	,0	,1}	,
  {ABCStarSubRegister::WRITEDISABLE		,0	,1	,1}	,
  {ABCStarSubRegister::STOPHPR			,0	,2	,1}	,
  {ABCStarSubRegister::TESTHPR			,0	,3	,1}	,
  {ABCStarSubRegister::EFUSEL			,0	,4	,1}	,
  {ABCStarSubRegister::LCBERRCNTCLR		,0	,5	,1}	,
  {ABCStarSubRegister::BVREF			,1	,0	,5}	,
  {ABCStarSubRegister::BIREF			,1	,8	,5}	,
  {ABCStarSubRegister::B8BREF			,1	,16	,5}	,
  {ABCStarSubRegister::BTRANGE			,1	,24	,5}	,
  {ABCStarSubRegister::BVT			,2	,0	,8}	,
  {ABCStarSubRegister::COMBIAS			,2	,8	,5}	,
  {ABCStarSubRegister::BIFEED			,2	,16	,5}	,
  {ABCStarSubRegister::BIPRE			,2	,24	,5}	,
  {ABCStarSubRegister::STR_DEL_R		,3	,0	,2}	,
  {ABCStarSubRegister::STR_DEL			,3	,8	,6}	,
  {ABCStarSubRegister::BCAL			,3	,16	,9}	,
  {ABCStarSubRegister::BCAL_RANGE		,3	,25	,1}	,
  {ABCStarSubRegister::ADC_BIAS			,4	,0	,4}	,
  {ABCStarSubRegister::ADC_CH			,4	,4	,4}	,
  {ABCStarSubRegister::ADC_ENABLE		,4	,8	,1}	,
  {ABCStarSubRegister::D_S			,6	,0	,15}	,
  {ABCStarSubRegister::D_LOW			,6	,15	,1}	,
  {ABCStarSubRegister::D_EN_CTRL		,6	,16	,1}	,
  {ABCStarSubRegister::BTMUX			,7	,0	,14}	,
  {ABCStarSubRegister::BTMUXD			,7	,14	,1}	,
  {ABCStarSubRegister::A_S			,7	,15	,15}	,
  {ABCStarSubRegister::A_EN_CTRL		,7	,31	,1}	,
  {ABCStarSubRegister::TEST_PULSE_ENABLE	,32	,4	,1}	,
  {ABCStarSubRegister::ENCOUNT			,32	,5	,1}	,
  {ABCStarSubRegister::MASKHPR			,32	,6	,1}	,
  {ABCStarSubRegister::PR_ENABLE		,32	,8	,1}	,
  {ABCStarSubRegister::LP_ENABLE		,32	,9	,1}	,
  {ABCStarSubRegister::RRMODE			,32	,10	,2}	,
  {ABCStarSubRegister::TM			,32	,16	,2}	,
  {ABCStarSubRegister::TESTPATT_ENABLE		,32	,18	,1}	,
  {ABCStarSubRegister::TESTPATT1		,32	,20	,4}	,
  {ABCStarSubRegister::TESTPATT2		,32	,24	,4}	,
  {ABCStarSubRegister::CURRDRIV			,33	,0	,3}	,
  {ABCStarSubRegister::CALPULSE_ENABLE		,33	,4	,1}	,
  {ABCStarSubRegister::CALPULSE_POLARITY	,33	,5	,1}	,
  {ABCStarSubRegister::LATENCY			,34	,0	,9}	,
  {ABCStarSubRegister::BCFLAG_ENABLE		,34	,23	,1}	,
  {ABCStarSubRegister::BCOFFSET			,34	,24	,8}	,
  {ABCStarSubRegister::DETMODE			,35	,0	,2}	,
  {ABCStarSubRegister::MAX_CLUSTER		,35	,12	,6}	,
  {ABCStarSubRegister::MAX_CLUSTER_ENABLE	,35	,18	,1}	,
  {ABCStarSubRegister::EN_CLUSTER_EMPTY		,36	,0	,1}	,
  {ABCStarSubRegister::EN_CLUSTER_FULL		,36	,1	,1}	,
  {ABCStarSubRegister::EN_CLUSTER_OVFL		,36	,2	,1}	,
  {ABCStarSubRegister::EN_REGFIFO_EMPTY		,36	,3	,1}	,
  {ABCStarSubRegister::EN_REGFIFO_FULL		,36	,4	,1}	,
  {ABCStarSubRegister::EN_REGFIFO_OVFL		,36	,5	,1}	,
  {ABCStarSubRegister::EN_LPFIFO_EMPTY		,36	,6	,1}	,
  {ABCStarSubRegister::EN_LPFIFO_FULL		,36	,7	,1}	,
  {ABCStarSubRegister::EN_PRFIFO_EMPTY		,36	,8	,1}	,
  {ABCStarSubRegister::EN_PRFIFO_FULL		,36	,9	,1}	,
  {ABCStarSubRegister::EN_LCB_LOCKED		,36	,10	,1}	,
  {ABCStarSubRegister::EN_LCB_DECODE_ERR	,36	,11	,1}	,
  {ABCStarSubRegister::EN_LCB_ERRCNT_OVFL	,36	,12	,1}	,
  {ABCStarSubRegister::EN_LCB_SCMD_ERR		,36	,13	,1}	,
  // {ABCStarSubRegister::DOFUSE			,37	,0	,24}	,
  {ABCStarSubRegister::LCB_ERRCOUNT_THR	        ,38	,0	,16}
};

AbcStarRegInfo::AbcStarRegInfo() {
    for (ABCStarRegister reg : ABCStarRegister::_values()) {
        int addr = reg;
        abcregisterMap[addr] = std::make_shared<RegisterInfo>(addr);
    }

    for (auto def : s_abcsubregdefs) {
        auto reg_id = std::get<0>(def);
        std::string subregname = std::string(reg_id._to_string());
        auto addr = std::get<1>(def);
        auto offset = std::get<2>(def);
        auto width = std::get<3>(def);
        abcSubRegisterMap_all[reg_id] = abcregisterMap[addr]->addSubRegister(subregname, offset, width);
    }

    ////# 256 TrimDac regs 4-bit lsb
    int channel=0;
    for(int i=ABCStarRegister::TrimDAC0; i<=ABCStarRegister::TrimDAC31;i++){
        int nthStartBit = 0;
        for(int j=0; j<8;j++){

            std::string trimDAC_name = "trimdac_4lsb_"+std::to_string(channel);
            ////std::to_string(((channel>>7)&1)+1)+"_"+std::to_string((channel&0x7f)+1); //trimdac_4lsb_<nthRow>_<nthCol>; row(1-2); col(1-256); match to histogram
            //  		std::cout << " reg[" << i <<"] for channel[" << channel  << "]----->" << trimDAC_name <<  "     @ nthStartBit: "<< nthStartBit<< std::endl;
            trimDAC_4LSB_RegisterMap_all[channel] = abcregisterMap[i]->addSubRegister(trimDAC_name, nthStartBit, 4);
            channel++;
            nthStartBit+=4;
        }
    }

    ////# 256 TrimDac regs 1-bit msb
    channel = 0;
    for(int i=ABCStarRegister::TrimDAC32; i<=ABCStarRegister::TrimDAC39;i++){
        for(int j=0; j<32;j++){
            std::string trimDAC_name = "trimdac_1msb_"+std::to_string(channel);
            ////std::to_string(((channel>>7)&1)+1)+"_"+std::to_string((channel&0x7f)+1); //trimdac_1msb_<nthRow>_<nthCol>; row(1-2); col(1-256); match to histogram
            //   		std::cout << " reg[" << i <<"] for channel[" << channel  << "]----->" << trimDAC_name << std::endl;;
            trimDAC_1MSB_RegisterMap_all[channel] = abcregisterMap[i]->addSubRegister(trimDAC_name, j, 1);
            channel++;
        }
    }
}

AbcCfg::AbcCfg()
  : m_info(AbcStarRegInfo::instance())
{
    setupMaps();
    setDefaults();
}

void AbcCfg::setupMaps() {
    auto len = ABCStarRegister::_size();
    // In case it's not already empty
    m_registerSet.clear();
    m_registerSet.reserve( len );

    /// TODO Still not sure if this is a good implementation; to-be-optimized.

    //DD    //Loop over each ABC register in the default list, and create the Register object
    //DD    //Add the location in memory of this Register to the register maps
    for (ABCStarRegister reg : ABCStarRegister::_values()) {
        int addr = reg;
        Register tmp_Reg(m_info->abcregisterMap[addr], 0);
        m_registerSet.push_back( std::move(tmp_Reg) ); //Save it to the list
        int lastReg = m_registerSet.size()-1;
        m_registerMap[addr] = lastReg; //Save it's position in memory to the registerMap
    }

    if(m_registerSet.size() != len) {
      logger->info("Mismatch between size {} and values {}", len, m_registerSet.size());
    }
}

void AbcCfg::setDefaults() {
    //// Initialize 32-bit register with default values
    ////#special reg
    getRegister(ABCStarRegister::SCReg).setValue(0x00000000);

    ////#Analog and DCS regs
    for (unsigned int iReg=ABCStarRegister::ADCS1; iReg<=ABCStarRegister::ADCS7; iReg++)
        getRegister(iReg).setValue(0x00000000);

    ////#Congfiguration regs
    for (unsigned int iReg=ABCStarRegister::CREG0; iReg<=ABCStarRegister::CREG6; iReg++) {
        if(iReg == ABCStarRegister::CREG0 + 5) {
            // Skip CREG5 as it's fuse register
            continue;
        }
        getRegister(iReg).setValue(0x00000000);
    }

    ////# Input (Mask) regs
    for (unsigned int iReg=ABCStarRegister::MaskInput0; iReg<=ABCStarRegister::MaskInput7; iReg++)
        getRegister(iReg).setValue(0x00000000);

    ////# Calibration Enable regs
    for (unsigned int iReg=ABCStarRegister::CalREG0; iReg<=ABCStarRegister::CalREG7; iReg++)
        getRegister(iReg).setValue(0xFFFFFFFF);

    ////# 256 TrimDac regs 4-bit lsb
    int channel=0;
    for(int i=ABCStarRegister::TrimDAC0; i<=ABCStarRegister::TrimDAC31;i++){
        getRegister(i).setValue(0xFFFFFFFF);
    }

    ////# 256 TrimDac regs 1-bit msb
    channel = 0;
    for(int i=ABCStarRegister::TrimDAC32; i<=ABCStarRegister::TrimDAC39;i++){
        getRegister(i).setValue(0x00000000);
    }
}

void AbcCfg::setTrimDACRaw(unsigned channel, int value) {
    std::string trimDAC_1msb_name = "trimdac_1msb_"+std::to_string(channel);

    if (m_info->trimDAC_4LSB_RegisterMap_all.find(channel) != m_info->trimDAC_4LSB_RegisterMap_all.end()) {
        auto info = m_info->trimDAC_4LSB_RegisterMap_all[channel];
        getRegister(info->m_regAddress).getSubRegister(info).updateValue(value&0xf);
    } else {
        logger->error("Could not find sub register for 4LSB for channel {} for chip[ID {}]",
                      channel, getABCchipID());
    }

    if (m_info->trimDAC_1MSB_RegisterMap_all.find(channel) != m_info->trimDAC_1MSB_RegisterMap_all.end()) {
        //		std::cout << " value: " << value << "  " << ((value>>4)&0x1) << std::endl;
        auto info = m_info->trimDAC_1MSB_RegisterMap_all[channel];
        getRegister(info->m_regAddress).getSubRegister(info).updateValue((value>>4)&0x1);
    } else {
        logger->error("Could not find sub register for 1MSB for channel {} for chip[ID {}]", channel, getABCchipID());
  }
}

int AbcCfg::getTrimDACRaw(unsigned channel) const {
    std::string trimDAC_4lsb_name = "trimdac_4lsb_"+std::to_string(channel);
    std::string trimDAC_1msb_name = "trimdac_1msb_"+std::to_string(channel);

    if (m_info->trimDAC_4LSB_RegisterMap_all.find(channel) == m_info->trimDAC_4LSB_RegisterMap_all.end()) {
        logger->error("Could not find sub register for 4LSB for channel {} for chip[ID {}]",
                      channel, getABCchipID());
        return 0;
    }

    auto info4 = m_info->trimDAC_4LSB_RegisterMap_all[channel];

    if (m_info->trimDAC_1MSB_RegisterMap_all.find(channel) == m_info->trimDAC_1MSB_RegisterMap_all.end()) {
        logger->error("Could not find sub register for 1MSB for channel {} for chip[ID {}]", channel, getABCchipID());
        return 0;
    }

    auto info1 = m_info->trimDAC_1MSB_RegisterMap_all[channel];

    unsigned trimDAC_4LSB = getRegister(info4->m_regAddress).getSubRegister(info4).getValue();
    unsigned trimDAC_1MSB = getRegister(info1->m_regAddress).getSubRegister(info1).getValue();

    if(trimDAC_4LSB > 15 )
      logger->error(" Sub register value for 4LSB channel {} for chip[ID {}] is larger than 15 with value {}",
                    channel, getABCchipID(), trimDAC_4LSB);

    if(trimDAC_1MSB > 1 )
      logger->error(" Sub register value for 1MSB channel {} for chip[ID {}] is larger than 1 with value {}",
                    channel, getABCchipID(), trimDAC_1MSB);

    return ( (trimDAC_1MSB<<4) | trimDAC_4LSB);
}

uint32_t AbcCfg::getRegisterValue(ABCStarRegister addr) const {
    return getRegister(addr).getValue();
}

void AbcCfg::setRegisterValue(ABCStarRegister addr, uint32_t val) {
    getRegister(addr).setValue(val);
}
