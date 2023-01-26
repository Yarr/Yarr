#ifndef __ROGUE_COM_H__
#define __ROGUE_COM_H__

#include <cstdint>
#include <mutex>
#include <memory>
#include <rogue/Logging.h>
#include <rogue/protocols/udp/Core.h>
#include <rogue/protocols/udp/Client.h>
#include <rogue/protocols/rssi/Client.h>
#include <rogue/protocols/rssi/Transport.h>
#include <rogue/protocols/rssi/Application.h>
#include <rogue/protocols/packetizer/CoreV2.h>
#include <rogue/protocols/packetizer/Transport.h>
#include <rogue/protocols/packetizer/Application.h>
#include <rogue/protocols/srp/SrpV3.h>
#include <rogue/interfaces/memory/Master.h>
#include <rogue/interfaces/memory/Constants.h>
#include <rogue/interfaces/stream/Frame.h>
#include <rogue/interfaces/stream/FrameIterator.h>
#include <rogue/interfaces/stream/Buffer.h>
#include <rogue/hardware/axi/AxiStreamDma.h>
#include <rogue/interfaces/stream/FrameLock.h>
#include <iomanip>
#include <queue>
#include <unistd.h>
#include <fstream>
class RogueCom;
class RogueSender:  public rogue::interfaces::stream::Master {
 public:
 RogueSender(uint32_t port, uint32_t type) : 
  tx_bytes(0),tx_max_pkt(0),tx_pkts(0),tx_min_pkt(0xffffffff), _type(type),
  _port(port) {}
  void send(uint8_t *data, uint32_t size);
  uint32_t tx_bytes;
  uint32_t tx_max_pkt;
  uint32_t tx_pkts;
  uint32_t tx_min_pkt;
 private:
  uint32_t _port;
  uint32_t _type;
};

class RogueReceiver :  public rogue::interfaces::stream::Slave {
 public:
 RogueReceiver(uint32_t port, uint32_t type, std::shared_ptr<RogueCom> com) :
  rx_bytes(0),rx_max_pkt(0),
    rx_pkts(0),rx_min_pkt(0xffffffff), _type(type),
    _port(port),  _com(com) {}  
  void acceptFrame ( std::shared_ptr<rogue::interfaces::stream::Frame> frame) override;
  void resetCnt(){ rx_bytes=0; rx_pkts=0; rx_max_pkt=0; rx_min_pkt=0xffffffff; }
  uint32_t rx_bytes;
  uint32_t rx_max_pkt;
  uint32_t rx_pkts;
  uint32_t rx_min_pkt;
 private:
  uint32_t _port;
  uint32_t _type;
  std::shared_ptr<RogueCom> _com;
};
std::pair<uint32_t, uint32_t> decodeRegRead(uint32_t higher, uint32_t lower);

class RogueCom  {
 public:
 RogueCom() : _txchannel(0), _rxchannel(0), m_doDataTransmissionCheck(true) , m_manualDlyCtrl(0) , m_tuningDly(false) {txfifo_cnt[0]=txfifo_cnt[1]=txfifo_cnt[2]=txfifo_cnt[3]=0; }
  static std::shared_ptr<RogueCom> getInstance() {
    if(!instance) {
      instance=std::make_shared<RogueCom>();
    }
    return instance;
  }
  void setTxChannel(uint32_t txchannel){_txchannel=txchannel;}
  void setRxChannel(uint32_t rxchannel){_rxchannel=rxchannel;}
  void setTxRxChannel(uint32_t txchannel, uint32_t rxchannel){_txchannel=txchannel;_rxchannel=rxchannel;}
  void setType(uint32_t inType){_type=inType;}
  void connect(const std::string &conn);
  void configPLL(const std::string PLLfile, bool forceConfig=true);
  virtual uint32_t getCurSize() ;
  virtual bool isEmpty();
  virtual uint32_t read32();
  virtual uint32_t readBlock32(uint32_t *buf, uint32_t length);
  virtual void write32(uint32_t);
  virtual void releaseFifo();	
  void enableLane(uint32_t mask);
  void enableDebugStream(bool enable);
  void reset() {
	  if(_type==0){
    writeRegister(rxPhyMon[0]+0x808,1);
    writeRegister(rxPhyMon[1]+0x808,1);
    writeRegister(rxPhyMon[2]+0x808,1);
    writeRegister(rxPhyMon[3]+0x808,1);
	  }
	  else if(_type==1){
		  for(int i=0;i<4;i++){
		  writeRegister(ctrl[i]+0xFFC,1); 
		  writeRegister(ctrl[i]+0xFFC,0); 
		  }
	  }
	  else{ std::cout<<" type="<<_type<<" not support"<<std::endl;}
  }

  void enableTrig() {
	  if(_type==0 or _type==1 ){
     writeRegister(trigEmu+0x0,1);
	  }
	  else{ std::cout<<" type="<<_type<<" not support"<<std::endl;}
  }

  bool trigBusy() {
    uint32_t temp;
	  if(_type==0 or _type==1 ){
     readRegister(trigEmu+0x14,temp);
    return ((temp&0x1)==0x1);
	  }
	  else{ std::cout<<" type="<<_type<<" not support"<<std::endl; return false;}

  }

  uint32_t trigBackPressure() {
    uint32_t temp;
    readRegister(trigEmu+0x10,temp);
	std::cout<<"Firmware Trigger Back Pressure Count: "<<temp<<std::endl;

    return temp;
  }

  uint32_t readTLUtrigword() {
    uint32_t temp;
     readRegister(trigTLU+0x4,temp);
    return temp;

  }
  uint32_t readNTC(int port) {
    uint32_t temp;
     readRegister(NTCReg+ port*4,temp);
	 std::cout<<"NTC port "<<port<<" = "<<temp<<std::endl;
    return temp;

  }
  void setTrigEmu(uint32_t *trigWords,uint32_t length,uint32_t freq,uint32_t iter) {
    // fill LUT

  
    for(unsigned i=0;i<length;i++) {
      uint32_t temp=trigWords[length-1-i];
      if(bram[i] != temp) {
	writeRegister(trigLUT+i*4,temp);
      }
      bram[i]=temp;
    }
    if(length!=trigLength) writeRegister(trigEmu+0x8,length-1); // max LUT address
    if(iter!=trigIter) writeRegister(trigEmu+0xC,iter-1); // # iterations
    if(freq!=trigFreq) writeRegister(trigEmu+0x4,160000000/freq); // timer /160MHz    
    trigLength=length;
    trigIter=iter;
    trigFreq=freq;
  }
  void printStats() {
	  for(int i=0;i<4;i++){
			  std::cout<<"===========================Check Port "<<i<<"=============================="<<std::endl;
    printf("RX Stats\n");
    printf("Bytes Received:        %d\n",dataStream[i]->rx_bytes);
    printf("Pkts Received:         %d\n",dataStream[i]->rx_pkts);
    printf("Max Pkt Size(bytes):   %d\n",dataStream[i]->rx_max_pkt);
    printf("Min Pkt Size(bytes):   %d\n",dataStream[i]->rx_min_pkt);
    printf("TX Stats\n");
    printf("Bytes Sent:            %d\n",configStream[i]->tx_bytes);
    printf("Pkts Sent:             %d\n",configStream[i]->tx_pkts);
    printf("Max Pkt Size(bytes):   %d\n",configStream[i]->tx_max_pkt);
    printf("Min Pkt Size(bytes):   %d\n",configStream[i]->tx_min_pkt);

	  }
    
  }
  void selectRate(uint32_t speed) {

	  m_speedrate=speed;
	  if(_type==1){

		  for(int i=0;i<4;i++){
			  uint32_t temp;

			  readRegister(ctrl[i]+0x80c,temp);
			  std::cout<<"selectRate0="<<std::hex<<temp<<std::dec<<std::endl;

			  temp=(temp&0xfffffc00)+(temp&0xff)+((speed&0x3)<<8);
			  std::cout<<"selectRate1="<<std::hex<<temp<<std::dec<<std::endl;

			  writeRegister(ctrl[i]+0x80c,temp);
		  }
	  }else{
		  std::cout<<" type="<<_type<<" not support"<<std::endl;
	  }
  }


  //For Zijun's old FW
  //void delayData(uint32_t indelay, uint32_t channel, uint32_t port) {
  //    if(channel<0 || channel>3 || port<0 || port>3){std::cout<<"only support channel/port 0-3"<<std::endl;return;}
  //    uint32_t add=0;
  //    if(channel==0)add=ctrl[port]+0x820; 
  //    if(channel==1)add=ctrl[port]+0x824; 
  //    if(channel==2)add=ctrl[port]+0x828; 
  //    if(channel==3)add=ctrl[port]+0x82a; 
  //    uint32_t temp;
  //   readRegister(add,temp);
  //   writeRegister(add,indelay);
  //    std::cout<<"delay port="<<port<<" channel="<<channel<<" add="<<add<<" old value="<<temp<<" value="<<indelay<<std::endl;
  //}

  //update for Larry's new FW
  void readDelayData(uint32_t port, uint32_t channel) {
	  if(channel<0 || channel>3 || port<0 || port>3){std::cout<<"only support channel/port 0-3"<<std::endl;return;}
	  //enable UserDly Cfg at first
	  uint32_t add=-1;
	  uint32_t temp_EnUsrDlyCfg=-1;
	  uint32_t temp_UsrRxDlyTap=-1;
	  uint32_t temp_RxDlyTap=-1;
	  //uint32_t temp_RxDlyEdge0=-1;
	  //uint32_t temp_RxDlyEdge1=-1;
	  //uint32_t temp_RxDlyHalf =-1;
	  uint32_t temp_GearBoxBitSlipCnt=-1;

	  //En Usr Dly
	  add=ctrl[port]+0x814;
	  readRegister(add,temp_EnUsrDlyCfg);
	  //Usr Rx Dly Tap
	  add=ctrl[port]+0x82c-0x4*channel; 
	  readRegister(add,temp_UsrRxDlyTap);
	  // Rx Dly Tap
	  add=ctrl[port]+0x42c-0x4*channel; 
	  readRegister(add,temp_RxDlyTap);
	  //// Rx Dly Edge0
	  //add=ctrl[port]+0x43c-0x4*channel; 
	  //readRegister(add,temp_RxDlyEdge0);
	  //// Rx Dly Edge1
	  //add=ctrl[port]+0x44c-0x4*channel; 
	  //readRegister(add,temp_RxDlyEdge1);
	  //// Rx Dly Half
	  //add=ctrl[port]+0x45c-0x4*channel; 
	  //readRegister(add,temp_RxDlyHalf);
	  //read GearBoxBitSlipCnt
  	  add=ctrl[port]+0x04c-0x4*channel;
	  readRegister(add,temp_GearBoxBitSlipCnt);

	  std::cout<<"Port "<<port<<" channel="<<channel<<" EnUsrDlyCfg="<<temp_EnUsrDlyCfg
	  <<"; Usr Dly Tap="<<temp_UsrRxDlyTap
	  <<"; Dly Tap="<<temp_RxDlyTap
	  //<<"; Dly Edge0="<<temp_RxDlyEdge0
	  //<<"; Dly Edge1="<<temp_RxDlyEdge1
	  //<<"; Dly Half="<<temp_RxDlyHalf
	  <<"; GearBoxBitSlipCnt value="<<temp_GearBoxBitSlipCnt<<std::endl;
}

  void delayData(uint32_t port, uint32_t channel, uint32_t indelay) {
	  if(channel<0 || channel>3 || port<0 || port>3){std::cout<<"only support channel/port 0-3"<<std::endl;return;}
	  //enable UserDly Cfg at first
	  uint32_t add=ctrl[port]+0x814;
	  uint32_t temp=-1;
	  //readRegister(add,temp);
	  //std::cout<<"Port "<<port<<" EnUsrDlyCfg="<<temp<<std::endl;
	  temp=1;
	  writeRegister(add,temp);
	  //temp=-1;
	  //readRegister(add,temp);
	  //std::cout<<"Port "<<port<<" EnUsrDlyCfg="<<temp<<std::endl;



	  //write the Dly Cfg value
	  add=ctrl[port]+0x82c-0x4*channel; 
	  indelay=indelay&0x1ff;

	  temp=-1;
	  readRegister(add,temp);
	  writeRegister(add,indelay);
	  std::cout<<"delay port="<<port<<" channel="<<channel<<" add="<<add<<" old Usr Rx Delay Tap value="<<temp<<" Usr Rx Delay Tap value="<<indelay<<std::endl;

	  //read the Dly Cfg value
	  add=ctrl[port]+0x42c-0x4*channel; 
	  temp=-1;
	  readRegister(add,temp);
	  std::cout<<"delay port="<<port<<" channel="<<channel<<" add="<<add<<"  Rx Delay Tap value="<<temp<<std::endl;

  }

//obsolete
//  void autoDelayData(uint32_t dlyCfg, uint32_t dlyCfgDir) {
//	//  dlyCfgDir=0;//dlyCfgDir is abandoned
//	  
//	  for(int port=0;port<4;port++){
//		  uint32_t add=ctrl[port]+0x818;
//		  uint32_t temp=-1;
//		  //readRegister(add,temp);
//		  //std::cout<<std::dec<<"Port "<<port<<" old Auto Dly Cfg="<<(temp&0x3f)<<" Auto Dly Direction="<<((temp>>16)&0x1)<<std::endl;
//		  //std::cout<<std::dec<<"Port write"<<port<<" new Auto Dly Cfg="<<dlyCfg<<" Auto Dly Direction="<<dlyCfgDir<<std::endl;
//		  //write the Dly Cfg value
//		  temp=(dlyCfg&0x3f)+ ((dlyCfgDir&0x1)<<16);
//		  //std::cout<<std::dec<<"Port write"<<port<<" Auto Dly Cfg="<<(temp&0x3f)<<" Auto Dly Direction="<<((temp>>16)&0x1)<<std::endl;
//		  writeRegister(add,temp);
//		  ////read back
//		  //temp=-1;
//		  //readRegister(add,temp);
//		  //std::cout<<std::dec<<"Port read"<<port<<" Auto Dly Cfg="<<(temp&0x3f)<<" Auto Dly Direction="<<((temp>>16)&0x1)<<std::endl;
//	  }
//
//
//  }

  void setAutoLockingCntCfg(uint32_t inCfg) {
	  for(int port=0;port<4;port++){
		  uint32_t add=ctrl[port]+0x818;
		  uint32_t temp=inCfg;
		  if(temp>16777215){ //2^24 maximum
			  std::cout<<"Maximum is 2^24! reduce from 0x"<<std::hex<<temp<<" to 0xffffff; Enter to continue"<<std::dec<<std::endl;
			  std::cin.ignore();
			  temp=16777215;
		  }
		  writeRegister(add,temp);
	  }
  }



  void setEyeScan(uint32_t eyeScanCfg) {
	  //some time the auto mode failed to find the right edge of BER curve during eye scan. So, change the eyeScan cfg value could help

	  if(_type==1){
		  uint32_t temp=eyeScanCfg&0xff;
		  writeRegister(ctrl[0]+0x830,temp);
		  writeRegister(ctrl[1]+0x830,temp);
		  writeRegister(ctrl[2]+0x830,temp);
		  writeRegister(ctrl[3]+0x830,temp);
		  writeRegister(ctrl[0]+0x834,temp);
		  writeRegister(ctrl[1]+0x834,temp);
		  writeRegister(ctrl[2]+0x834,temp);
		  writeRegister(ctrl[3]+0x834,temp);
		  writeRegister(ctrl[0]+0x838,temp);
		  writeRegister(ctrl[1]+0x838,temp);
		  writeRegister(ctrl[2]+0x838,temp);
		  writeRegister(ctrl[3]+0x838,temp);
		  writeRegister(ctrl[0]+0x83c,temp);
		  writeRegister(ctrl[1]+0x83c,temp);
		  writeRegister(ctrl[2]+0x83c,temp);
		  writeRegister(ctrl[3]+0x83c,temp);
	  }
	  else{ std::cout<<" type="<<_type<<" not support delay or invert CMD"<<std::endl;}
  }

  void inv_delay_CMD(uint32_t invdelay) {
	  //invdelay=00: no inv no delay
	  //invdelay=01: inv, no delay
	  //invdelay=10: no inv, delay
	  //invdelay=11: inv, delay

	  //std::cout<<"invdelay="<<invdelay<<std::endl;std::cin.ignore();
	  if(_type==1){
		  uint32_t temp=invdelay&0x3;
		  writeRegister(ctrl[0]+0x808,temp);
		  writeRegister(ctrl[1]+0x808,temp);
		  writeRegister(ctrl[2]+0x808,temp);
		  writeRegister(ctrl[3]+0x808,temp);
	  }
	  else{ std::cout<<" type="<<_type<<" not support delay or invert CMD"<<std::endl;}
  }

  void doDataTransmissionCheck(bool in_doDataTransmissionCheck){ m_doDataTransmissionCheck=in_doDataTransmissionCheck; }
  void enManualDlyCtrl(int in_manualDlyCtrl){
	  //std::cout<<"enManualDlyCtrl: in_manualDlyCtrl="<<in_manualDlyCtrl<<std::endl;
	  m_manualDlyCtrl=in_manualDlyCtrl;
	  setManualDlyCtrl();
  }
  void enTuningDly(bool in_tuningDly){ m_tuningDly=in_tuningDly;}
  void setManualDlyCtrl(){
	  if(m_manualDlyCtrl==1){
		  std::cout<<"GlobalMonitor in setManualDlyCtrl"<<std::endl;
		  GlobalMonitor(1, -1);
		  ////mDP 0
		  //delayData(0, 0, 185);
		  //delayData(0, 1, 195);
		  //delayData(0, 2, 200);
		  //delayData(0, 3, 165);
		  ////mDP 1
		  //delayData(1, 0, 25);
		  //delayData(1, 1, 220);
		  //delayData(1, 2, 215);
		  //delayData(1, 3, 30);
		  ////mDP 2
		  //delayData(2, 0,  10);
		  //delayData(2, 1,  20);
		  //delayData(2, 2,  20);
		  //delayData(2, 3, 180);
		  ////mDP 3
		  //delayData(3, 0,490);
		  //delayData(3, 1,500);
		  //delayData(3, 2,495);
		  //delayData(3, 3,155);
		  //sleep(1);
		  //std::cout<<"sleep 1 sec ..."<<std::endl;
		  //writeRegister(ctrl[0]+0xFFC,1); 
		  //writeRegister(ctrl[0]+0xFFC,0); 
		  //writeRegister(ctrl[1]+0xFFC,1); 
		  //writeRegister(ctrl[1]+0xFFC,0); 
		  //writeRegister(ctrl[2]+0xFFC,1); 
		  //writeRegister(ctrl[2]+0xFFC,0); 
		  //writeRegister(ctrl[3]+0xFFC,1); 
		  //writeRegister(ctrl[3]+0xFFC,0); 
		  ////m_manualDlyCtrl=0;

	  }else if(m_manualDlyCtrl==0) {

		  for(int port=0;port<4;port++){
			  uint32_t add=ctrl[port]+0x814;
			  //std::cout<<"Disable Port "<<port<<" Manual DlyCfg"<<std::endl;
			  writeRegister(add, 0);
		  }

	  }
  }

  void GlobalMonitor(bool reset=false, int looptime=1) { 

	  if(_type==0){ 

		  for(int i=0;i<4;i++){
			  //reset Cnt
			  if(reset){
				  writeRegister(rxPhyMon[i]+0xFFC,1); 
				  writeRegister(rxPhyMon[i]+0xFFC,0); 
			  }
		  }

		  uint32_t totalChBond=0;

		  for(int iloop=0;iloop<looptime;iloop++){

			  for(int i=0;i<4;i++){
				  std::cout<<"===========================Check Port "<<i<<"=============================="<<std::endl;
				  ////reset Cnt

				  uint32_t temp;
				  //linkUpCnt for 4 chs
				  temp=0;
				  readRegister(rxPhyMon[i]+0x008,temp); 
				  std::cout<<"LinkUpCnt for ch0: "<<std::hex<<temp<<std::dec<<std::endl;
				  temp=0;
				  readRegister(rxPhyMon[i]+0x00c,temp); 
				  std::cout<<"LinkUpCnt for ch1: "<<std::hex<<temp<<std::dec<<std::endl;
				  temp=0;
				  readRegister(rxPhyMon[i]+0x010,temp); 
				  std::cout<<"LinkUpCnt for ch2: "<<std::hex<<temp<<std::dec<<std::endl;
				  temp=0;
				  readRegister(rxPhyMon[i]+0x014,temp); 
				  std::cout<<"LinkUpCnt for ch3: "<<std::hex<<temp<<std::dec<<std::endl;

				  //ChBondCnt
				  temp=0;
				  readRegister(rxPhyMon[i]+0x018,temp); 
				  std::cout<<"ChBondCnt: "<<std::hex<<temp<<std::dec<<std::endl;
				  totalChBond+=temp;

				  ////ConfigDropCnt
				  //temp=0;
				  //readRegister(rxPhyMon[i]+0x014,temp); 
				  //std::cout<<"ConfigDropCnt: "<<std::hex<<temp<<std::dec<<std::endl;

				  //DataDropCnt
				  temp=0;
				  readRegister(rxPhyMon[i]+0x000,temp); 
				  std::cout<<"DataDropCnt: "<<std::hex<<temp<<std::dec<<std::endl;

				  //ChBond&LinkUp
				  temp=0;
				  readRegister(rxPhyMon[i]+0x400,temp); 
				  std::cout<<"ChBond&LinkUp: "<<std::hex<<(temp>>2)<<std::dec<<std::endl;


			  }

		  }


		  if(totalChBond>0 && m_doDataTransmissionCheck){
			  std::cout<<"Link Unstable! Enter to continue";std::cin.ignore();
		  }
	  }else if(_type==1){ 

		  for(int i=0;i<4;i++){
			  //reset Cnt
			  if(reset){
				  writeRegister(ctrl[i]+0xFFC,1); 
				  writeRegister(ctrl[i]+0xFFC,0); 
			  }
		  }

		  uint32_t totalChBond=0;
		  uint32_t linkUp=0;
		  uint32_t totalBadAuroraHead=0;



		  //looptime<0 means endless loop for debug purpose
		  for(int iloop=0;iloop<looptime || looptime<0;iloop++){

			  for(int i=0;i<4;i++){
				  linkUp=0;
				  std::cout<<"===========================Check Port "<<i<<"=============================="<<std::endl;
				  ////reset Cnt

				  uint32_t temp;
				  //linkUpCnt for 4 chs
				  temp=0;
				  readRegister(ctrl[i]+0x000,temp); 
				  std::cout<<"LinkUpCnt for ch0: "<<std::hex<<temp<<std::dec<<std::endl;
				  temp=0;
				  readRegister(ctrl[i]+0x004,temp); 
				  std::cout<<"LinkUpCnt for ch1: "<<std::hex<<temp<<std::dec<<std::endl;
				  temp=0;
				  readRegister(ctrl[i]+0x008,temp); 
				  std::cout<<"LinkUpCnt for ch2: "<<std::hex<<temp<<std::dec<<std::endl;
				  temp=0;
				  readRegister(ctrl[i]+0x00c,temp); 
				  std::cout<<"LinkUpCnt for ch3: "<<std::hex<<temp<<std::dec<<std::endl;

				  //ChBondCnt
				  temp=0;
				  readRegister(ctrl[i]+0x010,temp); 
				  std::cout<<"ChBondCnt: "<<std::hex<<temp<<std::dec<<std::endl;
				  totalChBond+=temp;

				  //ConfigDropCnt
				  temp=0;
				  readRegister(ctrl[i]+0x014,temp); 
				  std::cout<<"ConfigDropCnt: "<<std::hex<<temp<<std::dec<<std::endl;

				  //DataDropCnt
				  temp=0;
				  readRegister(ctrl[i]+0x018,temp); 
				  std::cout<<"DataDropCnt: "<<std::hex<<temp<<std::dec<<std::endl;

				  //ChBond&LinkUp
				  temp=0;
				  readRegister(ctrl[i]+0x400,temp); 
				  uint32_t linkUpStat0=temp&0x1;
				  uint32_t linkUpStat1=(temp&0x2)>>1;
				  uint32_t linkUpStat2=(temp&0x4)>>2;
				  uint32_t linkUpStat3=(temp&0x8)>>3;
				  uint32_t ChBondStat=temp>>4;
				  std::cout<<"ChBondStat: "<<std::hex<<ChBondStat<<std::dec<<std::endl;
				  std::cout<<"LinkUp[3 to 0]: "<<linkUpStat3<<", "<<linkUpStat2<<", "<<linkUpStat1<<", "<<linkUpStat0<<", "<<std::endl;

				  if(temp==0x1f) linkUp=1;


		            //SingleHdrDetCnt
				  temp=0;
				  readRegister(ctrl[i]+0x020,temp); 
				  std::cout<<"SingleHdrDetCnt: "<<std::hex<<temp<<std::dec<<std::endl;

		            //DoubleHdrDetCnt
				  temp=0;
				  readRegister(ctrl[i]+0x024,temp); 
				  std::cout<<"DoubleHdrDetCnt: "<<std::hex<<temp<<std::dec<<std::endl;

		            //SingleHitDetCnt
				  temp=0;
				  readRegister(ctrl[i]+0x028,temp); 
				  std::cout<<"SingleHitDetCnt: "<<std::hex<<temp<<std::dec<<std::endl;

		            //DoubleHitDetCnt
				  temp=0;
				  readRegister(ctrl[i]+0x02C,temp); 
				  std::cout<<"DoubleHitDetCnt: "<<std::hex<<temp<<std::dec<<std::endl;


				  //reverse the add order to be consistent with the linkUpCnt address
		            //AuroraHdrErrDet
				  temp=0;
				  readRegister(ctrl[i]+0x03c,temp); 
				  std::cout<<"Ch0 AuroraHdrErrDet: "<<std::hex<<temp<<std::dec<<std::endl;
				  if(linkUp>0)totalBadAuroraHead+=temp;

		            //AuroraHdrErrDet
				  temp=0;
				  readRegister(ctrl[i]+0x038,temp); 
				  std::cout<<"Ch1 AuroraHdrErrDet: "<<std::hex<<temp<<std::dec<<std::endl;
				  if(linkUp>0)totalBadAuroraHead+=temp;

		            //AuroraHdrErrDet
				  temp=0;
				  readRegister(ctrl[i]+0x034,temp); 
				  std::cout<<"Ch2 AuroraHdrErrDet: "<<std::hex<<temp<<std::dec<<std::endl;
				  if(linkUp>0)totalBadAuroraHead+=temp;

		            //AuroraHdrErrDet
				  temp=0;
				  readRegister(ctrl[i]+0x030,temp); 
				  std::cout<<"Ch3 AuroraHdrErrDet: "<<std::hex<<temp<<std::dec<<std::endl;
				  if(linkUp>0)totalBadAuroraHead+=temp;

		            //WrdSentCnt
				  temp=0;
				  readRegister(ctrl[i]+0x01C,temp); 
				  std::cout<<"WrdSentCnt: "<<std::hex<<temp<<std::dec<<std::endl;

		          //  //TotalHdrDetCnt
				  //temp=0;
				  //readRegister(ctrl[i]+0x02C,temp); 
				  //std::cout<<"TotalHdrDetCnt: "<<std::hex<<temp<<std::dec<<std::endl;

		          //  //TotalHitDetCnt
				  //temp=0;
				  //readRegister(ctrl[i]+0x02C,temp); 
				  //std::cout<<"TotalHitDetCnt: "<<std::hex<<temp<<std::dec<<std::endl;

			      readDelayData(i,0);//ch0
			      readDelayData(i,1);//ch1
			      readDelayData(i,2);//ch2
			      readDelayData(i,3);//ch3
			  }

				//looptime<0 means endless loop for debug purpose
			  if(looptime<0 && m_manualDlyCtrl){
			      std::cout<<"change port/channel delay"<<std::endl;
			      std::cout<<"If want to stop manual setting, input port number >10"<<std::endl;
			      std::cout<<"If want to check link status without any manual setting, input port number >3"<<std::endl;
			      std::cout<<"input port number:"<<std::endl;
			      uint32_t port;std::cin>>port;
			      if(port>10){
					  std::cin.ignore();
					  //reset ctrl cnt 
					  writeRegister(ctrl[port]+0xFFC,1); 
					  writeRegister(ctrl[port]+0xFFC,0); 
					  break;
				  }
			      if(port>3)continue;
			      std::cout<<"input channel number:"<<std::endl;
			      uint32_t channel;std::cin>>channel;
			      std::cout<<"input delay value:"<<std::endl;
			      uint32_t indelay;std::cin>>indelay;
			      delayData(port,channel,indelay);
			      //sleep(1);
			      //std::cout<<"sleep 1 sec ..."<<std::endl;
				  //reset ctrl cnt 
			      writeRegister(ctrl[port]+0xFFC,1); 
			      writeRegister(ctrl[port]+0xFFC,0); 
			  }


			  if(m_speedrate==0) sleep(1);
			  if(m_speedrate==1) sleep(2);
			  if(m_speedrate==2) sleep(4);
			  if(m_speedrate==3) sleep(8);
			  std::cout<<"sleep 1 sec ..."<<std::endl;

		  }


		  if( (totalChBond>0 || totalBadAuroraHead>0) && m_doDataTransmissionCheck){
			  std::cout<<"totalChBond="<<totalChBond<<"; totalBadAuroraHead="<<totalBadAuroraHead<<std::endl;
			  std::cout<<"Link Unstable! Enter to continue";std::cin.ignore();
		  }
	  }
	  else{ std::cout<<" type="<<_type<<" not support GlobalMonitor"<<std::endl;}
  }




  void resetFirmware() {
	  if(_type==0){
		  std::cout<<"type 0 device don't support resetFirmware()"<<std::endl;
	  }else if(_type==1){
		  
		  uint32_t temp;
		  for(int i=0;i<4;i++){
			  readRegister(ctrl[i]+0xff4,temp);
			  temp=(temp&0xfffffffd)+(1<<1);
			  writeRegister(ctrl[i]+0xff4,temp);
		  }
		  sleep(1);
		  for(int i=0;i<4;i++){
			  readRegister(ctrl[i]+0xff4,temp);
			  temp=(temp&0xfffffffd)+(0<<1);
			  writeRegister(ctrl[i]+0xff4,temp);
		  }
		  std::cout<<"Enter to continue"<<std::endl;
		  std::cin.ignore();

	  }
}


  void setBatchSize(uint32_t sz) {
	  if(_type==0){
		uint32_t temp=sz&0xffff;
		writeRegister(sysReg+0x810,temp);
	  }else if(_type==1){

		  for(int i=0;i<4;i++){
	  uint32_t temp;
	  readRegister(ctrl[i]+0xff0,temp);
	  temp=(temp&0xffff0000)+(sz&0xffff);
	  writeRegister(ctrl[i]+0xff0,temp);
		  }

	  }
}
  void setBatchTimer(uint32_t t) {
	  if(_type==0){
		uint32_t temp=t&0xffff;
		writeRegister(sysReg+0x80C,temp);
	  }else if(_type==1){
		  for(int i=0;i<4;i++){
		uint32_t temp;
		readRegister(ctrl[i]+0xff0,temp);
		temp=(temp&0xffff)+((t&0xffff)<<16);
		writeRegister(ctrl[i]+0xff0,temp);
		  }
	  }
  }
void queue_data(uint32_t *data,uint32_t nwords, uint32_t queuenumber) {
	for(unsigned i=0;i<nwords;i++) 
	{
		rxfifo[queuenumber].push(data[i]);
		//std::cout<<"queue_data: "<<std::hex<<data[i]<<" queuenumber="<<queuenumber<<std::dec<<std::endl;
	}
}
void queue_data(uint32_t data,unsigned queuenumber) {
		rxfifo[queuenumber].push(data);
		//std::cout<<"queue_data: "<<std::hex<<data<<std::dec<<std::endl;
}
//  void queue_debug(uint32_t *data,uint32_t nwords, uint32_t queuenumber) {
//	  std::cout<<"queue_debug queuenumber="<<queuenumber<<std::endl;
////	std::cout<<"queue_debug nwords="<<nwords<<"           ";
//    for(unsigned i=0;i<nwords;i++) 
//	{
//      //rxdebugfifo[_rxchannel].push(data[i]);
//      //rxfifo[_rxchannel].push(data[i]);
//      rxdebugfifo[queuenumber].push(data[i]);
//      rxfifo[queuenumber].push(data[i]);
//	  //std::cout<<"queue_debug: "<<std::hex<<data[i]<<std::dec<<std::endl;
//
//
//	}
//
//	////decode
//	//if(nwords==2){ decodeRegRead(data[0], data[1]); }
//	//else if(nwords==4){ decodeRegRead(data[0], data[1]); decodeRegRead(data[2], data[3]); }
//	//else{
//	//	std::cout<<"Warning! words number is strange:"<<nwords<<std::endl;
//	//}
//
//  }
  uint32_t tx_size() {
    uint32_t result= txfifo_cnt[_txchannel];
    return result;
  }
  void setFirmwareTrigger(bool enable) {
    firmwareTrigger=enable;
  }
  void setForceRelaseTxfifo(bool enable=true) {
    forceRelaseTxfifo=enable;
  }
  bool getFirmwareTrigger() const {return firmwareTrigger;}

  void flushBuffer();
 protected:
 private:

 uint32_t _txchannel;//0-3; tx is used for writing data to txfifo
 uint32_t _rxchannel;//0-3; rx is used for reading data to rxfifo 

#define  N_TXFIFO 4096
  uint32_t m_counter;
  uint32_t rxPhyMon[4];
  uint32_t ctrl[4];
  uint32_t trigLUT;
  uint32_t trigTLU;
  uint32_t trigEmu;
  uint32_t sysReg;
  uint32_t pllReg;
  uint32_t NTCReg;
  uint32_t txfifo_0[N_TXFIFO];
  uint32_t txfifo_1[N_TXFIFO];
  uint32_t txfifo_2[N_TXFIFO];
  uint32_t txfifo_3[N_TXFIFO];
  uint32_t txfifo_cnt[4];
  bool m_doDataTransmissionCheck;
  bool forceRelaseTxfifo;
  int m_manualDlyCtrl;//>0: manual mode; =0: auto mode; <0: ignore
  bool m_tuningDly;
  uint32_t m_speedrate;//0 for 1.28Gbps; 1,2,3 for 640, 320, 160 Mbps
  std::queue<uint32_t> rxfifo[8];// 0-3 for debug; 4-7 for data
  rogue::interfaces::memory::MasterPtr mast;
  std::shared_ptr<RogueSender> configStream[4];
  std::shared_ptr<RogueReceiver> debugStream[4];
  std::shared_ptr<RogueReceiver> dataStream[4];
  rogue::hardware::axi::AxiStreamDmaPtr axisrp;
  rogue::hardware::axi::AxiStreamDmaPtr axicfg[4];
  rogue::hardware::axi::AxiStreamDmaPtr axidata[4];
  rogue::protocols::srp::SrpV3Ptr srp;
  rogue::protocols::srp::SrpV3Ptr srpSrp;
  rogue::protocols::udp::ClientPtr udp;
  rogue::protocols::udp::ClientPtr udpSrp;
  rogue::protocols::rssi::ClientPtr rssi;
  rogue::protocols::rssi::ClientPtr rssiSrp;
  rogue::protocols::packetizer::CoreV2Ptr pack;
  rogue::protocols::packetizer::CoreV2Ptr packSrp;
  uint32_t trigLength;
  uint32_t trigFreq;
  uint32_t trigIter;
  uint32_t bram[1024];
  bool firmwareTrigger;
  static std::shared_ptr<RogueCom> instance;
  void writeRegister(uint32_t addr,const uint32_t &val) {
    uint32_t temp=val;
    mast->reqTransaction(addr,4,&temp,rogue::interfaces::memory::Write);
    mast->waitTransaction(0);
  }
  void readRegister(uint32_t addr,uint32_t &val) {
    uint32_t temp;
    mast->reqTransaction(addr,4,&temp,rogue::interfaces::memory::Read);
    mast->waitTransaction(0);
    val=temp;    
  }


  uint32_t _type=0;//type 1 for FMC; type 0 for old FEB
};

#endif
