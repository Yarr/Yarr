YARR: Yet Another Rapid Readout
===================

What is YARR?
-------------------------

YARR is a readout system based around the concept of moving intelligence from the FPGA firmware into the host computer software. The role of the FPGA is the one of a reconfigurable I/O interface and not anymore the one of a hardware accelerator. To move the raw data into the host memory a fast bus is needed, common to all modern computers is the high-speed PCIe bus a natural interface choice. The PCIe card, the SPEC board, has been developed by the CERN beams department and is available from multiple vendors.

Folder Structure
------------------------
.. code-block:: none

    Yarr
    |-- doc : Documentation
    |-- hdl : Firmware
    |    |-- common
    |    |-- ddr3-core
    |    |-- gn4124-core
    |    |-- ip_cores
    |    |-- rx_core
    |    |-- syn : Firmware synthesis tools
    |    |-- tx_core
    |-- src : Software
    |    |-- bin
    |    |-- build
    |    |-- kernel : Custom kernel driver
    |    |-- lib
    |    |-- libFei4
    |    |-- libSpec
    |    |-- libUtil
    |    |-- libYarr
    |    |-- tools : Command line tools
    |    |-- util : Scripts
    |-- gui: QT5 based GUI
          |-- YarrGui
          |-- bin
          |-- build

Content:
-------------------------
.. toctree::
   
   install
   firmware
   adapter
   fei4
