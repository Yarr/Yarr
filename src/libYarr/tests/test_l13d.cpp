#include "catch.hpp"

#include "AllHistogrammers.h"
#include "EventData.h"
#include "Histo3d.h"

TEST_CASE("HistogramL13D", "[Histogrammer][L13D]") {
    // This is for one FE
    std::unique_ptr<HistoDataProcessor> histo(new HistogrammerProcessor);
    auto& histogrammer = static_cast<HistogrammerProcessor&>(*histo);

    ClipBoard<EventDataBase> input;
    ClipBoard<HistogramBase> output;

    histogrammer.connect(&input, &output);

    histogrammer.addHistogrammer(StdDict::getHistogrammer("L13d"));
    // histogrammer.addHistogrammer(new L13d());

    histogrammer.init();
    histogrammer.run();

    {
        auto data = std::make_unique<FrontEndData>();
        data->newEvent(1, 2, 3);
        data->curEvent->addHit(1, 2, 3);
        input.pushData(std::move(data));
    }

    input.finish();
    histogrammer.join();

    REQUIRE (!output.empty());

    std::unique_ptr<HistogramBase> result = output.popData();

    // Only one thing
    REQUIRE (output.empty());

    REQUIRE (result->getXaxisTitle() == "Column");
    REQUIRE (result->getYaxisTitle() == "Row");
    REQUIRE (result->getZaxisTitle() == "L1A");

    auto histo_as_3d = dynamic_cast<Histo3d *>(&*result);

    REQUIRE (histo_as_3d);

    REQUIRE (histo_as_3d->numOfEntries() == 1);
    REQUIRE (histo_as_3d->getMean() == 1);

#if 0 // Comparing the whole array is slow (at least in debug)
    auto lowX = histo_as_3d->getXlow();
    auto highX = histo_as_3d->getXhigh();
    auto deltaX = histo_as_3d->getXbinWidth();
    auto lowY = histo_as_3d->getYlow();
    auto highY = histo_as_3d->getYhigh();
    auto deltaY = histo_as_3d->getYbinWidth();
    auto lowZ = histo_as_3d->getZlow();
    auto highZ = histo_as_3d->getZhigh();
    auto deltaZ = histo_as_3d->getZbinWidth();

    for(auto x=lowX+deltaX/2; x<highX; x+=deltaX) {
        for(auto y=lowY+deltaY/2; y<highY; y+=deltaY) {
            for(auto z=lowZ+deltaZ/2; z<highZ; z+=deltaZ) {
                auto bin = histo_as_3d->binNum(x, y, z);
                CAPTURE (x, y, z, bin);

                float val = 0.0f;
                if((int)x == 2 && (int)y == 1 && (int)z == 2) {
                    val = 1.0f;
                }
                REQUIRE (histo_as_3d->getBin(bin) == val);
            }
        }
    }
#else
    int x = 2, y = 1, z = 2;
    auto bin = histo_as_3d->binNum(x, y, z);
    CAPTURE (x, y, z, bin);

    float val = 1.0f;
    REQUIRE (histo_as_3d->getBin(bin) == val);
#endif
}
