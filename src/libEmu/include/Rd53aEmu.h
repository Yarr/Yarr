#ifndef __RD53A_EMU_H__
#define __RD53A_EMU_H__

#define HEXF(x,y) std::hex << "0x" << std::hex << std::setw(x) << std::setfill('0') << static_cast<int>(y) << std::dec

#include "Rd53aCfg.h"
#include "Rd53aPixelCfg.h"

#include "EmuShm.h"
#include "RingBuffer.h"
#include "Gauss.h"

#include "Rd53aLinPixelModel.h"
#include "Rd53aDiffPixelModel.h"

#include "FrontEndGeometry.h"
#include "json.hpp"

#include "Histo1d.h"
#include "Histo2d.h"

#include "RingBuffer.h"

#include <cstdint>
#include <memory>
#include <future>
#include <atomic>

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
     * Data generation and output is not implemented yet.
     *
     * The image of the data flow is as the following diagram:
     *
     *
     *                                              +--> commandFunc (e.g. ECR) -->--+
     *                                [Rd53aEmu]    |                                |
     * (Tx ring buffer) -->retrieve()--> (stream) --+--> commandFunc (e.g. Cal) -->--+--> (outStream) -->pushOutput()-->(Rx ring buffer)
     *                                              |                                |
     *                                              +--> commandFunc (e.g. Trg) -->--+
     *                                              |                                |
     *                                              +-->          ....          -->--+
     */
    
public:

    /** The ownsership of these ring buffers need to be designed properly */
    Rd53aEmu(RingBuffer * rx, RingBuffer * tx);
    ~Rd53aEmu();
    
    // the main loop which recieves commands from yarr
    void executeLoop();
    
    // functions for dealing with sending data to yarr
    void pushOutput(uint32_t value);
    
    volatile bool run;
    
private:

    //////////////////////////////////////////////////////////////////////////////////////////////////
    //
    // Static part
    //
    
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

    
    /** List of the first 8-bit of the trigger command word */
    enum class Triggers {
        Trg01 = 0x2b, Trg02 = 0x2d, Trg03 = 0x2e, Trg04 = 0x33, Trg05 = 0x35, Trg06 = 0x36, Trg07 = 0x39, Trg08 = 0x3a,
        Trg09 = 0x3c, Trg10 = 0x4b, Trg11 = 0x4d, Trg12 = 0x4e, Trg13 = 0x53, Trg14 = 0x55, Trg15 = 0x56
    };
    

    /** List of the 16-bit command words */
    enum class Commands {
        ECR = 0x5a5a, BCR = 0x5959, GlobalPulse = 0x5c5c, Cal = 0x6363,
        WrReg = 0x6666, RdReg = 0x6565, Noop = 0x6969, Sync = 0x817e, Zero = 0x0000, Dump = 0xffff
    };


    /** Concrete implementations */
    using CommandFunc = void(*)( Rd53aEmu* );

    static void doNoop        ( Rd53aEmu* );
    static void doECR         ( Rd53aEmu* );
    static void doBCR         ( Rd53aEmu* );
    static void doZero        ( Rd53aEmu* );
    static void doSync        ( Rd53aEmu* );
    static void doGlobalPulse ( Rd53aEmu* );
    static void doWrReg       ( Rd53aEmu* );
    static void doRdReg       ( Rd53aEmu* );
    static void doCal         ( Rd53aEmu* );
    static void doDump        ( Rd53aEmu* );
    static void doTrigger     ( Rd53aEmu* );
    
    
    /** Container of the command and trigger (static) functions
        as function tables.
     */
    static const std::map<enum Commands, CommandFunc> commandFuncs;
    static const std::map<enum Triggers, CommandFunc> triggerFuncs;
    

    /** This part of WrReg process will run asynchronously with threads
        Concurrent running helps speed-up a little.
     */
    static void writeRegAsync( Rd53aEmu*, const uint32_t /*data*/, const uint32_t /*address*/);
    
    
    /** This part of trigger process will run asynchronously with threads
        Concurrent running helps speed-up a little.
     */
    static void triggerAsync( Rd53aEmu*, const unsigned /*dc*/);


    //////////////////////////////////////////////////////////////////////////////////////////////////
    //
    // Non-static part
    //
    // In principle, the following parts can be all encapsulated by the pImpl indiom:
    //
    // class Impl;
    // std::unique_ptr<Impl> m_impl;
    // 


    /** ToDo
        ownership of these ring buffers need to be designed and revisited
    */
    RingBuffer * m_txRingBuffer;
    RingBuffer * m_rxRingBuffer;
    
    /** Rd53a configuration
        This is the emulation of the chip hardware global/pixel registers.
        Not supposed to be shared with other instances ==> unique_ptr.
     */
    std::unique_ptr<Rd53aCfg> m_feCfg;
    

    /** This variable mimicks the hardware's reigsrer for each pixel.
        First loop is for column, second loop is rows.
     */
    std::array< std::array<uint8_t, Rd53aPixelCfg::n_Row>, Rd53aPixelCfg::n_Col> m_pixelRegisters;

    
    /** It's unclear if we need such a large number of instances
        and these realistically needs to work as each pixel's emulation...
        Can't we think of flyweight design patter??
    */
    std::vector<std::vector< std::unique_ptr<Rd53aLinPixelModel> > >  m_rd53aLinPixelModelObjects;
    std::vector<std::vector< std::unique_ptr<Rd53aDiffPixelModel> > > m_rd53aDiffPixelModelObjects;

    
    /** This variable stream holds input commands
        as a format of 16bit data FIFO queue.
        Once the command is digested, the words are
        removed from the queue by pop_front().
    */
    std::deque<uint16_t> stream;

    
    /** Emulator receives commands from the Ring buffer by 32bit words.
        This function pushes back the command words as a format of
        16bit data to the deque defined above.
    */
    void retrieve();
    

    /** log level control */
    bool verbose  { false };

    
    /** container for async processing */
    std::vector<std::future<void> > m_async;
    
    
    /** Temporary used to keep records of hits,
        but we should rather ship-out data to software.
        ==> To be removed?
    */
    Histo1d* linScurve[136][Rd53aPixelCfg::n_Row];
    Histo1d* linThreshold;
    
    Histo1d* diffScurve[136][Rd53aPixelCfg::n_Row];
    Histo1d* diffThreshold;
    
    Histo2d* analogHits;
    

    /**
    * temporary counters to count hits
    * using atomic for concurrency
    */
    std::atomic<int> totalDigitalHits;
    std::atomic<int> diffAnalogHits;
    std::atomic<int> linAnalogHits;
    std::atomic<int> syncAnalogHits;
    

};

#endif //__RD53A_EMU_H__
