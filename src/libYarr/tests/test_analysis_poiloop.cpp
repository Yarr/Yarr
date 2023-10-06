#include "catch.hpp"

#include "AllChips.h"
#include "ScanFactory.h"
#include "LoopActionBase.h"
#include "AnalysisAlgorithm.h"
#include "Histo1d.h"
#include "Histo2d.h"
#include "FeedbackBase.h"

#include "EmptyFrontEnd.h"
#include "EmptyHw.h"

namespace {

  ////////
  // A loop action writes histograms directly to a ClipBoard
  class MyHistoGenerator : public LoopActionBase {

  public:
    MyHistoGenerator() : LoopActionBase(LOOP_STYLE_NOP) {
      min = 0;
      max = 0;
      step = 1;
    }

    void connect(ClipBoard<HistogramBase> *clip) { clipHisto = clip; }

  private:
    ClipBoard<HistogramBase> *clipHisto;
    std::unique_ptr<HistogramBase> h;

    void init() {  m_done = false; }

    void execPart1() {
      // Create and fill a dummy histogram
      Histo1d* h1d = new Histo1d("myHisto", 10, -0.5, 9.5, g_stat->record());
      h1d->fill(1, 23);
      h1d->fill(4, 56);
      h1d->fill(7, 89);
      h.reset(h1d);
    }

    void execPart2() { m_done = true; }

    void end() {
      clipHisto->pushData(std::move(h));
    }
  };

  ////////
  // A dummy analysis algorithm that fills some value to a 1-bin 1D histogram
  class MyAnalyzer : public AnalysisAlgorithm {

  public:
    MyAnalyzer() : AnalysisAlgorithm() {}
    ~MyAnalyzer() {}

    void init(ScanBase *s) {
      n_count = 1;
      for (unsigned n=0; n<s->size(); n++) {
        std::shared_ptr<LoopActionBase> l = s->getLoop(n);
        if ( not( l->getStyle()==LOOP_STYLE_NOP or (l->isParameterLoop() and isPOILoop(dynamic_cast<StdParameterLoop*>(l.get()))) ) ) {
          // outer loops
          loops.push_back(n);
          loopMax.push_back((unsigned)l->getMax());
        } else {
          unsigned cnt = (l->getMax()-l->getMin())/l->getStep();
          if (l->isParameterLoop()) {
            cnt++; // Parameter loop interval is inclusive
          }
          if (cnt == 0) cnt = 1;
          n_count = n_count*cnt;
        }
      }
    }

    void loadConfig(const json &j) {
      for (unsigned i=0; i<j["parametersOfInterest"].size(); i++) {
        m_parametersOfInterest.push_back(j["parametersOfInterest"][i]);
      }
    }

    void processHistogram(HistogramBase *h) {
      if (h->getName() != "myHisto")
        return;

      unsigned ident = 0;
      unsigned offset = 0;

      double outpar_product = 1.;
      for (unsigned n=0; n<loops.size(); n++) {
        ident += h->getStat().get(loops[n])+offset;
        offset += loopMax[n];
        outpar_product *= (h->getStat().get(loops[n]));
      }

      // Check if the output histogram exists
      if (hMap[ident] == nullptr) {
        // 1-bin histogram
        hMap[ident] = std::make_unique<Histo1d>("h1", 1, 0.5, 1.5, h->getStat());
        innerCnt[ident] = 0;
      }

      // Do something with the input h
      // Get its mean
      double hmean = ((Histo1d*)h)->getMean();
      // Divde the mean by outpar_product
      hmean /= outpar_product;
      // Fill the 1-bin histogram hMap[ident] with the value as weight
      hMap[ident]->fill(1, hmean);
      innerCnt[ident]++;

      // Got all data, finish up Analysis
      if (innerCnt[ident] == n_count) {
        output->pushData(std::move(hMap[ident]));
      }
    }

  private:
    std::vector<unsigned> loops;
    std::vector<unsigned> loopMax;
    unsigned n_count;

    std::map<unsigned, std::unique_ptr<Histo1d>> hMap;
    std::map<unsigned, unsigned> innerCnt;
  };

  ////////
  // A dummy analysis algorithm that fills a 2D histogram using the histogram from its upstream algorithm
  class MyOtherAnalyzer : public AnalysisAlgorithm {

  public:
    MyOtherAnalyzer() {}
    ~MyOtherAnalyzer() {}

    void init(ScanBase *s) {
      for (unsigned n=0; n<s->size(); n++) {
        std::shared_ptr<LoopActionBase> l = s->getLoop(n);
        if ( l->isParameterLoop() and isPOILoop(dynamic_cast<StdParameterLoop*>(l.get())) ) {
          pois_min.push_back(l->getMin());
          pois_max.push_back(l->getMax());
          pois_step.push_back(l->getStep());
          pois_cnt.push_back( (l->getMax()-l->getMin())/l->getStep() + 1 );
          pois_loopindex.push_back(n);
        }
      }

      // Assume two parameters of interest for the test here
      assert(pois_min.size() == 2);

      // Create the output 2D histogram
      unsigned nbins_x = pois_cnt[0];
      double xmin = pois_min[0] - pois_step[0]/2.;
      double xmax = pois_max[0] + pois_step[0]/2.;
      unsigned nbins_y = pois_cnt[1];
      double ymin = pois_min[1] - pois_step[1]/2.;
      double ymax = pois_max[1] + pois_step[1]/2.;
      hxy.reset(new Histo2d("houtput", nbins_x, xmin, xmax, nbins_y, ymin, ymax));
    }

    void loadConfig(const json &j) {
      for (unsigned i=0; i<j["parametersOfInterest"].size(); i++) {
        m_parametersOfInterest.push_back(j["parametersOfInterest"][i]);
      }
    }

    void processHistogram(HistogramBase *h) {
      if (h->getName() != "h1") return;

      // Get the content of the 1-bin histogram h
      double z = ((Histo1d*)h)->getBin(0);

      // Set z as the bin content of the output 2d histogram
      unsigned x = h->getStat().get(pois_loopindex[0]);
      unsigned y = h->getStat().get(pois_loopindex[1]);

      hxy->fill(x, y, z);
    }

    void end() {
      output->pushData(std::move(hxy));
    }

  private:
    std::vector<double> pois_min;
    std::vector<double> pois_max;
    std::vector<unsigned> pois_step;
    std::vector<unsigned> pois_cnt;

    std::vector<unsigned> pois_loopindex;

    std::unique_ptr<Histo2d> hxy;
  };

  ////////
  // Because ScanBase::addLoop is protected ...
  class MyScan : public ScanFactory {
  public:
    MyScan(Bookkeeper *k, FeedbackClipboardMap *fb) : ScanFactory(k, fb){}

    void addLoop(std::shared_ptr<LoopActionBase> l) {
      ScanFactory::addLoop(l);
    }
  };
}

TEST_CASE("AnalysisPOILoops", "[Analysis]") {
  EmptyHw hwCtrl;
  Bookkeeper bookie(&hwCtrl, &hwCtrl);

  // Add one FE
  auto fe = std::make_unique<EmptyFrontEnd>();
  fe->setActive(true);
  unsigned channel = 42;
  bookie.addFe(fe.get(), channel);
  unsigned uid = bookie.getId(fe.release());

  // Global FE for scan
  auto g_fe = std::make_unique<EmptyFrontEnd>();
  g_fe->init(&hwCtrl, 0, 0);
  bookie.initGlobalFe(g_fe.release());

  MyScan scan(&bookie, nullptr);

  json scanCfg;
  scanCfg["scan"]["name"] = "TestAnalyses";

  ////
  // Loops
  scanCfg["scan"]["loops"][0]["loopAction"] = "StdParameterLoop";
  scanCfg["scan"]["loops"][0]["config"]["parameter"] = "X";
  scanCfg["scan"]["loops"][0]["config"]["min"] = 5;
  scanCfg["scan"]["loops"][0]["config"]["max"] = 15;
  scanCfg["scan"]["loops"][0]["config"]["step"] = 5;

  scanCfg["scan"]["loops"][1]["loopAction"] = "StdParameterLoop";
  scanCfg["scan"]["loops"][1]["config"]["parameter"] = "Y";
  scanCfg["scan"]["loops"][1]["config"]["min"] = 2;
  scanCfg["scan"]["loops"][1]["config"]["max"] = 8;
  scanCfg["scan"]["loops"][1]["config"]["step"] = 2;

  scanCfg["scan"]["loops"][2]["loopAction"] = "StdParameterLoop";
  scanCfg["scan"]["loops"][2]["config"]["parameter"] = "Z";
  scanCfg["scan"]["loops"][2]["config"]["min"] = 0;
  scanCfg["scan"]["loops"][2]["config"]["max"] = 9;
  scanCfg["scan"]["loops"][2]["config"]["step"] = 1;

//scanCfg["scan"]["loops"][3]["loopAction"] = "MyHistoGenerator";

  scan.loadConfig(scanCfg);

  // The last loop "MyHistoGenerator" is not in the registry. Add it by hand.
  std::unique_ptr<LoopActionBase> hgen = std::make_unique<MyHistoGenerator>();
  // Connect histo clipboard
  static_cast<MyHistoGenerator*>(hgen.get())->connect(&(bookie.getEntry(uid).fe->clipHisto));
  // This loop directly writes to the input ClipBoard of the analysis chain
  // It bypasses DataProcessors and Histogrammers, which are not the test targets here.
  scan.addLoop(std::move(hgen));

  ////
  // Analysis
  scanCfg["scan"]["analysis"]["0"]["algorithm"] = "MyAnalyzer";
  scanCfg["scan"]["analysis"]["0"]["config"]["parametersOfInterest"] = {"Z"};

  scanCfg["scan"]["analysis"]["1"]["algorithm"] = "MyOtherAnalyzer";
  scanCfg["scan"]["analysis"]["1"]["dependOn"] = {"MyAnalyzer"};
  scanCfg["scan"]["analysis"]["1"]["config"]["parametersOfInterest"] = {"X", "Y"};

  scanCfg["scan"]["analysis"]["n_count"] = 2;

  // Build analysis processors for one FE
  // cf. ScanHelper::buildAnalyses
  std::vector<std::unique_ptr<AnalysisProcessor>> analyses;

  // First tier: MyAnalyzer
  analyses.emplace_back(new AnalysisProcessor(&bookie, uid));

  // Create ClipBoard for its output and make connections
  bookie.getEntry(uid).fe->clipResult.emplace_back(new ClipBoard<HistogramBase>());
  analyses[0]->connect(&scan, &(bookie.getEntry(uid).fe->clipHisto), bookie.getEntry(uid).fe->clipResult.back().get(), nullptr);

  // Add algorithm MyAnalyzer
  auto algo1 = std::make_unique<MyAnalyzer>();
  algo1->loadConfig(scanCfg["scan"]["analysis"]["0"]["config"]);
  analyses[0]->addAlgorithm(std::move(algo1));

  // Second tier: MyOtherAnalyzer
  analyses.emplace_back(new AnalysisProcessor(&bookie, uid));

  // Create and connect result ClipBoard
  bookie.getEntry(uid).fe->clipResult.emplace_back(new ClipBoard<HistogramBase>());
  analyses[1]->connect(&scan, bookie.getEntry(uid).fe->clipResult[0].get(), bookie.getEntry(uid).fe->clipResult[1].get(), nullptr, true);

  // Add algorithm MyOtherAnalyzer
  auto algo2 = std::make_unique<MyOtherAnalyzer>();
  algo2->loadConfig(scanCfg["scan"]["analysis"]["1"]["config"]);
  analyses[1]->addAlgorithm(std::move(algo2));

  // Done building analyses
  ////

  // Start processor threads
  for (auto& ana : analyses) {
    ana->init();
    ana->run();
  }

  // Start scan
  scan.init();
  scan.run();

  ////
  // Join threads
  bookie.getEntry(uid).fe->clipHisto.finish();

  for (unsigned i=0; i<analyses.size(); ++i) {
    analyses[i]->join();
    bookie.getEntry(uid).fe->clipResult[i]->finish();
  }

  // Check the final output
  auto &output = *(bookie.getEntry(uid).fe->clipResult.back());

  REQUIRE (!output.empty());

  while(!output.empty()) {
    auto result = output.popData();
    CAPTURE (result->getName());
    if (result->getName() == "houtput") {
      auto hh = static_cast<Histo2d*>(result.get());
      // 2D histogram:
      // X bin center = {5, 10, 15}
      // Y bin center = {2, 4, 6, 8}
      // Bin contain for (x, y) = 870/168*10/(x*y)
      // ( Mean of the histogram from MyHistoGenerator: 870/168
      //   h1 bin 0 is filled n times with 870/168/(x*y)
      //   n = number of Z iterations = 10 )
      //hh->toStream(std::cout);
      REQUIRE (hh->numOfEntries() == 3*4);
      for (unsigned x=5; x<=15; x+=5) {
        for (unsigned y=2; y<=8; y+=2) {
          int ibin = hh->binNum(x, y);
          REQUIRE ( fabs(hh->getBin(ibin) - (float)(870./168*10/(x*y))) < 1e-10 );
        } // x
      } // y

    } // if (result->getName() == "houtput")
  } // while(!output.empty())

}
