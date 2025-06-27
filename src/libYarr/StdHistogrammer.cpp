// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Histograms event data
// ################################

#include "StdHistogrammer.h"

#include "AllHistogrammers.h"
#include "Histo1d.h"
#include "Histo2d.h"
#include "Histo3d.h"

#include "logging.h"

namespace {
    auto alog = logging::make_log("StdHistogrammer");
}

namespace {
    bool da_registered =
      StdDict::registerHistogrammer("DataArchiver",
                                []() { return std::unique_ptr<HistogramAlgorithm>(new DataArchiver());});

    bool om_registered =
      StdDict::registerHistogrammer("OccupancyMap",
                                []() { return std::unique_ptr<HistogramAlgorithm>(new OccupancyMap());});

    bool tot_registered =
      StdDict::registerHistogrammer("TotMap",
                                []() { return std::unique_ptr<HistogramAlgorithm>(new TotMap());});

    bool tot2_registered =
      StdDict::registerHistogrammer("Tot2Map",
                                []() { return std::unique_ptr<HistogramAlgorithm>(new Tot2Map());});

    bool tot_dist_registered =
      StdDict::registerHistogrammer("TotDist",
                                []() { return std::unique_ptr<HistogramAlgorithm>(new TotDist());});

    bool tot3d_registered =
      StdDict::registerHistogrammer("Tot3d",
                                []() { return std::unique_ptr<HistogramAlgorithm>(new Tot3d());});

    bool l1dist_registered =
      StdDict::registerHistogrammer("L1Dist",
                                []() { return std::unique_ptr<HistogramAlgorithm>(new L1Dist());});
    
    bool tag_registered =
      StdDict::registerHistogrammer("TagMap",
                                []() { return std::unique_ptr<HistogramAlgorithm>(new TagMap());});

    bool tagdist_registered =
      StdDict::registerHistogrammer("TagDist",
                                []() { return std::unique_ptr<HistogramAlgorithm>(new TagDist());});

    bool tag_occupancy_registered =
      StdDict::registerHistogrammer("TagOccupancyMap",
                                []() { return std::unique_ptr<HistogramAlgorithm>(new TagOccupancyMap());});

    bool l13d_registered =
      StdDict::registerHistogrammer("L13d",
                                []() { return std::unique_ptr<HistogramAlgorithm>(new L13d());});

    bool hpe_registered =
      StdDict::registerHistogrammer("HitsPerEvent",
                                []() { return std::unique_ptr<HistogramAlgorithm>(new HitsPerEvent());});

    bool raw_registered =
      StdDict::registerHistogrammer("RawData",
                                []() { return std::unique_ptr<HistogramAlgorithm>(new RawDataHistogram());});
}

bool DataArchiver::open(std::string filename) {
    fileHandle.open(filename.c_str(), std::fstream::out | std::fstream::binary | std::fstream::trunc);
    return fileHandle.good();
}

void DataArchiver::processEvent(FrontEndData *data) {
    if(fileHandle.is_open()) {
        for (const FrontEndEvent &curEvent: data->events) {
            curEvent.toFileBinary(fileHandle);
        }
    }
}

void OccupancyMap::create(const LoopStatus &stat) {
    h = new Histo2d(outputName(), nCol, 0.5, nCol+0.5, nRow, 0.5, nRow+0.5, stat);
    h->setXaxisTitle("Column");
    h->setYaxisTitle("Row");
    h->setZaxisTitle("Hits");
    r.reset(h);
}

void OccupancyMap::processEvent(FrontEndData *data) {
    for (const FrontEndEvent &curEvent: data->events) {
        if (curEvent.nHits > 0) {
            for (const FrontEndHit &curHit: curEvent.hits) {
                if(curHit.tot > 0)
                    h->fill(curHit.col, curHit.row);
            }
        }
    }
}

void TotMap::create(const LoopStatus &stat) {
    h = new Histo2d(outputName(), nCol, 0.5, nCol+0.5, nRow, 0.5, nRow+0.5, stat);
    h->setXaxisTitle("Column");
    h->setYaxisTitle("Row");
    h->setZaxisTitle("Total ToT");
    r.reset(h);
}

void TotMap::processEvent(FrontEndData *data) {
    for (const FrontEndEvent &curEvent: data->events) {
        if (curEvent.nHits > 0) {
            for (const FrontEndHit &curHit: curEvent.hits) {   
                if(curHit.tot > 0)
                    h->fill(curHit.col, curHit.row, curHit.tot & 0x7ff);
            }
        }
    }
}

void Tot2Map::create(const LoopStatus &stat) {
    h = new Histo2d(outputName(), nCol, 0.5, nCol+0.5, nRow, 0.5, nRow+0.5, stat);
    h->setXaxisTitle("Column");
    h->setYaxisTitle("Row");
    h->setZaxisTitle("Total ToT2");
    r.reset(h);
}

void Tot2Map::processEvent(FrontEndData *data) {
    for (const FrontEndEvent &curEvent: data->events) {
        if (curEvent.nHits > 0) {
            for (const FrontEndHit &curHit: curEvent.hits) {   
                if(curHit.tot > 0)
                    h->fill(curHit.col, curHit.row, (curHit.tot & 0x7ff) * (curHit.tot & 0x7ff));
            }
        }
    }
}

void TotDist::create(const LoopStatus &stat) {
    h = new Histo1d(outputName(), 16, 0.5, 16.5, stat);
    h->setXaxisTitle("ToT [bc]");
    h->setYaxisTitle("# of Hits");
    r.reset(h);
}

void TotDist::processEvent(FrontEndData *data) {
    for (const FrontEndEvent &curEvent: data->events) {
        if (curEvent.nHits > 0) {
            for (const FrontEndHit &curHit: curEvent.hits) {   
                if(curHit.tot > 0)
                    h->fill(curHit.tot & 0x7ff);
            }
        }
    }
}

void Tot3d::create(const LoopStatus &stat) {
    h = new Histo3d("Tot3d", nCol, 0.5, nCol+0.5, nRow, 0.5, nRow+0.5, 16, 0.5, 16.5, stat);
    h->setXaxisTitle("Column");
    h->setYaxisTitle("Row");
    h->setZaxisTitle("ToT");
    r.reset(h);
}

void Tot3d::processEvent(FrontEndData *data) {
    for (const FrontEndEvent &curEvent: data->events) {
        if (curEvent.nHits > 0) {
            for (const FrontEndHit &curHit: curEvent.hits) {   
                if(curHit.tot > 0)
                    h->fill(curHit.col, curHit.row, curHit.tot & 0x7ff);
            }
        }
    }
}

void TagDist::create(const LoopStatus &stat) {
    h = new Histo1d(outputName(), 257, -0.5, 256.5, stat);
    h->setXaxisTitle("Tag");
    h->setYaxisTitle("Hits");
    r.reset(h);
}

void TagDist::processEvent(FrontEndData *data) {
    // Event Loop
    for (const FrontEndEvent &curEvent: data->events) {
        h->fill(curEvent.tag, curEvent.nHits);
    }
}

void TagMap::create(const LoopStatus &stat) {
    h = new Histo2d(outputName(), nCol, 0.5, nCol+0.5, nRow, 0.5, nRow+0.5, stat);
    h->setXaxisTitle("Column");
    h->setYaxisTitle("Row");
    h->setZaxisTitle("Tag");
    r.reset(h);
}

void TagMap::processEvent(FrontEndData *data) {
    for (const FrontEndEvent &curEvent: data->events) {
        if (curEvent.nHits > 0) {
            for (const FrontEndHit &curHit: curEvent.hits) {   
                if(curHit.tot > 0)
                    h->fill(curHit.col, curHit.row, curEvent.tag);
            }
        }
    }
}

void TagOccupancyMap::loadConfig(const json &cfg)
{
  try {
    cfg.at("tag_count").get_to(tag_count);
  } catch(json::out_of_range &) {
    // Leave at default
  }

  if(tag_count > 256) {
    // Hard-coded by 8-bit mask in processEvent
    alog->warn("TagOccupancyMap: Fixing limit of tag_count to 256");
    tag_count = 256;
  }
}

void TagOccupancyMap::create(const LoopStatus &stat) {
    h = new Histo3d(outputName(),
                    nCol, 0.5, nCol+0.5,
                    nRow, 0.5, nRow+0.5,
                    tag_count, -0.5, tag_count - 0.5,
                    stat);
    h->setXaxisTitle("Column");
    h->setYaxisTitle("Row");
    h->setZaxisTitle("Tag");
    r.reset(h);
}

void TagOccupancyMap::processEvent(FrontEndData *data) {
    for (const FrontEndEvent &curEvent: data->events) {
        if (curEvent.nHits > 0) {
            for (const FrontEndHit &curHit: curEvent.hits) {
                if(curHit.tot > 0)
                    h->fill(curHit.col, curHit.row, curEvent.tag & 0xff);
            }
        }
    }
}

void L1Dist::create(const LoopStatus &stat) {
    h = new Histo1d(outputName(), 16, -0.5, 15.5, stat);
    h->setXaxisTitle("L1A");
    h->setYaxisTitle("Hits");
    r.reset(h);
    l1id = 33;
    bcid_offset = 0;
}

void L1Dist::processEvent(FrontEndData *data) {
    // Event Loop
    for (const FrontEndEvent &curEvent: data->events) {
        if(curEvent.l1id != l1id) {
            l1id = curEvent.l1id;
            if (curEvent.bcid - bcid_offset > 16) {
                bcid_offset = curEvent.bcid;
                //current_tag++;
            } else if ((curEvent.bcid+32768) - bcid_offset > 16 &&
                       static_cast<int>(curEvent.bcid) - static_cast<int>(bcid_offset) < 0) {
                bcid_offset = curEvent.bcid;
                //current_tag++;
            }
        }

        int delta_bcid = curEvent.bcid - bcid_offset;
        if (delta_bcid < 0)
            delta_bcid += 32768;
        h->fill(delta_bcid, curEvent.nHits);

        //TODO hack to generate proper tag, should come from FE/FW
        //curEvent.tag = current_tag;

    }
}

void L13d::create(const LoopStatus &stat) {
    h = new Histo3d(outputName(), nCol, 0.5, nCol+0.5, nRow, 0.5, nRow+0.5, 16, -0.5, 15.5, stat);
    h->setXaxisTitle("Column");
    h->setYaxisTitle("Row");
    h->setZaxisTitle("L1A");
    r.reset(h);
    l1id = 33;
    bcid_offset = 0;
}

void L13d::processEvent(FrontEndData *data) {
    for (const FrontEndEvent &curEvent: data->events) {
        
        /*if(curEvent.l1id != l1id) {
            l1id = curEvent.l1id;
            if (curEvent.bcid - bcid_offset > 16) {
                bcid_offset = curEvent.bcid;
            } else if ((curEvent.bcid+32768) - bcid_offset > 16 &&
                       static_cast<int>(curEvent.bcid) - static_cast<int>(bcid_offset) < 0) {
                bcid_offset = curEvent.bcid;
            }
        }

        int delta_bcid = curEvent.bcid - bcid_offset;
        if (delta_bcid < 0)
            delta_bcid += 32768;
        */
        
        if (curEvent.nHits > 0) {
            for (const FrontEndHit &curHit: curEvent.hits) {   
                if(curHit.tot > 0)
                    h->fill(curHit.col, curHit.row, curEvent.l1id%16);
            }
        }
    }
}

void HitsPerEvent::create(const LoopStatus &stat) {
    h = new Histo1d(outputName(), 1000, -0.5, 999.5, stat);
    h->setXaxisTitle("Number of Hits");
    h->setYaxisTitle("Events");
    r.reset(h);
}

void HitsPerEvent::processEvent(FrontEndData *data) {
    // Event Loop
    for (const FrontEndEvent &curEvent: data->events) {
        h->fill(curEvent.nHits);
    }
}

void RawDataHistogram::loadConfig(const json &config)
{
    try {
        config.at("width").get_to(width);
    } catch(json::out_of_range &) {}
    try {
        config.at("offset").get_to(offset);
    } catch(json::out_of_range &) {}
}

void RawDataHistogram::create(const LoopStatus &stat) {
    h = new Histo1d(outputName(), width, -0.5, width - 0.5, stat);
    h->setXaxisTitle("Bits");
    h->setYaxisTitle("Accumulator");
    r.reset(h);
}

void RawDataHistogram::processEvent(FrontEndData *data) {
    size_t word_start = offset / 32;
    size_t word_end = (offset+width+31) / 32;
    size_t bit_first = offset % 32;
    size_t bit_last = ((offset+width-1) % 32) + 1;

    for (const FrontEndEvent &curEvent: data->events) {
        auto h_size = curEvent.hits.size();
        if(h_size < word_start) {
          continue;
        }

        size_t w_start = word_start;
        size_t w_end = word_end;
        size_t b_first = bit_first;
        size_t b_last = bit_last;

        if(h_size <= word_end) {
          w_end = h_size;
          if(h_size < word_end) {
            b_last = 32;
          } else if(b_last != 32) {
            b_last = std::min(((offset+31) % 32) + 1, b_last);
          }
        }

        for(size_t word_index = w_start; word_index < w_end; word_index ++) {
            const FrontEndHit &curHit = curEvent.hits[word_index];
            uint32_t word = curHit.row;
            word = (word << 16) | curHit.col;

            size_t bit_start = word_index == w_start
              ? b_first : 0;
            size_t bit_end = word_index == (w_end-1)
              ? b_last : 32;

            for(uint32_t bit=bit_start; bit<bit_end; bit++) {
                if(word & (1<<(31-bit))) {
                    h->fill((word_index * 32 + bit) - offset);
                }
            }
        }
    }
}
