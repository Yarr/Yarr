# scanConsole with Local DB

## Codes in `YARR/localdb`

- db_yarr_install.sh (sudo required): 

   Check the requirement packages (yum/pip) for DAQ Server.<br>
   If there are any missing packages, this script displays them and asks if you want to install them.<br>
   If answer 'y' (yes), the installation will be done, but if not, this script will be terminated.<br>
   If the requirements are satisfied, this script displays 'Quick Tutorial' and output 'setting/README' with them written.

- './setup_db.sh'

   Check the settings of Local DB and requirements packages (pip). <br>
   If there are any missing or mistakes, this script display some error message and will be terminated.<br>
   If the requirements are satisfied, this script sets database config file in `~/.yarr/localdb/${HOSTNAME}_database.json` and displays the usage of localdb-tools and output 'README' with them written.

- setting

   - default
      
      There are default config files for Local DB

   - check_python_version.py/check_python_modules.py

      This scripts can checks if the python version and its module satisfy the requirements. 

   - requirements-yum.txt/requirements-pip.txt

      The requirements for Local DB

- bin

   There are binary commands of Local DB Tools

- lib 

   There are the libraries for Local DB Tools

## LocalDB Tools

### LocalDB Tool Setup Upload Tool

- 'YARR/localdb/bin/localdbtool-upload scan <path to result directory>' can upload scan data
- 'YARR/localdb/bin/localdbtool-upload dcs <path to result directory>' can upload dcs data based on scan data
- 'YARR/localdb/bin/localdbtool-upload cache' can upload every cache data
- 'YARR/localdb/bin/localdbtool-upload --help' can show more usage.

### LocalDB Tool Setup Retrieve Tool
- 'YARR/localdb/bin/localdbtool-retrieve init' can initialize retrieve repository
- 'YARR/localdb/bin/localdbtool-retrieve remote add <remote name>' can add remote repository for Local DB/Master Server
- 'YARR/localdb/bin/localdbtool-retrieve --help' can show more usage.

## Usage
1. scanConsole with Local DB
   - 'bin/scanConsole -c <conn> -r <ctr> -s <scan> -W' can use Local DB schemes
2. Upload function
   - 'localdb/bin/localdbtool-upload cache' can upload every cache data
3. Retrieve function
   - 'localdb/bin/localdbtool-retrieve log' can show test data log in Local DB
   - 'localdb/bin/localdbtool-retrieve checkout <module name>' can restore the latest config files from Local DB
4. Viewer Application
   - Access 'http://127.0.0.1/localdb/' can display results in web browser if Viewer is running
5. More Detail
   - Check 'https://github.com/jlab-hep/Yarr/wiki'

