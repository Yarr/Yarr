# Software Installation

Adding BDAQ support to the Yarr software requires one extra dependency and enabling the BDAQ controller for compilation. To achieve this, follow the further instructions.

# Dependencies

1. Install the typical Yarr software dependencies [install.md#dependencies]
1. Install the following extra dependency

```bash
$ sudo yum install boost-devel
```

# Get the appropriate software version

BDAQ support is either on the devel branch or on appropriate tag.

```bash
$ git clone -b devel https://github.com/Yarr/Yarr.git Yarr
```

# Compile the software

```bash
$ cd Yarr/
$ mkdir build
$ cd build
$ cmake3 "-DYARR_CONTROLLERS_TO_BUILD=Spec;Emu;Bdaq" ../
```

# Firmware

The BDAQ hardware must have one of the following firmwares deployed to its FPGA:

- **Recommended:** [1.2.0_BDAQ53_1LANE_RX640] 
(https://gitlab.cern.ch/silab/bdaq53/uploads/936860f3e449cb8cd1a8fecc4f215318/1.2.0_BDAQ53_1LANE_RX640.tar.gz)
- [...](...)

# Connectivity

...