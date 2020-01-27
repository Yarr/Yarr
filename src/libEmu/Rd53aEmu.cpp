#include "Rd53aEmu.h"

#include "Histo2d.h"

#include "logging.h"

#define HEXF(x,y) std::hex << "0x" << std::hex << std::setw(x) << std::setfill('0') << static_cast<int>(y) << std::dec

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Static members and functions

// The following SLEEP_TIME was tuned by macOS 10.12.6, (2.2 GHz Intel Core i7)
#define SLEEP_TIME std::chrono::nanoseconds(10)

//____________________________________________________________________________________________________
// Instantiation is needed for constexpr
constexpr std::array<uint8_t, Rd53aEmu::sizeOf8bit> Rd53aEmu::eightToFive;

namespace {
auto rlog = logging::make_log("emu_rd53a");
}

namespace std
{
template<>
struct hash<Rd53aEmu::Commands> {
    size_t operator()(const Rd53aEmu::Commands command) const {
        return static_cast<unsigned>(command);
    }
};
template<>
struct hash<Rd53aEmu::Triggers> {
    size_t operator()(const Rd53aEmu::Triggers trigger) const {
        return static_cast<unsigned>(trigger);
    }
};
}


//____________________________________________________________________________________________________
// Instantiation of static const members with initializer-list
const std::unordered_map<enum Rd53aEmu::Commands, Rd53aEmu::CommandFunc> Rd53aEmu::commandFuncs {
    { Rd53aEmu::Commands::WrReg       , &Rd53aEmu::doWrReg       },
    { Rd53aEmu::Commands::RdReg       , &Rd53aEmu::doRdReg       },
    { Rd53aEmu::Commands::Cal         , &Rd53aEmu::doCal         },
    { Rd53aEmu::Commands::ECR         , &Rd53aEmu::doECR         },
    { Rd53aEmu::Commands::BCR         , &Rd53aEmu::doBCR         },
    { Rd53aEmu::Commands::Zero        , &Rd53aEmu::doZero        },
    { Rd53aEmu::Commands::GlobalPulse , &Rd53aEmu::doGlobalPulse },
    { Rd53aEmu::Commands::Noop        , &Rd53aEmu::doNoop        },
    { Rd53aEmu::Commands::Sync        , &Rd53aEmu::doSync        }
};


//____________________________________________________________________________________________________
// For the moment, all functions are identical -- later, different implemenation should be implemented
const std::unordered_map<enum Rd53aEmu::Triggers, uint8_t> Rd53aEmu::triggerPatterns {
    { Rd53aEmu::Triggers::Trg01, 0x1 },
    { Rd53aEmu::Triggers::Trg02, 0x2 },
    { Rd53aEmu::Triggers::Trg03, 0x3 },
    { Rd53aEmu::Triggers::Trg04, 0x4 },
    { Rd53aEmu::Triggers::Trg05, 0x5 },
    { Rd53aEmu::Triggers::Trg06, 0x6 },
    { Rd53aEmu::Triggers::Trg07, 0x7 },
    { Rd53aEmu::Triggers::Trg08, 0x8 },
    { Rd53aEmu::Triggers::Trg09, 0x9 },
    { Rd53aEmu::Triggers::Trg10, 0xa },
    { Rd53aEmu::Triggers::Trg11, 0xb },
    { Rd53aEmu::Triggers::Trg12, 0xc },
    { Rd53aEmu::Triggers::Trg13, 0xd },
    { Rd53aEmu::Triggers::Trg14, 0xe },
    { Rd53aEmu::Triggers::Trg15, 0xf }
};


//____________________________________________________________________________________________________
uint8_t Rd53aEmu::to5bit( uint8_t in ) {
    assert( eightToFive[in] != 0xff );
    return eightToFive[in];
}



//____________________________________________________________________________________________________
template<class AnalogFE>
class PixelModel {
public:
    uint8_t  m_register;
    AnalogFE m_analogFEModel;
    typedef AnalogFE AnalogFEType;
};



/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Non-static members



//____________________________________________________________________________________________________
Rd53aEmu::Rd53aEmu(EmuCom * rx, EmuCom * tx, std::string json_file_path)
    : m_txRingBuffer ( tx )
    , m_rxRingBuffer ( rx )
    , m_feCfg        ( new Rd53aCfg )
    , m_pool         ( new ThreadPool(1) )
    , m_pool2        ( new ThreadPool(2) )
    , analogHits     ( new Histo2d("analogHits", Rd53aPixelCfg::n_Col, -0.5, 399.5, Rd53aPixelCfg::n_Row, -0.5, 191.5, typeid(void)) )
{
    
    srand(time(NULL));
    
    run = true;

    std::ifstream file(json_file_path);
    json j = json::parse(file);
    
    // Initialization of the pixel geometry
    for( size_t icoreCol = 0; icoreCol < m_coreArray.size(); ++icoreCol ) {
      auto& coreRow = m_coreArray.at( icoreCol );
      for( size_t icore = 0; icore < coreRow.size(); ++icore ){
	auto& core = coreRow.at( icore );
	for( size_t irow = 0; irow < core.size(); ++irow ){
	  auto& row = core.at( irow );
	  for( size_t ipixel = 0; ipixel < row.size(); ++ipixel ){
	    auto& pixel = row.at( ipixel );

	    // Rd53a has 3 different analogFE flavors
	    // Core column [ 0:15]: Sync
	    // Core column [16:32]: Linear
	    // Core column [33:49]: Differential
	    size_t index = ipixel + irow * n_corePixelRows + icore * ( n_corePixelRows * n_corePixelCols ) + icoreCol * ( n_coreRows * n_corePixelRows * n_corePixelCols );

	    if( icoreCol < 16 ) {
	      pixel = PixelModel<Rd53aSyncPixelModel> { 0, Rd53aSyncPixelModel{ j["Vthreshold_mean_vector"][index], j["Vthreshold_sigma_vector"][index], j["Vthreshold_gauss_vector"][index], j["noise_sigma_mean_vector"][index], j["noise_sigma_sigma_vector"][index], j["noise_sigma_gauss_vector"][index] } };
                
	    } else if( icoreCol < 33 ) {
	      pixel = PixelModel<Rd53aLinPixelModel> { 0, Rd53aLinPixelModel{ j["Vthreshold_mean_vector"][index], j["Vthreshold_sigma_vector"][index], j["Vthreshold_gauss_vector"][index], j["noise_sigma_mean_vector"][index], j["noise_sigma_sigma_vector"][index], j["noise_sigma_gauss_vector"][index] } };
                
	    } else {
	      pixel = PixelModel<Rd53aDiffPixelModel> { 0, Rd53aDiffPixelModel{ j["Vthreshold_mean_vector"][index], j["Vthreshold_sigma_vector"][index], j["Vthreshold_gauss_vector"][index], j["noise_sigma_mean_vector"][index], j["noise_sigma_sigma_vector"][index], j["noise_sigma_gauss_vector"][index] } };
	    }
	  }
	}
      }
    }
    file.close();
    // Initializing trigger counters
    {
        triggerCounters[Triggers::Trg01] = 0;
        triggerCounters[Triggers::Trg02] = 0;
        triggerCounters[Triggers::Trg03] = 0;
        triggerCounters[Triggers::Trg04] = 0;
        triggerCounters[Triggers::Trg05] = 0;
        triggerCounters[Triggers::Trg06] = 0;
        triggerCounters[Triggers::Trg07] = 0;
        triggerCounters[Triggers::Trg08] = 0;
        triggerCounters[Triggers::Trg09] = 0;
        triggerCounters[Triggers::Trg10] = 0;
        triggerCounters[Triggers::Trg11] = 0;
        triggerCounters[Triggers::Trg12] = 0;
        triggerCounters[Triggers::Trg13] = 0;
        triggerCounters[Triggers::Trg14] = 0;
        triggerCounters[Triggers::Trg15] = 0;
    }

}


//____________________________________________________________________________________________________
Rd53aEmu::~Rd53aEmu() {}


//____________________________________________________________________________________________________
void Rd53aEmu::executeLoop() {
    
    rlog->info("Starting emulator loop");
    
    while (run) {
        
        if ( m_txRingBuffer->isEmpty()) {
            std::this_thread::sleep_for( SLEEP_TIME );
            continue;
        }
        
        
        rlog->debug("-----------------------------------------------------------");
        
        
        // read the command header
        if( commandStream.size() == 0 ) {
            retrieve();
        }
        
        if( !commandStream.empty() ) {
          rlog->debug("front = {:04x}, size = {}",
                      commandStream.front(), commandStream.size());
        }
        
        ///////////////////////////////////////////////////////////////////
        // 
        // All the rest commands are grouped here
        //
        
        const auto commandKey = static_cast<Commands>( commandStream.front() );
        
        auto commandFunc_itr = commandFuncs.find( commandKey );
        
        if( commandFunc_itr != commandFuncs.end() ) {
            
            rlog->debug("GlobalPulse = {:04x}", commandStream.front());
            commandStream.pop_front();
            
            for( auto& async : m_async ) { async.get(); }
            m_async.clear();

            // The following grammer is for static member function pointer ( passing this )
            ( commandFunc_itr->second )( this );
            
            continue;
        }
        
        
        ///////////////////////////////////////////////////////////////////
        // 
        // For the moment, all trigger commands are degenerate for simplicity
        // Later, the pattern of the trigger (L1a) and the timing needs to be
        // properly implemented.
        //
        
        const auto triggerKey = static_cast<Triggers>( commandStream.front()>>8 );
        
        auto triggerPattern_itr = triggerPatterns.find( triggerKey );
        
        if( triggerPattern_itr != triggerPatterns.end() ) {
            
            // The following grammer is for static member function pointer ( passing this )
            doTrigger( this, triggerPattern_itr->second, ( commandStream.front() & 0x00ff ) );
            
            commandStream.pop_front();
            
            triggerCounters.at( triggerPattern_itr->first )++;
            
            continue;
        }
        
        
        ///////////////////////////////////////////////////////////////////
        // 
        // Exception
        //
        
        printf("unrecognized header 0x%x, skipping for now (will eventually elegantly crash)\n", commandStream.front() );
        commandStream.pop_front();
        exit(1);
        
    }
    
    doDump( this );

    condition.notify_one();
}


//____________________________________________________________________________________________________
void Rd53aEmu::outputLoop() {
        auto tag = outTags.front();

        //////////////////////////////////////////////////////////////////////////////
        //
        // push the data out
        //
        
        uint32_t header = (0x7f << 25 ) | ( (l1id & 0x1f)<<20 ) | ( (tag & 0x1f) << 15 ) | (bcid & 0x7fff);
        pushOutput( header );
        
        //std::cout << "header = " << HEXF(8, header) << ", outWords size = " << outWords.size() << std::endl;
        
        for( auto& w : outWords[tag] ) {
            
            if( 0 == w ) continue;
            
            // ToT fields w/o hits need to be filled with 0xf.
            if( ( (w & 0x000f) >>  0 ) == 0x0 ) { w |= 0x000f; }
            if( ( (w & 0x00f0) >>  4 ) == 0x0 ) { w |= 0x00f0; }
            if( ( (w & 0x0f00) >>  8 ) == 0x0 ) { w |= 0x0f00; }
            if( ( (w & 0xf000) >> 12 ) == 0x0 ) { w |= 0xf000; }
            
            pushOutput( w );
        }

        outWords.erase( outWords.find( tag ) );
        outTags.pop_front();

}


//____________________________________________________________________________________________________
void Rd53aEmu::retrieve() {
    uint32_t d = m_txRingBuffer->read32();
    //std::cout << "push_back(): adding word = " << HEXF(8, d) << std::endl;
    commandStream.push_back( (d & 0xFFFF0000) >> 16 );
    commandStream.push_back( (d & 0x0000FFFF) );
}



//____________________________________________________________________________________________________
void Rd53aEmu::pushOutput(uint32_t value) {
    if (m_rxRingBuffer) {
        m_rxRingBuffer->write32(value);
    }
}



/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Again, static members and functions


//____________________________________________________________________________________________________
void Rd53aEmu::doECR( Rd53aEmu* emu ) {
    emu->l1id = 0;
    while( emu->m_pool->taskSize() ) { std::this_thread::sleep_for( SLEEP_TIME ); }
}


//____________________________________________________________________________________________________
void Rd53aEmu::doBCR( Rd53aEmu* emu ) {
    emu->bcid = 0;
    while( emu->m_pool->taskSize() ) { std::this_thread::sleep_for( SLEEP_TIME ); }
}


//____________________________________________________________________________________________________
void Rd53aEmu::doNoop( Rd53aEmu* emu ) {
    emu->calTiming += 4;
    emu->bcid      += 4;
}


//____________________________________________________________________________________________________
void Rd53aEmu::doZero( Rd53aEmu* emu ) {}


//____________________________________________________________________________________________________
void Rd53aEmu::doSync( Rd53aEmu* emu ) {
    while( emu->m_pool->taskSize() ) { std::this_thread::sleep_for( SLEEP_TIME ); }
}


//____________________________________________________________________________________________________
void Rd53aEmu::doGlobalPulse( Rd53aEmu* emu ) {
    
    while( emu->m_pool->taskSize() ) { std::this_thread::sleep_for( SLEEP_TIME ); }
    
#if 0
    auto word  = emu->commandStream.front();
    auto byte1 = ( word & 0xFF00 ) >> 8;
    auto byte2 = ( word & 0x00FF );
    auto id    = to5bit(byte1) >> 1;
    auto data  = to5bit(byte2);
    
    std::cout << "id = " << HEXF(4, id) << ", data = " << HEXF(5, data) << std::endl;
#endif
    emu->commandStream.pop_front();
}



//____________________________________________________________________________________________________
void Rd53aEmu::doCal( Rd53aEmu* emu ) {
    
    while( emu->m_pool->taskSize() ) { std::this_thread::sleep_for( SLEEP_TIME ); }
    
    // ToDo
    // For the moment, only pops 2x16-bit words
    // Informations stored there need to be used properly
    // See RD53a Manual section 9.2, p.47
    std::array<uint32_t, 3> input = readIDAddrData( emu, Rd53aEmu::Commands::Cal );
    
    emu->calTiming     = 0;
    emu->injectTiming  = emu->m_feCfg->LatencyConfig.read();
    emu->bcid         += 12; // Consuming 4BC * 3
    
}



//____________________________________________________________________________________________________
void Rd53aEmu::doTrigger( Rd53aEmu* emu,  const uint8_t pattern, const uint8_t tag ) {

    enum { Async, Pool };

    int mode { Async };
    int level { 0 };
    
    // Finish all async processes before triggering
    while( emu->m_pool->taskSize() ) { std::this_thread::sleep_for( SLEEP_TIME ); }
            
    //std::cout << __PRETTY_FUNCTION__ << std::endl;
    
    emu->triggerTagCounters[tag]++;
    
    emu->totalDigitalHits = 0;
    emu->diffAnalogHits   = 0;
    emu->linAnalogHits    = 0;
    emu->syncAnalogHits   = 0;

    emu->outWords[tag] = std::array<uint32_t, 100*192> {{ 0 }};
    
    // Streeam is already popped,
    // then the following part can be run in parallel.
    
    // Loop over 4 BCs
    for( size_t iBC = 0; iBC < 4; iBC++ ) {
        
        if( ( ( pattern >> (3-iBC) ) & 0x1 ) ) {
            
            switch( level ) {

            case 0:
                
                if( mode == Async ) {
                    emu->m_async.emplace_back( std::async( std::launch::deferred, &Rd53aEmu::triggerAsync0, emu, tag ) );
                } else {
                    emu->m_pool->enqueue( &Rd53aEmu::triggerAsync0, emu, tag );
                }

                break;

            case 1:
                    
                for( size_t icoreCol = 0; icoreCol < n_coreCols; ++icoreCol ) {
                        
                    if( mode == Async ) {
                        emu->m_async.emplace_back( std::async( std::launch::deferred, &Rd53aEmu::triggerAsync1, emu, tag, icoreCol ) );
                    } else {
                        emu->m_pool->enqueue( &Rd53aEmu::triggerAsync1, emu, tag, icoreCol );
                    }
                        
                }
                break;

            case 2:
                
                for( size_t icoreCol = 0; icoreCol < n_coreCols; ++icoreCol ) {
                for( size_t icoreRow = 0; icoreRow < n_coreRows; ++icoreRow ) {
                    
                    if( mode == Async ) {
                        emu->m_async.emplace_back( std::async( std::launch::deferred, &Rd53aEmu::triggerAsync2, emu, tag, icoreCol, icoreRow ) );
                    } else {
                        emu->m_pool->enqueue( &Rd53aEmu::triggerAsync2, emu, tag, icoreCol, icoreRow );
                    }
                    
                }}
                break;
                
            default:
                break;
                    
            }
            
            if( mode == Async ) {
                // Finish all async processes before next step
                for( auto& async : emu->m_async ) { async.get(); }
                emu->m_async.clear();
            
            } else {
        
                while( emu->m_pool->taskSize() ) {
                    //std::cout << "waiting for thread pool to empty... " << emu->m_pool->taskSize() << std::endl;
                    std::this_thread::sleep_for( SLEEP_TIME );
                }
            
                //std::cout << "wthread pool is empty... " << emu->m_pool->taskSize() << std::endl;
            }

            std::unique_lock<std::mutex> lock(emu->queue_mutex);
            emu->outTags.emplace_back( tag );
            //emu->condition.notify_one();
            emu->outputLoop();
        
            // Increment the timing counter
            emu->calTiming++;
            emu->bcid++;
        
        }
        
    }
    
    emu->l1id++;
    
    
    // for now, print the total number of hits - eventually, we should really just be writing hit data back to YARR
    //printf("Hits: total = %d [diif = %d, lin = %d]\n", static_cast<int>(emu->totalDigitalHits), static_cast<int>(emu->diffAnalogHits), static_cast<int>(emu->linAnalogHits) );
    //              printf("syncAnalogHits = %d\n", syncAnalogHits);
    //              printf("m_feCfg->VcalHigh.read() = %d\n", m_feCfg->VcalHigh.read());
}



//____________________________________________________________________________________________________
void Rd53aEmu::triggerAsync0( const uint32_t tag) {
    
    enum { CoreColSync = 32, CoreColLin1 = 33, CoreColLin2 = 34, CoreColDiff1 = 35, CoreColDiff2 = 36,
           CalColPrSync1 = 46, CalColPrSync2 = 47, CalColPrSync3 = 48, CalColPrSync4 = 49,
           CalColPrLin1  = 50, CalColPrLin2  = 51, CalColPrLin3  = 52, CalColPrLin4  = 53, CalColPrLin5  = 54,
           CalColPrDiff1 = 55, CalColPrDiff2 = 56, CalColPrDiff3 = 57, CalColPrDiff4 = 58, CalColPrDiff5 = 59
    };
    
#if 0
    auto coreColAddress = []( const unsigned& coreCol ) -> std::pair<unsigned, unsigned> {
        if        ( coreCol < 16 ) {
            return std::pair<unsigned, unsigned> { CoreColSync, coreCol };
        } else if ( coreCol < 32 ) {
            return std::pair<unsigned, unsigned> { CoreColLin1, coreCol-16 };
        } else if ( coreCol < 33 ) {
            return std::pair<unsigned, unsigned> { CoreColLin2, coreCol-32 };
        } else if ( coreCol < 49 ) {
            return std::pair<unsigned, unsigned> { CoreColLin2, coreCol-33 };
        } else {
            return std::pair<unsigned, unsigned> { CoreColLin2, coreCol-49 };
        }
    };
#endif
    
    auto colAddress = []( const unsigned& coreCol, const unsigned& icol ) -> std::pair<unsigned, unsigned> {
        // 0 [0:7] -> 46
        // 1 [0:7] -> 46
        // 2       -> 47
        // ...
        // 30[0:7] -> 53
        // 31[0:7] -> 53
        // 32[0:7] -> 54 /* last of Lin */
        // 33[0:7] -> 55
        // 34[0:7] -> 55
        
        if( coreCol < 33 ) {
            return std::pair<unsigned, unsigned> { CalColPrSync1 + coreCol/4,      icol/2 + (coreCol%4)*n_corePixelCols/2 };
        } else {
            return std::pair<unsigned, unsigned> { CalColPrDiff1 + (coreCol-33)/4, icol/2 + ((coreCol-33)%4)*n_corePixelCols/2 };
        }
    };
    
    
    /////////////////////////////////////////////////////////////////////////////////////////////////
    //
    // Hits are only created when the CAL injection timing matches
    // If we want to emulate time-walk behavior, this needs to be properly implemented
    // depending on the analog FE modeling.
    //
    
    if( injectTiming != calTiming ) return;
    

    for( size_t coreCol = 0; coreCol < n_coreCols; ++coreCol ) {
#if 0
    // put these checks into a function maybe
    // check pixels to see if the digital enable is set for "octo-columns" (columns of cores)
    if (             dc < 64  && !((m_feCfg->EnCoreColSync.read()  >> ((dc - 0)   / 4)) & 0x1)) return;
    if (64  <= dc && dc < 128 && !((m_feCfg->EnCoreColLin1.read()  >> ((dc - 64)  / 4)) & 0x1)) return;
    if (128 <= dc && dc < 132 && !((m_feCfg->EnCoreColLin2.read()  >> ((dc - 128) / 4)) & 0x1)) return;
    if (132 <= dc && dc < 196 && !((m_feCfg->EnCoreColDiff1.read() >> ((dc - 132) / 4)) & 0x1)) return;
    if (196 <= dc && dc < 200 && !((m_feCfg->EnCoreColDiff2.read() >> ((dc - 196) / 4)) & 0x1)) return;
#endif
    
#if 0
    auto coreColAddr = coreColAddress( coreCol );
    if(! ( ( ( m_feCfg->m_cfg.at( coreColAddr.first ) ) >> coreColAddr.second ) & 0x1 ) ) return;
#endif

    for( size_t coreRow = 0; coreRow < n_coreRows; ++coreRow ) {
        
        auto& core = m_coreArray[coreCol][coreRow];
    
    
        for( size_t icol = 0; icol < n_corePixelCols; ++icol ) {
        
            auto colAddr = colAddress( coreCol, icol );
        
        
#define calflag ( ( ( m_feCfg->m_cfg.at( colAddr.first ) ) >> colAddr.second ) & 0x1 )
        
            /*
              std::cout << "coreCol = " << coreCol << ", icol = " << icol << " ==> colAddr = (" << colAddr.first << ", " << colAddr.second << ") ==> " << HEXF(4, m_feCfg->m_cfg.at( colAddr.first ))
              << ", flag = " << calflag << std::endl;
            */
        
            if( !calflag ) continue;
        
#undef calflag
        
        
            for( size_t irow = 0; irow < n_corePixelRows; ++irow ) {
            
                auto& pixel = core[icol][irow];
            
                if( pixel.type() == typeid( PixelModel<Rd53aLinPixelModel> ) ) {
                
                    calculateSignal< PixelModel<Rd53aLinPixelModel> >( pixel, coreCol, coreRow, icol, irow, tag );
                
                } else if( pixel.type() == typeid( PixelModel<Rd53aDiffPixelModel> ) ) {
                
                    calculateSignal< PixelModel<Rd53aDiffPixelModel> >( pixel, coreCol, coreRow, icol, irow, tag );
                
                } else if( pixel.type() == typeid( PixelModel<Rd53aSyncPixelModel> ) ) {

                    calculateSignal< PixelModel<Rd53aSyncPixelModel> >( pixel, coreCol, coreRow, icol, irow, tag );
                
                } else {

                    throw std::runtime_error( "Invalid Rd53a analog FE model was detected!" );
                
                }
            
            }
        }
    
    }
    }
    
}


//____________________________________________________________________________________________________
void Rd53aEmu::triggerAsync1( const uint32_t tag, const unsigned coreCol) {
    
    enum { CoreColSync = 32, CoreColLin1 = 33, CoreColLin2 = 34, CoreColDiff1 = 35, CoreColDiff2 = 36,
           CalColPrSync1 = 46, CalColPrSync2 = 47, CalColPrSync3 = 48, CalColPrSync4 = 49,
           CalColPrLin1  = 50, CalColPrLin2  = 51, CalColPrLin3  = 52, CalColPrLin4  = 53, CalColPrLin5  = 54,
           CalColPrDiff1 = 55, CalColPrDiff2 = 56, CalColPrDiff3 = 57, CalColPrDiff4 = 58, CalColPrDiff5 = 59
    };
    
#if 0
    auto coreColAddress = []( const unsigned& coreCol ) -> std::pair<unsigned, unsigned> {
        if        ( coreCol < 16 ) {
            return std::pair<unsigned, unsigned> { CoreColSync, coreCol };
        } else if ( coreCol < 32 ) {
            return std::pair<unsigned, unsigned> { CoreColLin1, coreCol-16 };
        } else if ( coreCol < 33 ) {
            return std::pair<unsigned, unsigned> { CoreColLin2, coreCol-32 };
        } else if ( coreCol < 49 ) {
            return std::pair<unsigned, unsigned> { CoreColLin2, coreCol-33 };
        } else {
            return std::pair<unsigned, unsigned> { CoreColLin2, coreCol-49 };
        }
    };
#endif
    
    auto colAddress = []( const unsigned& coreCol, const unsigned& icol ) -> std::pair<unsigned, unsigned> {
        // 0 [0:7] -> 46
        // 1 [0:7] -> 46
        // 2       -> 47
        // ...
        // 30[0:7] -> 53
        // 31[0:7] -> 53
        // 32[0:7] -> 54 /* last of Lin */
        // 33[0:7] -> 55
        // 34[0:7] -> 55
        
        if( coreCol < 33 ) {
            return std::pair<unsigned, unsigned> { CalColPrSync1 + coreCol/4,      icol/2 + (coreCol%4)*n_corePixelCols/2 };
        } else {
            return std::pair<unsigned, unsigned> { CalColPrDiff1 + (coreCol-33)/4, icol/2 + ((coreCol-33)%4)*n_corePixelCols/2 };
        }
    };
    
    
    /////////////////////////////////////////////////////////////////////////////////////////////////
    //
    // Hits are only created when the CAL injection timing matches
    // If we want to emulate time-walk behavior, this needs to be properly implemented
    // depending on the analog FE modeling.
    //
    
    if( injectTiming != calTiming ) return;
    
    
#if 0
    // put these checks into a function maybe
    // check pixels to see if the digital enable is set for "octo-columns" (columns of cores)
    if (             dc < 64  && !((m_feCfg->EnCoreColSync.read()  >> ((dc - 0)   / 4)) & 0x1)) return;
    if (64  <= dc && dc < 128 && !((m_feCfg->EnCoreColLin1.read()  >> ((dc - 64)  / 4)) & 0x1)) return;
    if (128 <= dc && dc < 132 && !((m_feCfg->EnCoreColLin2.read()  >> ((dc - 128) / 4)) & 0x1)) return;
    if (132 <= dc && dc < 196 && !((m_feCfg->EnCoreColDiff1.read() >> ((dc - 132) / 4)) & 0x1)) return;
    if (196 <= dc && dc < 200 && !((m_feCfg->EnCoreColDiff2.read() >> ((dc - 196) / 4)) & 0x1)) return;
#endif
    
#if 0
    auto coreColAddr = coreColAddress( coreCol );
    if(! ( ( ( m_feCfg->m_cfg.at( coreColAddr.first ) ) >> coreColAddr.second ) & 0x1 ) ) return;
#endif

    for( size_t coreRow = 0; coreRow < n_coreRows; ++coreRow ) {
        
        auto& core = m_coreArray[coreCol][coreRow];
    
    
        for( size_t icol = 0; icol < n_corePixelCols; ++icol ) {
        
            auto colAddr = colAddress( coreCol, icol );
        
        
#define calflag ( ( ( m_feCfg->m_cfg.at( colAddr.first ) ) >> colAddr.second ) & 0x1 )
        
            /*
              std::cout << "coreCol = " << coreCol << ", icol = " << icol << " ==> colAddr = (" << colAddr.first << ", " << colAddr.second << ") ==> " << HEXF(4, m_feCfg->m_cfg.at( colAddr.first ))
              << ", flag = " << calflag << std::endl;
            */
        
            if( !calflag ) continue;
        
#undef calflag
        
        
            for( size_t irow = 0; irow < n_corePixelRows; ++irow ) {
            
                auto& pixel = core[icol][irow];
            
                if( pixel.type() == typeid( PixelModel<Rd53aLinPixelModel> ) ) {
                
                    calculateSignal< PixelModel<Rd53aLinPixelModel> >( pixel, coreCol, coreRow, icol, irow, tag );
                
                } else if( pixel.type() == typeid( PixelModel<Rd53aDiffPixelModel> ) ) {
                
                    calculateSignal< PixelModel<Rd53aDiffPixelModel> >( pixel, coreCol, coreRow, icol, irow, tag );
                
                } else if( pixel.type() == typeid( PixelModel<Rd53aSyncPixelModel> ) ) {

                    calculateSignal< PixelModel<Rd53aSyncPixelModel> >( pixel, coreCol, coreRow, icol, irow, tag );
                
                } else {

                    throw std::runtime_error( "Invalid Rd53a analog FE model was detected!" );
                
                }
            
            }
        }
    
    }
    
}


//____________________________________________________________________________________________________
void Rd53aEmu::triggerAsync2( const uint32_t tag, const unsigned coreCol, const unsigned coreRow) {
    
    enum { CoreColSync = 32, CoreColLin1 = 33, CoreColLin2 = 34, CoreColDiff1 = 35, CoreColDiff2 = 36,
           CalColPrSync1 = 46, CalColPrSync2 = 47, CalColPrSync3 = 48, CalColPrSync4 = 49,
           CalColPrLin1  = 50, CalColPrLin2  = 51, CalColPrLin3  = 52, CalColPrLin4  = 53, CalColPrLin5  = 54,
           CalColPrDiff1 = 55, CalColPrDiff2 = 56, CalColPrDiff3 = 57, CalColPrDiff4 = 58, CalColPrDiff5 = 59
    };
    
#if 0
    auto coreColAddress = []( const unsigned& coreCol ) -> std::pair<unsigned, unsigned> {
        if        ( coreCol < 16 ) {
            return std::pair<unsigned, unsigned> { CoreColSync, coreCol };
        } else if ( coreCol < 32 ) {
            return std::pair<unsigned, unsigned> { CoreColLin1, coreCol-16 };
        } else if ( coreCol < 33 ) {
            return std::pair<unsigned, unsigned> { CoreColLin2, coreCol-32 };
        } else if ( coreCol < 49 ) {
            return std::pair<unsigned, unsigned> { CoreColLin2, coreCol-33 };
        } else {
            return std::pair<unsigned, unsigned> { CoreColLin2, coreCol-49 };
        }
    };
#endif
    
    auto colAddress = []( const unsigned& coreCol, const unsigned& icol ) -> std::pair<unsigned, unsigned> {
        // 0 [0:7] -> 46
        // 1 [0:7] -> 46
        // 2       -> 47
        // ...
        // 30[0:7] -> 53
        // 31[0:7] -> 53
        // 32[0:7] -> 54 /* last of Lin */
        // 33[0:7] -> 55
        // 34[0:7] -> 55
        
        if( coreCol < 33 ) {
            return std::pair<unsigned, unsigned> { CalColPrSync1 + coreCol/4,      icol/2 + (coreCol%4)*n_corePixelCols/2 };
        } else {
            return std::pair<unsigned, unsigned> { CalColPrDiff1 + (coreCol-33)/4, icol/2 + ((coreCol-33)%4)*n_corePixelCols/2 };
        }
    };
    
    
    /////////////////////////////////////////////////////////////////////////////////////////////////
    //
    // Hits are only created when the CAL injection timing matches
    // If we want to emulate time-walk behavior, this needs to be properly implemented
    // depending on the analog FE modeling.
    //
    
    if( injectTiming != calTiming ) return;
    
    
#if 0
    // put these checks into a function maybe
    // check pixels to see if the digital enable is set for "octo-columns" (columns of cores)
    if (             dc < 64  && !((m_feCfg->EnCoreColSync.read()  >> ((dc - 0)   / 4)) & 0x1)) return;
    if (64  <= dc && dc < 128 && !((m_feCfg->EnCoreColLin1.read()  >> ((dc - 64)  / 4)) & 0x1)) return;
    if (128 <= dc && dc < 132 && !((m_feCfg->EnCoreColLin2.read()  >> ((dc - 128) / 4)) & 0x1)) return;
    if (132 <= dc && dc < 196 && !((m_feCfg->EnCoreColDiff1.read() >> ((dc - 132) / 4)) & 0x1)) return;
    if (196 <= dc && dc < 200 && !((m_feCfg->EnCoreColDiff2.read() >> ((dc - 196) / 4)) & 0x1)) return;
#endif
    
#if 0
    auto coreColAddr = coreColAddress( coreCol );
    if(! ( ( ( m_feCfg->m_cfg.at( coreColAddr.first ) ) >> coreColAddr.second ) & 0x1 ) ) return;
#endif
    
    auto& core = m_coreArray[coreCol][coreRow];
    
    
    for( size_t icol = 0; icol < n_corePixelCols; ++icol ) {
        
        auto colAddr = colAddress( coreCol, icol );
        
        
#define calflag ( ( ( m_feCfg->m_cfg.at( colAddr.first ) ) >> colAddr.second ) & 0x1 )
        
        /*
          std::cout << "coreCol = " << coreCol << ", icol = " << icol << " ==> colAddr = (" << colAddr.first << ", " << colAddr.second << ") ==> " << HEXF(4, m_feCfg->m_cfg.at( colAddr.first ))
          << ", flag = " << calflag << std::endl;
        */
        
        if( !calflag ) continue;
        
#undef calflag
        
        
        for( size_t irow = 0; irow < n_corePixelRows; ++irow ) {
            
            auto& pixel = core[icol][irow];
            
            if( pixel.type() == typeid( PixelModel<Rd53aLinPixelModel> ) ) {
                
                calculateSignal< PixelModel<Rd53aLinPixelModel> >( pixel, coreCol, coreRow, icol, irow, tag );
                
            } else if( pixel.type() == typeid( PixelModel<Rd53aDiffPixelModel> ) ) {
                
                calculateSignal< PixelModel<Rd53aDiffPixelModel> >( pixel, coreCol, coreRow, icol, irow, tag );
                
            } else if( pixel.type() == typeid( PixelModel<Rd53aSyncPixelModel> ) ) {

                calculateSignal< PixelModel<Rd53aSyncPixelModel> >( pixel, coreCol, coreRow, icol, irow, tag );
                
            } else {

                throw std::runtime_error( "Invalid Rd53a analog FE model was detected!" );
                
            }
            
        }
    }
    
}


//____________________________________________________________________________________________________
void Rd53aEmu::doWrReg( Rd53aEmu* emu ) {
  std::array<uint32_t, 3> input=readIDAddrData( emu, Rd53aEmu::Commands::WrReg );

  emu->m_pool->enqueue( &Rd53aEmu::writeRegAsync, emu, input[2], input[1] );
}


//____________________________________________________________________________________________________
void Rd53aEmu::writeRegAsync( Rd53aEmu* emu, const uint16_t data, const uint32_t address) {
    
#define BROADCAST_EN ( emu->m_feCfg->PixBroadcastEn.read() )
#define AUTOCOL      ( emu->m_feCfg->PixAutoCol.read() )
#define AUTOROW      ( emu->m_feCfg->PixAutoRow.read() )
#define DCOL         ( emu->m_feCfg->PixRegionCol.read() )
#define ROW          ( emu->m_feCfg->PixRegionRow.read() )
#define CORECOL      ( DCOL / 4 )
#define COREROW      ( ROW / 8 )
    
    if (address == 0x0) { // configure pixels based on what's in the GR

        rlog->trace("being asked to configure pixels; Broadcast = {}, AutoCol = {}, AutoRow = {}, RegionRow = {}", BROADCAST_EN, AUTOCOL, AUTOROW, ROW );

        if (BROADCAST_EN == 0x1) { // auto col = 1, auto row = 0, broadcast = 0
#if 0
            
            // configure all pixels in ROW with value sent
            for (unsigned dc = 0; dc < Rd53aPixelCfg::n_DC; dc++) {
                emu->m_pixelRegisters[dc * 2    ][ROW] = (uint8_t) (data & 0x00FF);
                emu->m_pixelRegisters[dc * 2 + 1][ROW] = (uint8_t) (data >> 8);
                rlog->debug("pixel {} {} 0x{:x}", dc * 2, ROW, data & 0x00FF);
            }
            // increment m_feCfg->RegionRow
            if ( static_cast<unsigned>( ROW + 1 ) < Rd53aPixelCfg::n_Col) {
                emu->m_feCfg->PixRegionRow.write(ROW + 1);
            }
            else {
                emu->m_feCfg->PixRegionRow.write(0);
            }
#endif
        }
        
        else {
	  emu->m_feCfg->pixRegs[DCOL*n_coreRows*n_corePixelRows+ROW] = data;            
        }
    }
    else { // configure the global register
        //std::cout << "calling with data " << HEXF(4, data) << " for global register address " << address << " (" << emu->m_feCfg->regName( address ) << ")" << std::endl;
        emu->m_feCfg->m_cfg.at( address ) = data; // this is basically where we actually write to the global register
    }
    
#undef ROW
#undef AUTOCOL
}

//____________________________________________________________________________________________________
void Rd53aEmu::doRdReg( Rd53aEmu* emu ) {
  std::array<uint32_t, 3> input=readIDAddrData( emu, Rd53aEmu::Commands::RdReg );
  
  std::pair<uint32_t, uint32_t> regFrame=assembleRegFrame( emu, input[1], 0x99, 0x0 );
  emu->pushOutput(regFrame.first);
  emu->pushOutput(regFrame.second);
}



//____________________________________________________________________________________________________
void Rd53aEmu::doDump( Rd53aEmu* emu ) {
    std::cout << __PRETTY_FUNCTION__ << std::endl;
    
    //////////////////////////////////////////////////////////////////
    //
    // This function is temporary bypassing the transmission of data to the software
    // and to be deprecated at some point.
    // (feature can be kept for internal monitoring)
    //
    
    for( auto& async : emu->m_async ) { async.get(); }
    emu->m_async.clear();
    
    emu->analogHits->plot("analogHits", "");
    
    std::cout << "analogHits entries = " << emu->analogHits->numOfEntries() << std::endl;
    
    for( auto& pair : emu->triggerCounters ) {
        std::cout << "trigger pattern " << HEXF(2, pair.first) << ": counter = " << pair.second << std::endl;
    }
    for( auto& pair : emu->triggerTagCounters ) {
        std::cout << "trigger tag pattern " << HEXF(2, pair.first) << ": counter = " << pair.second << std::endl;
    }
}



//____________________________________________________________________________________________________
void Rd53aEmu::formatWords( const uint32_t coreCol, const uint32_t coreRow, const uint32_t subCol, const uint32_t subRow, const uint32_t ToT, uint32_t tag ) {
    //std::cout << __PRETTY_FUNCTION__ << std::endl;
    
    // This function creates the output data format
    // and store it to the temporary buffer
    
  uint32_t word = ( (coreCol<<26) + (coreRow<<20) + (subRow<<17) + ( (subCol/4)<<16 ) + (ToT <<(4*(subCol%4))) );
    
    size_t coord = (coreCol*2+subCol/4)*192 + (coreRow*8+subRow);

    {
        std::unique_lock<std::mutex> lock(this->queue_mutex);
        outWords[tag].at(coord) |= word;
    }

    totalDigitalHits++;
    analogHits->fill( coreCol*n_corePixelCols + subCol, coreRow*n_corePixelRows + subRow);
    
    //std::cout << "core(" << coreCol << ", " << coreRow << "), pixel(" << subCol << ", " << subRow << "), word = " << HEXF(8, word) << std::endl;
};


//____________________________________________________________________________________________________
uint32_t Rd53aEmu::calculateToT( Rd53aSyncPixelModel& analogFE, int TDAC ) {

  uint32_t ToT = 0xf;
  
  const float injection_charge = m_feCfg->toCharge( m_feCfg->InjVcalDiff.read() ) ;

  auto noise_charge = analogFE.calculateNoise(); // overwrite the previous generic initialization
    
  float sync_global_threshold_with_smearing = analogFE.calculateThreshold(m_feCfg->SyncVth.read());
  float sync_global_threshold_charge        = -175.807 + 9.13438 * sync_global_threshold_with_smearing;
    
  if ( injection_charge + noise_charge > sync_global_threshold_charge ){
    ToT = analogFE.calculateToT( injection_charge + noise_charge - sync_global_threshold_charge );
    if(ToT >= 14) ToT=0xe;
  }
  
  rlog->debug("injection_charge = {}");
  rlog->debug("noise_charge = {}");
  rlog->debug("sync_global_threshold_charge = {}");
  rlog->debug("ToT = {}");
  
  return ToT;
}

//____________________________________________________________________________________________________
uint32_t Rd53aEmu::calculateToT( Rd53aLinPixelModel& analogFE, int TDAC ) {

    uint32_t ToT = 0xf;
    
    const float injection_charge = m_feCfg->toCharge( m_feCfg->InjVcalDiff.read() ) ;
    
    auto noise_charge = analogFE.calculateNoise(); // overwrite the previous generic initialization
    
    float lin_global_threshold_with_smearing = analogFE.calculateThreshold( m_feCfg->LinVth.read(), TDAC );
    float lin_global_threshold_charge        = -12827.1 + 39.298 * lin_global_threshold_with_smearing;
    
    if ( injection_charge + noise_charge > lin_global_threshold_charge ){
        ToT = analogFE.calculateToT( injection_charge + noise_charge - lin_global_threshold_charge );
	if(ToT >= 14) ToT=0xe;
    }

    if( ToT != 0xf ) {
      rlog->debug(m_feCfg->InjVcalDiff.read());
      rlog->debug("injection_charge = {}", injection_charge);
      rlog->debug("noise_charge = {}", noise_charge);
      rlog->debug("lin_global_threshold_charge = {}",
                  lin_global_threshold_charge);
      rlog->debug("ToT = {}", ToT);
    }

    return ToT;
}


//____________________________________________________________________________________________________
uint32_t Rd53aEmu::calculateToT( Rd53aDiffPixelModel& analogFE, int TDAC ) {

    uint32_t ToT = 0xf;
    
    const float injection_charge = m_feCfg->toCharge( m_feCfg->InjVcalDiff.read() ) ;
    
    auto noise_charge = analogFE.calculateNoise(); // overwrite the previous generic initialization
    
    float diff_global_threshold_with_smearing = analogFE.calculateThreshold( m_feCfg->DiffVth1.read(), m_feCfg->DiffVth2.read(), TDAC );
    float diff_global_threshold_charge = 335.111 + 4.73165 * diff_global_threshold_with_smearing;

    if ( injection_charge + noise_charge > diff_global_threshold_charge ){
        ToT = analogFE.calculateToT( injection_charge + noise_charge - diff_global_threshold_charge );
	if(ToT >= 14) ToT=0xe;
    }
                
    rlog->debug("injection_charge = {}", injection_charge);
    rlog->debug("noise_charge = {}", noise_charge);
    rlog->debug("diff_global_threshold_charge = {}",
                diff_global_threshold_charge);
    rlog->debug("ToT = {}", ToT);
    
    return ToT;
}

//____________________________________________________________________________________________________
std::pair<uint32_t, uint32_t> Rd53aEmu::assembleRegFrame( Rd53aEmu* emu, const uint16_t address, const uint8_t zz, const uint8_t status ){
  uint16_t regVal = emu->m_feCfg->m_cfg.at( address );
  uint32_t higher = ( ( zz << 4 | status ) << 20 ) | ( address << 10 ) | ( ( regVal & 0xFFC0 ) >> 6 );
  uint32_t lower = ( ( regVal & 0x003F ) << 26 );
  return std::make_pair(higher, lower);
}

//____________________________________________________________________________________________________
std::array<uint32_t, 3> Rd53aEmu::readIDAddrData( Rd53aEmu* emu, Rd53aEmu::Commands command ){
  enum { ID, Address, Data };
  unsigned fieldSize = 0;
  std::array<uint32_t, 3> output = {0, 0, 0}; // 0: ID, 1: address 2: data. Currently do not consider big data.
  if ( command == Rd53aEmu::Commands::GlobalPulse ) fieldSize = 1;
  else if ( command == Rd53aEmu::Commands::Cal || command == Rd53aEmu::Commands::RdReg ) fieldSize = 2;
  else if ( command == Rd53aEmu::Commands::WrReg ) fieldSize = 3;
  else{
    std::cout << "Require reading ID/address/data for empty filed following command " << HEXF(4, command) << std::endl;
    return output;
  }

  while( emu->commandStream.size() < fieldSize ) emu->retrieve();

  std::pair<uint8_t, uint8_t> bp1 = cmdTo5bitPair( emu->commandStream.at(0) );

  // ------------------ Get ID -------------------
  output[ID] = ( bp1.first >> 1 ); // Get ID, which is the same for all commands

  if ( command == Rd53aEmu::Commands::GlobalPulse ){ // GlobalPulse: only read first command and then return
    output[Data] = ( bp1.second >> 1 );
    popCmd( emu, fieldSize );
    return output;
  }

  if ( command == Rd53aEmu::Commands::WrReg && ( bp1.first & 0x1 ) ){ // In case of Big data, do not read anything
    popCmd( emu, fieldSize );
    return output;
  }
  
  // ------------------ Get Address and data -------------------
  std::pair<uint8_t, uint8_t> bp2 = cmdTo5bitPair( emu->commandStream.at(1) );
  
  if ( command == Rd53aEmu::Commands::WrReg || command == Rd53aEmu::Commands::RdReg ){ // Only relevant for WrReg and RdReg
    output[Address] = ( bp1.second << 4 ) + ( bp2.first >> 1 );
    if ( command == Rd53aEmu::Commands::RdReg ){ // For RdReg command, no data to read
      popCmd( emu, fieldSize );
      return output;
    }
    else{			// Data from WrReg
      std::pair<uint8_t, uint8_t> bp3 = cmdTo5bitPair( emu->commandStream.at(2) );
      output[Data] = ( ( bp2.first & 0x1) << 15 ) + ( bp2.second << 10 ) + ( bp3.first << 5 ) + bp3.second;
    }
  }
  else if ( command == Rd53aEmu::Commands::Cal ){
    output[Data] = ( ( bp1.first & 0x1) << 15 ) + ( bp1.second << 10 ) + ( bp2.first << 5 ) + bp2.second;
  }

  popCmd( emu, fieldSize );
  return output;
}

std::pair<uint8_t, uint8_t> Rd53aEmu::cmdTo5bitPair( uint16_t command ){
  uint8_t byte1 = ( command & 0xFF00 ) >> 8;
  uint8_t byte2 = ( command & 0x00FF );
  return std::make_pair( to5bit(byte1), to5bit(byte2) );
}

void Rd53aEmu::popCmd( Rd53aEmu* emu, const unsigned size ){
  if( emu->commandStream.size() < size ){
    std::cout << "Command stream size " << emu->commandStream.size()
	      << " smaller than requested pop size " << size
	      << " Exiting. " << std::endl;
    exit(1);
  }
  
  for( unsigned i = 0; i < size; i++ ) emu->commandStream.pop_front();
}

template<class PIXEL>
void Rd53aEmu::calculateSignal( anytype& pixel, const uint32_t coreCol, const uint32_t coreRow, const uint32_t subCol, const uint32_t subRow, uint32_t tag ) {
        
  auto& model    = pixel.getVar<PIXEL>();
  auto& reg      = model.m_register;
  auto& analogFE = model.m_analogFEModel;

  // See Manual Table 30 (p.72) for the behavior of the pixel register
  // Bit [0]   : pixel power or enable
  // Bit [1]   : injection enable
  // Bit [2]   : Hitbus enable
  // Bit [3]   : TDAC sign   (only for Diff)
  // Bit [4-7] : TDAC b[0-3] (only for Diff)

  uint32_t col = coreCol * n_corePixelCols + subCol;
  uint32_t row = coreRow * n_corePixelRows + subRow;

  if( !( m_feCfg->getEn( col, row ) ) ) return;
  if( !( m_feCfg->getInjEn( col, row ) ) ) return;
  
  formatWords( coreCol, coreRow, subCol, subRow, (m_feCfg->InjEnDig.read() ? 8 : calculateToT( analogFE, m_feCfg->getTDAC( col, row ) )), tag );
}
