/*
* Author: Ondra Kovanda, ondrej.kovanda at cern.ch
* Date: 05/2024
* Description: Core functionality of the ITkPixV2 emulator
*/

#ifndef ITKPIXV2EMUCOMMANDEXE_H
#define ITKPIXV2EMUCOMMANDEXE_H

#include "Itkpixv2Cfg.h"
#include "Itkpixv2EmuUtils.h"
#include "EmuCom.h"
#include "Itkpixv2Encoder.h"
#include <memory>
#include <map>
#include <set>


class Itkpixv2EmuCommandExe {

    public:

        Itkpixv2EmuCommandExe(EmuCom* rx, std::shared_ptr<Itkpixv2Cfg>& cfg);

        void exe(const Itkpixv2EmuUtils::Cmd cmd);

    private:

        void doSync(const Itkpixv2EmuUtils::Cmd& cmd);

        void doPLLlock(const Itkpixv2EmuUtils::Cmd& cmd);

        void doClear(const Itkpixv2EmuUtils::Cmd& cmd);

        void doGlobalPulse(const Itkpixv2EmuUtils::Cmd& cmd);

        void doCal(const Itkpixv2EmuUtils::Cmd& cmd);

        void doWrReg(const Itkpixv2EmuUtils::Cmd& cmd);

        void doRdReg(const Itkpixv2EmuUtils::Cmd& cmd);

        //Mapping from command tags to the actual functions
        std::map<uint8_t, void (Itkpixv2EmuCommandExe::*)(const Itkpixv2EmuUtils::Cmd& cmd)> commandMap =  {
            {Itkpixv2EmuUtils::Commands::Sync       , &Itkpixv2EmuCommandExe::doSync       },
            {Itkpixv2EmuUtils::Commands::PLLlock    , &Itkpixv2EmuCommandExe::doPLLlock    },
            {Itkpixv2EmuUtils::Commands::Clear      , &Itkpixv2EmuCommandExe::doClear      },
            {Itkpixv2EmuUtils::Commands::GlobalPulse, &Itkpixv2EmuCommandExe::doGlobalPulse},
            {Itkpixv2EmuUtils::Commands::Cal        , &Itkpixv2EmuCommandExe::doCal        },
            {Itkpixv2EmuUtils::Commands::WrReg      , &Itkpixv2EmuCommandExe::doWrReg      },
            {Itkpixv2EmuUtils::Commands::RdReg      , &Itkpixv2EmuCommandExe::doRdReg      }
        };


        //Output pipeline pointer
        EmuCom* m_rx;

        //Register map pointer
        std::shared_ptr<Itkpixv2Cfg> m_cfg;

        //Bookkeeping enabled pixel coordinates. The (col, row) coordinates
        //are flattened in the same manner as the ItkpixLayout indexing: col * 384 + row
        std::set<uint32_t> m_activePixels;

        //encoder
        std::shared_ptr<Itkpixv2Encoder> m_encoder;

};

#endif