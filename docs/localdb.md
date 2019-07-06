# Local DB Setup and Tools Installation

<span style="color:red">"PLAN" is the item which is not implemented completely yet, but will be implemented soon.</span>

In order to setup the Local DB system the following steps are needed:

1. Install libraries and Start up (requirement: sudo)
2. Start Viewer Application to check data in Local DB 
3. Setup Local DB system 
4. Compile YARR software including Local DB software
5. Scan with uploading data into Local DB

This instruction supports the following OS:
- centOS7

Local DB system is based on following systems (to be installed by the installer):
- [MongoDB](https://docs.mongodb.com/v3.6/) v3.6.3 
- [mongocxx](https://github.com/mongodb/mongo-cxx-driver/tree/releases/v3.2) v3.2.0
- Python3 v3.6

Viewer Application is based on following systems additionaly (<span style="color:red">not to be installed by the installer</span>):
- Python3
- PyROOT (executable by Python3) (<span style="color:red">PLAN: will be replaced by python module</span>)

## 1. Install Libraries

### Requirements
- sudo user account

Install necessary libraries and setup Local DB system for the machine.<br>
Check the detail here:
- [Installation](https://github.com/jlab-hep/Yarr/wiki/Installation)
  - [Automatic Installation](https://github.com/jlab-hep/Yarr/wiki/Automatic-Installetion)
  - [Manual Installation](https://github.com/jlab-hep/Yarr/wiki/Manual-Installation)
  
## 2. Start Viewer Application
  Start Viewer Application to check data in Local DB Server in this step. <br>
  This step is required for DB machine but not for DAQ machine.

### Requirements
- Pre Install for Viewer Application: [Installation](https://github.com/jlab-hep/Yarr/wiki/Installation)

### Setup
- Setting script: `localDB-tools/viewer/setup_viewer.sh`

  You can setup for [Viewer Application](https://github.com/jlab-hep/Yarr/wiki/Viewer) by this scripts in Local DB Server. <br>
  This script can ...
  - Set the config file
    ```bash
    $ cd path/to/localDB-tools
    $ cd viewer
    $ ./setup_viewer.sh
    # Additional arguments required if you run MongoDB on another machine
    # -i "IP address"   Local DB Server IP address (default: 127.0.0.1)
    # -p "port number"  Local DB Server port number (default: 27017)
    ```

- Start Viewer Application 

  <span style="color:red">(PLAN: Currently the application has to be manual running but will be automatic running)</span>
  ```bash
  $ source /opt/rh/devtoolset-7/enable
  $ source ${root_install_dir}/6.16.00-build/bin/thisroot.sh
  # if PyROOT is enabled
  
  $ cd path/to/localDB-tools
  $ cd viewer
  $ python3 app.py --config conf.yml
  
  Applying ATLAS style settings...
  
  Connecto to mongoDB server: mongodb://127.0.0.1:27017/localdb
   * Serving Flask app "app" (lazy loading)
   * Environment: production
     WARNING: Do not use the development server in a production environment.
     Use a production WSGI server instead.
   * Debug mode: off
  HI 2019-06-28 11:50:29,068 - werkzeug - INFO -  * Running on "http://127.0.0.1:5000/" (Press CTRL+C to quit)
  ```

- Confirmation

  Access "http://127.0.0.1:5000/localdb/" or "http://<IPaddress>/localdb" and Local DB Viewer page should be displayed on browser.

## 3. Setup Local DB system

- Setting script: `localDB-tools/viewer/setup_viewer.sh`

  You can setup for [Local DB system](https://github.com/jlab-hep/Yarr/wiki/Quick-tutorial-for-latest) by this scripts in DAQ Server. <br>
  This script can ...
  - Set the config files
  - make directory for storing cache and log
  ```bash
  $ cd path/to/YARR
  $ cd localdb
  $ ./setup_db.sh
  # Additional arguments required if you run MongoDB on another machine
  # -i "IP address"     Local DB Server IP address (default: 127.0.0.1)
  # -p "port"           Local DB Server port (default: 27017)
  # -c "path/to/cache"  path to Local DB cache directory (default: ${HOME}/.yarr/localdb)
  # -n "database name"  Local DB Name (default: localdb)

  Enter the institution name where this machine (MAC address: XX:XX:XX:Xx:XX:XX) is or 'exit' ... 
  > INSTITUTION NAME
 
  MongoDB Server Information
    IP address: 127.0.0.1
    port: 27017
   
  Test Site Information
    MAC address: XX:XX:XX:XX:XX:XX
    Machine Name: ${HOSTNAME}
    Institution: INSTITUTION NAME
 
  Are you sure it is correct? [y/n]
  # answer 'y' after the confirmation and move on to the installation
  > y
  ```

- Confirmation
  
  config files in path/to/cache/etc
  - database.json
  - address.json
  - ${USER}-user.json

## 4. Compile YARR software including Local DB software
Compile YARR software for using scanConsole with Local DB software in this step.
This step is required for DAQ machine but not for DB machine.

### Requirements
- Make sure you have YARR software

  <span style="color:red">(PLAN: Currently split into development branch  "devel-localdb" but will be merged)</span>.

  ```bash
  $ git clone https://gitlab.cern.ch/YARR/YARR.git
  $ git checkout devel-localdb 
  # WILL BE MERGED
  ```

- Load Path
  ```bash
  $ source /opt/rh/devtoolset-7/enable
  $ source /opt/rh/rh-mongodb36/enable
  ```

### Compile with Make file
```bash
$ cd YARR
$ yarr_dir=`pwd`
$ cd YARR/src
$ make -j4
<lots of text>
```

## 5. Scan with uploading data into Local DB
Scan Module with uploading data into Local DB in this step.

User can use scanConsole with Local DB system after setting machine: [step1](#1.Install-Libraries), [step3](#3.Setup-Local-DB-system) and [step4](#4.Compile-YARR-software-including_Local_DB_software)

### Requirements
- YARR software with Local DB system

### Quick Start
If you just want to see something running, execute the following: 

```bash
$ ./bin/scanConsole \
-r configs/controller/emuCfg.json \
-c configs/connectivity/example_fei4b_setup.json \
-s configs/scans/fei4/std_digitalscan.json \
-p \
-W
# scanConsolw with option 'W' can store cache files
```
This runs a digitalscan with the FE-I4B emulator and store cache files of the test.<br>
Cache files are stored in ${HOME}/.yarr/localdb/var/cache/scan/ in default.

```bash
$ ./bin/dbAccessor -R
# dbAccessor with option 'R' can upload data from the cache. 
```
This can upload data from cache file on the stable network to Local DB Server.

After that, you can check the result (NON-REGISTERED component) in Test Page of Viewer Application.

### Module Registration
You can store results associated with the registered module after the registration. <br>
Prepare the component information file and user information file.<br>
<span style="color:red">PLAN: to be prepared registeration page in Viewer Application</span> <br>

- user.json

  **Required information**
  - userName: your name (e.g. "John Doe")
  - institution: institution you belong (e.g. "ABC Laboratory")
  - userIdentity: just identifiable code not password (e.g. "account for testbeam")

  ```json
  {
    "userName": "FIRSTNAME LASTNAME",
    "institution": "INSTITUTION",
    "userIdentity": "default"
  } 
  ```

- component.json (RD53A)

  <span style="color:red">One file for one module!</span> 

  **Required information**
  - module.serialNumber: serial number of the module
  - module.componentType: "Module"
  - chipType: "FEI4B" or "RD53A"
  - chips: chips on the module
    - chips.i.serialNumber: serial number of the chip
    - chips.i.componentType: "Front-end Chip"
    - chips.i.chipId: chipID must be "int"

  <details><summary>for RD53A</summary><div>

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

  <details><summary>for FEI4B</summary><div>

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

And run the command `dbAccessor` with the option '-C <component.json>' and '-u <user.json>':
```bash
$ dbAccessor -C component.json -u user.json
```

This can register the components data written in component.json.

### Scan for REGISTERED Module

Connecivity config file should be prepared properly.

- connectivity.json

  **Required information**
  - stage: the test stage for the module, should be selected from the stage list written in [database.json](https://github.com/jlab-hep/Yarr/wiki/database-config-file)
  - module.serialNumber: serial number of the module
  - chipType: "FEI4B" or "RD53A"
  - chips: chips on the module
    - chips.i.chipId: chip ID, must be "int"
    - chips.i.geomId: geometrical ID, should be 1 for SCC, and from 1 to 4 for quad module (and usually same to chip ID)
    - chips.i.config: path to chip config file
    - chips.i.tx: must be "int"
    - chips.i.rx: must be "int"

  <details><summary>for RD53A</summary><div>

  ```json
  {
      "stage": "Testing",
      "module": {
          "serialNumber": "RD53A-001"
      },
      "chipType" : "RD53A",
      "chips" : [
          {
              "chipId": 0,
              "geomId": 1,
              "config" : "configs/defaults/default_rd53a.json",
              "tx" : 0,
              "rx" : 0
          }
      ]
  }
  ```

  </div></details>

  <details><summary>for FEI4B</summary><div>

  ```json
  {
      "stage": "Testing",
      "module": {
          "serialNumber": "FEI4B-001"
      },
      "chipType" : "FEI4B",
      "chips" : [
          {
              "chipId": 1,
              "geomId": 1,
              "config" : "configs/defaults/default_fei4b.json",
              "tx" : 0,
              "rx" : 0
          },
          {
              "chipId": 2,
              "geomId": 2,
              "config" : "configs/defaults/default_fei4b.json",
              "tx" : 0,
              "rx" : 1
          },
          {
              "chipId": 3,
              "geomId": 3,
              "config" : "configs/defaults/default_fei4b.json",
              "tx" : 0,
              "rx" : 2
          },
          {
              "chipId": 4,
              "geomId": 4,
              "config" : "configs/defaults/default_fei4b.json",
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
-p \
-W \
-u user.json
<Lots of text>
$ ./bin/dbAccessor -R
```

After that, you can check the result (non-registered component) in Module/Test Page of Viewer Application.

