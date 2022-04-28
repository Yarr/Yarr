#include <iostream>

#include <RogueCom.h>
#include <memory>
#include <unistd.h>
/*void RogueSender::send(uint8_t *data,uint32_t size) {
  rogue::interfaces::stream::FramePtr frame;
  rogue::interfaces::stream::FrameIterator it;
  
  // Request frame
  //frame = reqFrame(size+100,true);
  frame = reqFrame(100,true);
  
  // Get data write iterator
  it = frame->beginWrite();
  
  std::copy(data,data+size,it);
  
  // Set new frame size
  frame->setPayload(size);
  tx_bytes+=size;
  tx_pkts++;
  if(size<tx_min_pkt) tx_min_pkt=size;
  if(size>tx_max_pkt) tx_max_pkt=size;  
  //Send frame
  sendFrame(frame);
}*/

void RogueSender::send(uint8_t *data,uint32_t size) {
  rogue::interfaces::stream::FramePtr frame;
  rogue::interfaces::stream::FrameIterator it;
  
  // Request frame
  //frame = reqFrame(size+100,true);
  frame = reqFrame(size+100,true);
  
  // Get data write iterator
  it = frame->beginWrite();
  
  std::copy(data,data+size,it);
  
  // Set new frame size
  frame->setPayload(size);
  tx_bytes+=size;
  tx_pkts++;
  if(size<tx_min_pkt) tx_min_pkt=size;
  if(size>tx_max_pkt) tx_max_pkt=size;  
  //Send frame
  sendFrame(frame);
}


void RogueReceiver::acceptFrame ( std::shared_ptr<rogue::interfaces::stream::Frame> frame) {
	uint32_t nbytes = frame->getPayload();
	// Iterators to start and end of frame
        rogue::interfaces::stream::FrameIterator iter;
	std::shared_ptr<rogue::interfaces::stream::FrameLock> lock = frame->lock();
	iter = frame->beginRead();
	auto  end = frame->endRead();

	//Iterate through contigous buffers
	while ( iter != end ) {

		//  Get contigous size; size in byte
		auto size = iter.remBuffer ();

		// Get the data pointer from current position
		auto *src = iter.ptr ();
		uint8_t *data=(uint8_t*)src;

		unsigned queuenumber=0;
		if(_type==0){
			queuenumber=_port-1;
		}else if(_type==1){
			queuenumber=_port-0;
		}

		//dataPort to data
		//if((nbytes%8)==0) {
		if((size%8)==0) {//64 bits each Aurora Frame

			uint32_t *p=(uint32_t*)data;
			rx_bytes+=size;
			rx_pkts++;
			if(size<rx_min_pkt) rx_min_pkt=size;
			if(size>rx_max_pkt) rx_max_pkt=size;      
			//std::cout<<"start queue_data "<<"port="<<_port<<": "<<std::hex<<p[0]<<std::dec<<", size/4="<<size/4<<" , nbytes/4="<<std::dec<<nbytes/4<<std::dec<<std::endl;
			_com->queue_data(p,size/4, queuenumber);      
		}


		// Update destination pointer and source iterator
		iter += size;
	} 
}

/*void  RogueReceiver::acceptFrame ( std::shared_ptr<rogue::interfaces::stream::Frame> frame ) {

	//4 RX fifo queue
	unsigned queuenumber=0;
	if(_type==0){
		queuenumber=_port-1;
	}else if(_type==1){
		queuenumber=_port-0;
	}

	rogue::interfaces::stream::FrameIterator it;

	// Acquire lock on frame. Will be release when lock class goes out of scope
	std::shared_ptr<rogue::interfaces::stream::FrameLock> lock = frame->lock();

	// Here we get an iterator to the frame data in read mode
	it = frame->beginRead();

	while ( it != frame->endRead() ) {

		auto size = it.remBuffer ();
		std::cout<<"start queue_data size/4="<<size/4<<std::endl;

		uint32_t data32;

		for(int i=0;i<size/4;i++){
			// Read 32-bits and advance iterator 4 bytes
			fromFrame(it, 4, &data32);

			//std::cout<<std::hex<<"data="<<(data32)<<std::dec<<std::endl;
			_com->queue_data(data32, queuenumber);
		}

	}
}*/

/*void  RogueReceiver::acceptFrame ( std::shared_ptr<rogue::interfaces::stream::Frame> frame ) {

	//4 RX fifo queue
	unsigned queuenumber=0;
	if(_type==0){
		queuenumber=_port-1;
	}else if(_type==1){
		queuenumber=_port-0;
	}

	rogue::interfaces::stream::FrameIterator it;

	// Acquire lock on frame. Will be release when lock class goes out of scope
	std::shared_ptr<rogue::interfaces::stream::FrameLock> lock = frame->lock();

	// Here we get an iterator to the frame data in read mode
	it = frame->beginRead();

	while ( it != frame->endRead() ) {

		auto size = it.remBuffer ();
		auto sizeByte = size;
		//std::cout<<"start queue_data size/4="<<size/4<<std::endl;

		uint32_t data32_0;
		uint32_t data32_1;

		for(int i=0;i<sizeByte;i++){
			// Read 32-bits and advance iterator 4 bytes
			fromFrame(it, 4, &data32_0);
			fromFrame(it, 4, &data32_1);

			_com->queue_data(data32_0, queuenumber);
			_com->queue_data(data32_1, queuenumber);

			//if(data32_1==0xffffffff){
			//	std::cout<<"aurora frame= "<<"2"<<std::hex<<0x1e040000<<std::setfill('0')<<std::setw(8)<<data32_0<<std::dec<<std::endl;
			//}else{
			//	std::cout<<"aurora frame= "<<"1"<<std::hex<<std::setfill('0')<<std::setw(8)<<data32_1<<std::setfill('0')<<std::setw(8)<<data32_0<<std::dec<<std::endl;
			//}
		}
			rx_bytes+=sizeByte;
			rx_pkts++;
			if(sizeByte<rx_min_pkt) rx_min_pkt=sizeByte;
			if(sizeByte>rx_max_pkt) rx_max_pkt=sizeByte;      

	}
}*/


void RogueCom::connect(const std::string &conn) {
  for(unsigned i=0;i<1024;i++) bram[i]=0;
  trigLength=0;
  trigIter=0;
  trigFreq=0;
  if(conn.find("axis://") ==0  ) {
    std::string devname=conn;
    devname.erase(0,7);
	if(_type==0){//old FEB
	  axisrp  =  rogue::hardware::axi::AxiStreamDma::create(devname,0,true); 
	  axicfg[0]  =  rogue::hardware::axi::AxiStreamDma::create(devname,0+1,true); 
	  axicfg[1]  =  rogue::hardware::axi::AxiStreamDma::create(devname,1+1,true); 
	  axicfg[2]  =  rogue::hardware::axi::AxiStreamDma::create(devname,2+1,true); 
	  axicfg[3]  =  rogue::hardware::axi::AxiStreamDma::create(devname,3+1,true); 
      axidata[0] =  rogue::hardware::axi::AxiStreamDma::create(devname,0+5,true); 
      axidata[1] =  rogue::hardware::axi::AxiStreamDma::create(devname,1+5,true); 
      axidata[2] =  rogue::hardware::axi::AxiStreamDma::create(devname,2+5,true); 
      axidata[3] =  rogue::hardware::axi::AxiStreamDma::create(devname,3+5,true); 
	}else if(_type==1){//new FMC
		if(devname=="/dev/axi_stream_dma_0"){
			//this is specific for ZCU102 standalone mode
			axisrp  =  rogue::hardware::axi::AxiStreamDma::create("/dev/axi_stream_dma_1",8,true); 
		}else{
			axisrp  =  rogue::hardware::axi::AxiStreamDma::create(devname,8,true); 
		}
      axicfg[0]  =  rogue::hardware::axi::AxiStreamDma::create(devname,0+0,true); 
      axicfg[1]  =  rogue::hardware::axi::AxiStreamDma::create(devname,1+0,true); 
      axicfg[2]  =  rogue::hardware::axi::AxiStreamDma::create(devname,2+0,true); 
      axicfg[3]  =  rogue::hardware::axi::AxiStreamDma::create(devname,3+0,true); 
      axidata[0] =  rogue::hardware::axi::AxiStreamDma::create(devname,0+4,true); 
      axidata[1] =  rogue::hardware::axi::AxiStreamDma::create(devname,1+4,true); 
      axidata[2] =  rogue::hardware::axi::AxiStreamDma::create(devname,2+4,true); 
      axidata[3] =  rogue::hardware::axi::AxiStreamDma::create(devname,3+4,true); 
	}
    srp = rogue::protocols::srp::SrpV3::create();
    axisrp->addSlave(srp);
    srp->addSlave(axisrp);
    mast = rogue::interfaces::memory::Master::create();
    mast->setSlave(srp);
	if(_type==0){//old FEB
    configStream[0] = std::make_shared<RogueSender>(0+1, _type);
    configStream[1] = std::make_shared<RogueSender>(1+1, _type);
    configStream[2] = std::make_shared<RogueSender>(2+1, _type);
    configStream[3] = std::make_shared<RogueSender>(3+1, _type);
	}else if(_type==1){//new FMC
    configStream[0] = std::make_shared<RogueSender>(0+0, _type);
    configStream[1] = std::make_shared<RogueSender>(1+0, _type);
    configStream[2] = std::make_shared<RogueSender>(2+0, _type);
    configStream[3] = std::make_shared<RogueSender>(3+0, _type);
	}
    configStream[0]->addSlave(axicfg[0]);
    configStream[1]->addSlave(axicfg[1]);
    configStream[2]->addSlave(axicfg[2]);
    configStream[3]->addSlave(axicfg[3]);
	if(_type==0){//old FEB
      debugStream[0]=std::make_shared<RogueReceiver>(0+1, _type, this->getInstance());
      debugStream[1]=std::make_shared<RogueReceiver>(1+1, _type, this->getInstance());
      debugStream[2]=std::make_shared<RogueReceiver>(2+1, _type, this->getInstance());
      debugStream[3]=std::make_shared<RogueReceiver>(3+1, _type, this->getInstance());
      dataStream[0] = std::make_shared<RogueReceiver>(0+5, _type,this->getInstance());
      dataStream[1] = std::make_shared<RogueReceiver>(1+5, _type,this->getInstance());
      dataStream[2] = std::make_shared<RogueReceiver>(2+5, _type,this->getInstance());
      dataStream[3] = std::make_shared<RogueReceiver>(3+5, _type,this->getInstance());
	}else if(_type==1){//new FMC
      debugStream[0]=std::make_shared<RogueReceiver>(0+0, _type,this->getInstance());
      debugStream[1]=std::make_shared<RogueReceiver>(1+0, _type,this->getInstance());
      debugStream[2]=std::make_shared<RogueReceiver>(2+0, _type,this->getInstance());
      debugStream[3]=std::make_shared<RogueReceiver>(3+0, _type,this->getInstance());
      dataStream[0] = std::make_shared<RogueReceiver>(0+4, _type,this->getInstance());
      dataStream[1] = std::make_shared<RogueReceiver>(1+4, _type,this->getInstance());
      dataStream[2] = std::make_shared<RogueReceiver>(2+4, _type,this->getInstance());
      dataStream[3] = std::make_shared<RogueReceiver>(3+4, _type,this->getInstance());
	}
    axidata[0]->addSlave(dataStream[0]);
    axidata[1]->addSlave(dataStream[1]);
    axidata[2]->addSlave(dataStream[2]);
    axidata[3]->addSlave(dataStream[3]);
    axicfg[0]->addSlave(debugStream[0]);
    axicfg[1]->addSlave(debugStream[1]);
    axicfg[2]->addSlave(debugStream[2]);
    axicfg[3]->addSlave(debugStream[3]);
  }
  else if (conn.find("ip://")== 0 ) {
    std::string ip=conn;
    ip.erase(0,5);

	if(_type==0){//FEB board
      udp = rogue::protocols::udp::Client::create(ip,8192,true);
      udp->setRxBufferCount(128); 
      rssi = rogue::protocols::rssi::Client::create(udp->maxPayload());
      udp->addSlave(rssi->transport());
      rssi->transport()->addSlave(udp);

      // Packetizer, ibCrc = false, obCrc = true
      pack = rogue::protocols::packetizer::CoreV2::create(false,true,true);
      rssi->application()->addSlave(pack->transport());
      pack->transport()->addSlave(rssi->application());

      // Create an SRP master and connect it to the packetizer
      srp = rogue::protocols::srp::SrpV3::create();
      pack->application(0)->addSlave(srp);
      srp->addSlave(pack->application(0));

      // Create a memory master and connect it to the srp
      mast = rogue::interfaces::memory::Master::create();
      mast->setSlave(srp);
      configStream[0] = std::make_shared<RogueSender>(0+1, _type);
      configStream[1] = std::make_shared<RogueSender>(1+1, _type);
      configStream[2] = std::make_shared<RogueSender>(2+1, _type);
      configStream[3] = std::make_shared<RogueSender>(3+1, _type);
      configStream[0]->addSlave(pack->application(0+1));
      configStream[1]->addSlave(pack->application(1+1));
      configStream[2]->addSlave(pack->application(2+1));
      configStream[3]->addSlave(pack->application(3+1));
      dataStream[0]= std::make_shared<RogueReceiver>(0+5, _type,this->getInstance());
      dataStream[1]= std::make_shared<RogueReceiver>(1+5, _type,this->getInstance());
      dataStream[2]= std::make_shared<RogueReceiver>(2+5, _type,this->getInstance());
      dataStream[3]= std::make_shared<RogueReceiver>(3+5, _type,this->getInstance());
      debugStream[0]=std::make_shared<RogueReceiver>(0+1, _type,this->getInstance());
      debugStream[1]=std::make_shared<RogueReceiver>(1+1, _type,this->getInstance());
      debugStream[2]=std::make_shared<RogueReceiver>(2+1, _type,this->getInstance());
      debugStream[3]=std::make_shared<RogueReceiver>(3+1, _type,this->getInstance());
      pack->application(0+5)->addSlave(dataStream[0]);
      pack->application(1+5)->addSlave(dataStream[1]);
      pack->application(2+5)->addSlave(dataStream[2]);
      pack->application(3+5)->addSlave(dataStream[3]);
      pack->application(0+1)->addSlave(debugStream[0]);   
      pack->application(1+1)->addSlave(debugStream[1]);   
      pack->application(2+1)->addSlave(debugStream[2]);   
      pack->application(3+1)->addSlave(debugStream[3]);   
      rssi->start();    
      while ( ! rssi->getOpen() ) {
        sleep(1);
        std::cout << "Establishing link ...\n";
      }

	}else if(_type==1){//FMC board
      udp = rogue::protocols::udp::Client::create(ip,8192,true);
      udpSrp = rogue::protocols::udp::Client::create(ip,8193,true);
      udp->setRxBufferCount(128); 
      udpSrp->setRxBufferCount(128); 
      rssi = rogue::protocols::rssi::Client::create(udp->maxPayload());
      rssiSrp = rogue::protocols::rssi::Client::create(udpSrp->maxPayload());
      udp->addSlave(rssi->transport());
      udpSrp->addSlave(rssiSrp->transport());
      rssi->transport()->addSlave(udp);
      rssiSrp->transport()->addSlave(udpSrp);

      // Packetizer, ibCrc = false, obCrc = true
      pack = rogue::protocols::packetizer::CoreV2::create(false,true, true);
      packSrp = rogue::protocols::packetizer::CoreV2::create(false,true, true);
      rssi->application()->addSlave(pack->transport());
      rssiSrp->application()->addSlave(packSrp->transport());
      pack->transport()->addSlave(rssi->application());
      packSrp->transport()->addSlave(rssiSrp->application());

      // Create an SRP master and connect it to the packetizer
      srp = rogue::protocols::srp::SrpV3::create();
      srpSrp = rogue::protocols::srp::SrpV3::create();
      pack->application(0)->addSlave(srp);
      packSrp->application(0)->addSlave(srpSrp);
      srp->addSlave(pack->application(0));
      srpSrp->addSlave(packSrp->application(0));

      // Create a memory master and connect it to the srp
      mast = rogue::interfaces::memory::Master::create();
      //mast->addSlave(srp);
      mast->setSlave(srpSrp);
      configStream[0] = std::make_shared<RogueSender>(0, _type);
      configStream[1] = std::make_shared<RogueSender>(1, _type);
      configStream[2] = std::make_shared<RogueSender>(2, _type);
      configStream[3] = std::make_shared<RogueSender>(3, _type);
      configStream[0]->addSlave(pack->application(0));
      configStream[1]->addSlave(pack->application(1));
      configStream[2]->addSlave(pack->application(2));
      configStream[3]->addSlave(pack->application(3));
      dataStream[0]= std::make_shared<RogueReceiver>(0+4, _type,this->getInstance());
      dataStream[1]= std::make_shared<RogueReceiver>(1+4, _type,this->getInstance());
      dataStream[2]= std::make_shared<RogueReceiver>(2+4, _type,this->getInstance());
      dataStream[3]= std::make_shared<RogueReceiver>(3+4, _type,this->getInstance());
      debugStream[0]=std::make_shared<RogueReceiver>(0+0, _type,this->getInstance());
      debugStream[1]=std::make_shared<RogueReceiver>(1+0, _type,this->getInstance());
      debugStream[2]=std::make_shared<RogueReceiver>(2+0, _type,this->getInstance());
      debugStream[3]=std::make_shared<RogueReceiver>(3+0, _type,this->getInstance());
      pack->application(0+4)->addSlave(dataStream[0]);
      pack->application(1+4)->addSlave(dataStream[1]);
      pack->application(2+4)->addSlave(dataStream[2]);
      pack->application(3+4)->addSlave(dataStream[3]);
      pack->application(0+0)->addSlave(debugStream[0]);   
      pack->application(1+0)->addSlave(debugStream[1]);   
      pack->application(2+0)->addSlave(debugStream[2]);   
      pack->application(3+0)->addSlave(debugStream[3]);   
      rssi->start();    
      rssiSrp->start();    
      while ( ! rssi->getOpen() || ! rssiSrp->getOpen() ) {
        sleep(1);
        std::cout << "Establishing link ...\n";
      }
	}


  } else {
    throw;
  }

  if(_type==0){
    sysReg=0x00030000;
    NTCReg=0x00040000;
    rxPhyMon[0]= 0x01000000*(0+1) + 0x00100000;    
    rxPhyMon[1]= 0x01000000*(1+1) + 0x00100000;    
    rxPhyMon[2]= 0x01000000*(2+1) + 0x00100000;    
    rxPhyMon[3]= 0x01000000*(3+1) + 0x00100000;    
    trigEmu=0x05000000+0x20000;
    trigLUT=0x05000000+0x10000;
    trigTLU=0x05000000+0x00000;
  }else if(_type==1){
    // sysReg=0x00030000;
    pllReg=0x80000;
    //NTCReg=0x00040000;
    ctrl[0]= 0x010000*0;
    ctrl[1]= 0x010000*1;
    ctrl[2]= 0x010000*2;
    ctrl[3]= 0x010000*3;
    //trigEmu=0x05000000+0x20000;
    //trigLUT=0x05000000+0x10000;
    trigEmu=0xA0000;
    trigLUT=0x90000;
    //trigTLU=0x05000000+0x00000;

    //configPLL for the new FMC card
    //configPLL("Si5345-RevD-Registers.csv");

    //inv_delay_CMD(2);
  }

	//std::cout<<"GlobalMonitor of DAQ"<<std::endl;
	//GlobalMonitor();
  
}

void RogueCom::configPLL(const std::string PLLfile, bool forceConfig){


	uint32_t add=0;
	uint32_t data=0;
	uint32_t dataRead=0;

	//to check PLL is locked or not
	// to do 
	// address looks like wrong
	add=pllReg+(0x0E << 2);

	readRegister(add,dataRead);
	std::cout<<"dataRead="<<std::hex<<dataRead<<std::dec<<std::endl;
	dataRead=(dataRead&2)>>1;
	std::cout<<"dataRead="<<std::hex<<dataRead<<std::dec<<std::endl;
	if(dataRead==0){
		std::cout<<"PLL is stable now"<<std::endl;
		if(forceConfig==false) return;
	}else{
		std::cout<<"PLL is not stable now"<<std::endl;
	}



	//power down at first
	std::cout<<"power down at first"<<std::endl;
	add=pllReg+(0x1E<<2);
	data=0;
	readRegister(add,data);
	std::cout<<std::hex<<"add="<<add<<", data="<<data<<std::dec<<std::endl;
	data=((data>>1)<<1)+1;//power down
	std::cout<<std::hex<<"add="<<add<<", data="<<data<<std::dec<<std::endl;
    writeRegister(add,data);
    readRegister(add,data);
	std::cout<<std::hex<<"add="<<add<<", data="<<data<<std::dec<<std::endl;
	data=0;



	std::cout << "\n----------------------" <<std:: endl;
	std::ifstream in(PLLfile.c_str());
    if (!in.is_open()) return ;

//    typedef tokenizer< escaped_list_separator<char> > Tokenizer;
	std::string line_add;
	std::string line_val;



	std::cout << "\nStart to config PLL----------------------" << std::endl;
    while (getline(in,line_add, ',') )
    {
		getline(in,line_val);
//		std::cout<<line_add<<", "<<line_val<<std::endl;
		if(line_add=="Address")continue;

		add =static_cast<uint32_t>(std::stoul(line_add, 0, 16));
		add = add<<2;
		add = pllReg+add;
		data=static_cast<uint32_t>(std::stoul(line_val, 0, 16));

//		std::cout<<add<<", "<<data<<std::endl;
		readRegister(add,dataRead);
		std::cout<<"before write: "<<std::hex<<add<<", "<<dataRead<<std::dec<<std::endl;
		std::cout<<"will write  : "<<std::hex<<add<<", "<<data<<std::dec<<std::endl;
		writeRegister(add,data);
		readRegister(add,dataRead);
		std::cout<<"after write : "<<std::hex<<add<<", "<<dataRead<<std::dec<<std::endl;
		dataRead=0;

    }
	std::cout << "\n----------------------" << std::endl;

	//power up at first
	add=pllReg+(0x1E<<2);
	data=0;
	readRegister(add,data);
	std::cout<<std::hex<<"add="<<add<<", data="<<data<<std::dec<<std::endl;
	data=((data>>1)<<1)+0;
	std::cout<<std::hex<<"add="<<add<<", data="<<data<<std::dec<<std::endl;
    writeRegister(add,data);
    readRegister(add,data);
	std::cout<<std::hex<<"add="<<add<<", data="<<data<<std::dec<<std::endl;
	data=0;




	//to check PLL is locked or not
	// to do 
	// address looks like wrong
	add=pllReg+(0x0E << 2);

	while(1){
		sleep(1);
		readRegister(add,dataRead);
	std::cout<<"dataRead="<<std::hex<<dataRead<<std::dec<<std::endl;
		dataRead=(dataRead&2)>>1;
	std::cout<<"dataRead="<<std::hex<<dataRead<<std::dec<<std::endl;
		if(dataRead==0)break;
		std::cout<<"PLL is not stable now"<<std::endl;

	}
	std::cout<<"PLL is stable now"<<std::endl;
	std::cout << "PLL is configured. Please check the RD53 Chip Current. You may need power cycle it. After that, press Enter to Continue"; std::cin.ignore();
	return;



}

uint32_t  RogueCom::getCurSize(){  
  uint32_t size=rxfifo[4+_rxchannel].size(); //rx_data

  return size;
}
bool  RogueCom::isEmpty() {
  bool result=(txfifo_cnt[_txchannel]==0);

  if(result==false){
	  forceRelaseTxfifo=1;
	  releaseFifo();
	  result=(txfifo_cnt[_txchannel]==0);
  }

  return result;
}
uint32_t  RogueCom::read32(){
  uint32_t val=0;
  val=rxfifo[4+_rxchannel].front();
  rxfifo[4+_rxchannel].pop();
  return val;
}
uint32_t  RogueCom::readBlock32(uint32_t *buf, uint32_t length){
  unsigned l=rxfifo[4+_rxchannel].size();
  if(l>length) l=length;
  for(unsigned i=0;i<l;i++) {
    buf[i]=rxfifo[4+_rxchannel].front();
    rxfifo[4+_rxchannel].pop();
//  std::cout<<"RogueCom::read32: "<<std::hex<<buf[i]<<std::dec<<std::endl;
  } 
  return l;
}
void  RogueCom::write32(uint32_t value){
  if(_txchannel==0)txfifo_0[(txfifo_cnt[0])]=value;
  if(_txchannel==1)txfifo_1[(txfifo_cnt[1])]=value;
  if(_txchannel==2)txfifo_2[(txfifo_cnt[2])]=value;
  if(_txchannel==3)txfifo_3[(txfifo_cnt[3])]=value;
  txfifo_cnt[_txchannel]++;

  //std::cout << "[RogueCom::write32]\t\t" << std::hex << value << std::dec << std::endl;
}
void  RogueCom::releaseFifo(){
  //if(forceRelaseTxfifo || txfifo_cnt>2048){ 
  if(forceRelaseTxfifo || txfifo_cnt[_txchannel]>2048){ 
    if(_txchannel==0)configStream[0]->send((uint8_t*)(txfifo_0),sizeof(uint32_t)*(txfifo_cnt[0]));
    if(_txchannel==1)configStream[1]->send((uint8_t*)(txfifo_1),sizeof(uint32_t)*(txfifo_cnt[1]));
    if(_txchannel==2)configStream[2]->send((uint8_t*)(txfifo_2),sizeof(uint32_t)*(txfifo_cnt[2]));
    if(_txchannel==3)configStream[3]->send((uint8_t*)(txfifo_3),sizeof(uint32_t)*(txfifo_cnt[3]));
    //configStream[1]->send((uint8_t*)txfifo,sizeof(uint32_t)*txfifo_cnt);
    //configStream[2]->send((uint8_t*)txfifo,sizeof(uint32_t)*txfifo_cnt);
    //configStream[3]->send((uint8_t*)txfifo,sizeof(uint32_t)*txfifo_cnt);
    txfifo_cnt[_txchannel]=0;
    forceRelaseTxfifo=false;
  }

}
void RogueCom::enableLane(uint32_t mask){
  uint32_t temp=mask&0xf;
  if(_type==0){
  writeRegister(rxPhyMon[0]+0x800,temp);
  writeRegister(rxPhyMon[1]+0x800,temp);
  writeRegister(rxPhyMon[2]+0x800,temp);
  writeRegister(rxPhyMon[3]+0x800,temp);
  }else{
  writeRegister(ctrl[0]+0x800,temp);
  writeRegister(ctrl[1]+0x800,temp);
  writeRegister(ctrl[2]+0x800,temp);
  writeRegister(ctrl[3]+0x800,temp);
  }
}
void RogueCom::enableDebugStream(bool enable){
  uint32_t temp=enable?1:0;
  if(_type==0){
  writeRegister(rxPhyMon[0]+0x810,temp);
  writeRegister(rxPhyMon[1]+0x810,temp);
  writeRegister(rxPhyMon[2]+0x810,temp);
  writeRegister(rxPhyMon[3]+0x810,temp);
  }else{
  writeRegister(ctrl[0]+0x810,temp);
  writeRegister(ctrl[1]+0x810,temp);
  writeRegister(ctrl[2]+0x810,temp);
  writeRegister(ctrl[3]+0x810,temp);
  }
}


void RogueCom::flushBuffer(){
	for(unsigned i=0;i<4;i++) {
                 std::queue<int> empty;
		rxfifo[0+i]=std::queue<uint32_t>();
		rxfifo[4+i]=std::queue<uint32_t>();
	} 
	std::cout<<"zixu in RogueCom::flushBuffer"<<std::endl;
}

std::pair<uint32_t, uint32_t> decodeRegRead(uint32_t higher, uint32_t lower) {
    if ((higher & 0x55000000) == 0x55000000) {

		
       std::pair<uint32_t, uint32_t> answer =  std::make_pair((lower>>16)&0x3FF, lower&0xFFFF);
	   std::cout << "reg0 Addr (" << answer.first << ") Value(" << answer.second << ")" << std::endl;
	   return answer;

    } else if ((higher & 0x99000000) == 0x99000000) {

       std::pair<uint32_t, uint32_t> answer =  std::make_pair((higher>>10)&0x3FF, ((lower>>26)&0x3F)+((higher&0x3FF)<<6));
	   std::cout << "reg1 Addr (" << answer.first << ") Value(" << answer.second << ")" << std::endl;
	   return answer;

    } else if ((higher & 0xd2000000) == 0xd2000000) {

       std::pair<uint32_t, uint32_t> answer1 =  std::make_pair((lower>>16)&0x3FF, lower&0xFFFF);
       std::pair<uint32_t, uint32_t> answer2 =  std::make_pair((higher>>10)&0x3FF, ((lower>>26)&0x3F)+((higher&0x3FF)<<6));
	   std::cout << "reg0 Addr (" << answer1.first << ") Value(" << answer1.second << ")" << std::endl;
	   std::cout << "reg1 Addr (" << answer2.first << ") Value(" << answer2.second << ")" << std::endl;
	   return answer2;
 
    } else {
        std::cout << "#ERROR# Could not decode reg read!" << std::endl;
        return std::make_pair(999, 666);
    }


}


