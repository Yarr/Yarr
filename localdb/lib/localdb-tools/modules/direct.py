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
from pymongo          import MongoClient, ASCENDING, DESCENDING
from bson.objectid    import ObjectId 
from datetime         import datetime, timezone, timedelta
from dateutil.tz      import tzlocal
from tzlocal          import get_localzone
import pytz

# log
from logging import getLogger
logger = getLogger("Log").getChild("sub")
global localdb
# version of Local DB
db_version = 1.01

#########################################################
### Set localdb setting from localdb/bin/localdb-retrieve
def __set_localdb(i_localdb):
    global localdb
    localdb = i_localdb

###########################
### Set timestamp to string
def setTime(date):
    local_tz = get_localzone() 
    converted_time = date.replace(tzinfo=timezone.utc).astimezone(local_tz)
    time = converted_time.strftime('%Y/%m/%d %H:%M:%S')
    return time

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
##################################
### Get data document from data_id
def getData(i_type, i_dir, i_filename, i_data, i_bool=False):
    fs = gridfs.GridFS(localdb)
    if i_bool:
        data = i_data
    elif i_type=='json':
        data = json.loads(fs.get(ObjectId(i_data)).read().decode('ascii'))
    else:
        data = fs.get(ObjectId(i_data)).read().decode('ascii') 
    docs = {
        'type': i_type,
        'path': '{0}/{1}'.format(i_dir, i_filename),
        'data': data
    }
    return docs

################
### File to Json
### exist: return {json}
### not exist: return {}
### not json file: error
def toJson(i_file_path):
    logger.debug('\t\t\tConvert to json code from: {}'.format(i_file_path))

    file_json = {}
    if i_file_path:
        if os.path.isfile(i_file_path):
            try:
                with open(i_file_path, 'r') as f: file_json = json.load(f)
            except ValueError as e:
                logger.error('Could not parse {}'.format(i_file_path))
                logger.error('\twhat(): {}',format(e))
                logger.info('-----------------------')
                logger.debug('=======================')
                sys.exit(1)

    return file_json

#########################
### Display test data log
### Searchable by
### - chip name (perfect match)
### - user name (partial match)
### - site name (partial match)
def __log(args):
    global lines
    global size
    lines = 0
    size = shutil.get_terminal_size().lines-4

    arg_vars = vars(args)
    run_query = { 'dbVersion' : db_version }
    log_query = { 
        '$and': [], 
        'dbVersion' : db_version 
    }
    
    chip_name = arg_vars.get('chip',None)
    if chip_name:
        query = { 
            'name'      : chip_name,
            'dbVersion' : db_version
        }
        chip_entries = localdb.chip.find(query)
        chip_query = []
        for this_chip in chip_entries:
            chip_query.append({ 'chip': str(this_chip['_id']) })
        if chip_query==[]:
            logger.error('Not found chip data: {}'.format(chip_name))
            sys.exit(1)
        else:
            run_query.update({ '$or': chip_query })

    if not run_query=={}:
        run_entries = localdb.componentTestRun.find(run_query)
        run_oids = []
        for run_entry in run_entries:
            run_oids.append({ '_id': ObjectId(run_entry['testRun']) })
        if not run_oids==[]: log_query['$and'].append({ '$or': run_oids })

    if arg_vars.get('user',None):
        query = { 
            'userName'  : {'$regex': arg_vars['user'].lower().replace(' ','_')}, 
            'dbVersion' : db_version
        }
        entries = localdb.user.find(query)
        if entries.count()==0:
            logger.error('Not found user data: {}'.format(arg_vars['user']))
            sys.exit()
        user_oids = []
        for entry in entries:
            user_oids.append({ 'user_id': str(entry['_id']) })
        if not user_oids==[]: log_query['$and'].append({ '$or': user_oids })
    if arg_vars.get('site',None):
        query = { 
            'institution': {'$regex': arg_vars['site'].lower().replace(' ','_')}, 
            'dbVersion'  : db_version
        }
        entries = localdb.institution.find( query )
        if entries.count()==0:
            logger.error('Not found site data: {}'.format(arg_vars['site']))
            sys.exit()
        site_oids = []
        for entry in entries:
            site_oids.append({ 'address': str(entry['_id']) })
        if not site_oids==[]: log_query['$and'].append({ '$or': site_oids })

    if log_query['$and']==[]: log_query = { 'dbVersion' : db_version }

    run_entries = localdb.testRun.find(log_query).sort([('startTime', DESCENDING)])

    r_json = { 'log': [] }
    for this_run in run_entries:
        query = { 
            '_id'       : ObjectId(this_run['user_id']),
            'dbVersion' : db_version   
        }
        this_user = localdb.user.find_one( query )
        query = { 
            '_id'       : ObjectId(this_run['address']), 
            'dbVersion' : db_version   
        }
        this_site = localdb.institution.find_one( query )
        query = { 
            'testRun': str(this_run['_id']),
            'dbVersion' : db_version   
        }
        ctr_entries  = localdb.componentTestRun.find(query)
        chips = []
        this_dcs = {}
        for this_ctr in ctr_entries:
            if chip_name and this_ctr['name']==chip_name:
                chips.append('\033[1;31m{}\033[0m'.format(this_ctr['name']))
            else:
                chips.append(this_ctr['name'])
            if not this_ctr.get('environment','...')=='...': 
                this_dcs.update({ this_ctr['name']: [] })
                query = { 
                    '_id'       : ObjectId(this_ctr['environment']), 
                    'dbVersion' : db_version   
                }
                this_env = localdb.environment.find_one( query)
                for key in this_env:
                    if not key=='_id' and not key=='dbVersion' and not key=='sys':
                        this_dcs[this_ctr['name']].append(key)
        test_data = {
            'user'       : this_user['userName'],
            'site'       : this_site['institution'],
            'datetime'   : setTime(this_run['startTime']),
            'runNumber'  : this_run['runNumber'],
            'testType'   : this_run['testType'],
            'runId'      : str(this_run['_id']),
            'chips'      : chips,
            'dbVersion'  : this_run['dbVersion'],
            'environment': this_dcs
        }
        r_json['log'].append(test_data)

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
                if chip_name==chip:
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
    def __pull_test_run(i_oid, i_chip): 
        data_entries = []
        query = {
            '_id': ObjectId(i_oid),
            'dbVersion': db_version
        }
        this_tr = localdb.testRun.find_one(query)
        chip_type = this_tr.get('chipType', 'NULL')
        if chip_type=='FE-I4B': chip_type = 'FEI4B'
        log_json = {}
        for key in this_tr:
            if 'Cfg' in key and not this_tr[key]=='...':
                query = { 
                    '_id'       : ObjectId(this_tr[key]),
                    'dbVersion' : db_version   
                }
                this_cfg = localdb.config.find_one(query)
                docs = getData('json', dir_path, this_cfg['filename'], this_cfg['data_id'])
                data_entries.append( docs )
            elif key=='_id':
                log_json.update({ key: str(this_tr[key]) })
            elif key=='startTime' or key=='finishTime':
                log_json.update({ key: setTime(this_tr[key]) })
            elif not key=='address' and not key=='user_id' and not key=='sys':
                log_json.update({ key: this_tr[key] })
        docs = getData('json', dir_path, 'scanLog.json', log_json, True)
        data_entries.append( docs )

        query = {
            'testRun': i_oid,
            'dbVersion': db_version
        }
        entries = localdb.componentTestRun.find(query)
        conn_json = {
            'stage': this_tr.get('stage', '...'),
            'chipType': chip_type,
            'chips': []
        }
        chips = []
        for this_ctr in entries:
            if this_ctr['chip']=='module':
                query = {
                    '_id': ObjectId(this_ctr['component']),
                    'dbVersion': db_version
                }
                this_cmp = localdb.component.find_one(query)
                conn_json.update({
                    'module': {
                        'serialNumber': this_cmp['name'],
                        'componentType': this_cmp['componentType']
                    }
                })
                continue
            chip_conn = {}
            for key in this_ctr:
                if key=='config':
                    chip_conn.update({ key: '{0}/{1}'.format(dir_path, this_ctr[key]) })
                elif 'Cfg' in key and not this_ctr[key]=='...':
                    query = { 
                        '_id'       : ObjectId(this_ctr[key]), 
                        'dbVersion' : db_version   
                    }
                    this_cfg = localdb.config.find_one(query)
                    docs = getData('json', dir_path, '{0}_{1}.json'.format(this_ctr['name'], key), this_cfg['data_id'])
                    data_entries.append( docs )
                    if key=='beforeCfg':
                        docs = getData('json', dir_path, this_ctr.get('config','{}.json'.format(this_ctr['name'])), this_cfg['data_id'])
                        data_entries.append( docs )
                elif key=='attachments':
                    for attachment in this_ctr[key]:
                        docs = getData(attachment['contentType'], dir_path, '{0}_{1}'.format(this_ctr['name'], attachment['filename']), attachment['code'])
                        data_entries.append( docs )
                elif not key=='_id' and not key=='component' and not key=='sys' and not key=='chip' and not key=='testRun' and not key=='environment' and not key=='dbVersion':
                    chip_conn.update({ key: this_ctr[key] })
            conn_json['chips'].append(chip_conn)
            if i_chip and this_ctr['name']==i_chip:
                chips.append('\033[1;31m{}\033[0m'.format(this_ctr['name']))
            else:
                chips.append(this_ctr['name'])
        docs = getData('json', dir_path, 'connectivity.json', conn_json, True)
        data_entries.append( docs )
    
        query = { 
            '_id'       : ObjectId(this_tr['user_id']), 
            'dbVersion' : db_version   
        }
        this_user = localdb.user.find_one(query)
    
        query = { 
            '_id'       : ObjectId(this_tr['address']), 
            'dbVersion' : db_version   
        }
        this_site = localdb.institution.find_one(query)
    
        logger.info('\033[1;33mtest data ID: {0} \033[0m'.format(str(this_tr['_id']))) 
        logger.info('- User      : {0} at {1}'.format(this_user['userName'], this_site['institution']))
        logger.info('- Date      : {}'.format(setTime(this_tr['startTime'])))
        logger.info('- Chips     : {}'.format(', '.join(chips)))
        logger.info('- Run Number: {}'.format(this_tr['runNumber']))
        logger.info('- Test Type : {}'.format(this_tr['testType']))
    
        for data in data_entries:
            logger.info('Retrieve ... {}'.format(data['path']))
            if data['type']=='json':
                with open(data['path'], 'w') as f:
                    json.dump(data['data'], f, indent=4)
            else:
                with open(data['path'], 'w') as f:
                    f.write(data['data'])
    def __pull_component(i_chip):
        data_entries = []
        query = { 
            'name': i_chip,
            'dbVersion': db_version
        }
        this_cmp = localdb.component.find_one(query)
        this_ch = localdb.chip.find_one(query) 
        if not this_cmp and not this_ch:
            logger.error('Not found chip data: {}'.format(i_chip))
            sys.exit(1)
        if this_cmp: this_chip = this_cmp
        else: this_chip = this_ch
        chip_type = this_chip.get('chipType','NULL')
        if chip_type=='FE-I4B': chip_type = 'FEI4B'
        conn_json = {
            'stage': 'Testing',
            'chipType': chip_type,
            'chips': []
        }
 
        chips = []
        query = {
            'parent': str(this_chip['_id']),
            'status': 'active',
            'dbVersion': db_version
        }
        entries = localdb.childParentRelation.find(query)
        if entries.count()==0:
            chips.append(str(this_chip['_id']))
        else:
            conn_json.update({
                'module': {
                    'serialNumber': this_chip['serialNumber'],
                    'componentType': this_chip['componentType']
                }
            })
            for entry in entries:
                chips.append(entry['child'])
            
        cfg_path = 'configs/defaults/default_{}.json'.format(chip_type.lower())
        if not os.path.isfile(cfg_path):
            logger.error('Not found default chip config: {}'.format(cfg_path))
            sys.exit(1)
        for i, chip in enumerate(chips):
            cfg_json = toJson(cfg_path)
            query = {
                '_id': ObjectId(chip),
                'dbVersion': db_version
            }
            this = localdb.component.find_one(query)
            if not this: continue
            if chip_type=='FEI4B': 
                cfg_json['FE-I4B']['name'] = this['name']
                cfg_json[chip_type]['Parameter']['chipId'] = this.get('chipId', 0) 
            else: 
                cfg_json[chip_type]['Parameter']['Name'] = this['name']
                cfg_json[chip_type]['Parameter']['ChipId'] = this.get('chipId', 0)
            docs = getData('json', dir_path, '{}.json'.format(this['name']), cfg_json, True)
            data_entries.append( docs )
            conn_json['chips'].append({
                'config': '{0}/{1}.json'.format(dir_path, this['name']),
                'tx': i,
                'rx': i
            })
        docs = getData('json', dir_path, 'connectivity.json', conn_json, True)
        data_entries.append( docs )

        logger.info('\033[1;33mcomponent data ID: {0} \033[0m'.format(str(this_chip['_id']))) 
        logger.info('- Name: {}'.format(i_chip))
        logger.info('- Type: {}'.format(chip_type))
    
        for data in data_entries:
            logger.info('Retrieve ... {}'.format(data['path']))
            if data['type']=='json':
                with open(data['path'], 'w') as f:
                    json.dump(data['data'], f, indent=4)
            else:
                with open(data['path'], 'w') as f:
                    f.write(data['data'])
###
    arg_vars = vars(args)

    tr_oid = None
    if arg_vars.get('test',None):
        tr_oid = arg_vars['test']
    elif arg_vars.get('chip',None):
        query = { 
            'name'      : arg_vars['chip'], 
            'dbVersion' : db_version   
        }
        entries = localdb.componentTestRun.find(query)
        if not entries.count()==0:
            query = { 
                '$or'       : [],
                'dbVersion' : db_version   
            }
            for entry in entries:
                query['$or'].append({ '_id': ObjectId(entry['testRun']) })
            entry = localdb.testRun.find(query).sort([('startTime', DESCENDING)]).limit(1)
            if not entry.count()==0:
                tr_oid = str(entry[0]['_id'])
    else:
        query = { 'dbVersion' : db_version }
        entry = localdb.testRun.find(query).sort([('startTime', DESCENDING)]).limit(1)
        if not entry.count()==0:
            tr_oid = str(entry[0]['_id'])
####
#    if not tr_oid:
#        if arg_vars.get('chip',None):
#            logger.error('Not found test data of the component: {}'.format(arg_vars['chip']))
#        else:
#            logger.error('Not found test data')
#        sys.exit()
#    else:
#        query = { 
#            'testRun'   : tr_oid,
#            'dbVersion' : db_version
#        }
#        entries = localdb.componentTestRun.find(query)
#        if entries.count()==0:
#            logger.error('Not test data ID: {}'.format(tr_oid))
#            sys.exit()
####
    if tr_oid:
        query = { 
            'testRun'   : tr_oid,
            'dbVersion' : db_version
        }
        entries = localdb.componentTestRun.find(query)
        if entries.count()==0 and arg_vars.get('chip',None):
            logger.warning('Not test data ID: {}'.format(tr_oid))
            __pull_component(arg_vars['chip'])
        elif entries.count()==0:
            logger.error('Not test data ID: {}'.format(tr_oid))
            sys.exit(1)
        else:
            __pull_test_run(tr_oid, arg_vars.get('chip', None))
    elif arg_vars.get('chip',None):
        logger.warning('Not found test data of the component: {}'.format(arg_vars['chip']))
        __pull_component(arg_vars['chip'])
    else:
        logger.error('Not found test data')
        sys.exit(1)
###
#    data_entries = []
#
#    query = { 
#        '_id'       : ObjectId(tr_oid),
#        'dbVersion' : db_version   
#    }
#    this_tr = localdb.testRun.find_one(query)
#    chip_type = this_tr.get('chipType','NULL')
#    if chip_type=='FE-I4B': chip_type = 'FEI4B'
#    log_json = {}
#    for key in this_tr:
#        if 'Cfg' in key and not this_tr[key]=='...':
#            query = { 
#                '_id'       : ObjectId(this_tr[key]),
#                'dbVersion' : db_version   
#            }
#            this_cfg = localdb.config.find_one(query)
#            docs = getData('json', dir_path, this_cfg['filename'], this_cfg['data_id'])
#            data_entries.append( docs )
#        elif key=='_id':
#            log_json.update({ key: str(this_tr[key]) })
#        elif key=='startTime' or key=='finishTime':
#            log_json.update({ key: setTime(this_tr[key]) })
#        elif not key=='address' and not key=='user_id' and not key=='sys':
#            log_json.update({ key: this_tr[key] })
#    docs = getData('json', dir_path, 'scanLog.json', log_json, True)
#    data_entries.append( docs )
#    
#    query = { 
#        'testRun'   : tr_oid, 
#        'dbVersion' : db_version   
#    }
#    entries = localdb.componentTestRun.find(query)
#    conn_json = {
#        'stage'   : this_tr.get('stage','...'),
#        'chipType': chip_type,
#        'chips'   : []
#    }
#    chips = []
#    for this_ctr in entries:
#        if this_ctr['chip']=='module':
#            query = { 
#                '_id'       : ObjectId(this_ctr['component']), 
#                'dbVersion' : db_version   
#            }
#            this_cmp = localdb.component.find_one(query)
#            conn_json.update({
#                'module': {
#                    'serialNumber' : this_cmp['name'],
#                    'componentType': this_cmp['componentType']
#                }
#            })
#            continue
#        chip_conn = {}
#        for key in this_ctr:
#            if key=='config':
#                chip_conn.update({ key: '{0}/{1}'.format(dir_path, this_ctr[key]) })
#            elif 'Cfg' in key and not this_ctr[key]=='...':
#                query = { 
#                    '_id'       : ObjectId(this_ctr[key]), 
#                    'dbVersion' : db_version   
#                }
#                this_cfg = localdb.config.find_one(query)
#                docs = getData('json', dir_path, '{0}_{1}.json'.format(this_ctr['name'], key), this_cfg['data_id'])
#                data_entries.append( docs )
#                if key=='beforeCfg':
#                    docs = getData('json', dir_path, this_ctr.get('config','{}.json'.format(this_ctr['name'])), this_cfg['data_id'])
#                    data_entries.append( docs )
#            elif key=='attachments':
#                for attachment in this_ctr[key]:
#                    docs = getData(attachment['contentType'], dir_path, '{0}_{1}'.format(this_ctr['name'], attachment['filename']), attachment['code'])
#                    data_entries.append( docs )
#            elif not key=='_id' and not key=='component' and not key=='sys' and not key=='chip' and not key=='testRun' and not key=='environment' and not key=='dbVersion':
#                chip_conn.update({ key: this_ctr[key] })
#        conn_json['chips'].append(chip_conn)
#        if arg_vars.get('chip',None) and this_ctr['name']==arg_vars['chip']:
#            chips.append('\033[1;31m{}\033[0m'.format(this_ctr['name']))
#        else:
#            chips.append(this_ctr['name'])
#    docs = getData('json', dir_path, 'connectivity.json', conn_json, True)
#    data_entries.append( docs )
#
#    query = { 
#        '_id'       : ObjectId(this_tr['user_id']), 
#        'dbVersion' : db_version   
#    }
#    this_user = localdb.user.find_one(query)
#
#    query = { 
#        '_id'       : ObjectId(this_tr['address']), 
#        'dbVersion' : db_version   
#    }
#    this_site = localdb.institution.find_one(query)
#
#    logger.info('\033[1;33mtest data ID: {0} \033[0m'.format(str(this_tr['_id']))) 
#    logger.info('- User      : {0} at {1}'.format(this_user['userName'], this_site['institution']))
#    logger.info('- Date      : {}'.format(setTime(this_tr['startTime'])))
#    logger.info('- Chips     : {}'.format(', '.join(chips)))
#    logger.info('- Run Number: {}'.format(this_tr['runNumber']))
#    logger.info('- Test Type : {}'.format(this_tr['testType']))
#
#    for data in data_entries:
#        logger.info('Retrieve ... {}'.format(data['path']))
#        if data['type']=='json':
#            with open(data['path'], 'w') as f:
#                json.dump(data['data'], f, indent=4)
#        else:
#            with open(data['path'], 'w') as f:
#                f.write(data['data'])
#
#####################
### Display data list
### - component
### - user
### - site
def __list(opt):
    if opt=='component': __list_component()
    elif opt=='user': __list_user()
    elif opt=='site': __list_site()
    
def __list_component():
    global lines
    global size
    lines = 0
    size = shutil.get_terminal_size().lines-4

    docs_list = { 'parent': [], 'child': {} }
    entries = localdb.childParentRelation.find({ 'dbVersion' : db_version })
    oids = []
    for entry in entries:
        oids.append(entry['parent'])
    oids = list(set(oids))
    for oid in oids:
        query = { '_id': ObjectId(oid) }
        this = localdb.component.find_one(query)
        query = { '_id': ObjectId(this['user_id']) }
        this_user = localdb.user.find_one( query )
        query = { '_id': ObjectId(this['address']) }
        this_site = localdb.institution.find_one( query )
        docs = {
            'oid' : oid,
            'name': this['name'],
            'type': this['componentType'],
            'asic': this['chipType'],
            'chips': [],
            'user': this_user['userName'],
            'site': this_site['institution']
        }
        query = { 
            'parent'    : oid,
            'dbVersion' : db_version   
        }
        entries = localdb.childParentRelation.find(query)
        for entry in entries:
            docs['chips'].append(entry['child'])
        docs_list['parent'].append(docs)
    query = { 
        'componentType': 'front-end_chip',
        'dbVersion'    : db_version   
    }
    entries = localdb.component.find(query)
    oids = []
    for entry in entries:
        oids.append(str(entry['_id']))
    for oid in oids:
        query = { '_id': ObjectId(oid) }
        this = localdb.component.find_one(query)
        query = { '_id': ObjectId(this['user_id']) }
        this_user = localdb.user.find_one( query )
        query = { '_id': ObjectId(this['address']) }
        this_site = localdb.institution.find_one( query )
        docs_list['child'].update({
            oid: {
                'name': this['name'],
                'type': this['componentType'],
                'asic': this['chipType'],
                'chipId': this['chipId'],
                'user': this_user['userName'],
                'site': this_site['institution']
            }
        })
    docs_list['parent'] = sorted(docs_list['parent'], key=lambda x:(x['type']))
    printLog('')
    for docs in docs_list['parent']:
        printLog('\033[1;33m{0}: {1} \033[0m'.format(docs['type'], docs['name'])) 
        printLog('User      : {0} at {1}'.format(docs['user'], docs['site']))
        printLog('Chip Type : {0}'.format(docs['asic']))
        printLog('Chips({0})  :'.format(len(docs['chips'])))
        for oid in docs['chips']:
            chip_docs = docs_list['child'][oid]
            printLog('\033[1;33m    {0}: {1} \033[0m'.format(chip_docs['type'], chip_docs['name'])) 
            printLog('    User  : {0} at {1}'.format(chip_docs['user'], chip_docs['site']))
            printLog('    ChipId: {0}'.format(chip_docs['chipId']))
            del docs_list['child'][oid]
        printLog('')
    for oid in docs_list['child']:
        docs = docs_list['child'][oid]
        printLog('\033[1;33m{0}: {1} \033[0m'.format(docs['type'], docs['name'])) 
        printLog('User      : {0} at {1}'.format(docs['user'], docs['site']))
        printLog('Chip Type : {0}'.format(docs['asic']))
        printLog('ChipId    : {0}'.format(docs['chipId']))
        printLog('')

def __list_user():
    global lines
    global size
    lines = 0
    size = shutil.get_terminal_size().lines-4

    docs_list = {}
    entries = localdb.user.find({ 'dbVersion' : db_version })
    users = []
    for entry in entries:
        users.append(entry['userName'])
    for user in users:
        query = { 
            'userName'  : user,
            'dbVersion' : db_version   
        }
        entries = localdb.user.find(query)
        docs = []
        for entry in entries:
            docs.append(entry['institution'])
        docs = list(set(docs))
        docs_list.update({ user: docs })
    printLog('')
    for user in docs_list:
        printLog('\033[1;33mUser Name: {0}\033[0m'.format(user)) 
        for docs in docs_list[user]:
            printLog('- {0}'.format(docs))
        printLog('')

def __list_site():
    global lines
    global size
    lines = 0
    size = shutil.get_terminal_size().lines-4

    docs_list = []
    entries = localdb.institution.find({ 'dbVersion' : db_version })
    for entry in entries:
        docs_list.append(entry['institution'])
    docs_list = list(set(docs_list))
    printLog('')
    for site in docs_list:
        printLog('- {0}'.format(site)) 
    printLog('')
