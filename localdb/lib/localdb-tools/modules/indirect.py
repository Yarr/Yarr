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

def __log(args, serialnumber=None):
    global url
    global lines
    global size
    lines = 0
    size = shutil.get_terminal_size().lines-6

    params = {}

    params.update({ 'serialNumber': serialnumber })
    params.update({ 'dummy': args.dummy })
    params.update({ 'user': args.user })
    params.update({ 'site': args.site })

    viewer_url = '{0}/retrieve/log'.format(url)
    r_json = getJson(viewer_url, params)

    for test_data in r_json['log']:
        printLog('\033[1;33mtest data ID: {0} \033[0m'.format(test_data['runId'])) 
        printLog('User          : {0} at {1}'.format(test_data['user'], test_data['site']))
        printLog('Date          : {0}'.format(test_data['datetime']))
        printLog('Serial Number : {0}'.format(test_data['serialNumber']))
        printLog('Run Number    : {0}'.format(test_data['runNumber']))
        printLog('Test Type     : {0}'.format(test_data['testType']))
        printLog('DCS Data      :')
        if test_data.get('environment',[])==[]:
            printLog('DCS Data      : NULL')
        else:
            for key in test_data.get('environment',[]):
                printLog('DCS Data      : {}'.format(key))
        printLog('')

def __checkout(args, serialnumber=None, runid=None):
    global url
    configs = [{ 
        'type': 'ctrl',
        'name': 'controller',
        'col': 'testRun'
    },{
        'type': 'scan',
        'name': 'scan',
        'col': 'testRun'
    }]
    if not args.before or args.after:
        configs.append({
            'type': 'after',
            'name': 'chip(after)',
            'col': 'componentTestRun'
        })
    else:
        configs.append({
            'type': 'before',
            'name': 'chip(before)',
            'col': 'componentTestRun'
        })

    params = {}
    params.update({ 'serialNumber': serialnumber })
    params.update({ 'testRun': runid })
    params.update({ 'dummy': args.dummy })

    # get chip data
    viewer_url = '{0}/retrieve/component'.format(url)
    r_json = getJson(viewer_url, params)

    chip_data = r_json['chips']
    component_type = r_json['componentType']
    chip_type = r_json['chipType']

    # get run data
    viewer_url = '{0}/retrieve/testrun?testRun={1}'.format(url, r_json['testRun'])
    r_json = getJson(viewer_url)
    test_data = r_json 
    logger.info('test data information')
    logger.info('- Date          : {}'.format(test_data['datetime']))
    logger.info('- Serial Number : {}'.format(test_data['serialNumber']))
    logger.info('- Run Number    : {}'.format(test_data['runNumber']))
    logger.info('- Test Type     : {}'.format(test_data['testType']))
    logger.info('')

    # make directory
    if not args.directory: dir_path = './localdb-configs'
    else: dir_path = args.directory

    # get config data
    config_json = []
    test_data.update({ 'path': {} })
    for config in configs:
        for chip in chip_data:
            viewer_url = '{0}/retrieve/config?component={1}&testRun={2}&configType={3}'.format(url, chip['component'], test_data['testRun'], config['type'])
            r_json = getJson(viewer_url)
            if r_json['write']: 
                if config['col'] == 'testRun': 
                    file_path = '{0}/{1}'.format(dir_path, r_json['filename'])
                elif config['col'] == 'componentTestRun': 
                    file_path = '{0}/chip{1}-{2}'.format(dir_path, test_data['geomId'][chip['component']], r_json['filename'])
                config_data = {
                    'data': r_json['config'],
                    'path': file_path 
                } 
                test_data['path'][chip['component']] = 'chip{0}-{1}'.format(test_data['geomId'][chip['component']], r_json['filename'])
                config_json.append(config_data)
                logger.info('{0:<15} : {1:<10} --->   path: {2}'.format(config['name'], r_json['data'], file_path))
            else: 
                logger.info('{0:<15} : {1:<10}'.format(config['name'], r_json['data']))
            if config['col'] == 'testRun': break
 
    if component_type == 'Module':
        logger.info('{0:<15} : {1:<10} --->   path: {2}/{3}'.format('connectivity', 'Found', dir_path, 'connectivity.json'))
        conn_json = {
            'stage': 'Testing',
            'chipType': chip_type,
            'chips': []
        }
        if not args.dummy:
            conn_json.update({
                'module': {
                    'serialNumber': test_data['serialNumber'],
                    'componentType': 'Module'
                }
            })
        for chip in chip_data:
            chip_json = {
                'serialNumber': test_data['chips']['serialNumber'][chip['component']],
                'chipId': test_data['chips']['chipId'][chip['component']],
                'geomId': test_data['chips']['geomId'][chip['component']],
                'config': test_data['chips']['path'][chip['component']],
                'tx': test_data['chips']['tx'][chip['component']],
                'rx': test_data['chips']['rx'][chip['component']]
            }
            conn_json['chips'].append(chip_json)
        config_data = {
            'data': conn_json,
            'path': '{0}/connectivity.json'.format(dir_path) 
        }
        config_json.append(config_data)

    # make config files
    if os.path.isdir(dir_path): 
        shutil.rmtree(dir_path)
    os.makedirs(dir_path)

    for config in config_json:
        with open('{0}'.format(config['path']), 'w') as f: json.dump( config['data'], f, indent=4 )

def __fetch(args, remote):
    global url

    db_path = os.environ['HOME']+'/.localdb_retrieve'
    ref_path = db_path+'/refs/remotes'
    if not os.path.isdir(ref_path): 
        os.makedirs(ref_path)
 
    remote_path = ref_path+'/'+remote
    remote_file = open(remote_path, 'w')
    remote_data = { 'modules': [] }
    viewer_url = '{0}/retrieve/remote'.format(url)
    r_json = getJson(viewer_url)

    for module in r_json['modules']:
        if not module == '':
            remote_file.write('{}\n'.format(module['serialNumber']))
    remote_file.close()
    logger.info('Download Component Data of Local DB locally...')
    for j, module in enumerate(r_json['modules']):
        printLog('--------------------------------------')
        printLog('Component ({})'.format(j+1))
        printLog('    Chip Type: {}'.format(module['chipType']))
        printLog('    Module:')
        printLog('        serial number: {}'.format(module['serialNumber']))
        printLog('        component type: {}'.format(module['componentType']))
        printLog('        chips: {}'.format(len(module['chips'])))
        for i, chip in enumerate(module['chips']):
            printLog('    Chip ({}):'.format(i+1))
            printLog('        serial number: {}'.format(chip['serialNumber']))
            printLog('        component type: {}'.format(chip['componentType']))
            printLog('        chip ID: {}'.format(chip['chipId']))
    printLog('--------------------------------------')
    printLog('Done.')

    sys.exit()
