#include "catch.hpp"

#include "EventData.h"
#include "AllHistogrammers.h"
#include "Histo1d.h"

TEST_CASE("HistogramL1Dist", "[Histogrammer][L1Dist]") {
    // This is for one FE
    auto algo = StdDict::getHistogrammer("L1Dist");
    REQUIRE (algo);

    auto data = std::make_unique<FrontEndData>();
    data->newEvent(1, 2, 3);
    data->curEvent->addHit(1, 2, 3);

    // Create output histogram
    algo->create(data->lStat);

    algo->processEvent(data.get());

    std::unique_ptr<HistogramBase> result = algo->getHisto();
    REQUIRE (result);

    // Only one thing
    REQUIRE (!algo->getHisto());

    REQUIRE (result->getXaxisTitle() == "L1A");
    REQUIRE (result->getYaxisTitle() == "Hits");
    // REQUIRE (result->getZaxisTitle() == "Total ToT");

    auto histo_as_1d = dynamic_cast<Histo1d *>(&*result);

    REQUIRE (histo_as_1d);

    REQUIRE (histo_as_1d->getEntries() == 1);
    REQUIRE (histo_as_1d->getMean() == 3);

    double epsilon = 1e-9;
    REQUIRE (histo_as_1d->getOverflow() < epsilon);
    REQUIRE (histo_as_1d->getUnderflow() < epsilon);

    const unsigned int max = 16; // Fei4?
    REQUIRE (histo_as_1d->size() == max);

    auto lowX = -0.5;
    auto highX = max-0.5;
    auto deltaX = 1.0;

    for(auto x=lowX+deltaX/2; x<highX; x+=deltaX) {
        auto bin = (int)x;
        CAPTURE (x, bin);

        float val = 0.0f;
        if((int)x == 3) {
          // Must be something somewhere!
          val = 1.0f;
        }
        REQUIRE (histo_as_1d->getBin(bin) == val);
    }
}
