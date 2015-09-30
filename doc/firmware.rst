Firmware
===============

Dependencies
---------------
    - Xilinx ISE 14.7 needs to be installed on the machine
    - Source the setup file of ISE:

    .. code-block:: none
        :linenos:
    
         [theim@lambda ~]$ source /opt/Xilinx/14.7/ISE_DS/settings64.sh 
         . /opt/Xilinx/14.7/ISE_DS/common/.settings64.sh /opt/Xilinx/14.7/ISE_DS/common
         . /opt/Xilinx/14.7/ISE_DS/EDK/.settings64.sh /opt/Xilinx/14.7/ISE_DS/EDK
         . /opt/Xilinx/14.7/ISE_DS/PlanAhead/.settings64.sh /opt/Xilinx/14.7/ISE_DS/PlanAhead
         . /opt/Xilinx/14.7/ISE_DS/ISE/.settings64.sh /opt/Xilinx/14.7/ISE_DS/ISE

    - Replace /opt/Xilinx/14.7/ISE_DC with the path to your ISE installation

Create Makefile
---------------
    - Install hdlmake from http://www.ohwr.org/projects/hdl-make/wiki and create alias to execute

    .. code-block:: none
        :linenos:
        
        alias hdlmake='python path/to/hdl-make/hdlmake/__main__.py'

    - Specify the correct ucf-file in Yarr/hdl/syn/Manifest.py
    - Create Makefile (xilinx environment needs to be sourced)

    .. code-block:: none
        :linenos:

        cd Yarr/hdl/syn
        hdlmake
        <Lots of colorful text>
        make
        <A lot more text>

    - Look in the output that the synthesis was successful without errors:

    .. code-block:: none
        :linenos:

     All constraints were met.
     [..]
     Placer: Placement generated during map.
     Routing: Completed - No errors found.
     Timing: Completed - No errors found.

     Number of error messages: 0
     Number of warning messages: 12
     Number of info messages: 1

     Writing design to file yarr.ncd
     [...]
     Process "Generate Programming File" completed successfully
     INFO:TclTasksC:1850 - process run : Generate Programming File is done.

     - The bit-file will be called *yarr.bit*


