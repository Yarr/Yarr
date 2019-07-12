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
import json

import gridfs             
from pymongo          import MongoClient
from bson.objectid    import ObjectId 
from datetime         import datetime, timezone, timedelta
from dateutil.tz      import tzlocal
from tzlocal          import get_localzone
import pytz

authentication = False
global user
global pwd
global url
global localdb

def setTime(date):
    local_tz = get_localzone() 
    converted_time = date.replace(tzinfo=timezone.utc).astimezone(local_tz)
    time = converted_time.strftime('%Y/%m/%d %H:%M:%S')
    return time

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

def getConfigJson(cmp_oid, config, run_oid):
    global localdb
    fs = gridfs.GridFS(localdb)

    r_json = {}
    
    query = { 
        'component': cmp_oid,
        'testRun': run_oid
    }
    this_ctr = localdb.componentTestRun.find_one(query)
    if not this_ctr:
        print('ERROR: Not found test data: component: {0}, run: {1}'.format( cmp_oid, run_oid ))
        sys.exit()
    if config == 'ctrl' or config == 'scan':
        query = { '_id': ObjectId(run_oid) }
        this_run = localdb.testRun.find_one(query)
    elif config == 'after' or config== 'before':
        this_run = this_ctr
    else:
        print('ERROR: Not exist config type: {}'.format( config ))
        sys.exit()
    
    if this_run['{}Cfg'.format(config)] == '...':
        r_json.update({ 'data': 'Not found', 'write': False })
    else:
        query = { '_id': ObjectId(this_run['{}Cfg'.format(config)]) }
        this_cfg = localdb.config.find_one(query)
        r_json.update({ 
            'data': 'Found',
            'write': True,
            'config': json.loads(fs.get(ObjectId(this_cfg['data_id'])).read().decode('ascii')),
            'filename': this_cfg['filename'],
        }) 

    return r_json

def __log(args, serialnumber=None):
    global localdb
    global url

    max_server_delay = 1
    localdb = MongoClient(url, serverSelectionTimeoutMS=max_server_delay)['localdb']
    if authentication:
        localdb.authenticate(user, pwd)
    
    global lines
    global size
    lines = 0
    size = shutil.get_terminal_size().lines-4

    arg_vars = vars(args)
    run_query = {}
    log_query = {}
    if args.dummy:
        log_query.update({'dummy': True})
    elif serialnumber:
        query = { 'serialNumber': serialnumber }
        this_cmp = localdb.component.find_one(query)
        if not this_cmp:
            print('ERROR: Not found component data: {}'.format(serialnumber))
            sys.exit()
        run_query.update({ 'component': str(this_cmp['_id']) })

    if not run_query == {}:
        run_entries = localdb.componentTestRun.find(run_query)
        run_oids = []
        for run_entry in run_entries:
            run_oids.append({ '_id': ObjectId(run_entry['testRun']) })
        log_query.update({ '$or': run_oids })

    if arg_vars.get('user',None):
        query = { 'userName': arg_vars['user'] }
        this_user = localdb.user.find_one( query )
        if not this_user:
            print('ERROR: Not found user data: {}'.format(arg_vars['user']))
            sys.exit()
        log_query.update({ 'user_id': str(this_user['_id']) })
    if arg_vars.get('site',None):
        query = { 'institution': arg_vars['site'] }
        this_site = localdb.user.find_one( query )
        if not this_site:
            print('ERROR: Not found site data: {}'.format(arg_vars['site']))
            sys.exit()
        log_query.update({ 'address': str(this_site['_id']) })

    run_entries = localdb.testRun.find(log_query).sort([( '$natural', -1 )])

    r_json = { 'log': [] }
    if run_entries:
        for run_entry in run_entries:
            query = { '_id': ObjectId(run_entry['user_id']) }
            this_user = localdb.user.find_one( query )
            query = { '_id': ObjectId(run_entry['address']) }
            this_site = localdb.institution.find_one( query )
            test_data = {
                'user': this_user['userName'],
                'site': this_site['institution'],
                'datetime': setTime(run_entry['startTime']),
                'runNumber': run_entry['runNumber'],
                'testType': run_entry['testType'],
                'runId': str(run_entry['_id']),
                'serialNumber': run_entry['serialNumber']
            }
            r_json['log'].append(test_data)

    for test_data in r_json['log']:
        printLog('\033[1;33mtest data ID: {0} \033[0m'.format(test_data['runId'])) 
        printLog('User          : {0} at {1}'.format(test_data['user'], test_data['site']))
        printLog('Date          : {0}'.format(test_data['datetime']))
        printLog('Serial Number : {0}'.format(test_data['serialNumber']))
        printLog('Run Number    : {0}'.format(test_data['runNumber']))
        printLog('Test Type     : {0}'.format(test_data['testType']))
        printLog('')

def __checkout(args, serialnumber=None, runid=None):
    global localdb
    global url

    max_server_delay = 1
    localdb = MongoClient(url, serverSelectionTimeoutMS=max_server_delay)['localdb']
    fs = gridfs.GridFS(localdb)
    if authentication:
        localdb.authenticate(user, pwd)

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

    run_oid = None
    if args.dummy:
        query = { 'dummy': True }
        run_entry = localdb.testRun.find(query).sort([( '$natural', -1 )]).limit(1)
        if not run_entry.count()==0:
            run_oid = str(run_entry[0]['_id'])
            serialnumber = run_entry[0]['serialNumber']
    elif runid:
        query = { 'testRun': runid }
        this_run = localdb.componentTestRun.find_one(query)
        if this_run:
            run_oid = runid 
            query = { '_id': ObjectId(run_oid) }
            this_run = localdb.testRun.find_one(query)
            serialnumber = this_run['serialNumber']
    elif serialnumber:
        query = { 'serialNumber': serialnumber }
        this_cmp = localdb.component.find_one(query)
        if this_cmp:
            query = { 'component': str(this_cmp['_id']) }
            run_entry = localdb.componentTestRun.find(query).sort([( '$natural', -1 )]).limit(1)
            if not run_entry.count()==0:
                run_oid = run_entry[0]['testRun']
    else:
        run_entry = localdb.testRun.find({}).sort([( '$natural', -1 )]).limit(1)
        if not run_entry.count()==0:
            run_oid = str(run_entry[0]['_id'])
            serialnumber = run_entry[0]['serialNumber']

    if not run_oid:
        if serialnumber:
            print('ERROR: Not exist test data of the component: {}'.format(serialnumber))
        else:
            print('ERROR: Not exist test data')
        sys.exit()

    query = { 'serialNumber': serialnumber }
    this_cmp = localdb.component.find_one(query)
    if this_cmp: cmp_oid = str(this_cmp['_id'])
    else:        cmp_oid = serialnumber

    query = { '_id': ObjectId(run_oid) }
    this_run = localdb.testRun.find_one(query)
    chip_data = []
    if this_run['serialNumber'] == serialnumber:
        component_type = 'Module'
        chip_type = this_run['chipType']
        query = { 'testRun': run_oid, 'component': {'$ne': cmp_oid} }
        ctr_entries = localdb.componentTestRun.find(query)
        for ctr in ctr_entries:
            chip_data.append({ 'component': ctr['component'] })
    else:
        component_type = 'Front-end Chip'
        chip_type = this_run['chipType']
        chip_data.append({ 'component': cmp_oid })

    if chip_type == 'FE-I4B': chip_type = 'FEI4B'

    query = { '_id': ObjectId(this_run['user_id']) }
    this_user = localdb.user.find_one(query)

    query = { '_id': ObjectId(this_run['address']) }
    this_site = localdb.institution.find_one(query)

    query = { 'testRun': run_oid }
    run_entries = localdb.componentTestRun.find(query)
    test_data = {
        'testRun'     : run_oid,
        'runNumber'   : this_run['runNumber'],
        'testType'    : this_run['testType'],
        'datetime'    : setTime(this_run['startTime']),
        'serialNumber': this_run['serialNumber'],
        'user'        : this_user['userName'],
        'site'        : this_site['institution'],
        'geomId'      : {},
        'tx'          : {},
        'rx'          : {}
    }
    for run in run_entries:
        test_data['geomId'][run['component']] = run['geomId']
        test_data['tx'][run['component']] = run['tx']
        test_data['rx'][run['component']] = run['rx']
 
    print('test data information')
    print('- Date          : {}'.format(test_data['datetime']))
    print('- Serial Number : {}'.format(test_data['serialNumber']))
    print('- Run Number    : {}'.format(test_data['runNumber']))
    print('- Test Type     : {}'.format(test_data['testType']))
    print('')

    # make directory
    if not args.directory: dir_path = './localdb-configs'
    else: dir_path = args.directory

    # get config data
    config_json = []
    test_data.update({ 'path': {} })
    for config in configs:
        for chip in chip_data:
            r_json = getConfigJson(chip['component'], config['type'], test_data['testRun'])
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
                print('{0:<15} : {1:<10} --->   path: {2}'.format(config['name'], r_json['data'], file_path))
            else: 
                print('{0:<15} : {1:<10}'.format(config['name'], r_json['data']))
            if config['col'] == 'testRun': break
    
    if component_type == 'Module':
        print('{0:<15} : {1:<10} --->   path: {2}/{3}'.format('connectivity', 'Found', dir_path, 'connectivity.json'))
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
                'geomId': test_data['geomId'][chip['component']],
                'config': test_data['path'][chip['component']],
                'tx': test_data['tx'][chip['component']],
                'rx': test_data['rx'][chip['component']]
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
    global localdb
    global url

    max_server_delay = 1
    localdb = MongoClient(url, serverSelectionTimeoutMS=max_server_delay)['localdb']
    if authentication:
        localdb.authenticate(user, pwd)

    db_path = os.environ['HOME']+'/.yarr/localdb/.retrieve'
    ref_path = db_path+'/refs/remotes'
    if not os.path.isdir(ref_path): 
        os.makedirs(ref_path)
 
    remote_path = ref_path+'/'+remote
    remote_file = open(remote_path, 'w')
    remote_data = { 'modules': [] }
    query = { 'componentType': 'Module' }
    module_entries = localdb.component.find(query)
    for module in module_entries:
        if not module['serialNumber'] == '':
            remote_file.write('{}\n'.format(module['serialNumber']))
    remote_file.close()

    sys.exit()
