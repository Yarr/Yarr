# Support for testing via ITSDAQ FW

This provides minimal support for using the firmware used by ITSDAQ.
Most firmware options are not provided for.

Currently the only configuration is for the stream configuration.
See the ITSDAQ TWiki pages for information.

Basic communication testing:

```bash
bin/star_test configs/controller/itsdaq.json
```

Scan configuration (support is incomplete):

```bash
bin/scanConsole -r configs/controller/itsdaq.json -c configs/connectivity/example_star_setup.json -s configs/scans/star/std_digitalscan.json
```
