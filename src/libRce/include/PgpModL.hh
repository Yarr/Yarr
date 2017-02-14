//-----------------------------------------------------------------------------
// File          : CommLink.h
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 04/12/2011
// Project       : General Purpose
//-----------------------------------------------------------------------------
// Description :
// Generic communications link
//-----------------------------------------------------------------------------
// Copyright (c) 2011 by SLAC. All rights reserved.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------
// Modification history :
// 04/12/2011: created
//-----------------------------------------------------------------------------
#ifndef __PGPMODL_HH__
#define __PGPMODL_HH__

#include <string>
#include <sstream>
#include <map>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <resolv.h>
#include <stdint.h>

using namespace std;


//! Class to contain generic communications link
class PgpModL {

protected:
  
  // Thread pointers
  pthread_t rxThread_;
  
  // Thread Routines
  static void *rxRun ( void *t );
  
  // Thread condition variables
  pthread_cond_t  mainCondition_;
  pthread_mutex_t mainMutex_;
  
  // Condition set and wait routines
  void mainThreadWait(uint32_t usec);
  void mainThreadWakeup();
  
  // Run enable
  bool runEnable_;
  
  // IO handling routines
  virtual void rxHandler();
  
  // Max RX/Tx size
  uint32_t maxRxTx_;
  // PGP file descriptor
  int32_t fd_;
  
public:
  
  //! Constructor
  PgpModL ( );
  
  //! Deconstructor
  virtual ~PgpModL ( );
  
  //! Open link and start threads
  /*! 
   * Return true on success.
   * Throws string on error.
   */
  virtual void open ();
  
  //! Stop threads and close link
  virtual void close ();
  virtual void closeLink ();

  //! Set max rx/tx size 
  /*! 
   * \param maxRxTx  Maximum receive/transmit size in bytes
   */
  void setMaxRxTx ( uint32_t maxRxTx );
  
};

#endif

