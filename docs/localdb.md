# Local DB: Local Database

**Local Database (Local DB) is mainly data management/storage system based on MongoDB for YARR.** <br>
Please look at [Local DB Docs](https://localdb-docs.readthedocs.io/en/master/) to setup Local DB System and more detail.

## Local DB Setting

### 1. Setup YARR command with Local DB

```bash
$ cd YARR
$ git checkout devel
$ mkdir build && cd build
$ cmake3 ../
$ make -j4
$ make install
$ cd ../
```

### 2. Setup Local DB command

```bash
$ ./localdb/setup_db.sh
[LDB] Confirmation
<some texts>
[LDB] Continue? [y/n]
[LDB] > y
<some texts>
[LDB] Create Config file...
[LDB] DB Config: ${HOME}/.yarr/localdb/${HOSTNAME}_database.json
[LDB] Done.
<some texts>
[LDB] More detail:
[LDB]   Access 'https://localdb-docs.readthedocs.io/en/master/'
```
> [Advanced tutorial for setup_db.sh](https://localdb-docs.readthedocs.io/en/master/install/)

### 3. Confirmation

```bash
$./localdb/bin/localdbtool-upload init
#DB INFO# -----------------------
#DB INFO# Function: Initialize
#DB INFO# [Connection Test] DB Server: mongodb://127.0.0.1:27017/localdb
#DB INFO# ---> Connection is GOOD.
#DB INFO# -----------------------
$
$./localdb/bin/localdbtool-retrieve init
#DB INFO# -----------------------
#DB INFO# Function: Initialize
#DB INFO# [Connection Test] DB Server: mongodb://127.0.0.1:27017/localdb
#DB INFO# ---> Connection is GOOD.
#DB INFO# -----------------------
$
$./bin/dbAccessor -I
#DB INFO# -----------------------
#DB INFO# Function: Initialize
#DB INFO# Local DB Server: mongodb://127.0.0.1:27017/localdb
#DB INFO# ---> connection is good.
#DB INFO# -----------------------
```



## Quick Tutorial

### Upload

You can scan and upload the test data into Local DB by `scanConsole -W`:

```bash
$ ./bin/scanConsole \
-r configs/controller/emuCfg.json \
-c configs/connectivity/example_fei4b_setup.json \
-s configs/scans/fei4/std_digitalscan.json \
-W
<lots of text>
#DB INFO# -----------------------
#DB INFO# Function: Initialize
#DB INFO# Local DB Server: mongodb://127.0.0.1:27017
#DB INFO# ---> connection is good.
#DB INFO# -----------------------
#DB INFO# Uploading in the back ground. (log: ~/.yarr/localdb/log/)
```
> [More detail about uploading test data](https://localdb-docs.readthedocs.io/en/master/upload/#upload-test-data)

You can check if the upload is success in log file `${HOME}/.yarr/localdb/log/${day}.log`:

```log
2019-08-01 10:55:46,821 - INFO: -----------------------
2019-08-01 10:55:46,821 - INFO: Function: Upload Scan Data
2019-08-01 10:55:46,823 - INFO: Local DB Server: mongodb://127.0.0.1:27017
2019-08-01 10:55:46,826 - INFO: ---> connection is good.
2019-08-01 10:55:46,826 - INFO: Cache Directory: /home/akubata/work/YARR/data/000186_std_digitalscan/
2019-08-01 10:55:47,058 - INFO: Success
2019-08-01 10:55:47,060 - INFO: -----------------------
```
> [More detail about another upload functions](https://localdb-docs.readthedocs.io/en/master/upload/)

### Retrieve

You can check the uploaded test data log by `localdbtool-retrieve log`:

```bash
$ ./localdb/bin/localdbtool-retrieve log
#DB INFO# -----------------------
#DB INFO# [Connection Test] DB Server: mongodb://127.0.0.1:27017
#DB INFO#    The connection is GOOD.
test data ID: 5d8da5eda45ae057dbd1fbd6
User      : user at site
Date      : 2019/09/27 15:02:17
Chip      : JohnDoe_0
Run Number: 5635
Test Type : std_digitalscan
DCS Data  : NULL
# Ctrl+C can terminate the output test log
```

You can retrieve the uploaded data into the local directory by `localdbtool-retrieve pull`:

```bash
$ ./localdb/bin/localdbtool-retrieve pull
#DB INFO# -----------------------
#DB INFO# [Connection Test] DB Server: mongodb://127.0.0.1:27017
#DB INFO#    The connection is GOOD.
#DB INFO# test data ID: 5d8da5eda45ae057dbd1fbd6
#DB INFO# - User      : user at site
#DB INFO# - Date      : 2019/09/27 15:02:17
#DB INFO# - Chips     : JohnDoe_0
#DB INFO# - Run Number: 5635
#DB INFO# - Test Type : std_digitalscan
#DB INFO# Retrieve ... ./db-data/ctrlCfg.json
#DB INFO# Retrieve ... ./db-data/dbCfg.json
#DB INFO# Retrieve ... ./db-data/siteCfg.json
#DB INFO# Retrieve ... ./db-data/userCfg.json
#DB INFO# Retrieve ... ./db-data/std_digitalscan.json
#DB INFO# Retrieve ... ./db-data/scanLog.json
#DB INFO# Retrieve ... ./db-data/JohnDoe_0_EnMask.dat
#DB INFO# Retrieve ... ./db-data/JohnDoe_0_OccupancyMap.dat
#DB INFO# Retrieve ... ./db-data/JohnDoe_0_beforeCfg.json
#DB INFO# Retrieve ... ./db-data/fei4b_test.json
#DB INFO# Retrieve ... ./db-data/JohnDoe_0_afterCfg.json
#DB INFO# Retrieve ... ./db-data/connectivity.json
#DB INFO# -----------------------
```
> [More detail about retrieving funtion](https://localdb-docs.readthedocs.io/en/master/retrieve/)

* List of restored data (default dir: `YARR/db_data`)
    * Test Information (Data ID, User, Date, Chips, Run #, Test type)
    * connectivity config file
    * controller config file
    * scan config file
    * chip config file (original/before/after)
    * result data file
    * database config file
    * user config file
    * site config file

### Local DB Tools

You can handle data in Local DB using Local DB Tools:

* [Viewer Application](#viewer-application)
* [Synchronization Tool](#sync-tool)
* [Archive Tool](#archive-tool)

```bash
$ git clone https://gitlab.cern.ch/YARR/localdb-tools.git
```

#### Viewer Application

```bash
# 1. Set Application
$ cd localdb-tools/viewer
$ ./setup_viewer.sh

# 2. Run Application
$ ./app.py --config conf.yml &
# ---> Access 'http://127.0.0.1:5000/localdb/' or
#      'http://IPaddress/localdb/' on browser to check data in Local DB
```
> [More detail about Viewer Application](https://localdb-docs.readthedocs.io/en/master/viewer/)

#### Synchronization Tool

```bash
# 1. Set Tool
$ cd localdb-tools/sync-tool
$ ./setup_sync_tool.sh

# 2. Run Tool
$ ./bin/localdbtool-sync.py --sync-opt <option> --config my_configure.yml
```
> [More detail about Synchronization Tool](https://localdb-docs.readthedocs.io/en/master/sync/)

#### Archive Tool

```bash
# 1. Set Tool
$ cd localDB-tools/archive-tool
$ ./setup_archive_tool.sh

# 2. Run Tool
$ ./bin/localdbtool-archive.sh --config my_archive_configure.yml
```
> [More detail about Archive Tools](https://localdb-docs.readthedocs.io/en/master/archive/)

