# Local DB Quick Tutorial
<span style="color:red">"PLAN" is the item which is not implemented completely yet, but will be implemented soon.</span>

## About

**Local Database (Local DB) is mainly data managing system for YARR based on MongoDB.**

### Base system
- [MongoDB](https://docs.mongodb.com/v3.6/) v3.6.3 
- Python3 
- PyROOT (executable by Python3) (<span style="color:red">PLAN: will be replaced by python module</span>)

This instruction supports the following OS:
- centOS7

### Local DB Tools
|Function      |Tool Name           |
|:------------:|:------------------:|
|Storage System|Local DB            |
|Store Data    |Upload Tool         |
|Restore Data  |Retrieve Tool       |
|Share Data    |Synchronization Tool|
|Check Data    |Viewer Application  |

## Quick Tutorial

You can store result data by YARR immediately following the tutorial.

### Setup Local DB with YARR

Currently there are [another official repository for Local DB](https://gitlab.cern.ch/YARR/YARR/tree/devel-localdb) in [YARR](https://gitlab.cern.ch/YARR/YARR).<br>
(<span style="color:red">PLAN: will be merged into the official repository</span>)

```bash
$ cd YARR
$ git checkout devel-localdb
$ mkdir build-localdb
$ cd build-localdb
$ cmake3 ../
$ make -j4
$ make install
```

### Setup Local DB Tools

Set Local DB default config files and commands `localdbtool-upload` and `localdbtool-retrieve` in `${HOME}/.local/bin` by `setup_db.sh`.

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
```

If there are something wrong in this step, [FAQ](#FAQ) should be helpful for solving it.<br>
Also you can check ${HOME}/YARR/localdb/README for more information.

#### Confirmation

```bash
$ source ~/.local/lib/localdb/enable    # to enable tab-completion
$ localdbtool-upload init --database ~/.yarr/localdb/database.json 
#INFO# Local DB Server: mongodb://127.0.0.1:27017
#INFO# ---> connection is good.
```

If there are some problems in the step, [FAQ](#FAQ) should be helpful for solving it.

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
#DB INFO# Uploading in the back ground. (log: ~/.yarr/localdb/log/)
```

#### Confirmation

There are three ways to confirm the data uploaded.

1. Log File
   
   You can check if the upload is success in log file `${HOME}/.yarr/localdb/log/day.log`.
   
   ```log
   2019-07-22 18:42:23,818 - Log - INFO: -----------------------
   2019-07-22 18:42:23,820 - Log - INFO: Local DB Server: mongodb://127.0.0.1:27017
   2019-07-22 18:42:23,822 - Log - INFO: ---> connection is good.
   2019-07-22 18:42:23,822 - Log - INFO: Cache Directory: ${HOME}/YARR/data/000047_std_digitalscan/
   2019-07-22 18:42:24,050 - Log - INFO: Success
   ```

2. Retrieve Tool

   You can check uploaded test data by `localdbtool-retrieve log` in CUI

   ```bash
   $ localdbtool-retrieve init
   $ localdbtool-retrieve remote add origin
   #DB INFO# Create remote repostiory "origin"

   Enter the URL of DB Server (e.g. mongodb://127.0.0.1:27017/), or "None" if not to be set.
   mongodb://127.0.0.1:27017/

   Enter the URL of the Viewer Application (e.g. http://127.0.0.1:5000/localdb/), or "None" if not to be set.
   http://127.0.0.1:5000/localdb/

   #DB INFO#   remote: origin
   #DB INFO#   DB Server: mongodb://127.0.0.1:27017/      (connection: True)
   #DB INFO#   Viewer: http://127.0.0.1:5000/localdb/     (connection: True) 

   Are you sure that is correct? [y/n]
   y

   $ localdbtool-retrieve log
   #DB INFO# The connection of Local DB mongodb://127.0.0.1:27017/ is GOOD.
   
   test data ID: XXXXXXXXXXXXXXXXXXXXXXXX 
   User          : kubota at Tokyo_Institute_of_Technology
   Date          : 2019/07/21 03:49:37
   Serial Number : DUMMY_0
   Run Number    : 5576
   Test Type     : std_digitalscan
   ```

3. Viewer Application

   You can check uploaded test data in GUI when Viewer Application is running. <br>
   Access "http://127.0.0.1:5000/localdb/" or "http://IPaddress/localdb" in browser.

## Advanced Tutorial

### Module Registration

You can store results associated with the registered module after the registration. <br>
Prepare the component information file and user information file.<br>
(<span style="color:red">PLAN: to be deleted and will prepare the script to download the component data from ITk PD.</span>) <br>

- user config file (json)

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

- component config file (json)

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
<some texts>
Do you continue to upload data into Local DB? [y/n]
y
#DB INFO# Completed the registration successfuly.
```

This can register the components data written in component.json.

### scanConsole with registered Module

Connecivity config file should be prepared properly.

- connectivity config (json)

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

### DCS Registration

in edit

### Retrieve Tool

You can restore data from Local DB by `localdbtool-retrieve`.

- Initialize

   ```bash
   $ source ~/.local/lib/localdb/enable
   $ localdbtool-retrieve init
   ```

- Add Local DB Configuration for retrieve tool

   ```bash
   $ localdbtool-retrieve remote add origin
   #DB INFO# Create remote repostiory "origin"

   Enter the URL of DB Server (e.g. mongodb://127.0.0.1:27017/), or "None" if not to be set.
   mongodb://127.0.0.1:27017/

   Enter the URL of the Viewer Application (e.g. http://127.0.0.1:5000/localdb/), or "None" if not to be set.
   http://127.0.0.1:5000/localdb/

   #DB INFO#   remote: origin
   #DB INFO#   DB Server: mongodb://127.0.0.1:27017/      (connection: True)
   #DB INFO#   Viewer: http://127.0.0.1:5000/localdb/     (connection: True) 

   Are you sure that is correct? [y/n]
   y
   ```

   URL of DB Server: `mongodb://<IP address>:<port number>/`

   - IP address : IP address (or host name) of the Local DB Server
 
     It is '127.0.0.1' in the case that the Local DB Server working locally.<br>
     If you want to access the another machine remotely, set IP address or host name of the machine of the Local DB Server.

   - port number : Port number opened for the Local DB Server

     It should be '27017' in default.<br>

   URL of the Viewer Application: `http://<IP address>:<port number>/localdb/` or `http://<IP address>/localdb/`

   - IP address : IP address (or host name) of the Server where Viewer Application running
 
     It is '127.0.0.1' in the case that the Viewer Application working locally.<br>
     If you want to access the another machine remotely, set IP address or host name of the machine of the Server where Viewer Application running.

   - port number : Port number opened for the Viewer Application

     It should be '5000' in default.<br>
     It should not be needed if you access the another machine remotely.

   'connection: True/False' shows if the connection to the Local DB Server/Viewer Application is stable or unstable. <br>
   If the connection is False, you should check if Local DB Server/Viewer Application is working or if the url is correct.

- Log

   ```bash
   $ localdbtool-retrieve log
   #DB INFO# The connection of Local DB mongodb://127.0.0.1:27017/ is GOOD.
   
   test data ID: XXXXXXXXXXXXXXXXXXXXXXXX 
   User          : kubota at Tokyo_Institute_of_Technology
   Date          : 2019/07/21 03:49:37
   Serial Number : DUMMY_0
   Run Number    : 5576
   Test Type     : std_digitalscan

   <ctrl+C>
   ```
   Additional options:
   - `localdbtool-retrieve log <remote name>` : show the test data log of the 'remote name'
   - `localdbtool-retrieve log <serial number>` : show the test data of the module ('serial number')

- Fetch

   ```bash
   $ localdbtool-retrieve fetch
   #DB INFO# The connection of Local DB mongodb://atlaspc5.kek.jp:27017/ is GOOD.
   
   #DB INFO# Download Component Data of Local DB locally...
   --------------------------------------
   Component (1)
       Chip Type: FE-I4B
       Module:
           serial number: QU-09
           component type: Module
           chips: 4
       Chip (1):
           serial number: QU-09_chipId1
           component type: Front-end Chip
           chip ID: 1
       Chip (2):
           serial number: QU-09_chipId2
           component type: Front-end Chip
           chip ID: 2
       Chip (3):
           serial number: QU-09_chipId3
           component type: Front-end Chip
           chip ID: 3
       Chip (4):
           serial number: QU-09_chipId4
           component type: Front-end Chip
           chip ID: 4
   --------------------------------------
   <ctrl+C>
   ```

- Check Out

   ```bash
   $ localdbtool-retrieve checkout 
   #DB INFO# The connection of Local DB mongodb://127.0.0.1:27017/ is GOOD.

   #DB INFO# test data information
   #DB INFO# - Date          : 2019/07/21 03:49:37
   #DB INFO# - Serial Number : DUMMY_0
   #DB INFO# - Run Number    : 5576
   #DB INFO# - Test Type     : std_digitalscan
   #DB INFO# 
   #DB INFO# controller      : Found      --->   path: ./localdb-configs/controller.json
   #DB INFO# scan            : Found      --->   path: ./localdb-configs/std_digitalscan.json
   #DB INFO# chip(after)     : Found      --->   path: ./localdb-configs/chip0-chipCfg.json
   #DB INFO# connectivity    : Found      --->   path: ./localdb-configs/connectivity.json

   $ls localdb-configs/
   chip0-chipCfg.json  connectivity.json  controller.json  std_digitalscan.json
   ```
   Additional options:
   - `localdbtool-retrieve checkout <remote name>` : restore the latest test config files from the 'remote name'
   - `localdbtool-retrieve checkout <serial number>` : restore the latest test config files for 'serial number' from the 'remote name'
   - `localdbtool-retrieve checkout <test data id>` : restore the test 'test data id' config files from the 'remote name'

## FAQ 

### the detail of 'setup_db.sh'

`setup_db.sh` can set Local DB commands and the default configuration files for Local DB.

```bash
$ cd YARR
$ localdb/setup_db.sh
[LDB] Check the exist site config...NG!

[LDB] Enter the institution name where this machine is, or 'exit' ... 
> INSTITUTION NAME
```
Additional command line arguements:
    
- **-h** : this, prints all available command line arguments
- **-i  ``<IP address>``** : Set IP address of the Local DB Server (default: 127.0.0.1).
- **-p  ``<port number>``** : Set port number of the Local DB Server (default: 27017).
- **-n  ``<database name>``** : Set the Local DB Server Name (default: 'localdb').
- **-r** : Rseet the all settings for Local DB.

### '[LDB] Error checking python modules! Exit!'

You are missing some python modules incuded in the requirements.<br>
Check which python module missing by `python3 check_python_modules.py`
```bash
$ python3 check_python_modules.py 
[LDB] Welcome to Local Database Tools!
[LDB] Check Python version ... 3.6 ... OK!
[LDB] Check python modules: 
<some texts>
	requests...not found
	tzlocal...OK!
```
'\<module name\>...not found!' suggests that \<module name\> is missing,
so install the module by `$ sudo pip3 install <module name>` or `$ pip3 install --user <module name>`.

### '#DB ERROR# The connection of Local DB mongodb://127.0.0.1:27017 is BAD.'

Connection check by `localdbtool-upload init` is failed by some reasons.

1. Uncompleted the setup of Local DB Server

   You can check is as following:
   ```bash
   $ mongo
   # i. MongoDB not installed
   bash: mongo: command not found
   
   # ii. MongoDB not started
   <some texts>
   exception: connect failed
   ```

   1. MongoDB not installed

      You can setup Local DB Server automatically by the script:
      ```bash
      $ git clone https://github.com/jlab-hep/localDB-tools.git
      $ cd localDB-tools/setting
      $ sudo ./db_server_install.sh
      ```
      Check [Setup Local DB Server](#Setup-Local-DB-Server) for more detail.

   2. MongoDB not started

      You can start/restart/enable Mongo DB by systemctl (centOS7):
      ```bash
      $ systemctl restart mongod.service
      $ systemctl enable mongod.service
      ```

2. Mistake Local DB Server Information

   Confirm the database config file and set it properly.
   ```json
   {
       "hostIp": "127.0.0.1",
       "hostPort": "27000",
       "dbName": "localdb",
       "stage": [
           "Bare Module",
           "Wire Bonded",
           "Potted",
           "Final Electrical",
           "Complete",
           "Loaded",
           "Parylene",
           "Initial Electrical",
           "Thermal Cycling",
           "Flex + Bare Module Attachment",
           "Testing"
       ],
       "environment": [
           "vddd_voltage",
           "vddd_current",
           "vdda_voltage",
           "vdda_current",
           "hv_voltage",
           "hv_current",
           "temperature"
       ],
       "component": [
           "Front-end Chip",
           "Front-end Chips Wafer",
           "Hybrid",
           "Module",
           "Sensor Tile",
           "Sensor Wafer"
       ]
   }
   ```
   You might have to change 'hostIp' and 'hostPort' following your Local DB settings.

## Installation

### Automatic Installation

Check [this page](https://github.com/jlab-hep/Yarr/wiki/Automatic-Installetion).

### Manual Installation

Check [this page](https://github.com/jlab-hep/Yarr/wiki/Manual-Installation).

### Setup Viewer Application

Check [this page](https://github.com/jlab-hep/Yarr/wiki/Setup-Viewer-Application).

### Others

in edit

