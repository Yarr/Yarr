# scanConsole with Local DB

## Settings
- 'Makefile'
  - description: install required softwares and setup Local DB functions for the machine.
  - requirements: sudo user, git, net-tools
- './setup_db.sh'
  - description: setup Local DB functions for the user local.
  - requirements: required softwares

## LocalDB Tools
'source ${HOME}/.local/lib/localdb-tools/enable' can enable tab-completion

### LocalDB Tool Setup Upload Tool
- 'localdbtool-upload --scan <path to result directory>' can upload scan data
- 'localdbtool-upload --dcs <path to result directory>' can upload dcs data based on scan data
- 'localdbtool-upload --cache' can upload every cache data
- 'localdbtool-upload --help' can show more usage.

### LocalDB Tool Setup Retrieve Tool
- 'localdbtool-retrieve init' can initialize retrieve repository
- 'localdbtool-retrieve remote add <remote name>' can add remote repository for Local DB/Master Server
- 'localdbtool-retrieve --help' can show more usage.

## Usage
1. scanConsole with Local DB
   - './bin/scanConsole -c <conn> -r <ctr> -s <scan> -W' can use Local DB schemes
2. Upload function
   - 'localdbtool-upload --cache' can upload every cache data
3. Retrieve function
   - 'localdbtool-retrieve log' can show test data log in Local DB
   - 'localdbtool-retrieve checkout <module name>' can restore the latest config files from Local DB
4. Viewer Application
   - Access 'http://127.0.0.1/localdb/' can display results in web browser if Viewer is running
5. More Detail
   - Check 'https://github.com/jlab-hep/Yarr/wiki'

