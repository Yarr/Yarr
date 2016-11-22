EUDAQ Operation
=====================

Installation
---------------------
    - Compile YARR producer:

    .. code-block:: none
        :linenos:

        $ cd path/to/yarr/eudet
        $ make

    - Copy YARR converter plugin to EUDAQ and recompile EUDAQ:

    .. code-block:: none
        :linenos:

        $ cd path/to/yarr/eudet
        $ scp eudaq/YARRConverterPlugin.cc <user>@<host>:/path/to/eudaq/main/lib/plugins/.
        $ ssh <user>@<host>
        $ cd /path/to/eudaq/
        $ cmake build
        $ make -C build install

    - Make sure to program the new bit file "yarr_fe65p2_revC.bit" into the FPGA

Operation
---------------------
    - Place module config into /path/to/yarr/eudet/config
    - Config name is supposed to be <name>.json
    - Start run control
    - Start producer:

    .. code-block:: none
        :linenos:

        $ cd path/to/yarr/eudet
        $ bin/yarr_fe65p2_producer -r <RC ip address> -n <module name>
    
    - During each RC configuration step, the module is being reconfigured
    - At the end of each run the raw data and some histograms are being saved in path/to/yarr/eudet/data
