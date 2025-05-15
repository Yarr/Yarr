# Support for FELIX via FELIX client hardware controller

Felix-star is the latest dataflow architecture of a FELIX system consiting of a collection of applications responsible for data transfers between the FELIX card and the host. Communications with felix-star over the network is managed by NetIO-next. Felix-star and NetIO-next supersedes the previous felixcore and NetIO, respectively. 
More details [here](https://atlas-project-felix.web.cern.ch/atlas-project-felix/user/felix-doc/-/felix-user-manual/Latest/felix-user-manual/Latest/8_felix_star.html).

This hardware controller communicates with felix-star via the `felix-client-thread` API.

## Software setup

### FELIX

See [here](https://atlas-project-felix.web.cern.ch/atlas-project-felix/user/felix-doc/-/felix-user-manual/Latest/felix-user-manual/Latest/5_software_installation.html) and [here](https://atlas-project-felix.web.cern.ch/atlas-project-felix/user/felix-doc/-/felix-user-manual/Latest/felix-user-manual/Latest/9_orchestration.html) for installing FELIX software and running felix-star processes.
For the FELIX client to load properly, the path to `libfelix-client-lib.so` from the FELIX software need be included in `LD_LIBRARY_PATH`.

### YARR

To use the FELIX client controller, add `FelixClient` to the list of hardware controllers to build when running `cmake`: 
```bash
cmake .. -DYARR_CONTROLLERS_TO_BUILD=FelixClient <other_build_options>
```
or bulid with all controllers: `-DYARR_CONTROLLERS_TO_BUILD=all`.

**Note**: as of FELIX software version 4.2.1, the FelixClientThread would crash in its constructor if it is compiled using the GCC from rh devtoolset-9. However, it works fine if using GCC9 from the LCG release. With `cvmfs`, the compiler can be set up by:
```bash
source /cvmfs/sft.cern.ch/lcg/contrib/gcc/9/x86_64-centos7/setup.sh
```

## Usage

An example controller configuration file is provided in `configs/controller/felix_client.json`:
```json
{
  "ctrlCfg": {
    "type": "FelixClient",
    "cfg": {
      "FelixClient": {
        "local_ip_or_interface": "lo",
        "log_level": "info",
        "bus_interface": "<local_interface>",
        "bus_dir": "./bus",
        "bus_group_name": "FELIX",
        "verbose_bus": false,
        "verbose_zyre": false,
        "timeout": 1000,
        "netio_pages": 256,
        "netio_pagesize": 65536
      },
      "ToFLX": {
        "detector_id": 0,
        "connector_id": 0,
        "protocol": 0,
        "flip": false
      },
      "ToHost": {
        "detector_id": 0,
        "connector_id": 0,
        "protocol": 0,
        "flushTime_ms": 50,
        "enable_monitor": true,
        "monitor_interval_ms": 1000,
        "queue_limit_MB": 4000,
        "wait_time_us": 10000
      }
    }
  }
}
```

The fields under `"FelixClient"` are the ones required by the FelixClientThread and are passed to its constructor.
**In particular, the value of `"bus_dir"` must point to the FELIX bus directory created and updated by active felix-star processes.**

Optionally, if a user desires to handle communication with an Optoboard device or standalone LpGBT, an extra field can be added to the controller configuration, as follows:
```json
{
  "ctrlCfg": {
    "type": "FelixClient",
    "cfg": {
      ...
      "OpticalDevices": [
        {
          "version": 1,
          "i2cAddr": 0,
          "devAddr": 116,
          "devPrimaryAddr": 116,
          "type": "lpgbt",
          "txFid": 1152922810278051840,
          "rxFid": 1152922810278543360
        },
        {
          "version": 1,
          "i2cAddr": 0,
          "devAddr": 117,
          "devPrimaryAddr": 116,
          "type": "lpgbt",
          "txFid": 1152922810278051840,
          "rxFid": 1152922810278543360
        }
      ]
    }
  }
}
```
Where each LpGBT or GBCR object can be added under the field `"OpticalDevices"`.  Each parameter is optional and if no value is provided a default is chosen.  Descriptions of each variable and the default values are provided below:

`version` - The version of the device, either LpGBT or GBCR.  [Default = 1]

`i2cAddr` - The i2c acdress of the device.  Note i2c communication is used when multiple optical devices are connected (as in the case of an optoboard), and all communication is sent via the primary device down through the i2c channel to the secondary devices.  [Default = 0]

`devAddr` - The address of the device [Default = 116]

`devPrimaryAddr` - For a system with several devices connected to a single primary device (eg. an optoboard), the address of the primary device through which i2c communication will be used to send and receive data to secondary devices. [Default = 116]

`type` - The type of optical device, acceptable options are "lpgbt" and "gbcr" [Default = "lpgbt"]

`txFid` - The ic fid (unique FELIX ID) to be used for communication in the tx direction [Default = fid for tx = 0]

`rxFid` - The ic fid (unique FELIX ID) to be used for communication in the rx direction [Default = fid for rx = 0]

Note that the tx and rx fids for ic communication can be determined using the functions: 
`FelixTxCore::ic_fid_from_channel(tx_value)` and `FelixRxCore::ic_fid_from_channel(rx_value)`


Assuming e-link 0 and 1 are valid Rx and Tx channels, respectively, and are both enabled:

- To test basic data transmission to and from FELIX:

```bash
bin/testFelixClient configs/controller/felix_client.json -r 0 -t 1
```

- To test basic communication with Star chips:

```bash
bin/test_star configs/controller/felix_client.json -r 0 -t 1 -R
```

- To run scans:

```bash
bin/scanConsole -r configs/controller/felix_client.json -c <connectivity.json> -s <scan_config.json>
```

### Standalone applications

In addition to using the FelixController in scans, a few lightweight executables are also provided for interacting with the FELIX system.

#### [testFelixClient](../src/libFelixClient/app/testFelixClient.cpp)
`testFelixClient` can be used to test basic communications with FELIX.

To run:
```
bin/testFelixClient configs/controller/felix_client.json [OPTIONS...]
```

Available options:
```
 -t <TX_ELINK1> [<TX_ELINK2> ...] : A list of tx elinks for sending data.
 -d <32b HEX WORD> [<32b HEX WORD> ...] : A list of data words in hex format to be sent.
 -f <FILE_NAME> : Name of a file containing data words to be sent. It is expected each line has one 32-bit hex integer.
 -n NUMBER : The number of times to send the data. Default: 1
 -q FREQUENCY : Trigger frequency in Hz. If non-zero, send the data using TxCore::trigger(). Otherwise, send the data using TxCore::releaseFifo(). Default: 0

 -r <RX_ELINK1> [<RX_ELINK2> ...] : A list of rx elinks for receiving data.
 -w SECONDS : Number of seconds to wait for data. Default: 1

 -s : Read FELIX status registers
 -l LOG_CONFIG : Configuration for the logger.
```

Examples:
- Read FELIX status registers:
```
bin/testFelixClient configs/controller/felix_client.json -s
```
- Send some data e.g. 0xdeadbeef to elinks 1, 6, 11, and 16 twice:
```
bin/testFelixClient configs/controller/felix_client.json -t 1 6 11 16 -d 0xdeadbeef -n 2
```
- Receive data from elinks 0 and 2 for 5 seconds:
```
bin/testFelixClient configs/controller/felix_client.json -r 0 2 -w 5
```

#### [felixRegister](../src/libFelixClient/app/felixRegister.cpp)
`felixRegister` is an application to read and write FELIX registers.

To read FELIX registers:
```
bin/felixRgister configs/controller/felix_client.json REGISTER_NAME [OPTIONS...]
```

To write FELIX registers:
```
bin/felixRgister configs/controller/felix_client.json REGISTER_NAME REGISTER_VALUE [OPTIONS...]
```

Available options:
```
 -l LOG_CONFIG : Configuration for the logger.
```

Examples:
- Read the FELIX register FIRMWARE_MODE:
```
bin/felixRegister configs/controller/felix_client.json FIRMWARE_MODE
```
- Write 0xabcd to FELIX register BROADCAST_ENABLE_00:
```
bin/felixRegister configs/controller/felix_client.json BROADCAST_ENABLE_00 0xabcd
```

#### [elinkConfig](../src/libFelixClient/app/elinkConfig.cpp)
`elinkConfig` can be used to check and configure elink settings.

To run:
```
bin/elinkConfig COMMAND configs/controller/felix_client.json [OPTIONS...]
```

`COMMAND` can be one of the following:
- `get`:  Check if all e-links specified in options are enabled.
- `set`: Enable all e-links specified in options.
- `off`: Disable all e-links.

Available options:
```
 -t TX_CHANNELS : A list of Tx channels to be configured.
 -r RX_CHANNELS : A list of Rx channels to be configured.
 -c CHIP_CONFIG : Connectivity configuration file.
 -b BANDWIDTH : Rx bandwidth in Mbps. If provided, check or set the Rx elink width.
 -e : Channels specifeid in options are enabled exclusively. All other channels are disabled.
 -I : Include IC channels
 -E : Include EC channels
 -l LOG_CONFIG : Configuration for the logger.
 -v : Verbose mode. Set logging level to 'debug'. Overwritten by '-l LOG_CONFIG' if a logging configuration is provided.
```
Note that in case the option `-e` is used with the `set` (`get`) command, elinks specified via the options `-t` and `-r` will be enabled (checked if they are enabled), and **all other elinks on the same FELIX device** will be disabled (checked if they are disabled) . Without the option `-e`, only the elinks listed in the options `-t` and `-r` are enabled or checked.

Example:
- Check if all elinks corresponding to the Rx channels 0, 2, 4, 6, 8, and 10 are enabled and configured with a bandwidth of 640 Mbps:
```
bin/elinkConfig get configs/controller/felix_client.json -r 0 2 4 6 8 10 -b 640
```
- Enable exclusively the elinks specified in a connectivity config as well as the relevant IC and EC channels, and disable all other elinks:
```
bin/elinkConfig set configs/controller/felix_client.json -c <connectivity.json> -e -I -E
```
- Turn off all elinks in a connectivity config:
```
bin/elinkConfig off -c <connectivity.json>
```

#### [readWriteLpGBTRegister](../src/libFelixClient/app/readWriteLpGBTRegister.cpp)
`readWriteLpGBTRegister` can be used to read or write a register from an LpGBT device.

Note that if no variables are provided for the LpGBT in the controller configuration (see above example), default values will be chosen.  All default values are listed in the namespace [OptoUtils.h](../src/libFelixClient/include/OptoUtils.h).  This namespace also provides tools for determining LpGBT addresses and private functions for accessing different LpGBT registers.  A list of all available LpGBT registers is provided in the register maps [lpgbt-items-v0.h](../src/libFelixClient/include/lpgbt-items-v0.h) and [lpgt-items-v1.h](../src/libFelixClient/include/lpgbt-items-v1.h). 

To run: 
```
bin/readWriteLpGBTRegister [OPTIONS...]
```
Available options:
```
 -r : Path to controller configuration file.
 -n : Register name to read or write.
 -v : Optional parameter, register value to write. If no value provided, register will be read.
 -R : FID for communication in rx direction.
 -T : FID for communication in tx direction.
 -d : Device address for the LpGBT we want to access.
 -h : Display a help message to print out options and usage information.
```

## TODO

Support for Pixel readout chips.
