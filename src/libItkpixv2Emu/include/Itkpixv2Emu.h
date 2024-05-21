/*
* Author: Ondra Kovanda, ondrej.kovanda at cern.ch
* Date: 05/2024
* Description: Core functionality of the ITkPixV2 emulator
*/

#ifndef ITKPIXV2EMU_H
#define ITKPIXV2EMU_H

#include "ItkpixLayout.h"
#include "EmuCom.h"
#include "Itkpixv2Cfg.h"
#include "logging.h"
#include "Itkpixv2EmuCommandInterpreter.h"
#include "Itkpixv2EmuCommandExe.h"
#include <atomic>
#include <chrono>
#include <thread>
#include <future>
#include <memory>
#include <random>

class Itkpixv2Emu {
    public:
        
        Itkpixv2Emu(EmuCom* tx, EmuCom* rx, int seed = 0);
        ~Itkpixv2Emu();

        void executeLoop();

        void outputLoop();

        //The on/off switch visible to the controller
        std::atomic<bool> run;

    private:

        //Read the command from tx into the buffer
        void readCommand();

        //Initialize random (but fixed over time/instances) pixel thresholds
        //and noise levels
        void initPixels(int seed = 0);

        //internal pointers to the virtual tx and rx lanes,
        //aka the virtual chip's virtual pads to virtually connect
        //the virtual cables
        EmuCom* m_tx = 0;
        EmuCom* m_rx = 0;

        //Buffer for commands received from tx...
        //Legacy of Rd53aEmu, is this really needed?
        //We've got a ring buffer in tx...
        std::deque<uint16_t> m_commandStream;

        //Representation of all the chip registers. The uniqueness of this
        //pointer is a legacy of Rd53a emu, but probably doesn't hurt here
        std::unique_ptr<Itkpixv2Cfg> m_itkpixv2Cfg;

        //Utility class for preparation of the commands that arrive
        //through tx
        std::unique_ptr<Itkpixv2EmuCommandInterpreter> m_cmdInterpreter;

        //Utility class that takes care of executing the commands fetched
        //by command interpreter
        std::unique_ptr<Itkpixv2EmuCommandExe> m_cmdExe;

        //Pixel representations
        ItkpixLayout<float> m_thresholds;
        ItkpixLayout<uint16_t> m_tots;

        //Randomization
        std::mt19937 generator;

};

#endif