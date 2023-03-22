# ScanConsole

The ScanConsole is the main program which should be used to perform scans on FrontEnd ASICs.
It currently supports three types of FrontEnds:
    
- FE-I4B
- FE65-P2
- RD53A
- ABC/HCC STAR

## Quick Start

If you just want to see something running, execute the following:

```bash
$ bin/scanConsole -r configs/controller/emuCfg.json -c configs/connectivity/example_fei4b_setup.json -s configs/scans/fei4/std_digitalscan.json -p
```

This runs a digitalscan with the FE-I4B emulator. This does not use or require any hardware and will run purely in software.

## Command Line Arguments

ScanConsole normally requires at min. three types of configuration files (more details below):
    
- **-r ``<path>``** => Controller config : this config contains the information to select the right hardware controller (e.g. SPEC, RCE, or emulator) and the configuration of the hardware controller.
- **-c ``<path>``** => Connectivity/Setup config : this config contains the chipType (e.g. FE-I4B or RD53A) and a list chips which should be used in the scan
- **-s ``<path>``** => Scan config : this config contains the order and type of loop actions to be executed (and their configuration), the histogrammers to be used, and the analysis

Note: If you omit the scan config, your chip will only be configurated.


Additional command line arguments for the scanConsole are:
    
- **-h** : this, prints all available command line arguments
- **-t  ``<target_charge>`` [``<target_tot>``]** : Set target values for threshold (charge only) and tot (charge and tot).
- **-p** : Enable plotting of results.
- **-o ``<dir>``** : Output directory. (Default ./data/)
- **-m ``<int>``** : 0 = disable pixel masking, 1 = reset pixel masking, default = enable pixel masking
- **-k**: Report known items (Scans, Hardware etc.)
- **-l ``<path>``** => Logger config : this points to a json file to configure the [logging](logging.json) system. The default is to print info, warnings and errors to the console with appropriate colorization.

### Controller Config
Example of a controller config:
```json
{
    "ctrlCfg" : {
        "type": "spec",
        "cfg" : {
            "specNum" : 0
        }
    }
}
```
The "type" specifies which hardware controller should be used. Any fields in the "cfg" field are specific the hardware and details can be found here: [TODO](todo)

If you are running with multiple PCIexpress cards, each should have its own controller configuration with the appropriate value for `specNum`.

### Connectivity Config
Example of a connectivity config:
```json
{
    "chipType" : "RD53A",
    "chips" : [
        {
            "config" : "configs/rd53a_test.json",
            "path": "relToExec",
            "tx" : 0,
            "rx" : 0,
            "enable" : 1,
            "locked" : 0
        },
        {
            "config" : "configs/rd53a_test_1.json",
            "path": "relToExec",
            "tx" : 1,
            "rx" : 1,
            "enable" : 0,
            "locked" : 0
        }
    ]
}

```
The `path` can be as following ([MR](https://gitlab.cern.ch/YARR/YARR/-/merge_requests/599)):

- ``relToExec`` : relative to current executable path (default)
- ``relToCon`` : relative to dir of connectivity
- ``abs`` : absolute path
- ``relToYarrPath`` : relative to environmental variable ``YARR_CONFIG_PATH``
- ``db`` : (to be implemented) config to be loaded from DB by given "tag"

In the above example, the chip using tx 0 and rx 0 is enabled, meaning that transmission and reception will be established with that chip using the display port cable located in slot 0. Tx refers to transmission to the FE chip, while rx refers to reception of data from the FE chip. That chip will have a default configuration file made, called `rd53a_test.json`, located in the `configs/` folder. The chip on the tx/rx line 1 is ignored. 

The `chipType` can be one of three: `RD53A`, `FEI4B`, or `FE65P2`.
"chips" contains an array of chips, each element needs to contain the path to the config, and the tx and rx channel/link. Each chip can be read out individually by toggling "enable". The chip config can be prevented from overwriting if it is locked.

#### Configuration for multiple FE chips with each FE receiving its own command line
For each chip to receive its own command, the connectivity configuration needs to specify the `tx`, `rx`, and `enable` for each chip. 

An example configuration set to communicate with multiple FEs looks like this:
```json
{
    "chipType" : "RD53A",
    "chips" : [
        {
            "config" : "configs/rd53a_TripletA_IndCmdChipA.json",
            "tx" : 0,
            "rx" : 0,
            "enable" : 1,
            "locked" : 0
        },
        {
            "config" : "configs/rd53a_TripletA_IndCmdChipB.json",
            "tx" : 1,
            "rx" : 1,
            "enable" : 0,
            "locked" : 0
        },
        {
            "config" : "configs/rd53a_TripletA_IndCmdChipC.json",
            "tx" : 2,
            "rx" : 2,
            "enable" : 1,
            "locked" : 0
        }
    ]
}
```
Each chip is given its own configuration file named, labeled under `config`. In this example, the 2nd chip (tx/rx 1 is disabled) but all remaining chips are enabled.

#### Configuration for multiple FE chips with each FE sharing one command line
An example of this type of configuration is:
```json
{
    "chipType" : "RD53A",
    "chips" : [

        {
            "config" : "configs/rd53a_Quad_ChipA.json",
            "tx" : 0,
            "rx" : 0,
            "enable" : 1,
            "locked" : 0
        },
        {
            "config" : "configs/rd53a_Quad_ChipB.json",
            "tx" : 0,
            "rx" : 1,
            "enable" : 1,
            "locked" : 0
        }
    ]
}
```
In the above configuration, the command will be sent using tx0 but each chip uses its own rx line.


### Scan Config

The scan config can be split in multiple parts:
```json
{
  "scan": {
    "analysis": {...},
    "histogrammer": {...},
    "loops": [...],
    "name": "DigitalScan",
    "prescan": {...}
  }
}
```

The `ChipId` and the chip `Name`, which will be the name used in the plots, are set in the `Parameters` section of the configuration file.
```json
"Parameter": {
  "ChipId": 0,
  "Name": "JohnDoe_0",            
}
```
The default ChipId is 0 so please compare the wire-bonded chip ID with the one set in the configuration file.

1. Analysis:
   
Contains the list of analysis which should be executed on the data.
Example:
```json
"analysis": {
  "0": {
    "algorithm": "OccupancyAnalysis",
    "config": {
      "createMask": true
    }
  },
  "1": {
    "algorithm": "L1Analysis"
  },
  "n_count": 2
},
```
A list of analysis can be found [here](todo).

2. Histogrammer
   
Similar to the analysis the histogrammers which should be used are listed.

Example:
```json
"histogrammer": {
  "0": {
    "algorithm": "OccupancyMap",
    "config": {}
  },
  "1": {
    "algorithm": "TotMap",
    "config": {}
  },
  "2": {
    "algorithm": "Tot2Map",
    "config": {}
  },
  "3": {
    "algorithm": "L1Dist",
    "config": {}
  },
  "4": {
    "algorithm": "HitsPerEvent",
    "config": {}
  },
  "n_count": 5
}
```
A list of histogrammers and what they do can be found here [here](todo).

3. Loop Actions and pre scan

The loop array contains the list of loop actions in order of nesting, starting with the outermost loop.
Example:
```json
"loops": [
  {
    "config": {
      "max": 64,
      "min": 0,
      "step": 1
    },
    "loopAction": "Rd53aMaskLoop"
  },
  {
    "config": {
      "max": 50,
      "min": 0,
      "step": 1,
      "nSteps": 5
    },
    "loopAction": "Rd53aCoreColLoop"
  },
  {
    "config": {
      "count": 100,
      "delay": 56,
      "extTrigger": false,
      "frequency": 5000,
      "noInject": false,
      "time": 0,
      "edgeMode": true
    },
    "loopAction": "Rd53aTriggerLoop"
  },
  {
    "loopAction": "StdDataLoop"
  }
],
"prescan": {
    "InjEnDig": 1,
    "InjAnaMode": 0,
    "LatencyConfig": 58,
    "GlobalPulseRt": 16384
}
```
The 'prescan' config includes specific FrontEnd registers which are necessary for the scan and overwrite whatever is in the chip config (however these values will not be transferred into the chip config, they only exist for the time of the scan). Register names in the 'prescan' need to match those in the chip configuration.
Loop actions are specific to the FrontEnd type and are listed on the respective FrontEnd page:
    
- [RD53A](rd53a.md)
- [FE-I4](fei4.md)
- [FE65-P2](fe65p2.md)

**Important Notes:**
    
- Analysis require specific histograms, so the correct histogrammers have to be provided for each analysis (in the example here all histogrammers are enabled)
- While some loops can be switched w/o changing the scan result, and other can be interchanged to have completly different scan, **loop actions have to be ordered carefully**
- The scan config design gives maximum flexibility at the cost of possibily constructing a non-functional scan. If you don't know what you are doing, stick to the defaults.


