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

}

void Fei4Analysis::process() {
    while(!input->empty()) {
        HistogramBase *h = input->popData();
        for (unsigned i=0; i<algorithms.size(); i++) {
            algorithms[i]->processHistogram(h);
        }
        delete h;
    }
}

void Fei4Analysis::addAlgorithm(AnalysisAlgorithm *a) {
    algorithms.push_back(a);
}

void Fei4Analysis::plot(std::string basename) {
    for (std::deque<HistogramBase*>::iterator it = output->begin(); it != output->end(); ++it) {
        std::cout << "Plotting : " << (*it)->getName() << std::endl;
        (*it)->plot(basename);
    }
}
    

void OccupancyAnalysis::init(ScanBase *s) {
    n_count = 1;
    for (unsigned n=0; n<s->size(); n++) {
        std::shared_ptr<LoopActionBase> l = s->getLoop(n);
        if (!(l->type() == typeid(Fei4TriggerLoop) ||
                    l->type() == typeid(Fei4MaskLoop) ||
                    l->type() == typeid(StdDataLoop)||
                    l->type() == typeid(Fei4DcLoop))) {
            loops.push_back(n);
            loopMax.push_back(l->getMax());
        } else {
            unsigned cnt = (l->getMax() - l->getMin())/l->getStep();
            n_count = n_count*cnt;
        }

    }

}

void OccupancyAnalysis::processHistogram(HistogramBase *h) {
    // Select correct output container
    unsigned ident = 0;
    unsigned offset = 0;
    
    std::string name = "OccupancyMap";
    for (unsigned n=0; n<loops.size(); n++) {
        ident += h->getStat().get(n)+offset;
        offset += loopMax[n];
        name += "_" + std::to_string(h->getStat().get(n));
    }

    if (occMaps[ident] == NULL) {
        Histo2d *h = new Histo2d(name, 80, 0.5, 80.5, 336, 0.5, 336.5);
        occMaps[ident] = h;
        innerCnt[ident] = 0;
    }

    occMaps[ident]->add(*(Histo2d*)h);
    innerCnt[ident]++;

    if (innerCnt[ident] == n_count)
        output->pushData(occMaps[ident]);

    
}
