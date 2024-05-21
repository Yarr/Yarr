/*
* Author: Ondra Kovanda, ondrej.kovanda at cern.ch
* Date: 05/2024
* Description: Core functionality of the ITkPixV2 emulator
*/

#ifndef ITKPIXV2COMMANDEXE_H
#define ITKPIXV2COMMANDEXE_H

#include "Itkpixv2Cfg.h"
#include "Itkpixv2EmuUtils.h"
#include <map>

class Itkpixv2CommandExe {

    public:

        Itkpixv2CommandExe();

        void exe(const uint16_t command, const uint64_t payload = 0);

    private:

        static void doSync();

        static void doPLLlock();

        static void doClear();

        static void doGlobalPulse();

        static void doCal();

        static void doWrReg();

        static void doRdReg();

        //Mapping from command tags to the actual functions
        const std::map<uint8_t, void (*)()> commandMap {
            {Itkpixv2EmuUtils::Commands::Sync       , &doSync       },
            {Itkpixv2EmuUtils::Commands::PLLlock    , &doPLLlock    },
            {Itkpixv2EmuUtils::Commands::Clear      , &doClear      },
            {Itkpixv2EmuUtils::Commands::GlobalPulse, &doGlobalPulse},
            {Itkpixv2EmuUtils::Commands::Cal        , &doCal        },
            {Itkpixv2EmuUtils::Commands::WrReg      , &doWrReg      },
            {Itkpixv2EmuUtils::Commands::RdReg      , &doRdReg      }
        };


};

#endif