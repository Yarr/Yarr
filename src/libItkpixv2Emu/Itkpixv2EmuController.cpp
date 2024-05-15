/*
* Author: Ondra Kovanda, ondrej.kovanda at cern.ch
* Date: 05/2024
* Description: Controller for ITkPixV2 emulator
*/

#include "EmuController.h"
#include "AllHwControllers.h"
#include "Itkpixv2Emu.h"
#include "logging.h"

namespace {
    auto logger = logging::make_log("rd5a_emu_controller");
}

//bool emu_registered_Rd53a = StdDict::registerHwController("emu_Itkpixv2Emu", makeEmu<Itkpixv2, Itkpixv2Emu>);

