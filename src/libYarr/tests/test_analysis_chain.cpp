#include "catch.hpp"

#include "ScanHelper.h"
#include "AllChips.h"
#include "ScanFactory.h"

#include "EmptyHw.h"
#include "EmptyFrontEnd.h"

TEST_CASE("AnalysisHierarchy", "[Analysis]") {

  // Configuration
  json cfg;
  // Four analysis algorithms
  cfg["scan"]["analysis"]["0"]["algorithm"] = "AnalysisA";
  cfg["scan"]["analysis"]["0"]["dependOn"]  = {"AnalysisB"}; 

  cfg["scan"]["analysis"]["1"]["algorithm"] = "AnalysisB";

  cfg["scan"]["analysis"]["2"]["algorithm"] = "AnalysisC";
  cfg["scan"]["analysis"]["2"]["dependOn"]  = {"AnalysisB"};

  cfg["scan"]["analysis"]["3"]["algorithm"] = "AnalysisD";
  cfg["scan"]["analysis"]["3"]["dependOn"]  = {"AnalysisA", "AnalysisB"};

  cfg["scan"]["analysis"]["n_count"] = 4;

  std::vector<std::vector<int>> algoIndexTiers;
  ScanHelper::buildAnalysisHierarchy(algoIndexTiers, cfg["scan"]["analysis"]);

  // Expected algorithm tiers
  // tier 0:     B(1)
  //            / | \
  // tier 1: A(0) | C(2)
  //            \ |
  // tier 2:     D(3)
  std::vector<std::vector<int>> expIndexTiers = {{1}, {2, 0}, {3}};

  CAPTURE (algoIndexTiers);

  REQUIRE (algoIndexTiers == expIndexTiers);
}

TEST_CASE("AnalysisChainIO", "[Analysis]") {

  EmptyHw hwCtrl;
  Bookkeeper bookie(&hwCtrl, &hwCtrl);
  FeedbackClipboardMap fbData;
  ScanFactory scan(&bookie, &fbData);

  // Add one FE, with large enough histogram for testing hit
  auto fe = std::make_unique<EmptyFrontEnd>(FrontEndGeometry{10, 10});
  fe->setActive(true);

  REQUIRE ( fe );

  REQUIRE ( fe->isActive() );

  unsigned channel = 42;
  bookie.addFe(fe.release(), channel);

  // Create a dummy scan config with known analyses to test IO connections
  // The dependency here is for test only and is arbitrary and meaningless
  json scanCfg;
  // tier 0:  OccupancyAnalysis(0)  
  //           |               |
  // tier 1:   |         L1Analysis(1)
  //            \             /
  // tier 2:    TagAnalysis(2)

  scanCfg["scan"]["analysis"]["0"]["algorithm"] = "OccupancyAnalysis";

  scanCfg["scan"]["analysis"]["1"]["algorithm"] = "L1Analysis";
  scanCfg["scan"]["analysis"]["1"]["dependOn"]  = {"OccupancyAnalysis"};

  scanCfg["scan"]["analysis"]["2"]["algorithm"] = "TagAnalysis";
  scanCfg["scan"]["analysis"]["2"]["dependOn"]  = {"OccupancyAnalysis","L1Analysis"};

  scanCfg["scan"]["analysis"]["n_count"] = 3;


  // Build analyses
  std::map<unsigned, std::vector<std::unique_ptr<AnalysisDataProcessor>> > analyses;
  ScanHelper::buildAnalyses(analyses, scanCfg, bookie, &scan, &fbData, -1, "./", -1, -1);

  auto& AnalysisProcessors = analyses[bookie.getId(bookie.getLastFe())];
  
  // Check there are three analysis tiers
  REQUIRE (AnalysisProcessors.size() == 3);

  // Start the analysis processors
  for (auto& aproc : AnalysisProcessors) {
    aproc->init();
    aproc->run();
  }

  // Send an OccupancyMap histogram to the analysis chain
  unsigned nCol = bookie.getLastFe()->geo.nCol;
  unsigned nRow = bookie.getLastFe()->geo.nRow;
  Histo2d* occmap = new Histo2d("OccupancyMap", nCol, 0.5, nCol+0.5, nRow, 0.5, nRow+0.5);
  occmap->setBin(1, 43);

  std::unique_ptr<HistogramBase> h(occmap);
  bookie.getLastFe()->clipHisto.pushData(std::move(h));

  // The input histogram should be processed by the OccupancyAnalysis
  // The other two algorithms should do nothing but only pass the output of OccupancyAnalysis to the downstream

  // Join threads
  bookie.getLastFe()->clipHisto.finish();

  for (unsigned i=0; i<AnalysisProcessors.size(); ++i) {
    AnalysisProcessors[i]->join();
    bookie.getLastFe()->clipResult.at(i)->finish();
  }

  // Check the final output
  auto &output = *(bookie.getLastFe()->clipResult.back());

  REQUIRE (!output.empty());

  while(!output.empty()) {
    auto result = output.popData();
    CAPTURE (result->getName());
    // Two histograms are expected in the ouput: OccupancyMap and EnMask
    // Both are from the OccupancyAnalysis

    if (result->getName() == "OccupancyMap") {
      auto h2d = dynamic_cast<Histo2d*>(result.get());
      REQUIRE (h2d);

      CHECK (h2d->getOverflow() < 1e-9);
      CHECK (h2d->getUnderflow() < 1e-9);

      // Check its content
      REQUIRE (h2d->getBin(1) == 43);   
    }
  }

}
