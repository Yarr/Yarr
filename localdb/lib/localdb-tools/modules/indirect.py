#!/usr/bin/env python3
#################################
# Author: Arisa Kubota
# Email: arisa.kubota at cern.ch
# Date: July 2019
# Project: Local Database for YARR
#################################

# Common
import os
import sys
import shutil
import requests
import json
import signal

import argparse 
import yaml     # Read YAML config file

# log
from logging import getLogger
logger = getLogger("Log").getChild("sub")

global url

####################
### Response to Json
### exist: return {json}
### not exist: return {}
### not json file: error
def getJson(viewer_url, params={}):
    response = requests.get(viewer_url, params=params)
    try:
        r_json = response.json()
    except:
        logger.error('Something wrong in url and could not get json data')
        sys.exit()     
    if r_json.get('error'):
        logger.error(r_json['message'])
        sys.exit()
    return r_json

##########################
### Display log on console
def printLog(message):
    global lines
    global size
    if lines<size:
        print(message)
        lines+=1
    else:
        try:
            input(message)
        except KeyboardInterrupt:
            print('')
            sys.exit()

#########################
### Display test data log
### Searchable by
### - chip name (perfect match)
### - user name (partial match)
### - site name (partial match)
def __log(args, serialnumber=None):
    global url
    global lines
    global size
    lines = 0
    size = shutil.get_terminal_size().lines-6

    params = {}

    params.update({ 'chip': args.chip })
    params.update({ 'user': args.user })
    params.update({ 'site': args.site })

    viewer_url = '{0}/retrieve/log'.format(url)
    r_json = getJson(viewer_url, params)

    for test_data in r_json['log']:
        printLog('\033[1;33mtest data ID: {0} \033[0m'.format(test_data['runId'])) 
        printLog('User      : {0} at {1}'.format(test_data['user'], test_data['site']))
        printLog('Date      : {0}'.format(test_data['datetime']))
        printLog('Chip      : {0}'.format(', '.join(test_data['chips'])))
        printLog('Run Number: {0}'.format(test_data['runNumber']))
        printLog('Test Type : {0}'.format(test_data['testType']))
        if test_data.get('environment',{})=={}:
            printLog('DCS Data  : NULL')
        else:
            printLog('DCS Data  :')
            for chip in test_data.get('environment',{}):
                if args.chip==chip:
                    printLog('   \033[1;31m{0} ({1})\033[0m'.format(', '.join(test_data['environment'][chip]), chip))
                else:
                    printLog('   {0} ({1})'.format(', '.join(test_data['environment'][chip]), chip))
        printLog('')

######################
### Retrieve test data
### no input -> latest scan
### input chip name -> latest scan for the chip
### input test data ID -> scan specified by ID
### outputs:
### - test information
### - configs
### - data
def __pull(dir_path, args):
    global url

    params = {}
    params.update({ 'chip': args.chip })
    params.update({ 'test': args.test })
    params.update({ 'dir' : dir_path })

    # get chip data
    viewer_url = '{0}/retrieve/data'.format(url)
    r_json = getJson(viewer_url, params)

    logger.info('\033[1;33mtest data ID: {0} \033[0m'.format(r_json['info']['_id'])) 
    logger.info('- User      : {0} at {1}'.format(r_json['info']['user'], r_json['info']['site']))
    logger.info('- Date      : {}'.format(r_json['info']['date']))
    logger.info('- Chips     : {}'.format(', '.join(r_json['info']['chips'])))
    logger.info('- Run Number: {}'.format(r_json['info']['runNumber']))
    logger.info('- Test Type : {}'.format(r_json['info']['testType']))
    logger.info('')

    # get config data
    data_entries = []
    for entry in r_json['data']:
        if not entry['bool']:
            viewer_url = '{0}/retrieve/config?oid={1}&type={2}'.format(url, entry['data'], entry['type'])
            r_json = getJson(viewer_url)
            entry.update({ 'data': r_json['data'] })
        data_entries.append(entry)

    for data in data_entries:
        logger.info('Retrieve ... {}'.format(data['path']))
        if data['type']=='json':
            with open(data['path'], 'w') as f:
                json.dump(data['data'], f, indent=4)
        else:
            with open(data['path'], 'w') as f:
                f.write(data['data'])

#####################
### Display data list
### - component
### - user
### - site
def __list(opt):
    global url
    global lines
    global size
    lines = 0
    size = shutil.get_terminal_size().lines-6

    viewer_url = '{0}/retrieve/list?opt={1}'.format(url, opt)
    r_json = getJson(viewer_url)

    printLog('')
    if opt=='component':
        for docs in r_json['parent']:
            printLog('\033[1;33m{0}: {1} \033[0m'.format(docs['type'], docs['name'])) 
            printLog('User      : {0} at {1}'.format(docs['user'], docs['site']))
            printLog('Chip Type : {0}'.format(docs['asic']))
            printLog('Chips({0})  :'.format(len(docs['chips'])))
            for oid in docs['chips']:
                chip_docs = r_json['child'][oid]
                printLog('\033[1;33m    {0}: {1} \033[0m'.format(chip_docs['type'], chip_docs['name'])) 
                printLog('    User  : {0} at {1}'.format(chip_docs['user'], chip_docs['site']))
                printLog('    ChipId: {0}'.format(chip_docs['chipId']))
                del r_json['child'][oid]
            printLog('')
        for oid in r_json['child']:
            docs = r_json['child'][oid]
            printLog('\033[1;33m{0}: {1} \033[0m'.format(docs['type'], docs['name'])) 
            printLog('User      : {0} at {1}'.format(docs['user'], docs['site']))
            printLog('Chip Type : {0}'.format(docs['asic']))
            printLog('ChipId    : {0}'.format(docs['chipId']))
            printLog('')
    elif opt=='user':
        for user in r_json:
            printLog('\033[1;33mUser Name: {0}\033[0m'.format(user)) 
            for docs in r_json[user]:
                printLog('- {0}'.format(docs))
            printLog('')
    elif opt=='site':
        for site in r_json['site']:
            printLog('- {0}'.format(site)) 
        printLog('')
