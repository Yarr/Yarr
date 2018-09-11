#ifndef __RD53A_EMU_H__
#define __RD53A_EMU_H__

#include "Rd53aCfg.h"
#include "AnyType.h"
#include "ThreadPool.h"

#include <unordered_map>
#include <memory>
#include <future>
#include <atomic>

//////////////////////////////////////////////////////////////////////////////////////////////////
//
// Forward declarations
//

class RingBuffer;
class Rd53aLinPixelModel;
class Rd53aDiffPixelModel;
class Histo2d;

// Temporarily substituting with Lin model
// To be replaced in the future
using Rd53aSyncPixelModel = Rd53aLinPixelModel;




class Rd53aEmu {

    /*********************************
     *
     * [[ Structure ]]
     *
     * std::deque<uint16_t> Rd53aEmu::stream is used as the internal buffer of the command.
     * Commands are digested in executeLoop(), and depending on the command, the corresponding
     * strategy functions (commandFunc or triggerFunc) are called.
     * Used command words are popped from stream.
     *
     * Partially these command functions can run asynchronously using std::async.
     * 
     * The image of the data flow is as the following diagram:
     *
     *
     *                                                     +--> commandFunc (e.g. ECR) -->--+
     *                                   [Rd53aEmu]        |                                |
     * (Tx ring buffer) -->retrieve()--> (commandStream) --+--> commandFunc (e.g. Cal) -->--+--> (outWords) -->pushOutput()-->(Rx ring buffer)
     *                                                     |                                |
     *                                                     +--> commandFunc (e.g. Trg) -->--+
     *                                                     |                                |
     *                                                     +-->          ....          -->--+
     */
    
public:

    /** List of the first 8-bit of the trigger command word */
    enum class Triggers {
        Trg01 = 0x2b, Trg02 = 0x2d, Trg03 = 0x2e, Trg04 = 0x33, Trg05 = 0x35, Trg06 = 0x36, Trg07 = 0x39, Trg08 = 0x3a,
        Trg09 = 0x3c, Trg10 = 0x4b, Trg11 = 0x4d, Trg12 = 0x4e, Trg13 = 0x53, Trg14 = 0x55, Trg15 = 0x56
    };
    

    /** List of the 16-bit command words */
    enum class Commands {
        ECR = 0x5a5a, BCR = 0x5959, GlobalPulse = 0x5c5c, Cal = 0x6363,
        WrReg = 0x6666, RdReg = 0x6565, Noop = 0x6969, Sync = 0x817e, Zero = 0x0000
    };


    /** The ownsership of these ring buffers need to be designed properly */
    Rd53aEmu(RingBuffer * rx, RingBuffer * tx);
    ~Rd53aEmu();
    
    // the main loop which recieves commands from yarr
    void executeLoop();
    
    /** another thread for writing out data */
    void outputLoop();

    volatile bool run;
    
private:

    
    //////////////////////////////////////////////////////////////////////////////////////////////////
    //
    // Static part
    //
    
    /** Pixel Core is the 8x8 pixels */

    static constexpr unsigned n_corePixelCols = 8;
    static constexpr unsigned n_corePixelRows = 8;
    
    using PixelCore = std::array< std::array< anytype, n_corePixelRows>, n_corePixelCols> ;

    template<unsigned CoreCols, unsigned CoreRows>
    using PixelCoreArray = std::array< std::array< PixelCore, CoreRows>, CoreCols>;

    /** PixelCoreArray is the 2-dim array of PixelCores */

    static constexpr unsigned n_coreCols = 50;
    static constexpr unsigned n_coreRows = 24;
    
    
    /** Input commands */
    
    static constexpr size_t sizeOf8bit = 256;

    static constexpr std::array<uint8_t, sizeOf8bit> eightToFive {{
             //         0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F
             /* 0 */ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
             /* 1 */ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
             /* 2 */ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
             /* 3 */ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
             /* 4 */ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
             /* 5 */ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
             /* 6 */ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0x01, 0xFF, 0xFF, 0xFF, 
             /* 7 */ 0xFF, 0x02, 0x03, 0xFF, 0x04, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
             /* 8 */ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x05, 0xFF, 0x06, 0x07, 0xFF, 
             /* 9 */ 0xFF, 0xFF, 0xFF, 0x08, 0xFF, 0x09, 0x0A, 0xFF, 0xFF, 0x0B, 0x0C, 0xFF, 0x0D, 0xFF, 0xFF, 0xFF, 
             /* A */ 0xFF, 0xFF, 0xFF, 0x0E, 0xFF, 0x0F, 0x10, 0xFF, 0xFF, 0x11, 0x12, 0xFF, 0x13, 0xFF, 0xFF, 0xFF, 
             /* B */ 0xFF, 0x14, 0x15, 0xFF, 0x16, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
             /* C */ 0xFF, 0xFF, 0xFF, 0x17, 0xFF, 0x18, 0x19, 0xFF, 0xFF, 0x1A, 0x1B, 0xFF, 0x1C, 0xFF, 0xFF, 0xFF, 
             /* D */ 0xFF, 0x1D, 0x1E, 0xFF, 0x1F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
             /* E */ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
             /* F */ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
    }};
    

#if __cplusplus > 201103L
    
    // Comment by Hide (2018-MAY-27)
    // Validation of the above array in an intuitive way with static_assert
    // This only works with C++14, but was confirmed to work with -std=c++14.
    
    static_assert( eightToFive.at(0x6A) ==  0, "Invalid 8to5 (0)" );
    static_assert( eightToFive.at(0x6C) ==  1, "Invalid 8to5 (1)" );
    static_assert( eightToFive.at(0x71) ==  2, "Invalid 8to5 (2)" );
    static_assert( eightToFive.at(0x72) ==  3, "Invalid 8to5 (3)" );
    static_assert( eightToFive.at(0x74) ==  4, "Invalid 8to5 (4)" );
    static_assert( eightToFive.at(0x8B) ==  5, "Invalid 8to5 (5)" );
    static_assert( eightToFive.at(0x8D) ==  6, "Invalid 8to5 (6)" );
    static_assert( eightToFive.at(0x8E) ==  7, "Invalid 8to5 (7)" );
    static_assert( eightToFive.at(0x93) ==  8, "Invalid 8to5 (8)" );
    static_assert( eightToFive.at(0x95) ==  9, "Invalid 8to5 (9)" );
    static_assert( eightToFive.at(0x96) == 10, "Invalid 8to5 (10)" );
    static_assert( eightToFive.at(0x99) == 11, "Invalid 8to5 (11)" );
    static_assert( eightToFive.at(0x9A) == 12, "Invalid 8to5 (12)" );
    static_assert( eightToFive.at(0x9C) == 13, "Invalid 8to5 (13)" );
    static_assert( eightToFive.at(0xA3) == 14, "Invalid 8to5 (14)" );
    static_assert( eightToFive.at(0xA5) == 15, "Invalid 8to5 (15)" );
    static_assert( eightToFive.at(0xA6) == 16, "Invalid 8to5 (16)" );
    static_assert( eightToFive.at(0xA9) == 17, "Invalid 8to5 (17)" );
    static_assert( eightToFive.at(0xAA) == 18, "Invalid 8to5 (18)" );
    static_assert( eightToFive.at(0xAC) == 19, "Invalid 8to5 (19)" );
    static_assert( eightToFive.at(0xB1) == 20, "Invalid 8to5 (20)" );
    static_assert( eightToFive.at(0xB2) == 21, "Invalid 8to5 (21)" );
    static_assert( eightToFive.at(0xB4) == 22, "Invalid 8to5 (22)" );
    static_assert( eightToFive.at(0xC3) == 23, "Invalid 8to5 (23)" );
    static_assert( eightToFive.at(0xC5) == 24, "Invalid 8to5 (24)" );
    static_assert( eightToFive.at(0xC6) == 25, "Invalid 8to5 (25)" );
    static_assert( eightToFive.at(0xC9) == 26, "Invalid 8to5 (26)" );
    static_assert( eightToFive.at(0xCA) == 27, "Invalid 8to5 (27)" );
    static_assert( eightToFive.at(0xCC) == 28, "Invalid 8to5 (28)" );
    static_assert( eightToFive.at(0xD1) == 29, "Invalid 8to5 (29)" );
    static_assert( eightToFive.at(0xD2) == 30, "Invalid 8to5 (30)" );
    static_assert( eightToFive.at(0xD4) == 31, "Invalid 8to5 (31)" );
    
#endif


    /** Conversion function 8bit->5bit, with a simple input validation */
    static uint8_t to5bit( uint8_t );

    
    /** Concrete implementations */
    using CommandFunc = void(*)( Rd53aEmu* );
    using TriggerFunc = void(*)( Rd53aEmu*, const uint8_t, const uint8_t );

    inline static void doNoop        ( Rd53aEmu* );
    inline static void doECR         ( Rd53aEmu* );
    inline static void doBCR         ( Rd53aEmu* );
    inline static void doZero        ( Rd53aEmu* );
    inline static void doSync        ( Rd53aEmu* );
    inline static void doGlobalPulse ( Rd53aEmu* );
    inline static void doWrReg       ( Rd53aEmu* );
    inline static void doRdReg       ( Rd53aEmu* );
    inline static void doCal         ( Rd53aEmu* );
    inline static void doDump        ( Rd53aEmu* );
    
    inline static void doTrigger     ( Rd53aEmu*, const uint8_t /*pattern*/, const uint8_t /*tag*/ );

    
    /** Container of the command and trigger (static) functions
        as function tables.
     */
    static const std::unordered_map<enum Commands, CommandFunc> commandFuncs;
    static const std::unordered_map<enum Triggers, uint8_t>     triggerPatterns;

    std::map<enum Triggers, unsigned> triggerCounters;
    std::map<unsigned, unsigned>      triggerTagCounters;

    /** This part of WrReg process will run asynchronously with threads
        Concurrent running helps speed-up a little.
     */
    static void writeRegAsync( Rd53aEmu*, const uint16_t /*data*/, const uint32_t /*address*/);
    
    
    /** This part of trigger process will run asynchronously with threads
        Concurrent running helps speed-up a little.
     */
    void triggerAsync0( const uint32_t /*tag*/);
    void triggerAsync1( const uint32_t /*tag*/, const unsigned /*coreCol*/);
    void triggerAsync2( const uint32_t /*tag*/, const unsigned /*coreCol*/, const unsigned /*coreRow*/);


    /** Parameters for analog FE */
    static constexpr float capacitance_times_coulomb = 8000; // change this to the correct value later
    static constexpr float maximum_injection_voltage = 1.2;
    static constexpr float lin_maximum_global_threshold_voltage = 1.2; // what should this actually be?
    static constexpr float diff_maximum_global_threshold_voltage = 1.2; // what should this actually be?
    

    
    //////////////////////////////////////////////////////////////////////////////////////////////////
    //
    // Non-static part
    //
    // In principle, the following parts can be all encapsulated by the pImpl indiom:
    //
    // class Impl;
    // std::unique_ptr<Impl> m_impl;
    // 


    
    //////////////////////////////////////////////////////////////////////////////////////////////////
    //
    // Pixel geometries
    //

    PixelCoreArray<n_coreCols, n_coreRows> m_coreArray;


    
    //////////////////////////////////////////////////////////////////////////////////////////////////
    //
    // Communication interface
    //

    
    /** ToDo
        ownership of these ring buffers need to be designed and revisited
    */
    
    RingBuffer * m_txRingBuffer;
    RingBuffer * m_rxRingBuffer;

    
    /** This variable stream holds input commands
        as a format of 16bit data FIFO queue.
        Once the command is digested, the words are
        removed from the queue by pop_front().
    */
    std::deque<uint16_t> commandStream;
    

    /**
     * Temporary output word candidate storatege
     */

    std::mutex queue_mutex;
    std::condition_variable condition;
    
    // first = trigger tag, second = output words
    std::map< uint32_t, std::array<uint32_t, 400*192/4> > outWords;
    std::deque<uint32_t> outTags;
    //std::vector<uint32_t> outWords;
    

    /** Emulator receives commands from the Ring buffer by 32bit words.
        This function pushes back the command words as a format of
        16bit data to the deque defined above.
    */
    void retrieve();

    // functions for dealing with sending data to yarr
    void pushOutput(uint32_t value);
    
    

    //////////////////////////////////////////////////////////////////////////////////////////////////
    //
    // Internal states
    //

    /** Rd53a configuration
        This is the emulation of the chip hardware global/pixel registers.
        Not supposed to be shared with other instances ==> unique_ptr.
     */
    std::unique_ptr<Rd53aCfg> m_feCfg;

    
    /**
     * Chip-internal state: timing counter after CAL
     */

    unsigned calTiming;
    unsigned injectTiming;
    uint32_t l1id;
    uint32_t bcid;


    /** log level control */
    bool verbose  { false };

    
    /** container for async processing */
    std::unique_ptr<ThreadPool>     m_pool;
    std::unique_ptr<ThreadPool>     m_pool2;
    std::vector<std::future<void> > m_async;
    
    
    /** Temporary used to keep records of hits, */
    std::unique_ptr<Histo2d> analogHits;


    /**
    * temporary counters to count hits
    * using atomic for concurrency
    */
    std::atomic<int> totalDigitalHits;
    std::atomic<int> diffAnalogHits;
    std::atomic<int> linAnalogHits;
    std::atomic<int> syncAnalogHits;
    

    
    //////////////////////////////////////////////////////////////////////////////////////////////////
    //
    // Analog FE details
    //

    /** Analog FE calculations */
    
    uint8_t calculateToT( Rd53aLinPixelModel&  /*analogFE*/ );
    uint8_t calculateToT( Rd53aDiffPixelModel& /*analogFE*/ );
    //uint8_t calculateToT( Rd53aSyncPixelModel& /*analogFE*/ ); // To be implemented later
    

    template<class PIXEL>
    void calculateSignal( anytype& pixel, const uint32_t coreCol, const uint32_t coreRow, const uint32_t subCol, const uint32_t subRow, uint32_t tag ) {
        
        auto& model    = pixel.getVar<PIXEL>();
        auto& reg      = model.m_register;
        auto& analogFE = model.m_analogFEModel;

        // See Manual Table 30 (p.72) for the behavior of the pixel register
        // Bit [0]   : pixel power or enable
        // Bit [1]   : injection enable
        // Bit [2]   : Hitbus enable
        // Bit [3]   : TDAC sign   (only for Diff)
        // Bit [4-7] : TDAC b[0-3] (only for Diff)
                
        if( !( reg & 0x1 >>0 ) ) return;
        if( !( reg & 0x2 >>1 ) ) return;

        formatWords( coreCol, coreRow, subCol, subRow, calculateToT( analogFE ), tag );
    }


    /**
     * This function creates the encoded hit words
     * and store it to the temporary output buffer
     */
    void formatWords( const uint32_t /*coreCol*/, const uint32_t /*coreRow*/, const uint32_t /*subcol*/, const uint32_t /*subrow*/, uint32_t /*ToT*/, uint32_t /*tag*/ );
    
};

#endif //__RD53A_EMU_H__
