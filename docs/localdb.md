# Local DB Setup and Tools Installation

<span style="color:red">"PLAN" is the item which is not implemented completely yet, but will be implemented soon.</span>

In order to setup the Local DB system the following steps are needed:

1. Install libraries for using Local DB system in YARR software (requirement: sudo)
2. Start Local DB server (requirement: sudo)
3. Start Viewer Application to check data in Local DB (requirement: sudo)
4. Setup Local DB system (requirement: sudo)
5. Compile YARR software including Local DB software
6. Scan with uploading data into Local DB

This instruction supports the following OS:
- centOS7

Local DB system is based on following systems (to be installed in the installation step):
- [MongoDB](https://docs.mongodb.com/v3.6/) v3.6.3 
- [mongocxx](https://github.com/mongodb/mongo-cxx-driver/tree/releases/v3.2) v3.2.0
- Python3 v3.6
- ROOT <span style="color:red">which can use PyROOT by Python3</span>

## 1. Install Libraries <span style="color:red">
(PLAN: to be automatic installation)</span>
Install necessary libraries and setup Local DB system for the machine in this step.
This step is required both for DB machine and DAQ machine.

### Requirements
- user account who is authorized for sudo 

- Make sure you have git and net-tools:
```bash
$ sudo yum install -y git net-tools
```

### Libraries

- Add yum repository for MongoDB <span style="color:red">(Support: MongoDB v3.6.3)</span>
Create `/etc/yum.repos.d/mongodb-org-3.6.repo` as below:
```bash
[mongodb-org-3.6]
name=MongoDB Repository
baseurl=https://repo.mongodb.org/yum/redhat/7Server/mongodb-org/3.6/x86_64/
gpgcheck=1
enabled=1
gpgkey=https://www.mongodb.org/static/pgp/server-3.6.asc
```

- Install by yum
```bash
$ sudo yum install -y epel-release.noarch centos-release-scl.noarch cmake bc.x86_64 wget.x86_64 rh-mongodb36-mongo-cxx-driver-devel.x86_64 mongodb-org.x86_64 devtoolset-7.x86_64 gnuplot.x86_64
```

### Confirmation
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

## 2. Start Local DB Server
Start Local DB Server based on MongoDB in this step.
This step is required for DB machine but not for DAQ machine.

### Requirements
- user account who is authorized for sudo 

### Set MongoDB config file
Change host IP address (default: '127.0.0.1') in MongoDB config file `/etc/mongod.conf` if you will access Local DB Server from another DAQ machine:
```bash
# mongod.conf

# for documentation of all options, see:
#   http://docs.mongodb.org/manual/reference/configuration-options/

# where to write logging data.
systemLog:
  destination: file
  logAppend: true
  path: /var/log/mongodb/mongod.log

# Where and how to store data.
storage:
  dbPath: /var/lib/mongo
  journal:
    enabled: true

# how the process runs
processManagement:
  fork: true  # fork and run in background
  pidFilePath: /var/run/mongodb/mongod.pid  # location of pidfile
  timeZoneInfo: /usr/share/zoneinfo

# network interfaces
net:
  bindIp: 127.0.0.1, "host IP address"  # Listen to local interface only, comment to listen on all interfaces.
  port: 27000
```

### Clone default data set
Clone default Local DB data set if you want to check by Viewer in the next step.
```bash
$ if [ -e /var/lib/mongo ]; then today=`date +%y%m%d`; cd /var/lib; sudo tar zcf mongo-${today}.tar.gz mongo; cd - > /dev/null; sudo rm -rf /var/lib/mongo; else sudo mkdir -p /var/lib/mongo; fi 
$ wget --no-check-certificate http://osksn2.hep.sci.osaka-u.ac.jp/~hirose/mongo_example.tar.gz
$ tar zxf mongo_example.tar.gz
$ sudo mv ./var/lib/mongo /var/lib
$ sudo rm -rf ./var
$ sudo chcon -R -u system_u -t mongod_var_lib_t /var/lib/mongo/
$ sudo chown -R mongod:mongod /var/lib/mongo
```

### Start MongoDB Server

```bash
# 1. Start MongoDB Server
$ sudo systemctl start mongod.service

# 2. Check MongoDB working
$ mongo --host localhost --port 27017
MongoDB shell version v3.6.8
connecting to: mongodb://localhost:27017
MongoDB server version: 3.6.8
Server has startup warnings: 
```

## 3. Start Viewer Application
Start Viewer Application to check data in Local DB Server in this step.
This step is required for DB machine but not for DAQ machine.

### Requirements
- user account who is authorized for sudo 

### Pre Install for Viewer Application
<span style="color:red">Viewer Application needs Python3 and PyROOT for displaying plots in browser.</span>
- Python3
```bash
# 1. Check if Python3 is available
$ python36 --version  
Python 3.6.X # Available
bash: python36: command not found # Not available

# 2. Install Python3 if not available
$ sudo yum install -y python.x86_64 python36 python36-devel python36-pip python36-tkinter 

# 3. Confirm if the installation was successful
$ python36 --version
Python 3.6.X
```

- pip3
```bash
# 1. Check if pip3 is availavle
$ pip3 --version
pip 8.1.2 from /usr/lib/python3.6/site-packages (python 3.6) # Available
bash: pip3: command not found # Not available

# 2. Install pip3 if not available
$ sudo yum install -y python.x86_64 python36 python36-devel python36-pip python36-tkinter 

# 3. Confirm if the installation was successful
$ pip3 --version
pip 8.1.2 from /usr/lib/python3.6/site-packages (python 3.6)
```

- ROOT software with compiling for using PyROOT
```bash
# 1. Check if PyROOT is available
$ for ii in 1 2 3 4; do  if pydoc3 modules | cut -d " " -f${ii} | grep -x ROOT > /dev/null; then echo "PyROOT is available"; fi;  done
PyROOT is available # Abailable
"No message" # Not available 

# 2. Install ROOT software which can use PyROOT by Python3
$ root_install_dir=`pwd`
$ wget https://root.cern.ch/download/root_v6.16.00.source.tar.gz
$ tar zxf https://root.cern.ch/download/root_v6.16.00.source.tar.gz
$ rm -f https://root.cern.ch/download/root_v6.16.00.source.tar.gz
$ mv root-6.16.00 6.16.00
$ mkdir 6.16.00-build
$ cd 6.16.00-build
$ cmake -DCMAKE_INSTALL_PREFIX=${root_install_dir}/6.16.00-install -DPYTHON_EXECUTABLE=/usr/bin/python36 ../6.16.00
$ cmake --build . -- -j4
$ make install

# 3. Confirm if the installation was successful
$ source ${root_install_dir}/6.16.00-build/bin/thisroot.sh
$ for ii in 1 2 3 4; do  if pydoc3 modules | cut -d " " -f${ii} | grep -x ROOT > /dev/null; then echo "PyROOT is available"; fi;  done
PyROOT is available
```

### Install packages
```bash
# 1. Clone Local DB tool repository to DB machine
$ git clone https://github.com/jlab-hep/localDB-tools.git
$ cd localDB-tools
$ db_tools_dir=`pwd`

# 2. Install pip modules
$ cd ${db_tools_dir}/setting
$ pip3 install -r requirements.txt
```

### Set Viewer Application config file
Set Viewer Application config file `my_web_configure.yml` in `${db_tools_dir}/viewer` by replicating from `${db_tools_dir}/scripts/yaml/web-conf.yml`:
- <span style="color:red">Change mongoDB.host (default: '127.0.0.1') if you run MongoDB on another machine</span>
- <span style="color:red">Change flask.host (default: '127.0.0.1') if you will access Viewer page on browser from another machine</span>
```bash
$ cd ${db_tools_dir}/viewer
$ cp ../scripts/yaml/web-conf.yml my_web_configure.yml
```

- my_web_configure.yml:
```bash
# Configs for web viewer

mongoDB:
    host: 127.0.0.1 # IP address running mongoDB
    port: 27017     # port number running mongoDB
    db: localdb     # local database name
    version: 1

flask:
    host: 127.0.0.1 # IP address running app.py
    port: 5000      # port number running app.py

# python version
python: 3
```

### Start Viewer Application <span style="color:red">(PLAN: Currently the application has to be manual running but will be automatic running)</span>
```bash
$ source ${root_install_dir}/6.16.00-build/bin/thisroot.sh
$ cd ${db_tools_dir}/viewer
$ python36 app.py --config my_web_configure.yml

Applying ATLAS style settings...

Connecto to mongoDB server: mongodb://127.0.0.1:27017/localdb
 * Serving Flask app "app" (lazy loading)
 * Environment: production
   WARNING: Do not use the development server in a production environment.
   Use a production WSGI server instead.
 * Debug mode: off
HI 2019-06-28 11:50:29,068 - werkzeug - INFO -  * Running on "http://127.0.0.1:5000/" (Press CTRL+C to quit)
```
---> Access "http://127.0.0.1:5000/" and Local DB Viewer page should be displayed on browser.

### Setup apache (Advance)
Set httpd protocol and open port for Viewer if you will access Viewer page on browser from another machine
```bash
# 1. installation
$ sudo yum install -y httpd.x86_64

# 2. setup http protocol
$ sudo /usr/sbin/setsebool -P httpd_can_network_connect 1
$ cp ${db_tools_dir}/scripts/apache/config.conf /etc/httpd/conf.d/localDB-tools.conf

# 3. open port
$ sudo firewall-cmd --add-service=http --permanent
$ sudo firewall-cmd --reload
$ sudo firewall-cmd --zone=public --add-port=5000/tcp --permanent
$ sudo firewall-cmd --reload

# 4. start
$ sudo systemctl start httpd  
$ sudo systemctl enable httpd
```
---> Enable to access "http://<ip address>:5000/" and Local DB Viewer page should be displayed on browser from another machine.

## 4. Setup Local DB system

![Local DB structure]()

## 5. Compile YARR software including Local DB software
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

### Compile with Make file <span style="color:red">(PLAN: to be prepared CMake scheme)</span>
```bash
$ cd YARR
$ yarr_dir=`pwd`
$ cd YARR/src
$ make -j4
<lots of text>
```

## 6. Scan with uploading data into Local DB
Scan Module with uploading data into Local DB in this step.

User can use scanConsole with Local DB system after setting machine: step0 and step4
Check step 4 for more detail about Local DB structure.

### Requirements
- YARR software with Local DB system (step5)

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
