FE-I4 Operation
=====================

Scan Console
---------------------

    - Main scan execution program
    - Command line parameters:
        - "-s <scan name>": Sets the scan which should be performed, to see which scans are supported see below
        - "-c <filename>": Global config file, for formatting see below
        - "-p": Enable plot output (optional)
        - "-o <directory>": Specify plot output directory (optional)
    
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

