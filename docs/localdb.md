# Local DB: Local Database

**Local Database (Local DB) is mainly data management/storage system based on MongoDB for YARR.** <br>
Please look at [Local DB Docs](https://localdb-docs.readthedocs.io/en/master/) to setup Local DB System and more detail.

## Local DB Setting

### 1. Setup YARR command with Local DB

```bash
$ cd YARR
$ cmake3 ../
$ make -j4
$ make install
$ cd ../
```

### 2. Setup Local DB command

```bash
$ ./localdb/setup_db.sh
[LDB] Set editor command ... ## Enter editor command you use (e.g. vim, emacs)
[LDB] > vim
<some texts>
####################################################
## 1. Confirmation of python package requirements ##
## 2. Setting of config files                     ##
##    - Local DB config file                      ##
##    - User config file                          ##
##    - Site config file                          ##
## 3. Confirmation of the connection to Local DB  ##
####################################################
[LDB] More detail:
[LDB]   Access 'https://localdb-docs.readthedocs.io/en/master/'
```

### 3. Confirmation

```bash
$ ./localdb//bin/localdbtool-upload init
[  info  ][   Local DB    ]: ------------------------------
[  info  ][   Local DB    ]: Function: Initialize upload function and check connection to Local DB
[  info  ][   Local DB    ]: -> Setting database config: /root/.yarr/localdb/guest0_database.json (default)
[  info  ][   Local DB    ]: Checking connection to DB Server: mongodb://127.0.0.1:27000/localdb ...
[  info  ][   Local DB    ]: ---> Good connection!
[  info  ][   Local DB    ]: ------------------------------

$ ./localdb//bin/localdbtool-retrieve init
[  info  ][   Local DB    ]: -----------------------
[  info  ][   Local DB    ]: Function: Initialize
[  info  ][   Local DB    ]: Checking connection to DB Server: mongodb://127.0.0.1:27000/localdb ...
[  info  ][   Local DB    ]: ---> Good connection!
[  info  ][   Local DB    ]: -----------------------

$ ./bin/dbAccessor -N
[  info  ][  dbAccessor   ]: DBHandler: Check DB Connection
[  info  ][   Local DB    ]: ------------------------------
[  info  ][   Local DB    ]: Function: Initialize upload function and check connection to Local DB
[  info  ][   Local DB    ]: -> Setting database config: /root/.yarr/localdb/guest0_database.json (default)
[  info  ][   Local DB    ]: Checking connection to DB Server: mongodb://127.0.0.1:27000/localdb ...
[  info  ][   Local DB    ]: ---> Good connection!
[  info  ][   Local DB    ]: ------------------------------
```

## Quick Tutorial

### Upload

You can scan and upload the test data into Local DB by `scanConsole -W`. <br>
First please confirm if the default config files are prepared, the commands are enabled, and the connection is established using `setup_db.sh` following [this step](#2-setup-localdb-command),
and `scanConsole` use `${HOME}/.yarr/localdb/${HOSTNAME}_database.json`, `${HOME}/.yarr/localdb/user.json`, and `${HOME}/.yarr/localdb/${HOSTNAME}_site.json` as default config files.

```bash
$ ./bin/scanConsole \
-r configs/controller/emuCfg.json \
-c configs/connectivity/example_fei4b_setup.json \
-s configs/scans/fei4/std_digitalscan.json \
-W
<lots of text>
[  info  ][   Local DB    ]: Succeeded uploading scan data from /home/YARR/data/006237_std_digitalscan
[  info  ][   Local DB    ]: ------------------------------
```

You can check the log related to Local DB in log file `${HOME}/.yarr/localdb/log/log`:

### Retrieve

You can check the uploaded test data log by `dbAccessor -L`:

```bash
$ ./bin/dbAccessor -L
[  info  ][  dbAccessor   ]: DBHandler: Retrieve Test Log
[  info  ][   Local DB    ]: -----------------------
[  info  ][   Local DB    ]: Checking connection to DB Server: mongodb://127.0.0.1:27017/localdb ...
[  info  ][   Local DB    ]: ---> Good connection!
[  info  ][   Local DB    ]: -----------------------
test data ID: 5f4bb2f2a77dc1518b904e5d
User      : test_user at cern
Date      : 2020/08/30 23:08:27
Component : JohnDoe_0, DisabledChip_1
Run Number: 6236
Test Type : std_digitalscan
DCS Data  : NULL

test data ID: 5f4baef2daeb8bb44c258bb4
User      : test_user at cern
Date      : 2020/08/30 22:51:22
Component : JohnDoe_0, DisabledChip_1
Run Number: 6235
Test Type : std_digitalscan
DCS Data  : NULL

test data ID: 5f4baea6e0d2327bd8899c17
User      : test_user at cern
Date      : 2020/08/30 22:50:07
Component : JohnDoe_0, DisabledChip_1
Run Number: 6234
Test Type : std_digitalscan
DCS Data  : NULL
# Ctrl+C can terminate the output test log
```

You can retrieve the uploaded data into the local directory by `dbAccessor.`:

```bash
$ ./bin/dbAccessor -D
[  info  ][  dbAccessor   ]: DBHandler: Retrieve Config Files
[  info  ][   Local DB    ]: -----------------------
[  info  ][   Local DB    ]: Checking connection to DB Server: mongodb://127.0.0.1:27017/localdb ...
[  info  ][   Local DB    ]: ---> Good connection!
[  info  ][   Local DB    ]: Retrieve/Create data files in ./db-data
[warning ][   Local DB    ]: Already exist directory: ./db-data.

Override it? [y/n] (Exit without doing anything if "n")
> y

[  info  ][   Local DB    ]: testRun data ID: 5f4bb2f2a77dc1518b904e5d
[  info  ][   Local DB    ]: - User      : test_user at cern
[  info  ][   Local DB    ]: - Date      : 2020/08/30 23:08:27
[  info  ][   Local DB    ]: - Chips     : JohnDoe_0, DisabledChip_1
[  info  ][   Local DB    ]: - Run Number: 6236
[  info  ][   Local DB    ]: - Test Type : std_digitalscan
[  info  ][   Local DB    ]: - Stage     : ...
[  info  ][   Local DB    ]: Retrieve ... ./db-data/ctrlCfg.json
[  info  ][   Local DB    ]: Retrieve ... ./db-data/dbCfg.json
[  info  ][   Local DB    ]: Retrieve ... ./db-data/siteCfg.json
[  info  ][   Local DB    ]: Retrieve ... ./db-data/userCfg.json
[  info  ][   Local DB    ]: Retrieve ... ./db-data/std_digitalscan.json
[  info  ][   Local DB    ]: Retrieve ... ./db-data/JohnDoe_0_EnMask.json
[  info  ][   Local DB    ]: Retrieve ... ./db-data/JohnDoe_0_OccupancyMap.json
[  info  ][   Local DB    ]: Retrieve ... ./db-data/JohnDoe_0_L1Dist.json
[  info  ][   Local DB    ]: Retrieve ... ./db-data/rd53a_test.json
[  info  ][   Local DB    ]: Retrieve ... ./db-data/rd53a_test.json.before
[  info  ][   Local DB    ]: Retrieve ... ./db-data/rd53a_test.json.after
[  info  ][   Local DB    ]: Retrieve ... ./db-data/connectivity.json
[  info  ][   Local DB    ]: Retrieve ... ./db-data/scanLog.json
[warning ][   Local DB    ]: Please confirm "./db-data/connectivity" before running scanConsole.
[  info  ][   Local DB    ]: -----------------------
```

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

### Register Component

You can register the component data and upload test data associated with the registered component data.<br>
First please prepare connectivity file following [this sample format](https://localdb-docs.readthedocs.io/en/master/config/) and register by `dbAccessor -C -c <component connectivity file> -u <user config file> -i <site config file>`:<br>

```bash
$ ./bin/dbAccessor -C -c component.json -u user.json -i site.json
[  info  ][  dbAccessor   ]: DBHandler: Register Component Data
[  info  ][   Local DB    ]: ------------------------------
[  info  ][   Local DB    ]: Function: Register component data from specified connectivity file
[  info  ][   Local DB    ]: Component Config File: component.json
[  info  ][   Local DB    ]: -> Setting user config: user.json
[  info  ][   Local DB    ]: -> Setting site config: site.json
[  info  ][   Local DB    ]: -> Setting database config: database.json (default)
[  info  ][   Local DB    ]: Checking connection to DB Server: mongodb://127.0.0.1:27017/localdb ...
[  info  ][   Local DB    ]: ---> Good connection!
[  info  ][   Local DB    ]: Loading user information ...
[  info  ][   Local DB    ]: ~~~ {
[  info  ][   Local DB    ]: ~~~     "name": "Test User",
[  info  ][   Local DB    ]: ~~~     "institution": "CERN"
[  info  ][   Local DB    ]: ~~~ }
[  info  ][   Local DB    ]: Loading site information ...
[  info  ][   Local DB    ]: ~~~ {
[  info  ][   Local DB    ]: ~~~     "institution": "CERN"
[  info  ][   Local DB    ]: ~~~ }
[  info  ][   Local DB    ]: Component Data:
[  info  ][   Local DB    ]:     Chip Type: RD53A
[  info  ][   Local DB    ]:     Module:
[  info  ][   Local DB    ]:         serial number: RD53A-001
[  info  ][   Local DB    ]:         component type: Module
[  info  ][   Local DB    ]:         # of chips: 1
[  info  ][   Local DB    ]:     Chip (1):
[  info  ][   Local DB    ]:         serial number: RD53A-001_chip1
[  info  ][   Local DB    ]:         component type: Front-end Chip
[  info  ][   Local DB    ]:         chip ID: 0
[warning ][   Local DB    ]: It will be override with the provided infomation if data already exists in Local DB.
[warning ][   Local DB    ]: Do you continue to upload data into Local DB? [y/n]
[warning ][   Local DB    ]: (Please answer Y/y to continue or N/n to exit.)
[y/n]: y
[  info  ][   Local DB    ]: Succeeded uploading component data
[  info  ][   Local DB    ]:
[  info  ][   Local DB    ]: You can retrieve config files from Local DB by
[  info  ][   Local DB    ]:     localdbtool-retrieve pull --chip RD53A-001
[  info  ][   Local DB    ]: ------------------------------
```

After registration, you can retrieve/generate the connectivity config file and the chip config files by `localdb-retrieve pull --chip <SERIAL NUMBER>`:

```bash
$ ./localdb/bin/localdbtool-retrieve pull --chip RD53A-001
[  info  ][   Local DB    ]: -----------------------
[  info  ][   Local DB    ]: Checking connection to DB Server: mongodb://127.0.0.1:27017/localdb ...
[  info  ][   Local DB    ]: ---> Good connection!
[  info  ][   Local DB    ]: Retrieve/Create data files in ./db-data
[warning ][   Local DB    ]: Already exist directory: ./db-data.

Override it? [y/n] (Exit without doing anything if "n")
> y

[  info  ][   Local DB    ]: component data ID: 5f4c8ba810d11528f565e182
[  info  ][   Local DB    ]: - Parent    : RD53A-001 (module)
[  info  ][   Local DB    ]: - Chip Type : RD53A
[  info  ][   Local DB    ]: - Chips     : RD53A-001_chip1
[  info  ][   Local DB    ]: - Stage     : Testing
[  info  ][   Local DB    ]: Retrieve ... ./db-data/RD53A-001_chip1.json
[  info  ][   Local DB    ]: Retrieve ... ./db-data/connectivity.json
[warning ][   Local DB    ]: Please confirm "./db-data/connectivity" before running scanConsole.
[  info  ][   Local DB    ]: -----------------------
```

or `dbAccessor -D -n <SERIALNUMBER>`:

```bash
./bin/dbAccessor -D -n RD53A-001_chip1
[  info  ][  dbAccessor   ]: DBHandler: Retrieve Config Files
[  info  ][   Local DB    ]: -----------------------
[  info  ][   Local DB    ]: Checking connection to DB Server: mongodb://127.0.0.1:27017/localdb ...
[  info  ][   Local DB    ]: ---> Good connection!
[  info  ][   Local DB    ]: Retrieve/Create data files in ./db-data
[warning ][   Local DB    ]: Already exist directory: ./db-data.

Override it? [y/n] (Exit without doing anything if "n")
> y

[  info  ][   Local DB    ]: component data ID: 5f4c8ba810d11528f565e183
[  info  ][   Local DB    ]: - Chip Type : RD53A
[  info  ][   Local DB    ]: - Chips     : RD53A-001_chip1
[  info  ][   Local DB    ]: - Stage     : Testing
[  info  ][   Local DB    ]: Retrieve ... ./db-data/RD53A-001_chip1.json
[  info  ][   Local DB    ]: Retrieve ... ./db-data/connectivity.json
[warning ][   Local DB    ]: Please confirm "./db-data/connectivity" before running scanConsole.
[  info  ][   Local DB    ]: -----------------------
```

And you can upload test data associated with component data using these config files by `scanConsole`.

```bash
$ ./bin/scanConsole \
-r configs/controller/emuCfg.json \
-c db-data/<SERIAL NUMBER>.json \
-s configs/scans/fei4/std_digitalscan.json \
-W \
-u user.json \
-i site.json
<lots of text>
[  info  ][   Local DB    ]: Succeeded uploading scan data from /home/YARR/data/006237_std_digitalscan
[  info  ][   Local DB    ]: ------------------------------
```

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
$ ./app.py --config conf.yml
# ---> Access 'http://127.0.0.1:5000/localdb/' or
#      'http://IPaddress/localdb/' on browser to check data in Local DB
```

#### Synchronization Tool

```bash
# 1. Set Tool
$ cd localdb-tools/sync-tool
$ ./setup_sync_tool.sh

# 2. Run Tool
$ ./bin/localdbtool-sync.py --sync-opt <option> --config my_configure.yml
```

#### Archive Tool

```bash
# 1. Set Tool
$ cd localDB-tools/archive-tool
$ ./setup_archive_tool.sh

# 2. Run Tool
$ ./bin/localdbtool-archive.sh --config my_archive_configure.yml
```
