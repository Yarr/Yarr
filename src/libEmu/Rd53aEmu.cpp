#include "Rd53aEmu.h"
#include "Gauss.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Static members and functions

// Instantiation is needed for constexpr
constexpr std::array<uint8_t, Rd53aEmu::sizeOf8bit> Rd53aEmu::eightToFive;


// Instantiation of static const members with initializer-list
const std::map<enum Rd53aEmu::Commands, Rd53aEmu::CommandFunc> Rd53aEmu::commandFuncs {
    { Rd53aEmu::Commands::WrReg       , &Rd53aEmu::doWrReg       },
    { Rd53aEmu::Commands::RdReg       , &Rd53aEmu::doRdReg       },
    { Rd53aEmu::Commands::Cal         , &Rd53aEmu::doCal         },
    { Rd53aEmu::Commands::ECR         , &Rd53aEmu::doECR         },
    { Rd53aEmu::Commands::BCR         , &Rd53aEmu::doBCR         },
    { Rd53aEmu::Commands::Zero        , &Rd53aEmu::doZero        },
    { Rd53aEmu::Commands::GlobalPulse , &Rd53aEmu::doGlobalPulse },
    { Rd53aEmu::Commands::Noop        , &Rd53aEmu::doNoop        },
    { Rd53aEmu::Commands::Sync        , &Rd53aEmu::doSync        },
    { Rd53aEmu::Commands::Dump        , &Rd53aEmu::doDump        }
};


// For the moment, all functions are identical -- later, different implemenation should be implemented
const std::map<enum Rd53aEmu::Triggers, Rd53aEmu::CommandFunc> Rd53aEmu::triggerFuncs {
    { Rd53aEmu::Triggers::Trg01, &Rd53aEmu::doTrigger },
    { Rd53aEmu::Triggers::Trg02, &Rd53aEmu::doTrigger },
    { Rd53aEmu::Triggers::Trg03, &Rd53aEmu::doTrigger },
    { Rd53aEmu::Triggers::Trg04, &Rd53aEmu::doTrigger },
    { Rd53aEmu::Triggers::Trg05, &Rd53aEmu::doTrigger },
    { Rd53aEmu::Triggers::Trg06, &Rd53aEmu::doTrigger },
    { Rd53aEmu::Triggers::Trg07, &Rd53aEmu::doTrigger },
    { Rd53aEmu::Triggers::Trg08, &Rd53aEmu::doTrigger },
    { Rd53aEmu::Triggers::Trg09, &Rd53aEmu::doTrigger },
    { Rd53aEmu::Triggers::Trg10, &Rd53aEmu::doTrigger },
    { Rd53aEmu::Triggers::Trg11, &Rd53aEmu::doTrigger },
    { Rd53aEmu::Triggers::Trg12, &Rd53aEmu::doTrigger },
    { Rd53aEmu::Triggers::Trg13, &Rd53aEmu::doTrigger },
    { Rd53aEmu::Triggers::Trg14, &Rd53aEmu::doTrigger },
    { Rd53aEmu::Triggers::Trg15, &Rd53aEmu::doTrigger }
};
    
        
uint8_t Rd53aEmu::to5bit( uint8_t in ) {
    assert( eightToFive[in] != 0xff );
    return eightToFive[in];
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Non-static members

Rd53aEmu::Rd53aEmu(RingBuffer * rx, RingBuffer * tx)
    : m_txRingBuffer (tx )
    , m_rxRingBuffer ( rx )
    , m_feCfg ( new Rd53aCfg )
{
    
    srand(time(NULL));
    
    run = true;
    
    for (int col = 0; col < 136; col++) {
        
        m_rd53aLinPixelModelObjects .emplace_back( std::vector< std::unique_ptr<Rd53aLinPixelModel > > {} );
        m_rd53aDiffPixelModelObjects.emplace_back( std::vector< std::unique_ptr<Rd53aDiffPixelModel> > {} );
        
        for (int row = 0; row < Rd53aPixelCfg::n_Col; row++) {
            std::string linName = "linScurveCol" + std::to_string(col) + "Row" + std::to_string(row);
            linScurve[col][row] = new Histo1d(linName.c_str(), 256, -0.5, 4095.5, typeid(void));
            m_rd53aLinPixelModelObjects[col].emplace_back( new Rd53aLinPixelModel(10, 2, 400, 100) );

            std::string diffName = "diffScurveCol" + std::to_string(col) + "Row" + std::to_string(row);
            diffScurve[col][row] = new Histo1d(diffName.c_str(), 256, -0.5, 4095.5, typeid(void));
            m_rd53aDiffPixelModelObjects[col].emplace_back( new Rd53aDiffPixelModel(10, 0, 10, 10) );
        }
    }
    
    linThreshold = new Histo1d("linThreshold", 256, -0.5, 4095.5, typeid(void));
    diffThreshold = new Histo1d("diffThreshold", 256, -0.5, 4095.5, typeid(void));

    analogHits = new Histo2d("analogHits", Rd53aPixelCfg::n_Col, -0.5, 399.5, Rd53aPixelCfg::n_Row, -0.5, 191.5, typeid(void));


}

Rd53aEmu::~Rd53aEmu() {}


void Rd53aEmu::executeLoop() {
    std::cout << "Starting emulator loop" << std::endl;

    while (run) {
        
        if ( m_txRingBuffer->isEmpty()) continue;
        
        
        if( verbose ) std::cout << __PRETTY_FUNCTION__ << ": -----------------------------------------------------------" << std::endl;

            
        // read the command header
        if( stream.size() == 0 ) {
            retrieve();
        }
            

        if( verbose && stream.size() ) {
            std::cout << __PRETTY_FUNCTION__ << ": L" << __LINE__ << ": front = " << HEXF(4, stream.front() ) << ", size = " << stream.size() << std::endl;
        }

        
        ///////////////////////////////////////////////////////////////////
        // 
        // For the moment, all trigger commands are degenerate for simplicity
        // Later, the pattern of the trigger (L1a) and the timing needs to be
        // properly implemented.
        //

        const auto triggerKey = static_cast<Triggers>( stream.front()>>8 );
        auto triggerFunc_itr = triggerFuncs.find( triggerKey );
        
        if( triggerFunc_itr != triggerFuncs.end() ) {
            
            // The following grammer is for static member function pointer ( passing this )
            ( triggerFunc_itr->second )( this );
            
            continue;
        }
        
            
        ///////////////////////////////////////////////////////////////////
        // 
        // All the rest commands are grouped here
        //

        const auto commandKey = static_cast<Commands>( stream.front() );
        auto commandFunc_itr = commandFuncs.find( commandKey );
        
        if( commandFunc_itr != commandFuncs.end() ) {
            
            // The following grammer is for static member function pointer ( passing this )
            ( commandFunc_itr->second )( this );
            
            continue;
        }
        
        
        ///////////////////////////////////////////////////////////////////
        // 
        // Exception
        //
        
        printf("unrecognized header 0x%x, skipping for now (will eventually elegantly crash)\n", stream.front() );
        stream.pop_front();
        exit(1);
        
    }
    
}



void Rd53aEmu::retrieve() {
    uint32_t d = m_txRingBuffer->read32();
    //std::cout << "push_back(): adding word = " << HEXF(8, d ) << std::endl;
    stream.push_back( (d & 0xFFFF0000) >> 16 );
    stream.push_back( (d & 0x0000FFFF) );
}



void Rd53aEmu::pushOutput(uint32_t value) {
    if (m_rxRingBuffer) {
        m_rxRingBuffer->write32(value);
    }
}



/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Again, static members and functions

void Rd53aEmu::doECR( Rd53aEmu* emu ) {
    
    for( auto& async : emu->m_async ) { async.get(); }
    emu->m_async.clear();
    
    if( emu->verbose ) std::cout << __PRETTY_FUNCTION__ << ": L" << __LINE__ << ": ECR = " << HEXF(4, emu->stream.front() ) << std::endl;
    emu->stream.pop_front();
}


void Rd53aEmu::doBCR( Rd53aEmu* emu ) {
    
    for( auto& async : emu->m_async ) { async.get(); }
    emu->m_async.clear();
    
    if( emu->verbose ) std::cout << __PRETTY_FUNCTION__ << ": L" << __LINE__ << ": BCR = " << HEXF(4, emu->stream.front() ) << std::endl;
    emu->stream.pop_front();
}


void Rd53aEmu::doNoop( Rd53aEmu* emu ) {
    if( emu->verbose ) std::cout << __PRETTY_FUNCTION__ << ": L" << __LINE__ << ": Noop = " << HEXF(4, emu->stream.front() ) << std::endl;
    emu->stream.pop_front();
}


void Rd53aEmu::doZero( Rd53aEmu* emu ) {
    
    for( auto& async : emu->m_async ) { async.get(); }
    emu->m_async.clear();
    
    if( emu->verbose ) std::cout << __PRETTY_FUNCTION__ << ": L" << __LINE__ << ": Zero = " << HEXF(4, emu->stream.front() ) << std::endl;
    emu->stream.pop_front();
}


void Rd53aEmu::doSync( Rd53aEmu* emu ) {
    
    for( auto& async : emu->m_async ) { async.get(); }
    emu->m_async.clear();
    
    if( emu->verbose ) std::cout << __PRETTY_FUNCTION__ << ": L" << __LINE__ << ": Sync = " << HEXF(4, emu->stream.front() ) << std::endl;
    emu->stream.pop_front();
}


void Rd53aEmu::doGlobalPulse( Rd53aEmu* emu ) {
    
    for( auto& async : emu->m_async ) { async.get(); }
    emu->m_async.clear();
    
    if( emu->verbose ) std::cout << __PRETTY_FUNCTION__ << ": L" << __LINE__ << ": GlobalPulse = " << HEXF(4, emu->stream.front() ) << std::endl;
    emu->stream.pop_front();

#if 0
    auto word  = emu->stream.front();
    auto byte1 = ( word & 0xFF00 ) >> 8;
    auto byte2 = ( word & 0x00FF );
    auto id    = to5bit(byte1) >> 1;
    auto data  = to5bit(byte2);

    std::cout << "id = " << HEXF(4, id) << ", data = " << HEXF(5, data) << std::endl;
#endif
    emu->stream.pop_front();
}



void Rd53aEmu::doCal( Rd53aEmu* emu ) {
    
    for( auto& async : emu->m_async ) { async.get(); }
    emu->m_async.clear();
    
    if( emu->verbose ) std::cout << __PRETTY_FUNCTION__ << ": L" << __LINE__ << ": Cal = " << HEXF(4, emu->stream.front() ) << std::endl;
    emu->stream.pop_front();

    // ToDo
    // For the moment, only pops 2x16-bit words
    // Informations stored there need to be used properly
    // See RD53a Manual section 9.2, p.47
    
    emu->retrieve();
    emu->stream.pop_front();
    emu->stream.pop_front();
}
    


void Rd53aEmu::doTrigger( Rd53aEmu* emu ) {

    // Finish all async processes before triggering
    for( auto& async : emu->m_async ) { async.get(); }
    emu->m_async.clear();
    
    
    if( emu->verbose ) {
        std::cout << __PRETTY_FUNCTION__ << ": L" << __LINE__ << ": got Trigger command " << HEXF(8, emu->stream.front() ) << std::endl;
        std::cout << __PRETTY_FUNCTION__ << ": L" << __LINE__ << ": BCID = " << HEXF(2, to5bit( emu->stream.front() & 0xff ) ) << std::endl;
    }
    
    emu->stream.pop_front();

    emu->totalDigitalHits = 0;
    emu->diffAnalogHits   = 0;
    emu->linAnalogHits    = 0;
    emu->syncAnalogHits   = 0;

    // Streeam is already popped,
    // then the following part can be run in parallel.
    for (unsigned dc = 0; dc < Rd53aPixelCfg::n_DC; dc++) {
        
        emu->m_async.emplace_back( std::async( std::launch::deferred, &Rd53aEmu::triggerAsync, emu, dc ) );
        
    }

    // Finish all async processes before next step
    for( auto& async : emu->m_async ) { async.get(); }
    emu->m_async.clear();
    
    // for now, print the total number of hits - eventually, we should really just be writing hit data back to YARR
    //printf("Hits: total = %d [diif = %d, lin = %d]\n", totalDigitalHits, diffAnalogHits, linAnalogHits);
    //              printf("syncAnalogHits = %d\n", syncAnalogHits);
    //              printf("m_feCfg->VcalHigh.read() = %d\n", m_feCfg->VcalHigh.read());
}



void Rd53aEmu::triggerAsync( Rd53aEmu* emu, const unsigned dc) {
    
    // put these checks into a function maybe
    // check pixels to see if the digital enable is set for "octo-columns" (columns of cores)
    if (             dc < 64  && !((emu->m_feCfg->EnCoreColSync.read()  >> ((dc - 0)   / 4)) & 0x1)) return;
    if (64  <= dc && dc < 128 && !((emu->m_feCfg->EnCoreColLin1.read()  >> ((dc - 64)  / 4)) & 0x1)) return;
    if (128 <= dc && dc < 132 && !((emu->m_feCfg->EnCoreColLin2.read()  >> ((dc - 128) / 4)) & 0x1)) return;
    if (132 <= dc && dc < 196 && !((emu->m_feCfg->EnCoreColDiff1.read() >> ((dc - 132) / 4)) & 0x1)) return;
    if (196 <= dc && dc < 200 && !((emu->m_feCfg->EnCoreColDiff2.read() >> ((dc - 196) / 4)) & 0x1)) return;
    // check pixels to see if double columns are enabled for injections
    if (             dc < 16  && !((emu->m_feCfg->CalColprSync1.read() >> (dc - 0)   & 0x1))) return;
    if (16  <= dc && dc < 32  && !((emu->m_feCfg->CalColprSync2.read() >> (dc - 16)  & 0x1))) return;
    if (32  <= dc && dc < 48  && !((emu->m_feCfg->CalColprSync3.read() >> (dc - 32)  & 0x1))) return;
    if (48  <= dc && dc < 64  && !((emu->m_feCfg->CalColprSync4.read() >> (dc - 48)  & 0x1))) return;
    if (64  <= dc && dc < 80  && !((emu->m_feCfg->CalColprLin1.read()  >> (dc - 64)  & 0x1))) return;
    if (80  <= dc && dc < 96  && !((emu->m_feCfg->CalColprLin2.read()  >> (dc - 80)  & 0x1))) return;
    if (96  <= dc && dc < 112 && !((emu->m_feCfg->CalColprLin3.read()  >> (dc - 96)  & 0x1))) return;
    if (112 <= dc && dc < 128 && !((emu->m_feCfg->CalColprLin4.read()  >> (dc - 112) & 0x1))) return;
    if (128 <= dc && dc < 132 && !((emu->m_feCfg->CalColprLin5.read()  >> (dc - 128) & 0x1))) return;
    if (132 <= dc && dc < 148 && !((emu->m_feCfg->CalColprDiff1.read() >> (dc - 132) & 0x1))) return;
    if (148 <= dc && dc < 164 && !((emu->m_feCfg->CalColprDiff2.read() >> (dc - 148) & 0x1))) return;
    if (164 <= dc && dc < 180 && !((emu->m_feCfg->CalColprDiff3.read() >> (dc - 164) & 0x1))) return;
    if (180 <= dc && dc < 196 && !((emu->m_feCfg->CalColprDiff4.read() >> (dc - 180) & 0x1))) return;
    if (196 <= dc && dc < 200 && !((emu->m_feCfg->CalColprDiff5.read() >> (dc - 196) & 0x1))) return;

    if( emu->verbose ) std::cout << "dc = " << dc << std::endl;

    for (unsigned row = 0; row < Rd53aPixelCfg::n_Row; row++) {
        float capacitance_times_coulomb = 8000; // change this to the correct value later

        float maximum_injection_voltage = 1.2;
        if( emu->verbose && dc < 132 && row == 0 ) {
            printf("m_feCfg->VcalHigh.read() = %d\n", emu->m_feCfg->InjVcalHigh.read());
            printf("m_feCfg->VcalMed.read() = %d\n", emu->m_feCfg->InjVcalMed.read());
            printf("m_feCfg->VcalHigh.read() + m_feCfg->VcalMed.read() = %d\n", emu->m_feCfg->InjVcalHigh.read() + emu->m_feCfg->InjVcalMed.read());
        }
        float injection_voltage = (emu->m_feCfg->InjVcalHigh.read() + emu->m_feCfg->InjVcalMed.read()) * maximum_injection_voltage / 4096.0;
        float injection_charge = injection_voltage * capacitance_times_coulomb;
            
        if( emu->verbose && dc < 132 && row == 0 ) std::cout << "injection_voltage = " << injection_voltage << std::endl;

        float noise_charge = Gauss::rand_normal(0, 50, 1); // generic, should remove

            // sync front end
            if (dc < 64) {
                // check the final pixel enable, and for now, just increment the number of hits - eventually, we should really just be writing hit data back to YARR
                if (emu->m_pixelRegisters[dc * 2    ][row] & 0x1) emu->totalDigitalHits++;
                if (emu->m_pixelRegisters[dc * 2 + 1][row] & 0x1) emu->totalDigitalHits++;
            }
            // linear front end
            if (64 <= dc && dc < 132) {
                for (int pix = 0; pix <= 1; pix++) {
                    noise_charge = emu->m_rd53aLinPixelModelObjects[dc * 2 + pix - 128][row]->calculateNoise(); // overwrite the previous generic initialization
                    float lin_maximum_global_threshold_voltage = 1.2; // what should this actually be?
                    //printf("m_feCfg->VthresholdLin.read() = %d\n", emu->m_feCfg->VthresholdLin.read());
                    float lin_global_threshold_with_smearing = emu->m_rd53aLinPixelModelObjects[dc * 2 + pix - 128][row]->calculateThreshold(emu->m_feCfg->LinVth.read());
                    float lin_global_threshold_voltage = (lin_global_threshold_with_smearing) * lin_maximum_global_threshold_voltage / 1024.0;
                    float lin_global_threshold_charge = lin_global_threshold_voltage * capacitance_times_coulomb; // I imagine this might need a different capacitance
                    
                    // Temporary hard-set at 1000[e] for the moment.
                    //lin_global_threshold_charge = 1000.;
                    
                    if( emu->verbose && row == 0 ) {
                        std::cout << "lin_global_threshold_voltage = " << lin_global_threshold_voltage << std::endl;
                        std::cout << "injection_charge = " << injection_charge << std::endl;
                        std::cout << "noise_charge = " <<  noise_charge << std::endl;
                        std::cout << "lin_global_threshold_charge = " << lin_global_threshold_charge << std::endl;
                    }

                    if (injection_charge + noise_charge > lin_global_threshold_charge ) {
                        // check the final pixel enable, and for now, just increment the number of hits - eventually, we should really just be writing hit data back to YARR
                        if ( emu->m_pixelRegisters[dc * 2 + pix][row] & 0x1) {
                            emu->linAnalogHits++;
                            emu->analogHits->fill(dc * 2 + pix, row);
                            emu->linScurve[dc * 2 + pix - 128][row]->fill((emu->m_feCfg->InjVcalHigh.read() - emu->m_feCfg->InjVcalMed.read()));
                        }
                    }
                    
                    if (emu->m_pixelRegisters[dc * 2 + pix][row] & 0x1) emu->totalDigitalHits++;
                }
            }
            // differential front end
            if (132 <= dc && dc < Rd53aPixelCfg::n_DC) {
                for (int pix = 0; pix <= 1; pix++) {
                    noise_charge = emu->m_rd53aDiffPixelModelObjects[dc * 2 + pix - 264][row]->calculateNoise(); // overwrite the previous generic initialization
                    float diff_maximum_global_threshold_voltage = 1.2; // what should this actually be?
                    //                              printf("m_feCfg->Vth1Diff.read() = %d\n", emu->m_feCfg->Vth1Diff.read());
                    //                              printf("m_feCfg->Vth2Diff.read() = %d\n", emu->m_feCfg->Vth2Diff.read());
                    float diff_global_threshold_with_smearing = emu->m_rd53aDiffPixelModelObjects[dc * 2 + pix - 264][row]->calculateThreshold(emu->m_feCfg->DiffVth1.read(), emu->m_feCfg->DiffVth2.read());
                    float diff_global_threshold_voltage = (diff_global_threshold_with_smearing) * diff_maximum_global_threshold_voltage / 1024.0;
                    float diff_global_threshold_charge = diff_global_threshold_voltage * capacitance_times_coulomb; // I imagine this might need a different capacitance

                    //                              printf("diff_global_threshold_voltage = %f\n", diff_global_threshold_voltage);
                    //                              printf("injection_charge = %f\n", injection_charge);
                    //                              printf("noise_charge = %f\n", noise_charge);
                    //                              printf("diff_global_threshold_charge = %f\n", diff_global_threshold_charge);

                    if (injection_charge + noise_charge - diff_global_threshold_charge > 0) {
                        // check the final pixel enable, and for now, just increment the number of hits - eventually, we should really just be writing hit data back to YARR
                        if (emu->m_pixelRegisters[dc * 2 + pix][row] & 0x1) {
                            emu->diffAnalogHits ++;
                            emu->analogHits->fill(dc * 2 + pix, row);
                            emu->diffScurve[dc * 2 + pix - 264][row]->fill((emu->m_feCfg->InjVcalHigh.read() - emu->m_feCfg->InjVcalMed.read()));
                        }
                    }

                    if (emu->m_pixelRegisters[dc * 2 + pix][row] & 0x1) emu->totalDigitalHits++;
                }
            }
    }
}

    
void Rd53aEmu::doWrReg( Rd53aEmu* emu ) {
    
    if( emu->verbose ) std::cout << __PRETTY_FUNCTION__ << ": L" << __LINE__ << ": WrReg = " << HEXF(4, emu->stream.front() ) << std::endl;
    emu->stream.pop_front();
    
    emu->retrieve();
    
    //m_id_address_some_data = m_txRingBuffer->read32();
    //              printf("Rd53aEmu got id_address_some_data word: 0x%x\n", m_id_address_some_data);

    uint8_t byte1 = (emu->stream.at(0) & 0xFF00 ) >> 8;
    uint8_t byte2 = (emu->stream.at(0) & 0x00FF );
    uint8_t byte3 = (emu->stream.at(1) & 0xFF00 ) >> 8;
    uint8_t byte4 = (emu->stream.at(1) & 0x00FF );

    byte1 = to5bit( byte1 );
    byte2 = to5bit( byte2 );
    byte3 = to5bit( byte3 );
    byte4 = to5bit( byte4 );
                
    bool isBig = (byte1 & 0x1);

    auto id = ( byte1 >> 1 );
    uint32_t address = ( byte2 << 4 ) + ( byte3 >> 1 );
    //uint32_t data_up6 = ( (byte3 & 0x1) << 5 ) + byte4;
              
    emu->stream.pop_front();
    emu->stream.pop_front();
                
    if ( isBig ) { // check the bit which determines whether big data or small data should be read
        //printf("big data expected\n");
        // 50(?) more bits of data
    }
    
    else {
        //printf("small data expected\n");
        // 10(?) more bits of data
        //                    m_small_data = m_txRingBuffer->read32();
        //                  printf("Rd53aEmu got the small_data word 0x%x\n", m_small_data);

        emu->retrieve();

        uint8_t byte5 = (emu->stream.at(0) & 0xFF00 ) >> 8;
        uint8_t byte6 = (emu->stream.at(0) & 0x00FF );

        uint32_t data = ( (byte3 & 0x1) << 15 ) + (byte4 << 10) + ( to5bit(byte5) << 5 ) + to5bit(byte6);

        if( emu->verbose ) printf(" >> WrReg: id: 0x%x, address: 0x%x, data = 0x%x\n", id, address, data);
                    
        emu->stream.pop_front();

        if( emu->verbose ) std::cout << __PRETTY_FUNCTION__ << ": " << __LINE__ << ": stream front = " << HEXF(4, emu->stream.front() ) << std::endl;

        emu->m_async.emplace_back( std::async(std::launch::deferred, &Rd53aEmu::writeRegAsync, emu, data, address ) );

    }
    
}


void Rd53aEmu::writeRegAsync( Rd53aEmu* emu, const uint32_t data, const uint32_t address) {
    if (address == 0x0) { // configure pixels based on what's in the GR
            
        if( emu->verbose ) {
            printf("being asked to configure pixels; PixAutoCol = %d\n", emu->m_feCfg->PixAutoCol.read() );
            //printf("m_feCfg->PixMode.read() = 0x%x\n", m_feCfg->PixMode.read());
            //printf("m_feCfg->BMask.read() = 0x%x\n", m_feCfg->BMask.read());
        }
            
        //std::cout << __PRETTY_FUNCTION__ << ": L" << __LINE__ << std::endl;
        if (emu->m_feCfg->PixAutoCol.read() == 0x0) { // auto col = 1, auto row = 0, broadcast = 0
            // configure all pixels in row m_feCfg->RegionRow.read() with value sent
            for (unsigned dc = 0; dc < Rd53aPixelCfg::n_DC; dc++) {
                emu->m_pixelRegisters[dc * 2    ][emu->m_feCfg->PixRegionRow.read()] = (uint8_t) (data & 0x00FF);
                emu->m_pixelRegisters[dc * 2 + 1][emu->m_feCfg->PixRegionRow.read()] = (uint8_t) (data >> 8);
                if( emu->verbose ) printf("pixel %d %d 0x%x\n", dc * 2, emu->m_feCfg->PixRegionRow.read(), data & 0x00FF);
            }
            // increment m_feCfg->RegionRow
            if (emu->m_feCfg->PixRegionRow.read() + 1 < Rd53aPixelCfg::n_Col) {
                emu->m_feCfg->PixRegionRow.write(emu->m_feCfg->PixRegionRow.read() + 1);
            }
            else {
                emu->m_feCfg->PixRegionRow.write(0);
            }
            //std::cout << __PRETTY_FUNCTION__ << ": L" << __LINE__ << std::endl;
        }
    }
    else { // configure the global register
        emu->m_feCfg->m_cfg[address] = data; // this is basically where we actually write to the global register
    }
    //std::cout << __PRETTY_FUNCTION__ << ": L" << __LINE__ << std::endl;
}


void Rd53aEmu::doRdReg( Rd53aEmu* emu ) {
    
    if( emu->verbose ) std::cout << __PRETTY_FUNCTION__ << ": L" << __LINE__ << ": RdReg = " << HEXF(4, emu->stream.front() ) << std::endl;
    emu->stream.pop_front();

}



void Rd53aEmu::doDump( Rd53aEmu* emu ) {

    //////////////////////////////////////////////////////////////////
    //
    // This function is temporary bypassing the transmission of data to the software
    // and to be deprecated at some point.
    // (feature can be kept for internal monitoring)
    //
    
    for( auto& async : emu->m_async ) { async.get(); }
    emu->m_async.clear();
    
    for (int col = 0; col < 136; col++) {
        for (int row = 0; row < Rd53aPixelCfg::n_Row; row++) {
            if (emu->diffScurve[col][row]->getEntries() != 0 ) {
                emu->diffScurve[col][row]->scale(1.0/100.0); // hardcoded for now - the number of times we scan the same pixel
                for (int bin = 0; bin < 256; bin++) {

                    if (emu->diffScurve[col][row]->getBin(bin) > 0.5) {
                        emu->diffThreshold->fill(bin * 16);
                        break;
                    }
                }
                if (col == 0 && row == 0) emu->diffScurve[col][row]->plot("scurve", "");
                //                            if (col == 0 && row == 0) emu->diffScurve[col][row]->toFile("scurve", "", 0);
            }

            if (emu->linScurve[col][row]->getEntries() != 0 ) {
                emu->linScurve[col][row]->scale(1.0/100.0); // hardcoded for now - the number of times we scan the same pixel
                for (int bin = 0; bin < 256; bin++) {

                    if (emu->linScurve[col][row]->getBin(bin) > 0.5) {
                        emu->linThreshold->fill(bin * 16);
                        break;
                    }
                }
                if (col == 0 && row == 0) emu->linScurve[col][row]->plot("scurve", "");
                //                            if (col == 0 && row == 0) emu->linScurve[col][row]->toFile("scurve", "", 0);
            }
        }
    }

    emu->diffThreshold->plot("threshold", "");
    emu->linThreshold->plot("threshold", "");
    emu->analogHits->plot("analogHits", "");
    emu->m_rxRingBuffer->write32(0xA); // test writing back
}


