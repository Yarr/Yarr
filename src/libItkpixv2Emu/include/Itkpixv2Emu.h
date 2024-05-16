/*
* Author: Ondra Kovanda, ondrej.kovanda at cern.ch
* Date: 05/2024
* Description: Core functionality of the ITkPixV2 emulator
*/

#ifndef ITKPIXV2EMU_H
#define ITKPIXV2EMU_H

#include "ItkpixLayout.h"
#include "EmuCom.h"
#include "logging.h"
#include <atomic>
#include <chrono>
#include <thread>
#include <future>
#include <memory>

class Itkpixv2Emu {
    public:
        
        Itkpixv2Emu(EmuCom* tx, EmuCom* rx);
        ~Itkpixv2Emu();

        void executeLoop();

        void outputLoop();

        //The on/off switch visible to the controller
        std::atomic<bool> run;

    private:

        //Read the command from tx into the buffer
        void readCommand();

        //internal pointers to the virtual tx and rx lanes,
        //aka the virtual chip's virtual pads to virtually connect
        //the virtual cables
        EmuCom* m_tx;
        EmuCom* m_rx;

        //Buffer for commands received from tx
        std::deque<uint16_t> m_commandStream;

        //Pixel representations
        ItkpixLayout<uint16_t> m_ToT_map; //just a dummy

        //Utilities
        std::chrono::nanoseconds m_ns10 = std::chrono::nanoseconds(10);
};

#endif