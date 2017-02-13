#include "fei4reghelper.h"

unsigned int Fei4RegHelper::getMOffset() const {
    return mOffset;
}

unsigned int Fei4RegHelper::getBOffset() const {
    return bOffset;
}

unsigned int Fei4RegHelper::getMask() const {
    return mask;
}

bool Fei4RegHelper::getMsbRight() const {
    return msbRight;
}

void Fei4RegHelper::printReg() const {
    std::cout << std::showbase << std::hex;

    std::cout << std::setw(25) << mOffset << '\t'
              << std::setw(25) << bOffset << '\t'
              << std::setw(25) << mask << '\t'
              << std::setw(5) << msbRight << '\n';

    std::cout << std::dec << std::noshowbase;
}

void Fei4RegHelper::writeReg(Fei4 * fe, uint16_t v) {
    fe->wrGR16(mOffset, bOffset, mask, msbRight, v);

    return;
}

bool Fei4RegHelper::operator==(Fei4RegHelper const& other) const {
    if(other.getMOffset() != mOffset) return false;
    if(other.getBOffset() != bOffset) return false;
    if(other.getMask() != mask) return false;
    if(other.getMsbRight() != msbRight) return false;
    return true;
}

//###################################################################################

void Fei4PLHelper::init() {
    m_done = false;
    cur = min;
    g_fe->wrGR16(mOffset, bOffset, mask, msbRight, cur);

    return;
}

void Fei4PLHelper::execPart1() {
    g_stat->set(this, cur);

    return;
}

void Fei4PLHelper::execPart2() {
    cur += step;
    if(cur > max) {
        m_done = true;
        return;
    }

    g_fe->wrGR16(mOffset, bOffset, mask, msbRight, cur);
    while(!g_tx->isCmdEmpty()) {
        ;
    }

    return;
}

void Fei4PLHelper::end() {

    return;
}

//###################################################################################

void Fei4GFHelper::init() {
    m_done = false;
    cur = 0;
    // Init all maps:
//    for(unsigned int k=0; k<keeper->feList.size(); k++) {
//        if(keeper->feList[k]->getActive()) {
//            unsigned ch = keeper->feList[k]->getRxChannel();
//            localStep[ch] = step;
//            values[ch] = max;
//            oldSign[ch] = -1;
//            doneMap[ch] = false;
//        }
//    }
    for(unsigned int k=0; k < keeper->activeFeList.size(); k++){
        unsigned ch = dynamic_cast<Fei4*>(keeper->activeFeList[k])->getRxChannel();
        localStep[ch] = step;
        values[ch] = max;
        oldSign[ch] = -1;
        doneMap[ch] = false;
    }
    this->writePar();

    return;
}

void Fei4GFHelper::execPart1() {
    g_stat->set(this, cur);
    // Lock all mutexes if open
    for(unsigned int k=0; k<keeper->activeFeList.size(); k++) {
        keeper->mutexMap[dynamic_cast<Fei4*>(keeper->activeFeList[k])->getRxChannel()].try_lock();
    }
    m_done = allDone();

    return;
}

void Fei4GFHelper::execPart2() {
    // Wait for mutexes to be unlocked by feedback
    for(unsigned int k=0; k<keeper->activeFeList.size(); k++) {
        keeper->mutexMap[dynamic_cast<Fei4*>(keeper->activeFeList[k])->getRxChannel()].lock();
        if (verbose)
            std::cout << " --> Received Feedback on Channel "
                      << dynamic_cast<Fei4*>(keeper->activeFeList[k])->getRxChannel() << " with value: "
                      << values[dynamic_cast<Fei4*>(keeper->activeFeList[k])->getRxChannel()] << std::endl;
    }
    cur++;
    this->writePar();

    return;
}

void Fei4GFHelper::end() {
    for(unsigned int k=0; k<keeper->activeFeList.size(); k++) {
        unsigned ch = dynamic_cast<Fei4*>(keeper->activeFeList[k])->getRxChannel();
        std::cout << " --> Final parameter of Fe " << ch << " is " << values[ch] << std::endl;
    }

    return;
}

void Fei4GFHelper::writePar() {
    for(unsigned int k=0; k<keeper->activeFeList.size(); k++) {
        g_tx->setCmdEnable(1 << dynamic_cast<Fei4*>(keeper->activeFeList[k])->getTxChannel());
        dynamic_cast<Fei4*>(keeper->activeFeList[k])->wrGR16(mOffset,
                                                             bOffset,
                                                             mask,
                                                             msbRight,
                                                             values[dynamic_cast<Fei4*>(keeper->activeFeList[k])->getRxChannel()]);
        while(!g_tx->isCmdEmpty());
    }
    g_tx->setCmdEnable(keeper->getTxMask());

    return;
}

bool Fei4GFHelper::allDone() {
    for(unsigned int k=0; k<keeper->activeFeList.size(); k++) {
        unsigned ch = dynamic_cast<Fei4*>(keeper->activeFeList[k])->getRxChannel();
        if(!doneMap[ch]){
            return false;
        }
    }
    return true;
}
