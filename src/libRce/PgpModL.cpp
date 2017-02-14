// File          : CommLink.cpp
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 04/12/2011
// Project       : General Purpose
//-----------------------------------------------------------------------------
// Description :
// Generic communications link
//-----------------------------------------------------------------------------
// Modification history :
// 04/12/2011: created
//-----------------------------------------------------------------------------

#include <sstream>
#include <iostream>
#include <iomanip>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include "RCDImasterL.hh"
#include "PgpModL.hh"
using namespace std;

// RX Thread
void * PgpModL::rxRun ( void *t ) {
   PgpModL *ti;
   ti = (PgpModL *)t;
   ti->rxHandler();
   pthread_exit(NULL);
   return(NULL);
}
// Constructor
PgpModL::PgpModL ( ): fd_(-1) {

   pthread_mutex_init(&mainMutex_,NULL);

   pthread_cond_init(&mainCondition_,NULL);
}

// Deconstructor
PgpModL::~PgpModL ( ) { 
   close();
}

//! Stop link
void PgpModL::closeLink () {
   ::close(fd_);
   fd_       = -1;
}
// Open link and start threads
void PgpModL::open () {
   stringstream err;
   stringstream tmp;
   err.str("");

   closeLink();
   runEnable_=true;

   const char path[]="/dev/axi_stream_dma_0";
   fd_ = ::open(path,O_RDWR | O_NONBLOCK);

   if ( fd_ < 0 ) {
      tmp.str("");
      tmp << "MultDestPgp::open -> Could Not Open AXIS path " << path <<std::endl;
      throw tmp.str();
   }
   PgpTrans::RCDImaster::instance()->setFd(fd_); 
   
   //reset the HSIO trigger and receiver in case there was a crash
   PgpTrans::RCDImaster::instance()->sendCommand(5); //stop run
   PgpTrans::RCDImaster::instance()->sendCommand(9); //rst receiver
   

   // Start rx thread
   if ( pthread_create(&rxThread_,NULL,rxRun,this) ) {
      err << "CommLink::open -> Failed to create rxThread"  <<std::endl;
      std::cout << err.str();
      closeLink();
      throw(err.str());
   }
#ifdef ARM
   else pthread_setname_np(rxThread_,"cLinkRxThread");
#endif

   usleep(1000); // Let threads catch up
}

// Stop threads and close link
void PgpModL::close () {

   // Stop the thread
   runEnable_ = false;

   // Wake up threads
   //dataThreadWakeup();
   usleep(1100); // Give enough time to threads to stop

   // Wait for thread to stop
   pthread_join(rxThread_, NULL);
}

// Receive Thread
void PgpModL::rxHandler() {
   fd_set             fds;
   int32_t            maxFd;
   struct timeval     timeout;

   // While enabled
   while ( runEnable_ ) {

      FD_ZERO(&fds);
      FD_SET(fd_,&fds);
      maxFd=fd_;
      // Setup timeout
      timeout.tv_sec  = 0;
      timeout.tv_usec = 1000;

      // Select
      if ( maxFd < 0 ) usleep(1000); // Nothing to listen to
      else if ( select(maxFd+1, &fds, NULL, NULL, &timeout) <= 0 ) continue;

      // Process each dest
      if ( FD_ISSET(fd_, &fds) ) {
	// Receive
	PgpTrans::RCDImaster::instance()->receive ();
      }
   }
}


// Set max rx size
void PgpModL::setMaxRxTx (uint32_t size) {
   stringstream err;

   if ( runEnable_ ) {
     err << "CommLink::setMaxRxTx -> Cannot set maxRxTx while open" << std::endl;
     std::cout << err.str();
     throw(err.str());
   }

   maxRxTx_ = size;

}

// Wake in main thread
void PgpModL::mainThreadWait(uint32_t usec) {
   struct timespec timeout;

   pthread_mutex_lock(&mainMutex_);

   clock_gettime(CLOCK_REALTIME,&timeout);

   timeout.tv_sec  += usec / 1000000;
   timeout.tv_nsec += (usec % 1000000) * 1000;

   pthread_cond_timedwait(&mainCondition_, &mainMutex_, &timeout);
   pthread_mutex_unlock(&mainMutex_);
}

// Wakeup main thread
void PgpModL::mainThreadWakeup() {
   pthread_cond_signal(&mainCondition_);
}

