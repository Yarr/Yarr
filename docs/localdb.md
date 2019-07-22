# Local DB Quick Tutorial
<span style="color:red">"PLAN" is the item which is not implemented completely yet, but will be implemented soon.</span>

## About

**Local Database (Local DB) is mainly data managing system for YARR based on MongoDB.**

### Base system
Local DB system is based on following systems (to be installed by the installer):
- [MongoDB](https://docs.mongodb.com/v3.6/) v3.6.3 
- Python3 
- PyROOT (executable by Python3) (<span style="color:red">PLAN: will be replaced by python module</span>)

This instruction supports the following OS:
- centOS7

### Local DB Tools
What we can do with Local DB system:
- Store result data: Local DB, Upload Tool
- Restore data from Local DB: Retrieve Tool
- Share data with another Local DB: Synchronization Tool
- Chek data in browser: Viewer Application

## Quick Tutorial

You can store result data by YARR immediately following the tutorial.

### Setup Local DB with YARR

Currently there are [another official repository for Local DB](https://gitlab.cern.ch/YARR/YARR/tree/devel-localdb) in [YARR](https://gitlab.cern.ch/YARR/YARR).

```
$ cd YARR
$ git checkout devel-localdb
$ mkdir build-localdb
$ cd build-localdb
$ cmake3 ../
$ make -j4
$ make install
```

### Setup Local DB Tools

Set Local DB commands `localdbtool-upload` and `localdbtool-retrieve` in `${HOME}/.local/bin` by `setup_db.sh`.

```bash
$ cd YARR
$ localdb/setup_db.sh
[LDB] Check the exist site config...NG!

[LDB] Enter the institution name where this machine is, or 'exit' ... 
> INSTITUTION NAME
# e.g.) Tokyo Institute of Technology
# e.g.) LBNL

[LDB] Confirmation
	-----------------------
	--  Mongo DB Server  --
	-----------------------
	IP address: 127.0.0.1
	port: 27017
	-----------------
	--  Test Site  --
	-----------------
	MAC address: 18:31:bf:cc:92:b8
	Machine Name: nakedsnail.dhcp.lbl.gov
	Institution: LBNL

[LDB] Are you sure that is correct? [y/n]
> y

<lots of text>
This description is saved as ${HOME}/YARR/localdb/README

$ source ~/.local/lib/localdb/enable
```

If there are something wrong in this step, [FAQ]() should be helpful for solving it.<br>
Also you can check ${HOME}/YARR/localdb/README for more information.

### Confirmation

Check the connection to Local DB by `localdbtool-upload init`.

```bash
$localdbtool-upload init --database ~/.yarr/localdb/database.json 
#INFO# Local DB Server: mongodb://127.0.0.1:27017
#INFO# ---> connection is good.
```

If there are some problems in the step, [FAQ]() should be helpful for solving it.

### scanConsole with Local DB

You can scan with default config files and upload result data into Local DB by `scanConsole -W`

```bash
$ bin/scanConsole \
-r configs/controller/emuCfg.json \
-c configs/connectivity/example_fei4b_setup.json \
-s configs/scans/fei4/std_digitalscan.json \
-W
<lots of text>
#DB INFO# Local DB Server: mongodb://127.0.0.1:27017
#DB INFO# ---> connection is good.
#DB INFO# Uploading in the back ground.
```

You can check if the upload is success in log file `${HOME}/.yarr/localdb/log/day.log`.

```log
2019-07-22 18:42:23,818 - Log - INFO: -----------------------
2019-07-22 18:42:23,820 - Log - INFO: Local DB Server: mongodb://127.0.0.1:27017
2019-07-22 18:42:23,822 - Log - INFO: ---> connection is good.
2019-07-22 18:42:23,822 - Log - INFO: Cache Directory: ${HOME}/YARR/data/000047_std_digitalscan/
2019-07-22 18:42:24,050 - Log - INFO: Success
```

Also you can check the test data in browser when Viewer Application is running.<br>
Access "http://127.0.0.1:5000/localdb/" or "http://<IPaddress>/localdb".

## Advanced Tutorial

### Module Registration

You can store results associated with the registered module after the registration. <br>
Prepare the component information file and user information file.<br>
<span style="color:red">PLAN: to be prepared registeration page in Viewer Application</span> <br>

- user config file

  **Required information**
  - userName: your name (e.g. "John Doe")
  - institution: institution you belong (e.g. "ABC Laboratory")
  - description: description for user account (e.g. "account for testbeam")

  <details><summary>user.json</summary><div>

  ```json
  {
    "userName": "FIRSTNAME LASTNAME",
    "institution": "INSTITUTION",
    "description": "default"
  } 
  ```

  </div></details>

- component config file (RD53A)

  <span style="color:red">One file for one module!</span> 

  **Required information**
  - module.serialNumber: serial number of the module
  - module.componentType: "Module"
  - chipType: "FEI4B" or "RD53A"
  - chips: chips on the module
    - chips.i.serialNumber: serial number of the chip
    - chips.i.componentType: "Front-end Chip"
    - chips.i.chipId: chipID must be "int"

  <details><summary>component.json for RD53A</summary><div>

    ```json
    {
        "module": {
            "serialNumber": "RD53A-001",
            "componentType": "Module"
        },
        "chipType" : "RD53A",
        "chips" : [
            {
                "serialNumber": "RD53A-001_chip1",
                "componentType": "Front-end Chip",
                "chipId": 0
            }
        ]
    }
    ```

  </div></details>

  <details><summary>component.json for FEI4B</summary><div>

    ```json
    {
        "module": {
            "serialNumber": "FEI4B-001",
            "componentType": "Module"
        },
        "chipType" : "FEI4B",
        "chips" : [
            {
                "serialNumber": "FEI4B-001-chip1",
                "componentType": "Front-end Chip",
                "chipId": 1
            },
            {
                "serialNumber": "FEI4B-001-chip2",
                "componentType": "Front-end Chip",
                "chipId": 2
            },
            {
                "serialNumber": "FEI4B-001-chip3",
                "componentType": "Front-end Chip",
                "chipId": 3
            },
            {
                "serialNumber": "FEI4B-001-chip4",
                "componentType": "Front-end Chip",
                "chipId": 4
            }
        ]
    }
    ```

  </div></details>

And execute the following: 
```bash
$ ./bin/dbAccessor -C component.json -u user.json
# dbAccessor with option 'C' can register component data. 

#DB INFO# Local DB Server: mongodb://127.0.0.1:27017
#DB INFO# ---> connection is good.
#DB INFO# Component Data:
#DB INFO#     Chip Type: FE-I4B
#DB INFO#     Module:
#DB INFO#         serial number: FEI4B-001
#DB INFO#         component type: Module
#DB INFO#         chips: 4
#DB INFO#     Chip (1):
#DB INFO#         serial number: FEI4B-001_chip1
#DB INFO#         component type: Front-end Chip
#DB INFO#         chip ID: 1
#DB INFO#     Chip (2):
#DB INFO#         serial number: FEI4B-001_chip2
#DB INFO#         component type: Front-end Chip
#DB INFO#         chip ID: 2
#DB INFO#     Chip (3):
#DB INFO#         serial number: FEI4B-001_chip3
#DB INFO#         component type: Front-end Chip
#DB INFO#         chip ID: 3
#DB INFO#     Chip (4):
#DB INFO#         serial number: FEI4B-001_chip4
#DB INFO#         component type: Front-end Chip
#DB INFO#         chip ID: 4
#DB INFO# 
Do you continue to upload data into Local DB? [y/n]
y
#DB INFO# Completed the registration successfuly.
```

This can register the components data written in component.json.

### scanConsole with registered Module

Connecivity config file should be prepared properly.

- connectivity config

  **Required information**
  - stage: the test stage for the module, should be selected from the stage list written in [database.json](https://github.com/jlab-hep/Yarr/wiki/database-config-file)
  - module.serialNumber: serial number of the module
  - chipType: "FEI4B" or "RD53A"
  - chips: chips on the module
    - chips.i.serialNumber: serial number of the chip
    - chips.i.config: path to chip config file
    - chips.i.tx: must be "int"
    - chips.i.rx: must be "int"

  <details><summary>connectivity.json for RD53A</summary><div>

  ```json
  {
      "stage": "Testing",
      "module": {
          "serialNumber": "RD53A-001"
      },
      "chipType" : "RD53A",
      "chips" : [
          {
              "serialNumber": "RD53A-001_chip1",
              "config" : "configs/defaults/default_rd53a.json",
              "tx" : 0,
              "rx" : 0
          }
      ]
  }
  ```

  </div></details>

  <details><summary>connectivity.json for FEI4B</summary><div>

  ```json
  {
      "stage": "Testing",
      "module": {
          "serialNumber": "FEI4B-001"
      },
      "chipType" : "FEI4B",
      "chips" : [
          {
              "serialNumber": "FEI4B-001-chip1",
              "config" : "configs/chip1.json",
              "tx" : 0,
              "rx" : 0
          },
          {
              "serialNumber": "FEI4B-001-chip2",
              "config" : "configs/chip2.json",
              "tx" : 0,
              "rx" : 1
          },
          {
              "serialNumber": "FEI4B-001-chip3",
              "config" : "configs/chip3.json",
              "tx" : 0,
              "rx" : 2
          },
          {
              "serialNumber": "FEI4B-001-chip4",
              "config" : "configs/chip4.json",
              "tx" : 0,
              "rx" : 3
          }
      ]
  }
  ```

  </div></details>

And run the `scanConsole`:

```bash
$ ./bin/scanConsole \
-r configs/controller/emuCfg.json \
-c configs/connectivity.json \
-s configs/scans/fei4/std_digitalscan.json \
-W \
-u user.json
<Lots of text>
$ ./bin/dbAccessor -R
```

After that, you can check the result (registered component) in Module/Test Page of Viewer Application.

### FAQ 

in edit

### Installation

in edit

