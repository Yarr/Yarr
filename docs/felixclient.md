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

## TODO

Support for Pixel readout chips.
