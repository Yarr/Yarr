#include "catch.hpp"
#include "logging.h"

#include "AllHistogrammers.h"
#include "EventData.h"
#include "Histo1d.h"

namespace {
    auto alog = logging::make_log("TestRawDataHistogrammer");
}

void check_histogram_full(const Histo1d &h1, uint32_t width) {
    CAPTURE ( h1.size() );
    CAPTURE ( h1.getEntries() );

    CHECK ( h1.size() == h1.getEntries() );
    CHECK ( h1.size() == width );
    CHECK ( h1.getUnderflow() == Catch::Approx(0.0) );
    CHECK ( h1.getOverflow() == Catch::Approx(0.0) );
}

void log_event_data(const FrontEndData &d) {
  for(auto &e: d.events) {
    alog->trace("Event tag {} l1 {} bc {} hits {}",
                e.tag, e.l1id, e.bcid, e.nHits);
    for (auto hit : e.hits) {
      int col = hit.col;
      int row = hit.row;
      alog->trace("Event hits {:04x} {:04x} {:016x}",
                  col, row, *(uint64_t*)&hit);
    }
  }
}

struct TC {
  json cfg;
  uint32_t width;
  uint32_t offset;
};

TEST_CASE("HistogramRawData", "[Histogrammer][RawData]") {
    auto algo = StdDict::getHistogrammer("RawData");

    auto tc = GENERATE
      (TC{json::object(), 32, 0},
       TC{json{{"width", 4}}, 4, 0},
       TC{json{{"offset", 4}}, 32, 4},

       TC{json{{"width", 31}}, 31, 0},
       TC{json{{"width", 31}, {"offset", 1}}, 31, 1},
       TC{json{{"width", 32}, {"offset", 1}}, 32, 1},
       TC{json{{"width", 32}, {"offset", 32}}, 32, 32}
       );

    algo->loadConfig(tc.cfg);
    CAPTURE ( tc.offset, tc.width );

    SECTION ("CheckFull") {
      auto data = std::make_unique<FrontEndData>();
      data->newEvent(0, 0, 0); // Ignored

      // All 1s
      int input_len = (tc.offset + tc.width + 32) / 32;
      CAPTURE ( input_len );
      for(int i=0; i<input_len; i++) {
        data->curEvent->addHit(FrontEndHit{0xffff, 0xffff});
      }

      // Create output histogram
      algo->create(data->lStat);

      algo->processEvent(data.get());

      std::unique_ptr<HistogramBase> result = algo->getHisto();
      REQUIRE (result);

      CHECK (result->getXaxisTitle() == "Bits");
      CHECK (result->getYaxisTitle() == "Accumulator");

      auto histo_as_1d = dynamic_cast<Histo1d *>(&*result);

      REQUIRE ( histo_as_1d );
      check_histogram_full(*histo_as_1d, tc.width);
    }

    SECTION ("CheckOffset") {
      auto data = std::make_unique<FrontEndData>();
      auto offset = tc.offset;

      for(size_t o=0; o<offset + 4; o++) {
        data->newEvent(0, 0, 0); // Ignored

        size_t todo = o + 1;
        while (todo >= 32) {
          data->curEvent->addHit(FrontEndHit{0, 0});
          todo -= 32;
        }
        // Start the 1s at offset so we can check the first bin
        uint16_t first = 0xffff;
        if(todo > 16) {
          first = 0;
        } else {
          first >>= todo%32;
        }
        uint16_t second = 0xffff;
        if(todo > 16) {
          second >>= (todo%32)-16;
        }
        FrontEndHit f;
        f.row = first;
        f.col = second;
        data->curEvent->addHit(f);

        data->curEvent->addHit(FrontEndHit{0xffff, 0xffff});
      }

      log_event_data(*data);
      // Create output histogram
      algo->create(data->lStat);

      algo->processEvent(data.get());

      std::unique_ptr<HistogramBase> result = algo->getHisto();
      REQUIRE (result);

      CHECK (result->getXaxisTitle() == "Bits");
      CHECK (result->getYaxisTitle() == "Accumulator");

      auto histo_as_1d = dynamic_cast<Histo1d *>(&*result);

      REQUIRE ( histo_as_1d );

      CHECK ( histo_as_1d->getBin(0) == Catch::Approx(offset) );
      if ( histo_as_1d->size() > 1) {
        CHECK ( histo_as_1d->getBin(1) == Catch::Approx(offset+1) );
      }
    }
}
