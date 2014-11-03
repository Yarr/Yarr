// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Analysis Base class
// # Comment: 
// ################################

#include "Fei4Analysis.h"

Fei4Analysis::Fei4Analysis() {

}

Fei4Analysis::~Fei4Analysis() {

}

void Fei4Analysis::init() {
    for (unsigned i=0; i<algorithms.size(); i++) {
        algorithms[i]->connect(output);
        algorithms[i]->init(scan);
    }
}

void Fei4Analysis::process() {
    while(!input->empty()) {
        HistogramBase *h = input->popData();
        if (h != NULL) {
            for (unsigned i=0; i<algorithms.size(); i++) {
                algorithms[i]->processHistogram(h);
            }
            delete h;
        }
    }
}

void Fei4Analysis::addAlgorithm(AnalysisAlgorithm *a) {
    algorithms.push_back(a);
}

void Fei4Analysis::plot(std::string basename) {
    if (output->empty())
        return;
    for (std::deque<HistogramBase*>::iterator it = output->begin(); it != output->end(); ++it) {
        std::cout << "Plotting : " << (*it)->getName() << std::endl;
        (*it)->plot(basename);
    }
}
    

void OccupancyAnalysis::init(ScanBase *s) {
    n_count = 1;
    for (unsigned n=0; n<s->size(); n++) {
        std::shared_ptr<LoopActionBase> l = s->getLoop(n);
        if ((l->type() != typeid(Fei4TriggerLoop*) &&
                    l->type() != typeid(Fei4MaskLoop*) &&
                    l->type() != typeid(StdDataLoop*) &&
                    l->type() != typeid(Fei4DcLoop*))) {
            loops.push_back(n);
            loopMax.push_back((unsigned)l->getMax());
        } else {
            unsigned cnt = (l->getMax() - l->getMin())/l->getStep();
            if (cnt == 0)
                cnt = 1;
            n_count = n_count*cnt;
        }
    }
}

void OccupancyAnalysis::processHistogram(HistogramBase *h) {
    // Select correct output container
    unsigned ident = 0;
    unsigned offset = 0;
    
    // Determine identifier
    std::string name = "OccupancyMap";
    for (unsigned n=0; n<loops.size(); n++) {
        ident += h->getStat().get(loops[n])+offset;
        offset += loopMax[n];
        name += "-" + std::to_string(h->getStat().get(loops[n]));
    }
    
    // Check if Histogram exists
    if (occMaps[ident] == NULL) {
        Histo2d *h = new Histo2d(name, 80, 0.5, 80.5, 336, 0.5, 336.5);
        h->setXaxisTitle("Column");
        h->setYaxisTitle("Row");
        h->setZaxisTitle("Hits");
        occMaps[ident] = h;
        innerCnt[ident] = 0;
    }

    // Add up Histograms
    occMaps[ident]->add(*(Histo2d*)h);
    innerCnt[ident]++;

    // Got all data, finish up Analysis
    if (innerCnt[ident] == n_count) {
        std::cout << "Collected all inner loops for " << occMaps[ident]->getName() << "(" << ident << ")" << std::endl;
        output->pushData(occMaps[ident]);
    }

    
}
