#include <cmath>

#include "catch.hpp"

#include "AllAnalyses.h"
#include "Bookkeeper.h"
#include "EmptyHw.h"
#include "EmptyFrontEnd.h"
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

    std::function<double(double, const double*)> gainFunction;
    auto linearGain = [](double x, const double* par){return par[1];};
    auto polynomialGain = [](double x, const double*par){return par[1] + 2*par[2]*x;};
    auto exponentialGain = [](double x, const double*par){return (par[0] * exp(-1 * x / par[1])) / (par[1] * pow(1 + exp(-1 * x / par[1]), 2));};

    std::vector<double> desiredFitParams, tolerances;
    SECTION("Linear") {
        analysisConfig["fitFunction"] = "linear";
        fitFunction = linear;
        gainFunction = linearGain;
        desiredFitParams = {1, 3};
        tolerances = {0.01, 0.01};
    }
    SECTION("Polynomial") {
        analysisConfig["fitFunction"] = "polynomial";
        fitFunction = polynomial;
        gainFunction = polynomialGain;
        desiredFitParams = {1, 3, 0.25};
        tolerances = {0.01, 0.01, 0.01};
    }
    SECTION("Exponential") {
        analysisConfig["fitFunction"] = "exponential";
        fitFunction = exponential;
        gainFunction = exponentialGain;
        desiredFitParams = {20, 1.8, -9};
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
        LoopStatus stat{{injections[i]}, {LOOP_STYLE_PARAMETER}};
        auto thresholdHist = std::make_unique<Histo2d>("ThresholdMap",
                                                nCol, 0.5, nCol+0.5,
                                                nRow, 0.5, nRow+0.5,
                                                stat);
        auto noiseHist = std::make_unique<Histo2d>("NoiseMap",
                                                nCol, 0.5, nCol+0.5,
                                                nRow, 0.5, nRow+0.5,
                                                stat);
        
        for (int c=0; c<nCol; c++) {
            for (int r=0; r<nRow; r++) {
                thresholdHist->fill(c+1, r+1, responses[i]);
                CHECK(thresholdHist->getBin(thresholdHist->binNum(c, r))-responses[i] < 0.01);
                noiseHist->fill(c+1, r+1, noises[i]);
                CHECK(noiseHist->getBin(noiseHist->binNum(c+1, r+1))-noises[i] < 0.01);
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
        CAPTURE(analysisConfig["fitFunction"]);
        CAPTURE(injections);
        CAPTURE(responses);
        CAPTURE(noises);
        CAPTURE(desiredFitParams);

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
            for (unsigned c = 0; c < nCol; c++) {
                CAPTURE(c);
                for (unsigned r = 0; r < nRow; r++) {
                    CAPTURE(r);
                    for (unsigned i = 0; i < h->getZbins(); i++) {
                        CAPTURE(i);
                        CAPTURE(h->binNum(c,r,i));
                        CAPTURE(h->getBin(h->binNum(c,r,i)));
                        auto p1 = h->getBin(h->binNum(c,r,1));
                        auto p2 = h->getBin(h->binNum(c,r,2));
                        CAPTURE(p1);
                        CAPTURE(p2);
                        REQUIRE_THAT(h->getBin(h->binNum(c,r,i)), Catch::Matchers::WithinRel(desiredFitParams[i], 1e-1) || Catch::Matchers::WithinAbs(0, 1e-5));
                    }
                }
            }
        } else if (output_name.find("OutputNoise") != std::string::npos) {
            auto h = dynamic_cast<Histo3dT<float>*>(result.get());
            REQUIRE(h);

            REQUIRE(h->size() == nCol * nRow * injections.size());
            for (unsigned c = 0; c < nCol; c++) {
                CAPTURE(c);
                for (unsigned r = 0; r < nRow; r++) {
                    CAPTURE(r);
                    for (unsigned i = 0; i < h->getZbins(); i++) {
                        CAPTURE(i);
                        CAPTURE(h->binNum(c,r,i));

                        // output noise should be the same as the histogram we input
                        REQUIRE_THAT(h->getBin(h->binNum(c,r, i)), Catch::Matchers::WithinAbs(noises[i], 1e-5));
                    }
                }
            }
        } else if (output_name.find("InputNoise") != std::string::npos) {
            auto h = dynamic_cast<Histo3dT<float>*>(result.get());
            REQUIRE(h);

            REQUIRE(h->size() == nCol * nRow * injections.size());
            for (unsigned c = 0; c < 1; c++) {
                CAPTURE(c);
                for (unsigned r = 0; r < 1; r++) {
                    CAPTURE(r);
                    for (unsigned i = 0; i < h->getZbins(); i++) {
                        CAPTURE(i);
                        CAPTURE(h->binNum(c,r,i));
                        CAPTURE(h->getBin(h->binNum(c,r,i)));
                        CAPTURE(gainFunction(injections[i], desiredFitParams.data()));

                        // input noise = output noise / gain (assuming no extra conversion by default)
                        double expected = noises[i] / gainFunction(injections[i], desiredFitParams.data());
                        CAPTURE(expected);
                        REQUIRE_THAT(h->getBin(h->binNum(c,r,i)), Catch::Matchers::WithinRel(expected, 1e-1) || Catch::Matchers::WithinAbs(0, 1e-5));
                    }
                }
            }
        }
    }

}