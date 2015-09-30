Adapter Boards
================

A list of adapter boards developed specifically for the usage with YARR

FE-I4
---------------

Quad RJ45 rev. A
^^^^^^^^^^^^^^^

    .. image:: _static/yarr_quad_fei4_revA.png

    - FMC-LPC adapter board with four RJ45 connectors with custom FE-I4 pinout
    - LVDS lines buffered by a TI DS90LV004
    - One Lemo connector
    - Issues:
        - RJ45 connectors on the wrong side, problematic in a PC
        - Lemo connector facing towards motherboard
        - Only LVCMOS 2.5 avaiable on LEMO
    - Files:
        - Layout (pdf)
        - Schematic (pdf)
        - Eagel design files

Quad RJ45 rev. B
^^^^^^^^^^^^^^^

    .. image:: _static/yarr_quad_fei4_revB.png

    - FMC-LPC adapter board with four RJ45 connectors with custom FE-I4 pinout
    - LVDS lines buffered by a TI DS90LV004
    - Three Lemo connector with signals via through a TI SN74LVCC3245A
    - NTC lines of FE-I4 can be read out with ADC
    - Issues:
        - Signal direction on all Lemos the same
    - Files:
        - Layout (pdf)
        - Schematic (pdf)
        - Eagel design files
