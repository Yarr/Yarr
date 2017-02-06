Emulator
=====================

Scan Console
---------------------

    - Main scan execution program
    - Command line parameters:
        - "-s <scan name>": Sets the scan which should be performed, to see which scans are supported see below
        - "-r <file>" : Controller config file, provide emulator config to use emulator e.g. config/emuCfg.json
        - "-c <file1> [<file2> ...]": Chip config files. If file does not exist, new config from default is created. Rx and Tx Channel are chosen to be the next available channel
        - "-p": Enable plot output (optional)
        - "-o <directory>": Specify plot output directory (default ./data/)
        - "-n <number>": Specify board number (>= 30 for emulator)

Emulator
---------------------
    - The Scan Console program will interact with the emulator, which is started in a seperate thread inside the main program.

Supported Scans
^^^^^^^^^^^^^^^^^^^^^
    - *digitalscan* : Performs a digital scan, testing the digital functionality and excluding the analog pixel region
    - *analogscan* : Performs an analog scan, testing the analog pixel region by injecting a high charge
    - *totscan* : Injects a specific charge (def. 16000e) and measures the ToT response of each pixel
    - *thresholdscan* : Determines the threshold of each pixel by measuring its s-curve
    - *tune_globalthreshold* : Calibrates the global threshold DAC to the target value (def. 1500e)
    - *tune_pixelthreshold* : Calibrates the per pixel threshold DAC to the target value (def. 1500e)

Not yet supported Scans
^^^^^^^^^^^^^^^^^^^^^^^
    - *tune_globalpreamp* : Calibrates the global feedback current DAC to give a certain ToT response for a specific charge (def. 10 ToT bc @ 16000e)
    - *tune_pixelpreamp* : Calibrates the per pixel feedback current DAC to give a certain ToT reponse for a specific charge (def. 10 ToT bc @ 16000e)
    - *noisescan* : Sets the chip in run configuration and sends trigger with a high frequency (def. 1 MHz) for a certain amount of time (def. 30 s) and masks all pixels which have an occupancy higher than 10e-6 per trigger

Example Scan:
^^^^^^^^^^^^^^^^^^^^^

    The simplest way to run a scan is using the helper script::

        $ bin/scanConsole -r emuCfg.json -c configs/test.json -s digitalscan -p
        #####################################
        # Welcome to the YARR Scan Console! #
        #####################################
        -> Parsing command line parameters ...
        SPEC Nr: 0
        Scan Type: digitalscan
        Chips: 
        configs/test.json
        Target Threshold: 2500
        Output Plots: 1
        Output Directory: ./data/000306_digitalscan/

        Timestamp: 2017-02-05_21:36:35
        Run Number: 306
        #################
        # Init Hardware #
        #################
        -> Opening controller config: emuCfg.json
        -> Found Emulator config
        -> Starting Emulator
        configs/emu_fe0.json
        Starting emulator loop
        #######################
        ##  Loading Configs  ##
        #######################
        Found FE-I4B: "JohnDoe"
        void Bookkeeper::addFe(FrontEnd *, unsigned int, unsigned int) -> Added FE: Tx(0), Rx(0)

        #################
        # Configure FEs #
        #################
        -> Configuring JohnDoe
        -> All FEs configured in 1 ms !
        -> Setting Tx Mask to: 0x1
        -> Setting Rx Mask to: 0x1

        ##############
        # Setup Scan #
        ##############
        -> Selecting Scan: digitalscan
        -> Found Digital Scan
        -> Running pre scan!
        -> Starting 8 processor Threads:
        -> Processor thread #0 started!
        -> Processor thread #1 started!
        -> Processor thread #2 started!
        -> Processor thread #3 started!
        -> Processor thread #4 started!
        -> Processor thread #5 started!
        -> Processor thread #6 started!
        -> Processor thread #7 started!
        -> Starting histogrammer and analysis threads:
        -> Analysis thread of Fe 0

        ########
        # Scan #
        ########
        -> Starting scan!
        ---> Mask Stage 0
        ---> Mask Stage 1
        ---> Mask Stage 2
        ---> Mask Stage 3
        ---> Mask Stage 4
        ---> Mask Stage 5
        ---> Mask Stage 6
        ---> Mask Stage 7
        ---> Mask Stage 8
        ---> Mask Stage 9
        ---> Mask Stage 10
        ---> Mask Stage 11
        ---> Mask Stage 12
        ---> Mask Stage 13
        ---> Mask Stage 14
        ---> Mask Stage 15
        ---> Mask Stage 16
        ---> Mask Stage 17
        ---> Mask Stage 18
        ---> Mask Stage 19
        ---> Mask Stage 20
        ---> Mask Stage 21
        ---> Mask Stage 22
        ---> Mask Stage 23
        ---> Mask Stage 24
        ---> Mask Stage 25
        ---> Mask Stage 26
        ---> Mask Stage 27
        ---> Mask Stage 28
        ---> Mask Stage 29
        ---> Mask Stage 30
        ---> Mask Stage 31
        -> Scan done!
        -> Waiting for processors to finish ...
        -> Processor done, waiting for analysis ...
        -> All done!

        ##########
        # Timing #
        ##########
        -> Configuration: 1 ms
        -> Scan:          2803 ms
        -> Processing:    55 ms
        -> Analysis:      55 ms

        ###########
        # Cleanup #
        ###########
        -> Saving config of FE JohnDoe to configs/test.json
        -> Plotting histograms of FE 0
        Plotting : EnMask
        Warning: empty cb range [1:1], adjusting to [0.99:1.01]
        Plotting : OccupancyMap
        Warning: empty cb range [100:100], adjusting to [99:101]
        Plotting : L1Dist
        Saving : EnMask
        Saving : OccupancyMap
        Saving : L1Dist
        libc++abi.dylib: terminating
        Abort trap: 6
