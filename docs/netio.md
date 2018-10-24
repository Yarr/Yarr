# Support for FELIX via NetIO hardware controller

## Software installation

First you will need to install the felix software 
https://atlas-project-felix.web.cern.ch/atlas-project-felix/.

Not all of this is actually required as that will be running on a separate
host computer. Currently it is easiest to follow the instructions to build 
everything.

Once built, the YARR build instructions can be followed, with the addition
of arguments pointing to the FELIX installation directory.

So to configure with cmake use:

```bash
$ cmake .. -DCMAKE_TOOLCHAIN_FILE=../cmake/linux-gcc -DENABLE_NETIO:BOOL=ON -DNETIO_DIR:PATH=${FELIX_DIR}/x86_64-centos7-gcc62-opt
```

Or compile with Make:

```bash
$ make -j4 NETIO_DIR=/path/to/felix/software
```

## Usage

This is preliminary, but the idea is to connect to the felix server running
elsewhere. This is done by providing the appropriate configuration to 
scanConsole. The first part of the configuration is the host and port to 
connect to the server. The second part of the port configuration allows
for some tweaking of how the data is sent to the server.

Example controller config (this is subject to change):
```json
{
  "ctrlCfg": {
    "type": "Netio",
    "cfg": {
      "NetIO": {
        "host": "felixhost",
        "txport": 1234,
        "manchester": False,
        "flip": False,
        "extend": True
      }
    }
  }
}
```
