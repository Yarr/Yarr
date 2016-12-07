Installation
====================

Setting up your machine
-------------------
    - Make sure you have GCC version 4.8 or higher installed:

    .. code-block:: none
        :linenos:

        $ g++ --version
        g++ (GCC) 4.8.3 20140911 (Red Hat 4.8.3-9)
        Copyright (C) 2013 Free Software Foundation, Inc.
        This is free software; see the source for copying conditions.  There is NO
        warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

    - If not a specifc version can also be sourced from AFS (CERN AFS access necessary):

    .. code-block:: none
        :linenos:

        $ source /afs/cern.ch/sw/lcg/contrib/gcc/4.8/x86_64-slc6/setup.sh

    - If not installed before, you need some standard packages:

    .. code-block:: none
        :linenos:

        $ sudo yum install gnuplot texlive-epstopdf

Initialise repository
-------------------
    - Clone the repository to your local machine:
    
    .. code-block:: none
        :linenos:

        $ git clone https://github.com/Yarr/Yarr.git Yarr
 
    - The master branch should contain a stable release, the most up-to date version can be found in the devel branch

Compile and load the kernel driver
------------------
    - Compile the kernel driver:
    
    .. code-block:: none
        :linenos:

        $ cd Yarr/src/kernel
        $ make
        <Lots of text>
        $ sudo make install
        <Copying files>
        $ sudo depmod
        <Builds dependencies>
        $ sudo modprobe -v specDriver
        $ insmod /lib/modules/3.10.0-229.14.1.el7.x86_64/extra/specDriver.ko

    - If the kernel driver is properly loaded can be checked with ``dmesg``

    .. code-block:: none
        :linenos:
    
        $ dmesg
        <Lots of text>
        [246519.712618] specDriver: Major 247 allocated to nodename 'spec'
        [246519.712637] specDriver: Found SPEC card at 0000:01:00.0
        [246519.712689] specDriver: 64bits bus master DMA capable
        [246519.712706] specDriver 0000:01:00.0: irq 47 for MSI/MSI-X
        [246519.713400] specDriver: Device /dev/spec0 added
        [246519.713452] specDriver: Mapped BAR0 at 0xF7900000 with length 1048576
        [246519.713495] specDriver: 
        [246519.713496] Mapped BAR2 at 0xF7800000 with length 1048576
        [246519.713498] specDriver: 
        [246519.713499] Mapped BAR4 at 0xF7A00000 with length 4096
        [246519.713526] specDriver: 
        [246519.713527] Module loaded

    - Loading the kernel driver manually is only necessary directly after installation, it will be loaded automatically when the system starts and a SPEC board is installed

    - In case you run into a problem during ``modprobe`` which looks like this: ``modprobe: ERROR: could not insert 'specDriver': Required key not available``. Do the following:

    .. code-block:: none
        :linenos:

        $ sudo yum install mokutil
        $ sudo mokutil -disable-validation

    - This will require you to create a password with at least 8 characters. Reboot after that and the UEFI will ask you to change the security settings. Choose "yes", reenter the password or whatever it asks for.

Compile the software and load the firmware
----------------
    - Compile the software:

    .. code-block:: none
        :linenos:

        $ cd Yarr/src
        $ make
        <Lots of text>

    - Program the FPGA on the SPEC board

    .. code-block:: none
        :linenos:

        $ cd Yarr/src
        $ bin/programFpga ../hdl/syn/yarr_quad_fei4_revB.bit 
        Opening file: ../hdl/syn/yarr_quad_fei4_revB.bit
        Size: 1.41732 MB
        =========================================
        File info:
        Design Name: yarr.ncd;HW_TIMEOUT=FALSE;UserID=0xFFFFFFFF
        Device:      6slx45tfgg484
        Timestamp:   2015/08/25 12:20:08
        Data size:   1486064
        =========================================
        Reading file.
        Opening Spec device.
        void SpecController::init() -> Opening SPEC with id #0
        void SpecController::init() -> Mapping BARs
        void SpecController::init() -> Mapped BAR0 at 0x0x7f5902cd1000 with size 0x100000
        void SpecController::init() -> Mapped BAR4 at 0x0x7f5903deb000 with size 0x1000
        void SpecController::configure() -> Configuring GN412X
        void SpecController::configure() -> MSI needs to be configured!
        Starting programming ...
        int SpecController::progFpga(const void*, size_t) -> Setting up programming of FPGA
        int SpecController::progFpga(const void*, size_t) -> Starting programming!
        int SpecController::progFpga(const void*, size_t) -> Programming done!!
        int SpecController::progFpga(const void*, size_t) -> FCL IRQ: 0x38
        int SpecController::progFpga(const void*, size_t) -> FCL IRQ indicates CONFIG_DONE
        int SpecController::progFpga(const void*, size_t) -> FCL Status: 0x2c
        int SpecController::progFpga(const void*, size_t) -> FCL STATUS indicates SPRI_DONE
        ... done!

    - Look for the flags ``CONFIG_DONE`` and ``SPRI_DONE`` to signal successful programming
    - Four LEDs on the SPEC board (close to the PCIe connector) should blink
    - Which exact bit-file needs to be programmed depends on the usecase, in this example the quad FE-I4 rev. B adapter board is targetted

Test your setup
------------------
    - Communication between all components (Computer, SPEC board, firmware, kernel driver and software) can be tested with a test program:

    .. code-block:: none
        :linenos:

        $ cd Yarr/src
        $ bin/test 
        void SpecController::init() -> Opening SPEC with id #0
        void SpecController::init() -> Mapping BARs
        void SpecController::init() -> Mapped BAR0 at 0x0x7f885e98c000 with size 0x100000
        void SpecController::init() -> Mapped BAR4 at 0x0x7f885eab7000 with size 0x1000
        void SpecController::configure() -> Configuring GN412X
        Starting DMA write/read test ...
        ... writing 8192 byte.
        ... read 8192 byte.
        Success! No errors.

    - If this basic test fails or even freezes the system, support should be seeked. It might be because of incompatible hardware.
    - Other command line tools exist to test and benchmark the system further
        - ``bin/errorCheckDma`` : Transfers 5 GB of random generated data to the FPGA and back to the CPU and checks for erros
        - ``bin/benchmarkSingle`` : Performs a benchmark of the single write/read transfer
        - ``bin/benchmarkDma`` : Performs a benchmark of the DMA write/read transfer

