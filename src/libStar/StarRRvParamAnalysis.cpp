// #################################
// # Author: Jenna Chisholm
// # Email: jenna.lori.chisholm@cern.ch
// # Project: Yarr
// # Description: Star RR vs. Parameter Analysis
// ################################

#include <string>
#include <vector>
#include <tuple>
 
#include "StarConstants.h"
#include "AllAnalyses.h"
#include "StarRegDefs.h"

#include "StarRRvParamAnalysis.h"

#include "StarRRHistogrammer.h"
#include "logging.h"

namespace {
    auto alog = logging::make_log("StarRRvParamAnalysis");
}

namespace { 
    bool rrvreg_registered =
        StdDict::registerAnalysis("StarRRvParamAnalysis",
                []() { return std::unique_ptr<AnalysisAlgorithm>(new StarRRvParamAnalysis());});
}

void StarRRvParamAnalysis::init(const ScanLoopInfo *s) {

    int poi_n = 0;
    for (unsigned n = 0; n < s->size(); n++) {
        auto l = s->getLoop(n);
        if (isPOILoop(l)) {
            m_paramNames.push_back(m_parametersOfInterest[poi_n]);
            m_paramLoopIndices.push_back(n);
            m_paramMaxBits.push_back((int)log2(l->getMax())+1);
            m_paramMaxs.push_back(l->getMax());
            m_paramMins.push_back(l->getMin());
            m_paramSteps.push_back(l->getStep());
            m_paramNBins.push_back(((m_paramMaxs[poi_n]-m_paramMins[poi_n])/m_paramSteps[poi_n]) + 1);
            poi_n++;
            readOnly = false;
        }
    }

    // map resizing
    unsigned int max_size = 1;
    if (!readOnly){
        if (m_paramNames.size() == 1){ // just use the number of bins
            max_size = m_paramMaxs[0]+1;
        }
        else { // get the total number of bins for all permuations
            for(unsigned i=0; i < m_paramNames.size(); i++){
                max_size = max_size * (m_paramMaxs[i]+1);
            }
        }  
    } 
    HCC_map.resize(max_size);
    HCC_info_map.resize(max_size);
    ABC_maps.resize(Star::MaxABCsPerHCC);
    ABC_info_maps.resize(Star::MaxABCsPerHCC);
    for (unsigned abc_ch=0; abc_ch < Star::MaxABCsPerHCC; abc_ch++){
        ABC_maps[abc_ch].resize(max_size);
        ABC_info_maps[abc_ch].resize(max_size);
    }
}


void StarRRvParamAnalysis::processHistogram(HistogramBase *h) {

    // Check if right histogram
    std::string hname = h->getName();
    if (h->getName().find(StarHCCRRDist::outputName()) != 0 && h->getName().find(StarABCRRDist::outputName()) != 0){
        return;
    }

    // Divide histogram processing based on chip (ABC or HCC)
    if (hname=="StarHCCRRDist"){
        processHCCHistograms(h);
        filledHCCMap = true;
    }
    else if (hname=="StarABCRRDist"){
        processABCHistograms(h);
        filledABCMaps = true;
    }
    else{
        alog->warn("Only StarHCCRRDist or StarABCRRDist histograms can be used in RR vs. Parameter analysis. Please use one of these in scan config.");
    }
}


void StarRRvParamAnalysis::processHCCHistograms(HistogramBase *h) {

    // Copy in the histogram info
    auto histo = dynamic_cast<Histo1d*>(h);
    if (!histo){
        alog->error("StarHCCRRDist is not Histo1d");
        return;
    }
    
    // Get the current param value (if there is one)
    int param_val = 0;
    if (!readOnly){
        if (m_paramNames.size() == 1){ // for creating regular, single parameter plots, just get the param value
            param_val = histo->getStat().get(m_paramLoopIndices[0]);
        }
        else { // for nested loops, create a combined param value that ranges over all permutations
            for(unsigned i=0; i < m_paramNames.size(); i++){
                int cur_param_val = histo->getStat().get(m_paramLoopIndices[i]);
                int param_max_bit_size = m_paramMaxBits[i];
                param_val =  (param_val << param_max_bit_size) | cur_param_val;            
            }
        }
    }

    // Fill maps
    for (int rr_bit = 0; rr_bit < 32; rr_bit++) { // loop over 32 bits of register read
        int binNum = histo->binNum(rr_bit);
        int binData = histo->getBin(binNum);
        HCC_map[param_val].push_back(binData);
    }
    for (int ra_bit = 0; ra_bit < 8; ra_bit++) { // loop over 8 bits of register address
        int binNum = histo->binNum(ra_bit+33);
        int binData = histo->getBin(binNum);
        HCC_info_map[param_val].push_back(binData);
    }
    HCC_info_map[param_val].push_back(histo->getBin(histo->binNum(32))); // also add "packet read" flag
}

void StarRRvParamAnalysis::processABCHistograms(HistogramBase *h){
    
    // Copy in the histogram info
    auto histo = dynamic_cast<Histo2d*>(h);
    if (!histo){
        alog->error("StarABCRRDist is not Histo2d");
        return;
    }

    // Get the current param value (if there is one)
    int param_val = 0;
    if (!readOnly){
        if (m_paramNames.size() == 1){ // for creating regular, single parameter plots, just get the param value
            param_val = histo->getStat().get(m_paramLoopIndices[0]);
        }
        else { // for nested loops, create a combined param value that ranges over all permutations
            for(unsigned i=0; i < m_paramNames.size(); i++){
                int cur_param_val = histo->getStat().get(m_paramLoopIndices[i]);
                int param_max_bit_size = m_paramMaxBits[i];
                param_val =  (param_val << param_max_bit_size) | cur_param_val;            
            }
        }
    }

    // Fill maps
    for (unsigned abc_ch = 0; abc_ch < Star::MaxABCsPerHCC; abc_ch++){
        for (int rr_bit = 0; rr_bit < 32; rr_bit++) { // loop over 32 bits of register read
            int binNum = histo->binNum(rr_bit, abc_ch);
            int binData = histo->getBin(binNum);
            ABC_maps[abc_ch][param_val].push_back(binData);
        }
        for (int ra_bit = 0; ra_bit < 8; ra_bit++) { // loop over 8 bits of register address
            int binNum = histo->binNum(ra_bit+33, abc_ch);
            int binData = histo->getBin(binNum);
            ABC_info_maps[abc_ch][param_val].push_back(binData);
        }
        ABC_info_maps[abc_ch][param_val].push_back(histo->getBin(histo->binNum(32, abc_ch))); // also add "packet read" flag
    }
}


void StarRRvParamAnalysis::loadConfig(const json &j) {

    if (j.contains("parametersOfInterest")) {
        for (unsigned i = 0; i < j["parametersOfInterest"].size(); i++) {
            m_parametersOfInterest.push_back(j["parametersOfInterest"][i]);
        }
    }

    if (j.contains("registerOfInterest")) {
        m_registerOfInterest = j["registerOfInterest"];
    }
    else {
        m_registerOfInterest = "RR";
    }

    if (j.contains("yAxisTitle")){
        m_yAxisTitle = j["yAxisTitle"];
    }

}


void StarRRvParamAnalysis::end() {

    // Fix parameter name so it looks nice on histogram
    std::string fixed_y_title = "";
    if (!readOnly){
        if(m_yAxisTitle.empty()){ // if nothing specified, append all param names into one string
            std::string m_yAxisTitle = m_paramNames[0];
            for (unsigned i=1; i < m_paramNames.size(); i++){
                m_yAxisTitle += " + "+m_paramNames[i];
            }   
            fixed_y_title = m_yAxisTitle;
        }
        else{
            fixed_y_title = m_yAxisTitle;
        }
        for(std::string::size_type i = 0; i < fixed_y_title.size(); ++i) { // find underscores and fix them
            if (fixed_y_title[i]=='_'){
                fixed_y_title.insert(i, "\\");
                i+=2;
            }
        }
    }

    // HCCs
    if (filledHCCMap){

        auto [paramMin,paramMax,paramNBin] = getHistogramSettings(HCC_map);

        // Create final 2D histogram
        std::unique_ptr<Histo2d> hh ( new Histo2d("HCCRRvParamMap", 32, 0-0.5, 32-0.5, paramNBin, paramMin-0.5, paramMax+0.5) );
        hh->setXaxisTitle("HCC "+m_registerOfInterest);
        hh->setYaxisTitle(fixed_y_title);
        hh->setZaxisTitle("Hits");

        // Also histogram for register address (for sanity checks)
        std::unique_ptr<Histo2d> hhinfo ( new Histo2d("HCCInfovParamMap", 9, 0-0.5, 9-0.5, paramNBin, paramMin-0.5, paramMax+0.5) );
        hhinfo->setXaxisTitle("HCC Register Address ("+m_registerOfInterest+") + Packet Read Flag");
        hhinfo->setYaxisTitle(fixed_y_title);
        hhinfo->setZaxisTitle("Hits");

        // Fill histograms from map
        for(int param_val=paramMin; param_val <= paramMax; param_val++){
            if (HCC_map[param_val].size()!=0){
                for(int bit=0; bit < 32; bit++){ // RR histogram
                    int hits = HCC_map[param_val][bit];
                    int binNum = hh->binNum(bit,param_val);
                    hh->setBin(binNum, hits);
                    if (readOnly){ // need to fill the bottom half of histogram, since we can't have only one y bin with hist2d :(
                        int binNum2 = hh->binNum(bit,param_val-0.5);
                        hh->setBin(binNum2, hits);
                    }
                }
                for(int bit=0; bit < 9; bit++){ // reg addr histogram
                    int hits = HCC_info_map[param_val][bit];
                    int binNum = hhinfo->binNum(bit,param_val);
                    hhinfo->setBin(binNum, hits);
                    if (readOnly){ // need to fill the bottom half of histogram, since we can't have only one y bin with hist2d :(
                        int binNum2 = hhinfo->binNum(bit,param_val-0.5);
                        hhinfo->setBin(binNum2, hits);
                    }
                }
            }
        }

        output->pushData(std::move(hh));
        output->pushData(std::move(hhinfo));
    }


    // ABCs
    if (filledABCMaps){

        std::tuple<int, int, int> param_settings;
        for (unsigned abc_ch=0; abc_ch < ABC_maps.size(); abc_ch++){
            if(ABC_maps[abc_ch].size()!=0){
                param_settings = getHistogramSettings(ABC_maps[abc_ch]);
                break;
            }
        }
        auto [paramMin,paramMax,paramNBin] = param_settings;
        
        for(unsigned abc_ch=0; abc_ch < ABC_maps.size(); abc_ch++){
        
            // Create final 2D histogram
            std::unique_ptr<Histo2d> hh ( new Histo2d("ABCRRvParamMap\\_"+std::to_string(abc_ch), 32, 0-0.5, 32-0.5, paramNBin, paramMin-0.5, paramMax+0.5) );
            hh->setXaxisTitle("ABC "+m_registerOfInterest);
            hh->setYaxisTitle(fixed_y_title);
            hh->setZaxisTitle("Hits");

            // Also histogram for register address (for sanity checks)
            std::unique_ptr<Histo2d> hhinfo ( new Histo2d("ABCInfovParamMap\\_"+std::to_string(abc_ch), 9, 0-0.5, 9-0.5, paramNBin, paramMin-0.5, paramMax+0.5) );
            hhinfo->setXaxisTitle("ABC Register Address ("+m_registerOfInterest+") + Packet Read Flag");
            hhinfo->setYaxisTitle(fixed_y_title);
            hhinfo->setZaxisTitle("Hits");

            // Fill histograms from maps
            for(int param_val=paramMin; param_val <= paramMax; param_val++){
                if (ABC_maps[abc_ch][param_val].size()!=0){
                    for(int bit=0; bit < 32; bit++){
                        int hits = ABC_maps[abc_ch][param_val][bit];
                        int binNum = hh->binNum(bit,param_val);
                        hh->setBin(binNum, hits);
                        if (readOnly){ // need to fill the bottom half of histogram, since we can't have only one y bin with hist2d :(
                            int binNum2 = hh->binNum(bit,param_val-0.5);
                            hh->setBin(binNum2, hits);
                        }
                    }
                    for(int bit=0; bit < 33; bit++){
                        int hits = ABC_info_maps[abc_ch][param_val][bit];
                        int binNum = hhinfo->binNum(bit,param_val);
                        hhinfo->setBin(binNum, hits);
                        if (readOnly){ // need to fill the bottom half of histogram, since we can't have only one y bin with hist2d :(
                            int binNum2 = hhinfo->binNum(bit,param_val-0.5);
                            hhinfo->setBin(binNum2, hits);
                        }
                    }
                }
            }

            output->pushData(std::move(hh));
            output->pushData(std::move(hhinfo));

        }
    }
}


std::tuple<int, int, int> StarRRvParamAnalysis::getHistogramSettings(std::vector<std::vector<int> > map){

    int paramMin;
    int paramMax;
    int paramNBin;
    if (readOnly){
        paramMin = 0;
        paramMax = 0;
        paramNBin = 2; // hist2d requires at least two bins
    }
    else{
        if (m_paramNames.size()==1){
            paramMin = m_paramMins[0];
            paramMax = m_paramMaxs[0];
            paramNBin = m_paramNBins[0];
        }
        else{
            paramNBin = 0;
            bool min_is_set = false;
            for (unsigned i=0; i < map.size(); i++){
                if (map[i].size()!=0){
                    paramNBin++;
                    paramMax = i;
                    if (!min_is_set){
                        paramMin = i;
                        min_is_set = true;
                    }
                }
            }
        }
    }

    return std::make_tuple(paramMin,paramMax,paramNBin);
}