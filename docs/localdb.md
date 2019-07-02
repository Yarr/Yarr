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





----
in editing

### Create user config file
Scan data is uploaded into Local DB along with user information obtained from the user config file.
```bash
$ cd ${yarr_dir}/localdb
$ dbaccount="account name" # you can set account name freely
$ ./createUserCfg.sh ${dbaccount}
# Enter user information following the instructions
# - your name : <first name> <last name>  (e.g. John Doe)
# - your institution : (e.g. ABC Laboratory)
# - identification keyword : not a password but just an identifier (not need to set)
#
# Create User Config file: `${HOME}/.yarr/${dbaccount}_user.json`
```

### Create connectivity config file
<span style="color:red">PLAN: Currently the file has to be prepared manually by user but plans to be generated automatically based on Module data retrieved from ITk PD</span>
- `${yarrdir}/localdb/db_connectivity.json`
```json
{
    "stage": "Testing",
    "module": {
        "serialNumber": "RD53A-001",
        "componentType": "Module"
    },
    "chipType" : "RD53A",
    "chips" : [
        {
            "serialNumber": "RD53A-001_chip1",
            "componentType": "Front-end Chip",
            "chipId": 1,
            "config" : "configs/rd53a/RD53A-001/RD53A-001_chip1.json",
            "tx" : 0,
            "rx" : 0
        }
    ]
}
```
- stage : test stage
- serialNumber : Module/Chip serial number
- componentType : "Module" for module or "Front-ent Chip" for chip
- chipType : one of three: "RD53A", "FEI4B", or "FE65P2"
- chipId : chipId written in chip config file

### Register component data from connectivity file
<span style="color:red">PLAN: Cuurently the Module data is uploaded into Local DB from connectivity file so the file needs to be prepared properly but plans to be uploaded by another way into ITk PD and Local DB retrieves Module data from ITk PD.</span>
Module and Chip data have to be uploaded into Local DB before test them.
```bash
$ cd ${yarrdir}/src
$ ./bin/dbAccessor -C ../localdb/default/db_connectivity.json -u ${dbaccount}
```

### scanConsole
`scanConsole -W ${dbaccount}` can upload test data into Local DB after scan.
<span style="color:blue">(Acturally, this command just create cache files in local directory, so it works both on stable connection and unstable connection to Local DB Server. Then upload test data from cahce files by `dbAccessor -R "cache directory"`)</span>
```bash
$ cd ${yarrdir}/src
$ ./bin/scanConsole -c ../localdb/default/db_connectivity.json -r configs/controller/specCfg.json -s configs/scans/rd53a/std_digitalscan.json -W ${dbaccount}
```

### Upload data from cache files
<span style="color:red">PLAN: Currently cache directory is set in `Yarr/localDB/var` but plans to be shared with users on the same machine. </span>

```bash
$ cd ${yarrdir}/src
$ ./bin/dbAccessor -R ../localDB/var/cache/scan/${timestamp}
```
