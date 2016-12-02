FE-I4 Operation
=====================

Scan Console
---------------------

    - Main scan execution program
    - Command line parameters:
        - "-s <scan name>": Sets the scan which should be performed, to see which scans are supported see below
        - "-c <file1> [<file2> ...]": Chip config files. If file does not exist, new config from default is created. Rx and Tx Channel are chosen to be the next available channel
        - "-p": Enable plot output (optional)
        - "-o <directory>": Specify plot output directory (default ./data/)
    
Supported Scans
^^^^^^^^^^^^^^^^^^^^^
    - *digitalscan* : Performs a digital scan, testing the digital functionality and excluding the analog pixel region
    - *analogscan* : Performs an analog scan, testing the analog pixel region by injecting a high charge
    - *totscan* : Injects a specific charge (def. 16000e) and measures the ToT response of each pixel
    - *thresholdscan* : Determines the threshold of each pixel by measuring its s-curve
    - *tune_globalthreshold* : Calibrates the global threshold DAC to the target value (def. 1500e)
    - *tune_pixelthreshold* : Calibrates the per pixel threshold DAC to the target value (def. 1500e)
    - *tune_globalpreamp* : Calibrates the global feedback current DAC to give a certain ToT response for a specific charge (def. 10 ToT bc @ 16000e)
    - *tune_pixelpreamp* : Calibrates the per pixel feedback current DAC to give a certain ToT reponse for a specific charge (def. 10 ToT bc @ 16000e)
    - *noisescan* : Sets the chip in run configuration and sends trigger with a high frequency (def. 1 MHz) for a certain amount of time (def. 30 s) and masks all pixels which have an occupancy higher than 10e-6 per trigger

Example Scan:
^^^^^^^^^^^^^^^^^^^^^

    .. code-block:: none
        $ bin/scanConsole -s analogscan -c configs/test.json -p
        #####################################
        # Welcome to the YARR Scan Console! #
        #####################################
        -> Parsing command line parameters ...
         SPEC Nr: 0
         Scan Type: analogscan
        Chips: 
            configs/test.json
         Target Threshold: 2500
         Output Plots: 1
         Output Directory: ./data/

        Timestamp: 2016-12-02_08:40:44
        Run Number: 1
        #################
        # Init Hardware #
        #################
        -> Init SPEC 0 : 
        void SpecController::init() -> Opening SPEC with id #0
        void SpecController::init() -> Mapping BARs
        void SpecController::init() -> Mapped BAR0 at 0x0x7f3edff3e000 with size 0x100000
        void SpecController::init() -> Mapped BAR4 at 0x0x7f3ee0059000 with size 0x1000
        void SpecController::configure() -> Configuring GN412X
        #######################
        ##  Loading Configs  ##
        #######################
        Found FE-I4B: "JohnDoe"
        void Bookkeeper::addFe(FrontEnd*, unsigned int, unsigned int) -> Added FE: Tx(0), Rx(0)

        #################
        # Configure FEs #
        #################
        -> Configuring JohnDoe
        -> All FEs configured in 33 ms !
        -> Setting Tx Mask to: 0x1
        -> Setting Rx Mask to: 0x1

        ##############
        # Setup Scan #
        ##############
        -> Selecting Scan: analogscan
        -> Found Analog Scan
        -> Running pre scan!
        -> Starting 4 processor Threads:
          -> Processor thread #0 started!
          -> Processor thread #1 started!
          -> Processor thread #2 started!
          -> Processor thread #3 started!
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
        -> Configuration: 33 ms
        -> Scan:          806 ms
        -> Processing:    25 ms
        -> Analysis:      129 ms

        ###########
        # Cleanup #
        ###########
        -> Saving config of FE JohnDoe to configs/test.json
        -> Plotting histograms of FE 0
        Plotting : EnMask
        Plotting : OccupancyMap
        Plotting : L1Dist
        Saving : EnMask
        Saving : OccupancyMap
        Saving : L1Dist
    
Global configuration
^^^^^^^^^^^^^^^^^^^^^
    - The global configuration should contain the following information for each attached chip per line:
        - Name <string> : Name/Id of chip (not allowed to contain whitespace), e.g. DC56874 or Mary
        - ChipId <unsigned> : Chip Id as specified by the wirebonds attached to the chip, typically 0 for single-chip cards, 7 for single-chip modules and 7/6 foer double-chip modules
        - Tx link <unsigned> : Channel in the FPGA the CMD stream is transmitted through
        - Rx link <unsigned> : Channel data from the FE is received in the FPGA
        - Config <string> : Path to file and filename of config file for this chip, is created if it does not exist
    - N.B.: Tx and Rx link are always the same for single chip modules, but can one Tx link is shared by a double chip module. In porper definition of the Tx link will lead to the chip not being properly configured and giving wrong scan results (Scans are run with broadcast)
    - Example:

    .. code-block:: none
        
        Huey    7   0   0   configs/Huey.cfg
        # A comment starts with a #, inline comments do not work
        Dewey   7   1   1   configs/Dewey.cfg
        Louie   6   1   2   configs/Louie.cfg

    - Here Dewey and Louie are a double-chip module sharing the CMD line in TX link 1

Tuning
^^^^^^^^^^^^^^^^^^^^^
    - A chip can be tuned by executing the tuning scans in a specific order, e.g.:
        - digitalscan
        - analogscan
        - tune_globalthreshold
        - tune_globalpreamp
        - tune_pixelthreshold
        - tune_globalpreamp
        - tune_pixelpreamp
        - tune_pixelthreshold
        - noisescan
        - thresholdscan
        - totscan

    - This order will ensure porper tuning of per pixel threshold and preamp and run two verification scans at the end

