#include "catch.hpp"
#include "logging.h"

#include <bitset>

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

TEST_CASE("HistogramRawDataBounds", "[Histogrammer][RawData]") {
    auto algo = StdDict::getHistogrammer("RawData");

    struct TCB {
      json cfg;
      uint32_t width;
      // Input data, multiples of 32
      uint32_t len;
      // Expected count
      uint32_t count;
    };

    auto tc = GENERATE
      (
       // Check with histogram around same length as input
       TCB{json{{"width", 4}}, 4, 1, 4},
       TCB{json{{"width", 31}}, 31, 1, 31},
       TCB{json{{"width", 32}}, 32, 1, 32},
       TCB{json{{"width", 33}}, 33, 1, 32},
       TCB{json{{"width", 40}}, 40, 1, 32},

       TCB{json{{"offset", 0}}, 32, 1, 32},
       TCB{json{{"offset", 4}}, 32, 1, 28},
       TCB{json{{"offset", 31}}, 32, 1, 1},
       TCB{json{{"offset", 32}}, 32, 1, 0},
       TCB{json{{"offset", 33}}, 32, 1, 0},
       TCB{json{{"offset", 0}}, 32, 2, 32},
       TCB{json{{"offset", 4}}, 32, 2, 32},
       TCB{json{{"offset", 31}}, 32, 2, 32},
       TCB{json{{"offset", 32}}, 32, 2, 32},
       TCB{json{{"offset", 33}}, 32, 2, 31},

       TCB{json{{"width", 31}, {"offset", 1}}, 31, 1, 31},
       TCB{json{{"width", 32}, {"offset", 1}}, 32, 1, 31},
       TCB{json{{"width", 32}, {"offset", 32}}, 32, 2, 32},

       // Histogram much wider than data
       TCB{json{{"width", 100}}, 100, 1, 32},
       TCB{json{{"width", 100}, {"offset", 1}}, 100, 1, 31},
       TCB{json{{"width", 100}, {"offset", 31}}, 100, 1, 1},

       // Fixed param with different input size
       TCB{json{{"width", 10}, {"offset", 30}}, 10, 0, 0},
       TCB{json{{"width", 10}, {"offset", 30}}, 10, 1, 2},
       TCB{json{{"width", 10}, {"offset", 30}}, 10, 2, 10},
       TCB{json{{"width", 10}, {"offset", 100}}, 10, 1, 0}
       );

    algo->loadConfig(tc.cfg);
    CAPTURE ( tc.width );

    auto data = std::make_unique<FrontEndData>();
    data->newEvent(0, 0, 0); // Ignored

    // All 1s
    int input_len = tc.len;
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

    CAPTURE ( histo_as_1d->size() );
    CAPTURE ( histo_as_1d->getEntries() );

    std::bitset<64> bins;
    
    for(unsigned b=0; b<histo_as_1d->size(); b++) {
      if(histo_as_1d->getBin(b)>0.5) {
        // Reverse order we print
        bins.set(63-b);
      }
    }

    CAPTURE ( bins );

    CHECK ( histo_as_1d->getEntries() == tc.count );
    CHECK ( histo_as_1d->size() == tc.width );
    CHECK ( histo_as_1d->getUnderflow() == Catch::Approx(0.0) );
    CHECK ( histo_as_1d->getOverflow() == Catch::Approx(0.0) );
}
