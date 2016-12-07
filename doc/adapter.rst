Adapter Boards
================

A list of adapter boards developed specifically for the usage with YARR.
For more information (schematics, layout and design files) please see: https://twiki.cern.ch/twiki/bin/view/Main/TimonHeim?forceShow=1

FE-I4
---------------

Quad RJ45 rev. A
^^^^^^^^^^^^^^^

    .. image:: _static/yarr_quad_fei4_revA.png
        :width: 500px

    - FMC-LPC adapter board with four RJ45 connectors with custom FE-I4 pinout
    - LVDS lines buffered by a TI DS90LV004
    - One Lemo connector
    - Issues:
        - RJ45 connectors on the wrong side, problematic in a PC
        - Lemo connector facing towards motherboard
        - Only LVCMOS 2.5 avaiable on LEMO
    - Use firmware: ``hdl/syn/yarr_quad_fei4_revA.bit``

Quad RJ45 rev. B
^^^^^^^^^^^^^^^

    .. image:: _static/yarr_quad_fei4_revB.jpg
        :width: 500px


    - FMC-LPC adapter board with four RJ45 connectors with custom FE-I4 pinout
    - LVDS lines buffered by a TI DS90LV004
    - Three Lemo connector with signals via through a TI SN74LVCC3245A
    - NTC lines of FE-I4 can be read out with ADC
    - Issues:
        - Signal direction on all Lemos the same
        - ADC SPI not connected to FMC
    - Use firmware: ``hdl/syn/yarr_quad_fei4_revB.bit``

FE65-P2
---------------

FE65-P2 Adapter Card rev. B
^^^^^^^^^^^^^^
    
    .. image:: _static/fe65p2_adapter_revB.jpg
        :width: 500px


    - To be connected via FMC to VHDCI adapter
    - Issues:
        - Injection circuit only capable of going up to 0.6 V
    - Use firmware: ``yarr_fe65p2_revB.bit``

