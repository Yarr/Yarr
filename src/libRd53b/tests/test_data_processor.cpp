#include "catch.hpp"

#include <iostream>

#include "AllProcessors.h"

#include "EventData.h"

#include "Rd53bCfg.h"

#include "rd53b_test_stream.h"
#include "rd53b_test_truth.h"

TEST_CASE("Rd53bDataProcessor", "[rd53b][data_processor]") {
  std::shared_ptr<FeDataProcessor> proc = StdDict::getDataProcessor("RD53B");

  REQUIRE (proc);

  ClipBoard<RawDataContainer> rd_cp;
  ClipBoard<EventDataBase> em_cp;

  Rd53bCfg cfg;  

  proc->connect(&cfg, &rd_cp, &em_cp );

  proc->init();
  proc->run();

  RawDataPtr rd = std::make_shared<RawData>(0, nWords);
  uint32_t *buffer = rd->getBuf();
  buffer[nWords-1] = 0;

  std::copy(words, words+nWords, buffer);


  std::unique_ptr<RawDataContainer> rdc(new RawDataContainer(LoopStatus()));
  rdc->add(std::move(rd));
  rd_cp.pushData(std::move(rdc));

  rd_cp.finish();

  proc->join();

  REQUIRE (!em_cp.empty());

  auto data = em_cp.popData();
  FrontEndData &rawData = *(FrontEndData*)data.get();

  REQUIRE (rawData.events.size() == truth_nEvents);

  unsigned ihit = 0;
  for (auto &event : rawData.events){
	  for(auto &hit : event.hits){
		  REQUIRE(hit.col == (truth_hits[ihit][0]+1));
		  REQUIRE(hit.row == (truth_hits[ihit][1]+1));
		  REQUIRE(hit.tot == truth_hits[ihit][2]);
		  ihit++;
	  }
  }

  REQUIRE (ihit == truth_nHits);

  // Only one thing
  REQUIRE (em_cp.empty());
}
