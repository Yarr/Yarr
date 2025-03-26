#include "catch.hpp"

#include "AllAnalyses.h"
#include "AllChips.h"
#include "Bookkeeper.h"
#include "GraphErrors.h"
#include "Histo1d.h"
#include "JsonData.h"
#include "ScanFactory.h"
#include "StarChips.h"

#include "EmptyHw.h"

#include "logging.h"

namespace {
  auto logger = logging::make_log("test_analysis_strobe_delay");
}

/**
   Test of strobe delay analysis.

   Generate sequence of histograms (from OccupancyMap), and check expected
   output.
*/
TEST_CASE("StarStrobeDelayAnalysis", "[Analysis][Star][SD]") {

    // Need enough points that the fit has enough data for 4 params!
    std::vector<float> bin_points{0, 0, 0.2, 0.5, 0.8, 1, 1, 0.8, 0.5, 0.2, 0, 0};
    int n_injections = 50;

    json analysisCfg;
    // if (j.contains("dumpDebugSDPlots")) {
    //     m_dumpDebugSDPlots = j["dumpDebugSDPlots"];
    // }

    int chip_count = 1;

    SECTION ("Default") {
    }

    SECTION ("More chips") {
      chip_count = 2;
    }

    // SECTION ("POI") {
    //   analysisCfg["parametersOfInterest"][0] = "TEST_PARAM";
    // }

    // TODO: test more things? Add trigger loop to get trig count from

    unsigned bin_count = bin_points.size();

    CAPTURE (bin_count);
    CAPTURE (chip_count);

    EmptyHw empty;
    Bookkeeper bookie(&empty, &empty);

    int rx_channel = 0;

    // This is for one FE
    AnalysisProcessor analysis(rx_channel);

    int nCol = 128 * chip_count;
    int nRow = 2;

    {
      auto ana = StdDict::getAnalysis("StarStrobeDelayAnalysis");

      REQUIRE (ana);

      ana->loadConfig(analysisCfg);

      ana->setMapSize(nCol, nRow);

      analysis.addAlgorithm(std::move(ana));
    }

    // Need scan loops to lookup trigger loop
    ScanFactory scan(&bookie, nullptr);

    // Setup the loops that analysis needs to know about
    {
      json scanCfg;
      scanCfg["scan"]["name"] = "TestSDAnalysis";

      // Create Loop objects so they're available to analysis
      scanCfg["scan"]["loops"][0]["loopAction"] = "StdParameterLoop";
      scanCfg["scan"]["loops"][0]["config"]["min"] = 0;
      scanCfg["scan"]["loops"][0]["config"]["max"] = bin_count - 1;
      scanCfg["scan"]["loops"][0]["config"]["step"] = 1;
      scanCfg["scan"]["loops"][0]["config"]["parameter"] = "STR_DEL";

      scan.loadConfig(scanCfg);
    }

    ClipBoard<HistogramBase> input;
    ClipBoard<HistogramBase> output;

    analysis.connect(&scan, &input, &output, nullptr);

    analysis.init();
    analysis.run();

    for(unsigned i=0; i<bin_count; i++) {
        LoopStatus stat{{i}, {LOOP_STYLE_PARAMETER}};
        auto hist = std::make_unique<Histo2d>("OccupancyMap",
                                                nCol, 0.5, nCol + 0.5,
                                                nRow, 0.5, nRow + 0.5, stat);

        CHECK(hist->getYbins() == nRow);
        CHECK(hist->getXbins() == nCol);

        int val = n_injections * bin_points[i];

        for(int c=0; c<nCol; c++) {
          for(int r=0; r<nRow; r++) {
            hist->fill(c + 1, r + 1, val);
          }
        }

        input.pushData(std::move(hist));

        if (!output.empty()) {
            logger->debug("Exit histo loop as have output to check");
            break;
        }
    }

    input.finish();
    analysis.join();

    REQUIRE (!output.empty());

    int histo_count = 0;

    // Do some very basic checks on output
    while(!output.empty()) {
        std::unique_ptr<HistogramBase> result = output.popData();

        auto output_name = result->getName();

        CAPTURE (output_name);

        histo_count ++;

        if(output_name == "JsonData_StarStrobeDelayResult") {
            CHECK (result->getXaxisTitle() == "x");
            CHECK (result->getYaxisTitle() == "y");
            CHECK (result->getZaxisTitle() == "z");

            auto jd = dynamic_cast<JsonData*>(result.get());
            CHECK (jd != nullptr);

            json j_out;
            jd->toJson(j_out);
            REQUIRE (j_out.contains("Name"));
            REQUIRE (j_out.contains("Type"));
            std::string jn = j_out["Name"];
            std::string jt = j_out["Type"];
            CHECK (jn == output_name);
            CHECK (jt == output_name);

            REQUIRE (j_out.contains("ABCStar_0"));
            REQUIRE (j_out["ABCStar_0"].contains("OptimalStrobeDelay"));
            REQUIRE (j_out["ABCStar_0"]["OptimalStrobeDelay"].contains("Data"));
            // clang finds the overload resolution ambigous if compare
            // variant with Approx directly
            double chip_0_opt_sd = j_out["ABCStar_0"]["OptimalStrobeDelay"]["Data"][0];
            CHECK (chip_0_opt_sd == Catch::Approx(5.0));
            if(chip_count > 1) {
                REQUIRE (j_out.contains("ABCStar_1"));
                REQUIRE (j_out["ABCStar_1"].contains("OptimalStrobeDelay"));
                REQUIRE (j_out["ABCStar_1"]["OptimalStrobeDelay"].contains("Data"));
                double chip_1_opt_sd = j_out["ABCStar_1"]["OptimalStrobeDelay"]["Data"][0];
                CHECK (chip_1_opt_sd == Catch::Approx(5.0));
            }

            REQUIRE (j_out["ABCStar_0"].contains("LeftEdge"));
            REQUIRE (j_out["ABCStar_0"].contains("RightEdge"));

            auto left = j_out["ABCStar_0"]["LeftEdge"];
            auto right = j_out["ABCStar_0"]["RightEdge"];

            REQUIRE (left.contains("Row0"));
            REQUIRE (left["Row0"].contains("Data"));
            REQUIRE (right.contains("Row1"));
            REQUIRE (right["Row1"].contains("Data"));

            // For each channel
            CHECK (left["Row0"]["Data"][0] == 3.0);
            CHECK (left["Row0"]["Data"][10] == 3.0);
            CHECK (right["Row0"]["Data"][0] == 8.0);
            CHECK (right["Row0"]["Data"][10] == 8.0);

            CHECK (j_out["ABCStar_0"]["Width"]["Row0"]["Data"][0] == 5.0);
        } else if(output_name.find("OccVsStrobeDelayVsChan_Row") == 0) {
            CHECK (result->getXaxisTitle() == "Channel number");
            CHECK (result->getYaxisTitle() == "Strobe Delay");
            CHECK (result->getZaxisTitle() == "Occupancy");

            auto hh = dynamic_cast<Histo2d*>(result.get());
            REQUIRE (hh != nullptr);

            CHECK (hh->getXbinWidth() == 1.0);
            CHECK (hh->getYbinWidth() == 1.0);
            CHECK (hh->getXbins() == 128 * chip_count);
            CHECK (hh->getYbins() == bin_count);
        } else if(output_name.find("OccVsStrobeDelayVsChanChip") == 0) {
            if(output_name.find("_pfy") != std::string::npos) {
              CHECK (result->getXaxisTitle() == "Strobe Delay");
              CHECK (result->getYaxisTitle() == "y");
              CHECK (result->getZaxisTitle() == "z");

              auto hh = dynamic_cast<Histo1d*>(result.get());
              REQUIRE (hh != nullptr);

              // CHECK (hh->getXbinWidth() == 1.0);
              CHECK (hh->size() == bin_count);
            } else {
              CHECK (result->getXaxisTitle() == "Channel number");
              CHECK (result->getYaxisTitle() == "Strobe Delay");
              CHECK (result->getZaxisTitle() == "Occupancy");

              auto hh = dynamic_cast<Histo2d*>(result.get());
              REQUIRE (hh != nullptr);

              CHECK (hh->getXbinWidth() == 1.0);
              CHECK (hh->getYbinWidth() == 1.0);
              CHECK (hh->getXbins() == 128);
              CHECK (hh->getYbins() == bin_count);
            }
        } else if(output_name.find("StrobeDelay") == 0) {
            CHECK (result->getXaxisTitle() == "Strobe Delay");
            CHECK (result->getYaxisTitle() == "Occupancy");
            CHECK (result->getZaxisTitle() == "z");

            auto hh = dynamic_cast<Histo1d*>(result.get());
            REQUIRE (hh != nullptr);

            // CHECK (hh->binWidth() == 1.0);
            CHECK (hh->size() == bin_count);
        } else if(output_name.find("LeftEdgeDist") == 0) {
            CHECK (result->getXaxisTitle() == "Left edge");
            CHECK (result->getYaxisTitle() == "Number of channels");
            CHECK (result->getZaxisTitle() == "z");

            auto hh = dynamic_cast<Histo1d*>(result.get());
            REQUIRE (hh != nullptr);

            // CHECK (hh->getXbinWidth() == 1.0);
            CHECK (hh->size() == bin_count);
        } else if(output_name.find("RightEdgeDist") == 0) {
            CHECK (result->getXaxisTitle() == "Right edge");
            CHECK (result->getYaxisTitle() == "Number of channels");
            CHECK (result->getZaxisTitle() == "z");

            auto hh = dynamic_cast<Histo1d*>(result.get());
            REQUIRE (hh != nullptr);

            // CHECK (hh->getXbinWidth() == 1.0);
            CHECK (hh->size() == bin_count);
        } else {
            // Info about any other data
            CAPTURE (result->getXaxisTitle());
            CAPTURE (result->getYaxisTitle());
            CAPTURE (result->getZaxisTitle());

            logger->error("Extra histogram {}: {}", output_name,
                          [&]() -> std::string {
                json j; result->toJson(j);
                std::stringstream ss; ss << j; return ss.str(); }());

            CHECK (false);
        }
    }

    // OccVsStrobeDelayVsChanChip0Row1 is per chip (+ profile)
    // OccVsStrobeDelayVsChan_Row1 is per row
    CHECK (histo_count == 5 + chip_count * 4);
}

struct SdTestValueInfo {
    int ic;
    int sd_val;
};

void check_strobe_values(const FrontEnd &fe,
                         const std::vector<SdTestValueInfo> &info) {
    auto star_fe = dynamic_cast<const StarChips*> (&fe);
    REQUIRE(star_fe);

    for(const auto &fe_info: info) {
        // This is now looked up by input channel
        int fe_ic = fe_info.ic;
        CAPTURE (fe_info.ic, fe_info.sd_val);
        CHECK ( star_fe->getABCSubRegisterValue(fe_ic, ABCStarSubRegister::STR_DEL) == fe_info.sd_val );
    }
}

/**
   Test of feedback for strobe delay.

   Run scan loop with StarParamFeedback. Feed back the appropriate strobedelay
   parameters to the ABC chip configurations.
*/
TEST_CASE("StarStrobeDelayFeedback", "[Analysis][Star][SD]") {

    int chip_count = 1;
    int fe_count = 1;

    SECTION ("Default") {
    }

    SECTION ("More chips") {
      chip_count = 2;
    }

    // TODO: Add more configs with strange ABC mappings (barrel and endcap)

    CAPTURE (chip_count);
    CAPTURE (fe_count);

    EmptyHw empty;
    Bookkeeper bookie(&empty, &empty);

    int rx_channel = 0;
    FrontEndConnectivity fe_conn(0,rx_channel);

    {
      // HCCv1 so chips are in expected order
      auto fe = StdDict::getFrontEnd("Star_vH1A1");
      fe->setActive(true);

      fe->init(&empty, FrontEndConnectivity(0,0));

      auto star_fe = dynamic_cast<StarChips*> (&*fe);
      for(int c=0; c<chip_count; c++) {
        // This is the chip_id
        star_fe->addABCchipID(c);
      }

      // Normally set up by StarChips::configre
      star_fe->geo.nCol = 128 * chip_count;

      int ic_mask = 0;
      for(int i=0; i<chip_count; i++) {
        ic_mask <<= 1;
        ic_mask |= 1;
      }
      fe->writeNamedRegister("HCC_ICENABLE", ic_mask);

      bookie.addFe(std::move(fe), fe_conn);
    }
    unsigned feUid = bookie.getId(bookie.getLastFe());

    std::vector<SdTestValueInfo> sd_info;
    for(int i=0; i<chip_count; i++) {
      sd_info.push_back({i, 0});
    }

    // Get the entry from BK
    auto &fe = *bookie.getEntry(feUid).fe;

    // Test before making changes
    check_strobe_values(fe, sd_info);

    // This is what we want to change things to
    for(auto &sd_entry: sd_info) {
      sd_entry.sd_val = sd_entry.ic + 2;
    }

    // Run scan loops, which wait for the feedback
    FeedbackClipboardMap fb;
    ScanFactory scan(&bookie, &fb);

    // Setup the loops that analysis needs to know about
    {
      json scanCfg;
      scanCfg["scan"]["name"] = "TestSDAnalysis";

      // Create Loop objects so they're available to analysis
      scanCfg["scan"]["loops"][0]["loopAction"] = "StarParamFeedback";
      scanCfg["scan"]["loops"][0]["config"]["parameter"] = "STR_DEL";

      scan.loadConfig(scanCfg);
    }

    scan.init();

    // Send data back from analysis to store the correct configuration
    PixelFeedbackSender send(&fb[feUid]);

    uint32_t feedback_count = 0;

    const int max_loops = 10;

    // If exception is thrown this won't be cleared
    bool thread_failure = true;

    std::thread t([&]() {
      int loop_count = 0;
      while(feedback_count < 1) {
        std::shared_ptr<RawDataContainer> data = std::make_shared<RawDataContainer>(LoopStatus({2}, {LOOP_STYLE_PIXEL_FEEDBACK}));
        logger->debug("Generate feedback data");

        const LoopStatus &stat = data->stat;

        REQUIRE (stat.size() == 1);

        logger->trace("Current loop status: {} {} {}",
                      stat.get(0));

        auto feedbackData = std::make_unique<Histo2d>("feedback", chip_count, 0, chip_count, 1, 0, 1);
        for(int b=0; b<feedbackData->size(); b++) {
          feedbackData->fill(b, 0, b+2);
        }

        // As there's no inner loop, send feedback immediately
        send.feedback(feUid, std::move(feedbackData));
        feedback_count ++;

        logger->debug("Sent feedback at iteration {}", loop_count);
        loop_count ++;
      }

      logger->warn("Finish unlocking thread after {} loop", max_loops);
      thread_failure = false;
    });

    scan.run();

    fe.clipRawData.finish();

    t.join();

    // Only feedback at the end, but once per front end
    REQUIRE (feedback_count == fe_count);

    check_strobe_values(fe, sd_info);

    REQUIRE (!thread_failure);
}
