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
- PyROOT (executable by Python3)

## 1. Install Libraries
Install necessary libraries and setup Local DB system for the machine in this step.

### Requirements
- sudo user account
- Make sure you have git and net-tools:
  ```bash
  $ sudo yum install -y git net-tools
  ```
- Clone `localDB-tools`
  ```bash
  $ git clone https://github.com/jlab-hep/localDB-tools.git
  ```
- Clone `YARR`
  ```bash
  $ git clone https://gitlab.cern.ch/YARR/YARR.git
  $ git checkout devel-localdb
  ```

### Libraries for DB Server
- Installer: `localDB-tools/setting/db_server_install.sh`

  You can install [required libraries](https://github.com/jlab-hep/Yarr/wiki/Install-libraries) by this installer for setting up Local DB Server in the first time.<br>
  This installer can ...
  - Install libraries by yum with avoiding conflict
  - Install Modules by pip3 with avoiding conflict
  - Start services 
  - Open firewall port for accessing to Viewer Application from other machine 
  - Initialize MongoDB data set 

  Usage:
  ```bash
  $ cd path/to/localDB-tools
  $ cd setting
  $ sudo ./db_server_install.sh
  Local DB Server IP address: XXX.XXX.XXX.XXX
  
  Are you sure it is correct? [y/n]
  # answer 'y' and move on to the installation
  y
  ```

- Confirmation

  Check the MongoDB service enabled by the command below:
  ```bash
  $ mongo
  MongoDB shell version v3.6.8
  connecting to: mongodb://localhost:27017
  MongoDB server version: 3.6.8
  Server has startup warnings:  
  ```
  
### Libraries for DAQ Server
- Installer: `YARR/localdb/db_yarr_install.sh`

  You can install [required libraries](https://github.com/jlab-hep/Yarr/wiki/Install-libraries) by this installer for setting up scanConsole with Local DB Function in the first time. <br>
  This installer can ...
  - Install libraries by yum with avoiding conflict
  - Install Modules by pip3 with avoiding conflict

  Usage:
  ```bash
  $ cd path/to/YARR
  $ cd localdb
  $ sudo ./db_yarr_install.sh
  # move on to the installation
  ```

- Confirmation

  Check the MongoDB Libraries by the command below:
  ```bash
  # 1. Load path
  $ source /opt/rh/rh-mongodb36/enable
  
  # 2. confirmation
  $ echo $(pkg-config --cflags --libs libmongocxx)
  -I/opt/rh/rh-mongodb36/root/usr/include/mongocxx/v_noabi -I/opt/rh/rh-mongodb36/root/usr/include/bsoncxx/v_noabi -L/opt/rh/rh-mongodb36/root/usr/lib64 -lmongocxx -lbsoncxx
  # The header files will be at /opt/rh/rh-mongodb36/root/usr/include
  # The libraries will be at /opt/rh/rh-mongodb36/root/usr/lib64
  ```

## 2. Start Viewer Application
  Start Viewer Application to check data in Local DB Server in this step. <br>
  This step is required for DB machine but not for DAQ machine.

### Requirements
- Pre Install for Viewer Application: [Installation](#Libraries_for_DB_Server)
- Python3 and PyROOT for displaying plots in browser: [Installation](https://github.com/jlab-hep/Yarr/wiki/Install-libraries)

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
  # -i "IP address"    Set Local DB Server IP address (default: 127.0.0.1)
  # -p "port"          Set Local DB Server port (default: 27017)
  # e.g.) ./setup_viewer.sh -i 111.111.1.0 -p 27017
  ```

- Start Viewer Application 

  <span style="color:red">(PLAN: Currently the application has to be manual running but will be automatic running)</span>
  ```bash
  $ source /opt/rh/devtoolset-7/enable
  $ source ${root_install_dir}/6.16.00-build/bin/thisroot.sh
  # if PyROOT is enabled
  
  $ cd path/to/localDB-tools
  $ cd viewer
  $ python36 app.py --config conf.yml
  
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

  Access "http://127.0.0.1:5000/localdb/" or "http://<IPaddress>:5000/localdb" and Local DB Viewer page should be displayed on browser.

## 3. Setup Local DB system

![Local DB structure]()

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
  # -i "IP address"    Set Local DB Server IP address (default: 127.0.0.1)
  # -p "port"          Set Local DB Server port (default: 27017)
  # -c "path/to/cache" Set path to Local DB cache directory (default: ${HOME}/.yarr/localdb)
  # -n "database name" Set Local DB Name (default: localdb)
  # e.g.) ./setup_db.sh -i 111.111.1.0 -p 27017 -c `pwd`

  Enter the institution name where this machine (MAC address: 30:9c:23:ab:70:cf) is or 'exit' ... 
  > Tokyo Institute of Technology
 
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

User can use scanConsole with Local DB system after setting machine: [step1](#1._Install_Libraries), [step3](#3._Setup_Local_DB_system) and [step4](#4._Compile_YARR_software_including_Local_DB_software)

### Requirements
- YARR software with Local DB system

### Quick Start
If you just want to see something running, execute the following: 

```bash
$ ./bin/scanConsole -r configs/controller/emuCfg.json -c configs/connectivity/example_fei4b_setup.json -s configs/scans/fei4/std_digitalscan.json -p -W
```
This runs a digitalscan with the FE-I4B emulator and store cache files of the test.
Cache files are stored in ${HOME}/.yarr/localdb/var/cache/scan/ in default.

```bash
$ ./bin/dbAccessor -R
```
This can upload data from cache file on the stable network to Local DB Server.

After that, you can check the result (non-registered component) in Test Page of Viewer Application.

### Module Registration
You can store results associated with the registered module after the registration. <br>
Prepare the component information file and user information file. <br>
<span style="color:red">PLAN: to be prepared registeration page in Viewer Application</span> <br>

  - user.json
    ```json
    {
      "userName": "FIRSTNAME LASTNAME",
      "institution": "INSTITUTION",
      "userIdentity": "default"
    } 
   
    # e.g.
    {
      "userName": "John Doe",
      "institution": "ABC Laboratory",
      "userIdentity": "default"
    } 
    ```
    - userName: your name
    - institution: institution you belong
    - userIdentity: just identifiable code not password
  
  - component.json (RD53A)

    <span style="color:red">One file for one module!</span> 

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
    - serialNumber: serial number of component
    - componentType: "Module" or "Front-end Chip"
    - chips: chips on the module

  - component.json (FEI4B)

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
                "chipId": 1,
            },
            {
                "serialNumber": "FEI4B-001-chip2",
                "componentType": "Front-end Chip",
                "chipId": 2,
            },
            {
                "serialNumber": "FEI4B-001-chip3",
                "componentType": "Front-end Chip",
                "chipId": 3,
            },
            {
                "serialNumber": "FEI4B-001-chip4",
                "componentType": "Front-end Chip",
                "chipId": 4,
            }
        ]
    }
    ```
  
And run the command `dbAccessor`:
```bash
$ dbAccessor -C component.json -u user.json
$ dbAccessor -M
```

First command can register the components written in component.json.
Second command can pull the component information registered in Local DB to local cache file: `${HOME}/.yarr/localdb/lib/modules.csv`.

### Scan for Registered Module

Connecivity config file should be prepared properly.

- connectivity.json
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
  - stage: test stage for the module
  - geomId: geometrical Id ... should be 1 for SSC, and from 1 to 4 for quad module

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

After that, you can check the result (non-registered component) in Module Page/Test Page of Viewer Application.
