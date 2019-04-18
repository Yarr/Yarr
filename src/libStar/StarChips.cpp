// #################################
// # Project: Yarr
// # Description: StarChips Library
// # Comment: StarChip FrontEnd class
// ################################

#include "StarChips.h"

#include <bitset>
#include <chrono>

#include "AllChips.h"

bool star_chips_registered =
StdDict::registerFrontEnd
  ("Star", []() { return std::unique_ptr<FrontEnd>(new StarChips); });

StarChips::StarChips()
  : StarCmd(), FrontEnd()
{
    txChannel = 99;
    rxChannel = 99;
    active = false;
    geo.nRow = 0;
    geo.nCol = 0;
}

StarChips::StarChips(TxCore *arg_core)
  : StarCmd(), FrontEnd()
{
    m_txcore  = arg_core;
    txChannel = 99;
    rxChannel = 99;
    active = true;
    geo.nRow = 0;
    geo.nCol = 0;
}

StarChips::StarChips(TxCore *arg_core, unsigned arg_channel)
  : StarCmd(), FrontEnd()
{
    m_txcore  = arg_core;
    txChannel = arg_channel;
    rxChannel = arg_channel;

    active = true;
    geo.nRow = 0;
    geo.nCol = 0;
}

StarChips::StarChips(TxCore *arg_core, unsigned arg_txChannel, unsigned arg_rxChannel)
  : StarCmd(), FrontEnd()
{
    m_txcore  = arg_core;
    txChannel = arg_txChannel;
    rxChannel = arg_rxChannel;

    active = true;
    geo.nRow = 0;
    geo.nCol = 0;
}

void StarChips::init(TxCore *arg_core, unsigned arg_txChannel, unsigned arg_rxChannel) {
    m_txcore  = arg_core;
    txChannel = arg_txChannel;
    rxChannel = arg_rxChannel;
    active = true;
}

void StarChips::configure() {
}

void StarChips::writeNamedRegister(std::string n, uint16_t val) {
  // look up in register map.
}

void StarChips::toFileJson(json &j){
    HccStarCfg::toFileJson(j);
}

void StarChips::fromFileJson(json &j){
    HccStarCfg::fromFileJson(j);
}

