/*
 * Authors: T. Heim <timon.heim@cern.ch>,
 * Date: 2015-Jul-21
 */

#include "StdDataGatherer.h"

#include <chrono>
#include <iostream>
#include <thread>
#include <algorithm>
#include <set>
#include <csignal>

#include "logging.h"

using Clock = std::chrono::steady_clock;

namespace {
    auto sdglog = logging::make_log("StdDataGatherer");
}

StdDataGatherer::StdDataGatherer() : LoopActionBase(LOOP_STYLE_DATA) {
    loopType = typeid(this);
    min = 0;
    max = 0;
    step = 1;
    counter = 0;
}

void StdDataGatherer::init() {
    m_done = false;
    killswitch = false;
    auto trigAction = keeper->getTriggerAction();
    if (trigAction != nullptr) ntriggersToReceive = trigAction->getExpEvents() - g_rx->getTriggersLostTolerance();
    SPDLOG_LOGGER_TRACE(sdglog, "Number to events to expect = {}",ntriggersToReceive);
}

void StdDataGatherer::end() {
    SPDLOG_LOGGER_TRACE(sdglog, "");
}

void StdDataGatherer::execPart1() {
    SPDLOG_LOGGER_TRACE(sdglog, "");
    if (g_tx->getTrigEnable() == 0)
        SPDLOG_LOGGER_ERROR(sdglog, "Trigger is not enabled, will get stuck here!");

}

sig_atomic_t signaled = 0;

void StdDataGatherer::execPart2() {
    SPDLOG_LOGGER_TRACE(sdglog, "");
    unsigned count = 0;
    unsigned nAllRxReadIterations = 0;
    uint32_t triggerIsDone = 0;

    signaled = 0;
    signal(SIGINT, [](int signum){signaled = 1;});
    signal(SIGUSR1, [](int signum){signaled = 1;});

    // the RX channels that actually received data, and expect feedback                                                                                                     
    std::set<uint32_t> activeChannels;

    // the counter for the feedback from data processors                                                                                                                   
    std::map<uint32_t, uint32_t> channelReceivedTriggersCnt;
    for (unsigned id=0; id<keeper->getNumOfEntries(); id++) {
      channelReceivedTriggersCnt[id] = 0;
    }

    // to keep track of max time for the iteration                                                                                                                          
    std::chrono::microseconds timeElapsed;
    std::chrono::time_point<Clock> timeStart = Clock::now();

    // conditions to end the iteration: all triggers are received or out of time                                                                                            
    bool receivedAllTriggers = true;
    bool thereIsStillTime = false;

    //! initial wait before reading data
    std::this_thread::sleep_for(g_rx->getReadDelay());

    SPDLOG_LOGGER_WARN(sdglog, "IMPORTANT! Going into endless loop unless timelimit is set, interrupt with ^c (SIGINT)!");

    bool receivingRxData = true;
    while (receivingRxData) {
        std::vector<RawDataPtr> newData;
        std::map<uint32_t, std::unique_ptr<RawDataContainer>> rdcMap;
        
        newData =  g_rx->readData();
        nAllRxReadIterations++;

        // Read all data until buffer is empty
        while (newData.size() > 0 && count < 4096 && signaled == 0 && !killswitch) {
            if (newData.size() > 0) {
	        for (auto &dataChunk : newData) {
                    count += dataChunk->getSize();
                    for (unsigned &uid : keeper->getRxToId(dataChunk->getAdr())) {
                        if (rdcMap[uid] == nullptr) {
                            rdcMap[uid] = std::make_unique<RawDataContainer>(g_stat->record());
                        }

                        rdcMap[uid]->add(dataChunk);
                    }
                }
            }
            // Wait a little bit to increase chance of new data having arrived
            std::this_thread::sleep_for(std::chrono::microseconds(1));
            newData =  g_rx->readData();
	    nAllRxReadIterations++;
        }

	// Push the data to the Data processor
        for (auto &[id, rdc] : rdcMap) {
	  rdc->stat.is_end_of_iteration = false;
	  keeper->getEntry(id).fe->clipRawData.pushData(std::move(rdc));
	  activeChannels.insert(id);
        }

	// check for any feedback from data processing                                                                                                                      
	bool receivedFeedbackSomewhere = false;
        do {
	  receivedFeedbackSomewhere = false;

	  // check the active channels for feedback                                                                                                                        
	  for (auto& chan_id : activeChannels) {
	    // pull all the currently available feedback from this channel                                                                                               
	    while(bool receivedOnChan = keeper->getEntry(chan_id).fe->clipProcFeedback.waitNotEmptyOrDoneOrTimeout(g_rx->getAverageDataProcessingTime())) {
	      receivedFeedbackSomewhere |= receivedOnChan;
	      auto params = keeper->getEntry(chan_id).fe->clipProcFeedback.popData();

	      if (params->trigger_tag >=  0) {
		channelReceivedTriggersCnt[chan_id] += 1;
	      }
	    }
	  }
	} while (receivedFeedbackSomewhere);


        // test whether all channels received all triggers
	unsigned channelsWithAllTrigsN = 0;
	uint32_t nAllReceivedTriggersSoFar = 0;
        for (auto &[id, receivedTriggers] : channelReceivedTriggersCnt) {
	  nAllReceivedTriggersSoFar += receivedTriggers;

	  if (receivedTriggers >= ntriggersToReceive*0.95) {
	    channelsWithAllTrigsN += 1;
	  }
	}
	receivedAllTriggers = channelsWithAllTrigsN >= keeper->getNumOfEntries();

	if (count == 0) {
          SPDLOG_LOGGER_INFO(sdglog, "\033[1m\033[31m--> Received {} words in {} iterations!\033[0m", count, nAllRxReadIterations);
        } else {
          SPDLOG_LOGGER_INFO(sdglog, "--> Received {} words in {} iterations with {} events!", count, nAllRxReadIterations, nAllReceivedTriggersSoFar);
        }

        count = 0;

        if (signaled == 1 || killswitch) {
	  SPDLOG_LOGGER_WARN(sdglog, "Caught interrupt, stopping data taking!");
	  SPDLOG_LOGGER_WARN(sdglog, "Abort might leave data in buffers!");
	  g_tx->toggleTrigAbort();
	}

        // test whether there is still time for this iteration                                                                                                              
        timeElapsed = std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - timeStart);
        thereIsStillTime = timeElapsed.count() <  g_rx->getWaitTime().count(); // up to HW controller latency

        // Check if trigger is done                                                                                                                                         
        triggerIsDone = g_tx->isTrigDone();

        // Whether to execute another Rx cycle:                                                                                                                             
        receivingRxData = !triggerIsDone || (thereIsStillTime && !receivedAllTriggers);
        SPDLOG_LOGGER_INFO(sdglog, "one more Rx cycle: {} -- trigger_is_done = {} -- still time {} = {} < {} and all trigs = {} (n channels w all trigs = {}, n trigs = {})", receivingRxData, triggerIsDone, thereIsStillTime, timeElapsed.count(), g_rx->getWaitTime().count(), receivedAllTriggers, channelsWithAllTrigsN, nAllReceivedTriggersSoFar);

        // wait for the sampling time before the next Rx read cycle                                                                                                        
	if (receivingRxData) std::this_thread::sleep_for(g_rx->getReadInterval());
    }

    // the iteration end marker for the processing & analysis
    // send end-of-iteration empty container with LoopStatus::is_end_of_iteration = true
    LoopStatus loopStatusIterationEnd({0}, {LoopStyle::LOOP_STYLE_GLOBAL_FEEDBACK});
    loopStatusIterationEnd.is_end_of_iteration = true;
    for (unsigned id=0; id<keeper->getNumOfEntries(); id++) {
      std::unique_ptr<RawDataContainer> cIterEnd = std::make_unique<RawDataContainer>(std::move(loopStatusIterationEnd));
      keeper->getEntry(id).fe->clipRawData.pushData(std::move(cIterEnd));
    }

    m_done = true;
    counter++;
}
