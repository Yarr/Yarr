// #################################
// # Author: Jenna Chisholm
// # Email: jenna.lori.chisholm@cern.ch
// # Project: Yarr
// # Description: Star Histogrammer for RR
// ################################

#include "StarConstants.h"
#include "Histo1d.h"
#include "Histo2d.h"
#include "AllHistogrammers.h"
#include "StdHistogrammer.h"
#include "StarRRHistogrammer.h"
#include "logging.h"

namespace {
    auto alog = logging::make_log("StarRRHistogrammer");
}

namespace {
    bool star_hcc_rr_registered =
        StdDict::registerHistogrammer("StarHCCRRDist",
                                [](){return std::unique_ptr<HistogramAlgorithm>(new StarHCCRRDist());});

    bool star_abc_rr_registered =
        StdDict::registerHistogrammer("StarABCRRDist",
                                [](){return std::unique_ptr<HistogramAlgorithm>(new StarABCRRDist());});
}


void StarHCCRRDist::create(const LoopStatus &stat) {
    h = new Histo1d(outputName(), 41, -0.5, 41-0.5, stat);
    h->setXaxisTitle("RR");
    h->setYaxisTitle("Hits");
    r.reset(h);
}

void StarHCCRRDist::processEvent(FrontEndData *data) {

    for (const FrontEndEvent &curEvent: data->events) {
        if (curEvent.nHits > 0) {
            for (const FrontEndHit &curRR: curEvent.hits) {
                int packet_type =((curRR.tot >> 12) & 0x7);
                if (packet_type==5){ //Only want HCC RR data
                    uint8_t reg_addr = (curRR.tot >> 4) & 0xFF;
                    uint32_t reg_data = ((curRR.row & 0xffff) << 16) | (curRR.col & 0xffff);

                    for (int rr_bit=0; rr_bit<32; rr_bit++){ //Loop over the 32-bits of read register
                        bool bit_val = (reg_data>>rr_bit)&1;
                        if (bit_val){ //Only fill histogram if the bit is a 1
                            h->fill(rr_bit);
                        }
                    }
                    h->fill(32); //Fill 33rd column IF we got any data
                    for (int ra_bit=0; ra_bit<8; ra_bit++){ //Loop over the 8-bits of register address
                        bool bit_val = (reg_addr>>ra_bit)&1;
                        if (bit_val){ //Only fill histogram if the bit is a 1
                            h->fill(ra_bit+33);
                        }
                    }
                }
            }
        }
    }
}

void StarABCRRDist::create(const LoopStatus &stat) {

    int n_abcs = Star::MaxABCsPerHCC;

    h = new Histo2d(outputName(), 41, -0.5, 41-0.5, n_abcs, -0.5, n_abcs-0.5, stat);
    h->setYaxisTitle("ABC_channel");
    h->setZaxisTitle("Hits");
    r.reset(h);
}

void StarABCRRDist::processEvent(FrontEndData *data) {

    for (const FrontEndEvent &curEvent: data->events) {
        if (curEvent.nHits > 0) {
            for (const FrontEndHit &curRR: curEvent.hits) {
                int packet_type =((curRR.tot >> 12) & 0x7);
                if (packet_type==3){ //Only want ABC RR data
                    uint8_t reg_addr = (curRR.tot >> 4) & 0xFF;
                    uint32_t reg_data = ((curRR.row & 0xffff) << 16) | (curRR.col & 0xffff);
                    uint16_t abc_ch = (curRR.tot & 0xf);

                    for (int rr_bit=0; rr_bit<32; rr_bit++){ //Loop over the 32-bits of read register
                        bool bit_val = (reg_data>>rr_bit)&1;
                        if (bit_val){ //Only fill histogram if the bit is a 1
                            h->fill(rr_bit, abc_ch);
                        }
                    }
                    h->fill(32, abc_ch); //Fill 33rd column IF we got any data
                    for (int ra_bit=0; ra_bit<8; ra_bit++){ //Loop over the 8-bits of register address
                        bool bit_val = (reg_addr>>ra_bit)&1;
                        if (bit_val){ //Only fill histogram if the bit is a 1
                            h->fill(ra_bit+33, abc_ch);
                        }
                    }
                }
            }
        }
    }
}