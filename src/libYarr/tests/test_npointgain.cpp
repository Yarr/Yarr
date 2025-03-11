#include <cmath>

#include "catch.hpp"

#include "AllAnalyses.h"
#include "AllChips.h"
#include "Bookkeeper.h"
#include "EmptyHw.h"
#include "EmptyFrontEnd.h"
#include "GraphErrors.h"
#include "Histo3d.h"
#include "ScanFactory.h"

TEST_CASE("NPointGain", "[Yarr][Analysis][NPointGain]") {

    json analysisConfig;

    std::vector<unsigned> injections = {0, 1, 2, 3, 4};
    std::vector<double> responses;
    std::vector<double> noises;

    std::function<double(double, const double*)> fitFunction;
    auto linear = [](double x, const double* par){return par[0] + par[1]*x;};
    auto polynomial = [](double x, const double* par){return par[0] + par[1]*x + par[2]*x*x;};
    auto exponential = [](double x, const double* par){return par[2] + par[0] / (1 + exp(-x / par[1]));};

    std::vector<double> desiredFitParams, tolerances;
    SECTION("Linear") {
        analysisConfig["fitFunction"] = "linear";
        fitFunction = linear;
        desiredFitParams = {1, 3};
        tolerances = {0.01, 0.01};
    }
    SECTION("Polynomial") {
        analysisConfig["fitFunction"] = "polynomial";
        fitFunction = polynomial;
        desiredFitParams = {1, 3, 0.25};
        tolerances = {0.01, 0.01, 0.01};
    }
    SECTION("Exponential") {
        analysisConfig["fitFunction"] = "exponential";
        fitFunction = exponential;
        desiredFitParams = {1, 2, 0.5};
        tolerances = {100, 10, 100};
    }

    for (const auto& inj : injections) {
        responses.push_back(fitFunction(inj, desiredFitParams.data()));
        noises.push_back(inj);
        CAPTURE(responses);
        CAPTURE(noises);
    }

    analysisConfig["parametersOfInterest"] = {"BCAL"};

    EmptyHw empty;
    Bookkeeper bookie(&empty, &empty);
    
    unsigned channel = 0;
    auto frontEnd = std::make_unique<EmptyFrontEnd>();
    frontEnd->setActive(true);
    bookie.addFe(std::move(frontEnd), channel);

    AnalysisProcessor processor(channel);

    int nCol = 256;
    int nRow = 2;

    {
        auto analysis = StdDict::getAnalysis("NPointGain");
        REQUIRE(analysis);

        analysis->loadConfig(analysisConfig);
        analysis->setMapSize(nCol, nRow);
        analysis->setConfig(bookie.getFeCfg(channel));

        processor.addAlgorithm(std::move(analysis));
    }

    ScanFactory scan(&bookie, nullptr);

    {
      json scanCfg;
      scanCfg["scan"]["name"] = "TestNPointGain";

      scanCfg["scan"]["loops"][0]["loopAction"] = "StdParameterLoop";
      scanCfg["scan"]["loops"][0]["config"]["min"] = 0;
      scanCfg["scan"]["loops"][0]["config"]["max"] = 4;
      scanCfg["scan"]["loops"][0]["config"]["step"] = 1;
      scanCfg["scan"]["loops"][0]["config"]["parameter"] = "BCAL";

      scan.loadConfig(scanCfg);
    }

    ClipBoard<HistogramBase> input;
    ClipBoard<HistogramBase> output;

    processor.connect(&scan, &input, &output, nullptr);

    processor.init();
    processor.run();

    for (unsigned i = 0; i < injections.size(); i++) {
        LoopStatus stat{{i}, {LOOP_STYLE_PARAMETER}};
        auto thresholdHist = std::make_unique<Histo2d>("ThresholdMap",
                                                nCol, -0.5, nCol-0.5,
                                                nRow, -0.5, nRow-0.5,
                                                stat);
        auto noiseHist = std::make_unique<Histo2d>("NoiseMap",
                                                nCol, -0.5, nCol-0.5,
                                                nRow, -0.5, nRow-0.5,
                                                stat);
        
        for (int c=0; c<nCol; c++) {
            for (int r=0; r<nRow; r++) {
                thresholdHist->fill(c, r, responses[i]);
                CHECK(thresholdHist->getBin(thresholdHist->binNum(c, r))-responses[i] < 0.01);
                noiseHist->fill(c, r, noises[i]);
                CHECK(noiseHist->getBin(noiseHist->binNum(c, r))-noises[i] < 0.01);
            }
        }

        input.pushData(std::move(thresholdHist));
        input.pushData(std::move(noiseHist));
    }

    input.finish();
    processor.join();

    REQUIRE(!output.empty());

    int histo_count = 0;
    while (!output.empty()) {
        std::unique_ptr<HistogramBase> result = output.popData();
        histo_count++;

        auto output_name = result->getName();
        CAPTURE (output_name);

        auto h3 = dynamic_cast<Histo3dT<float>*>(result.get());

        if(h3) {
          CHECK_THAT (h3->getXbinWidth(), Catch::Matchers::WithinRel(1.0, 1e-5));
          CHECK_THAT (h3->getYbinWidth(), Catch::Matchers::WithinRel(1.0, 1e-5));
          CHECK_THAT (h3->getZbinWidth(), Catch::Matchers::WithinRel(1.0, 1e-5));
        }

        if (output_name.find("ResponseFitParams") != std::string::npos) {
            auto h = dynamic_cast<Histo3dT<float>*>(result.get());
            REQUIRE(h);

            REQUIRE(h->size() == nCol*nRow*desiredFitParams.size());
            for (unsigned c = 0; c < 1; c++) {
                for (unsigned r = 0; r < 1; r++) {
                    for (unsigned i = 0; i < h->getZbins(); i++) {
                        CAPTURE(responses);
                        CAPTURE(c);
                        CAPTURE(r);
                        CAPTURE(i);
                        CAPTURE(h->binNum(c,r,i));
                        CAPTURE(h->getBin(h->binNum(c,r,i-1)));
                        CAPTURE(h->getBin(h->binNum(c,r,i)));
                        CAPTURE(h->getBin(h->binNum(c,r,i+1)));
                        CAPTURE(h->getBin(h->binNum(c,r,i+2)));
                        CAPTURE(desiredFitParams);
                        REQUIRE(std::abs(h->getBin(h->binNum(c,r,i))-desiredFitParams[i]) < tolerances[i]);
                    }
                }
            }
        } else if (output_name.find("InputNoise") != std::string::npos) {
            auto h = dynamic_cast<Histo3dT<float>*>(result.get());
            REQUIRE(h);

            REQUIRE(h->size() == nCol * nRow * injections.size());
        }
    }

}