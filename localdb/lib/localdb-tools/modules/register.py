#!/usr/bin/env python3
#################################
# Author: Arisa Kubota
# Email: arisa.kubota at cern.ch
# Date: July 2019
# Project: Local Database for YARR
#################################

# Common
import os, sys, shutil, hashlib, json, uuid

import gridfs             
from pymongo          import MongoClient
from bson.objectid    import ObjectId 
from datetime         import datetime, timezone, timedelta
import time

# Log
from logging import getLogger
logger = getLogger("Log").getChild("sub")

global localdb
global localfs
home = os.environ['HOME']
max_server_delay = 1
localdb = MongoClient("mongodb://127.0.0.1:27017", serverSelectionTimeoutMS=max_server_delay)['localdb']

######################
### Local Function ###
######################

####################################################
# Update system info
# cts: created timestamp
# mts: modified timestamp
# rev: revision
# This function must be put after register function
def updateSys(i_oid, i_col):
    if i_oid in __global.updated.get(i_col, []): return

    logger.debug('\t\t\tUpdate system information: {0} in {1}'.format(i_oid, i_col))

    query = { '_id': ObjectId(i_oid) }
    this = localdb[i_col].find_one(query)
    now = datetime.utcnow()
    if not this: return
    if this.get('sys',{})=={}:
        doc_value = { 
            '$set': {
                'sys': {
                    'cts': now,
                    'mts': now,
                    'rev': 0
                }
            }
        }
    else:
        doc_value = {
            '$set': {
                'sys': {
                    'mts': now,
                    'rev': this['sys']['rev']+1
                }
            }
        }
    localdb[i_col].update_one( query, doc_value )

    if not i_col in __global.updated:
        __global.updated.update({ i_col: [] })
    __global.updated[i_col].append(i_oid)

#########################################################
# Add { i_key: i_value } to the document { '_id': i_oid }
def addValue(i_oid, i_col, i_key, i_value, i_type='string'):
    logger.debug('\t\t\tAdd document: {0} to {1}'.format(i_key, i_col))
    if i_type=='string': value = str(i_value)
    elif i_type=='bool': 
        if i_value=='true' or i_value=='True': value = True
        else: value = False
    elif i_type=='int': value = int(i_value)
    localdb[i_col].update_one(
        { '_id': ObjectId(i_oid) },
        { '$set': {
            i_key: i_value
        }}
    )

###################################
# Alert for error/warning message
# If error: output message and exit
# If warning: just output message
def alert(i_messages, i_type='error'):
    logger.debug('Alert "{}"'.format(i_type))

    if not type(i_messages)==type([]): i_messages = [ i_messages ]

    for message in i_messages:
        if i_type=='error':
            logger.error(message)
        elif i_type=='warning':
            logger.warning(message)

    if i_type=='error': 
        file_path = ''
        if __global.option=='scan': file_path = '{}/.yarr/run.dat'.format(home)
        elif __global.option=='dcs': file_path = '{}/.yarr/dcs.dat'.format(home)
        if not file_path=='':
            if os.path.isfile(file_path):
                with open(file_path,'r') as f:
                    cache_list = f.read().splitlines()
            else:
                cache_list = []
            cache_list.append(__global.dir_path)
            cache_list = list(set(cache_list))
            with open(file_path,'w') as f:
                for line in cache_list:
                    if not line or not line=='': f.write('{}\n'.format(line))

        logger.info('-----------------------')
        logger.debug('=======================')
        sys.exit(1)

########################################################
# File to Json
# If file path is not provided /or/ file dose not exist,
# return {}
# If file exist, return {json}
# But if file cannot be paresed, alert(error)
def toJson(i_file_path):
    logger.debug('\t\t\tConvert to json code from: {}'.format(i_file_path))

    file_json = {}
    if i_file_path:
        if os.path.isfile(i_file_path):
            try:
                with open(i_file_path, 'r') as f: file_json = json.load(f)
            except ValueError as e:
                message = []
                message.append('Could not parse {}'.format(i_file_path))
                message.append('\twhat(): {}',format(e))
                alert(message)

    return file_json

#################
# Write json file
def writeJson(i_key, i_value, i_file_path, i_file_json):
    logger.debug('\tWrite Json file: {}'.format(i_file_path))

    i_file_json[i_key] = i_value
    with open(i_file_path, 'w') as f:
        json.dump(i_file_json, f, indent=4)

#################################
# Generate hash value from json{}
def getHash(i_file_json):
    logger.debug('\t\t\tGet Hash Code from File')

    shaHashed = hashlib.sha256(json.dumps(i_file_json, indent=4).encode('utf-8')).hexdigest()

    return shaHashed

#########################
### Register Function ###
#########################

##################################################
# Register User
# This function must be put after __check_user
#    user_oid = __check_user(user_json)
#    if not user_oid: user_oid = __user(user_json) 
# All the information in i_json is registered.
def __user(i_json):
    logger.debug('\t\tRegister User')
    doc_value = i_json
    doc_value.update({
        'sys'      : {},
        'userType' : 'readWrite',
        'dbVersion': __global.db_version
    })
    oid = str(localdb.user.insert_one(doc_value).inserted_id)
    updateSys(oid, 'user')

    logger.debug('\t\tdoc   : {}'.format(oid))
    return oid

##################################################
# Register Site
# This function must be put after __check_site
#    site_oid = __check_site(site_json)
#    if not site_oid: site_oid = __site(site_json) 
# All the information in i_json is registered.
def __site(i_json):
    logger.debug('\t\tRegister Site') 

    doc_value = i_json 
    doc_value.update({
        'sys'      : {},
        'dbVersion': __global.db_version
    })
    oid = str(localdb.institution.insert_one(doc_value).inserted_id)
    updateSys(oid, 'institution')

    logger.debug('\t\tdoc   : {}'.format(oid))
    return oid

##################################################################################
# Register TestRun
# This function must be put after __check_test_run
#    status = __check_test_run(i_json, conn_json)
#    if status['passed']==False:
#        tr_oid = __test_run(i_json, conn_json.get('stage', '...'), status['_id'])
# Almost all the information in i_json is registered.
def __test_run(i_json, i_stage, i_tr_oid):
    logger.debug('\t\tRegister Test Run')

    doc_value = {}
    for key in i_json:
        if not key=='connectivity' and not 'Cfg' in key and not key=='startTime' and not key=='finishTime': 
            doc_value[key] = i_json[key] 
    doc_value.update({
        'testType' : i_json.get('testType', '...'),
        'runNumber': i_json.get('runNumber', -1),
        'stage'    : i_stage.lower().replace(' ','_'),
        'chipType' : __global.chip_type,
        'address'  : __global.site_oid,
        'user_id'  : __global.user_oid,
        'dbVersion': __global.db_version
    })
    if not i_tr_oid:
        doc_value.update({
            'sys'        : {},
            'environment': False,
            'plots'      : [],
            'passed'     : False,
            'qcTest'     : False,
            'qaTest'     : False,
            'summary'    : False,
            'startTime'  : datetime.utcfromtimestamp(i_json['startTime']),
            'finishTime' : datetime.utcfromtimestamp(i_json['finishTime']),
        })
        oid = str(localdb.testRun.insert_one(doc_value).inserted_id)
    else:
        oid = i_tr_oid
        query = { '_id': ObjectId(i_tr_oid) }
        localdb.testRun.update_one( query, {'$set': doc_value} )
    updateSys(oid, 'testRun')

    logger.debug('\t\tdoc   : {}'.format(oid))
    return oid

##################################################################################
# Register ComponentTestRun
# This function must be put after __check_component_test_run
#    ctr_oid = __check_component_test_run(chip_json, tr_oid)
#    if not ctr_oid: __component_test_run(chip_json, tr_oid)
# Almost all the information in chip_json is registered.
def __component_test_run(i_json, i_tr_oid):
    logger.debug('\t\tRegister Component-TestRun')

    doc_value = i_json
    doc_value.update({
        'sys'        : {},
        'component'  : i_json['component'],
        'chip'       : i_json.get('chip','module'),
        'testRun'    : i_tr_oid,
        'attachments': [],
        'tx'         : i_json.get('tx', -1),
        'rx'         : i_json.get('rx', -1),
        'environment': '...',
        'dbVersion'  : __global.db_version
    })
    oid = str(localdb.componentTestRun.insert_one(doc_value).inserted_id)
    updateSys(oid, 'componentTestRun')

    logger.debug('\t\tdoc   : {}'.format(oid))
    return oid

##################################################
# Register chip
# This function must be put after __check_chip
#    chip_oid = __check_chip(chip_json)
#    if not chip_oid: chip_oid = __chip(chip_json)
def __chip(i_chip_json):
    logger.debug('\t\tRegister Chip')

    doc_value = {
        'sys'          : {},
        'name'         : i_chip_json['name'],
        'chipId'       : i_chip_json['chipId'],
        'chipType'     : __global.chip_type,
        'componentType': 'front-end_chip',
        'dbVersion'    : __global.db_version
    }
    oid = str(localdb.chip.insert_one(doc_value).inserted_id)
    updateSys(oid, 'chip')

    logger.debug('\t\tdoc   : {}'.format(oid))
    return oid

#########################################################################
# Register Component
# This function must be put after __check_component
#    oid = __check_component(serial_number, True)
#    if oid=='...': oid = __component(serial_number, component_type, chip_id, chips)
# Almost all the information in i_json is registered.
def __component(i_serial_number, i_component_type, i_chip_id, i_chips, i_proDB=False, i_id=''):
    logger.debug('\t\tRegister Component')

    doc_value = {
        'sys'          : {},
        'serialNumber' : i_serial_number,
        'componentType': i_component_type.lower().replace(' ','_'),
        'chipType'     : __global.chip_type,
        'name'         : i_serial_number,
        'chipId'       : i_chip_id,
        'address'      : __global.site_oid,
        'user_id'      : __global.user_oid,
        'children'     : i_chips,
        'proDB'        : i_proDB,
        'dbVersion'    : __global.db_version
    }
    if i_proDB:
        doc_value.update({ '_id': ObjectId(i_id) })
    oid = str(localdb.component.insert_one(doc_value).inserted_id)
    updateSys(oid, 'component')

    logger.debug('\t\tdoc   : {}'.format(oid))
    return oid;

#########################################################################
# Register ChildParentRelation
# This function must be put after __check_child_parent_relation
#    cpr_oid = __check_child_parent_relation(mo_oid, ch_oid)
#    if not cpr_oid: __child_parent_relation(mo_oid, ch_oid, chip_id)
def __child_parent_relation(i_parent_oid, i_child_oid, i_chip_id):
    logger.debug('\t\tRegister Child Parent Relation.')
    
    doc_value = {
        'sys'      : {},
        'parent'   : i_parent_oid,
        'child'    : i_child_oid,
        'chipId'   : i_chip_id,
        'status'   : 'active',
        'dbVersion': __global.db_version
    }
    oid = str(localdb.childParentRelation.insert_one(doc_value).inserted_id)
    updateSys(oid, 'childParentRelation')

    logger.debug('\t\tdoc   : {}'.format(oid))
    return oid

#########################################################
# Register config data
# This function must be put after __check_config
#    oid = __check_config(title, col, tr_oid, chip_oid)
#    if oid: __config(config_json, filename, title, col, oid)
def __config(i_file_json, i_filename, i_title, i_col, i_oid):
    logger.debug('\t\tRegister Config Json')

    hash_code = getHash(i_file_json)
    data_id = __check_gridfs(hash_code)
    if not data_id: data_id = __grid_fs_file(i_file_json, '', i_filename+'.json', hash_code) 
    doc_value = {
        'filename' : i_filename+'.json',
        'chipType' : __global.chip_type,
        'title'    : i_title,
        'format'   : 'fs.files',
        'data_id'  : data_id,
        'dbVersion': __global.db_version
    }
    this_cfg = localdb.config.find_one(doc_value)
    if this_cfg:
        oid = str(this_cfg['_id'])
    else:
        doc_value.update({ 'sys' : {} })
        oid = str(localdb.config.insert_one(doc_value).inserted_id)
        updateSys(oid, 'config')
    addValue(i_oid, i_col, i_title, oid)
    updateSys(i_oid, i_col)

    logger.debug('\t\tdoc   : {}'.format(i_oid))
    logger.debug('\t\tconfig: {}'.format(oid))
    logger.debug('\t\tdata  : {}'.format(data_id))
    return oid

#########################################################
# Register dat data
# This function must be put after __check_dcs
# ctr_oid = __check_dcs(ctr_id, env_key, env_description, env_num)
# if not ctr_oid: __dcs(tr_oid, ctr_oid, env_key, env_j)
def __dcs(i_tr_oid, i_ctr_oid, i_env_key, i_env_j):
    logger.debug('\t\tRegister DCS')

    array = []
    env_mode = 'null'
    env_setting = 'null'
    if i_env_j['path']!='null':
        extension = i_env_j['path'].split('.')[len(i_env_j['path'].split('.'))-1]
        if extension=='dat': 
            separator = ' '
        elif extenstion=='csv': 
            saparator = ','
        else:
            message = 'This file ({}) format is not supported by Local DB, set to "dcs" or "csv"'.format(i_env_j['path'])
            alert(message, 'warning')
            return
        env_file = open(i_env_j['path'],'r')
        # key and num
        env_key_line = env_file.readline().splitlines()[0]
        env_num_line = env_file.readline().splitlines()[0]
        key = -1
        for j, tmp_key in enumerate(env_key_line.split(separator)):
            tmp_key = tmp_key.lower().replace(' ','_')
            tmp_num = env_num_line.split(separator)[j]
            if str(i_env_key)==str(tmp_key) and str(i_env_j['num'])==str(tmp_num):
                key = j
                break
        if key==-1:
            message = 'Not found DCS data { key: {0}, num: {1} } in data file: {2}'.format(i_env_key, i_env_j['num'], i_env_j['path'])
            alert(message, 'warning')
            return
        # mode
        env_line = env_file.readline()
        env_mode = env_line.split(separator)[key]
        # setting
        env_line = env_file.readline()
        env_setting = env_line.split(separator)[key]
        # value
        env_line = env_file.readline()

        while env_line:
            if len(env_line.split(separator)) < key: break
            date = int(env_line.split(separator)[1])
            value = float(env_line.split(separator)[key])
            if 'margin' in i_env_j:
                if starttime-date<i_env_j['margin'] and finishtime-date>i_env_j['margin']:
                    array.append({
                        'date': datetime.utcfromtimestamp(date),
                        'value': value
                    })
            else:
                array.append({
                    'date': datetime.utcfromtimestamp(date),
                    'value': value
                })
            env_line = env_file.readline()
    else:
        array.append({
            'date': this_run['startTime'],
            'value': i_env_j['value']
        })

    query = { '_id': ObjectId(i_ctr_oid) }
    this_ctr = localdb.componentTestRun.find_one(query)
    if this_ctr.get('environment', '...')=='...':
        doc_value = {
            'sys': {},
            i_env_key: [{
                'data': array,
                'description': i_env_j['description'],
                'mode': env_mode,
                'setting': env_setting,
                'num': i_env_j['num']
            }],
            'dbVersion': __global.db_version
        }
        oid = str(localdb.environment.insert_one(doc_value).inserted_id)
        addValue(i_ctr_oid, 'componentTestRun', 'environment', oid)
        updateSys(i_ctr_oid, "componentTestRun");
    else:
        oid = this_ctr['environment']
        doc_value = {
            '$push': {
                i_env_key: {
                    'data': array,
                    'description': i_env_j['description'],
                    'mode': env_mode,
                    'setting': env_setting,
                    'num': i_env_j['num']
                }
            }
        }
        query = { '_id': ObjectId(oid) }
        localdb.environment.update_one( query, doc_value )

    updateSys(oid, "environment");
    addValue(i_tr_oid, 'testRun', 'environment', True)
    updateSys(i_tr_oid, 'testRun')

    logger.debug('\t\tctr doc : {}'.format(i_ctr_oid))
    logger.debug('\t\tdcs data: {}'.format(oid))

#########################################################
# Register DCS data
# This function must be put after __check_attachment
#    oid = __check_attachment(histo_name, tr_oid, chip_oid)
#    if oid: __attachment(file_path, histo_name, oid)
def __attachment(i_file_path, i_histo_name, i_oid):
    logger.debug('\t\tRegister Attachment')
    
    oid = __grid_fs_file({}, i_file_path, '{}.dat'.format(i_histo_name)) 
    localdb.componentTestRun.update_one(
        { '_id': ObjectId(i_oid) },
        { '$push': {
            'attachments': {
                'code'       : oid,
                'dateTime'   : datetime.utcnow(),
                'title'      : i_histo_name,
                'description': 'describe',
                'contentType': 'dat',
                'filename'   : '{}.dat'.format(i_histo_name)
            }
        }}
    )
    updateSys(i_oid, 'componentTestRun')

    logger.debug('\t\tdoc   : {}'.format(i_oid))
    logger.debug('\t\tdata  : {}'.format(oid))
    return oid


######################################################
# Register component information from cnnectivity file
# * moduleComponent
# * chipComponent
# * childParentRelation
def __conn_cfg(i_conn_path):
    logger.debug('\t\tRegister from Connectivity')

    conn_json = toJson(i_conn_path)

    __global.chip_type = conn_json['chipType']
    if __global.chip_type=='FEI4B': __global.chip_type = 'FE-I4B'

    module_is_exist = False
    chip_is_exist = False
    cpr_is_fine = True

    # module
    mo_serial_number = conn_json["module"]["serialNumber"]
    mo_oid = __check_component(mo_serial_number, True) 
    if mo_oid!='...': module_is_exist = True
    # chips
    chips = 0
    for i, chip_conn_json in enumerate(conn_json['chips']):
        ch_serial_number = chip_conn_json['serialNumber']
        chip_oid = __check_component(ch_serial_number, True)
        if chip_oid!='...':
            chip_is_exist = True
            doc_value = {
                'parent': mo_oid,
                'child' : chip_oid,
                'status': 'active'
            }
            this_cpr = localdb.childParentRelation.find_one(doc_value)
            if not this_cpr: cpr_is_fine = False
        chips+=1
    if module_is_exist and not chip_is_exist:
        message = 'There are registered module in connectivity: {}'.format(i_conn_path)
        alert(message)
    elif not module_is_exist and chip_is_exist:
        message = 'There are registered chips in connectivity: {}'.format(i_conn_path)
        alert(message)
    elif not cpr_is_fine:
        message = 'There are wrong relationship between module and chips in connectivity: {}'.format(i_conn_path)
        alert(message)
    elif module_is_exist and chip_is_exist and cpr_is_fine:
        return False
    mo_component_type = conn_json['module']['componentType']
    mo_oid = __check_component(mo_serial_number, True)
    if mo_oid=='...':
        mo_oid = __component(mo_serial_number, mo_component_type, -1, chips)
    
    for i, chip_conn_json in enumerate(conn_json['chips']):
        ch_serial_number = chip_conn_json['serialNumber']
        ch_component_type = chip_conn_json['componentType']
        chip_id = chip_conn_json['chipId']
        ch_oid = __check_component(ch_serial_number, True)
        if ch_oid=='...':
            ch_oid = __component(ch_serial_number, ch_component_type, chip_id, -1)
        cpr_oid = __check_child_parent_relation(mo_oid, ch_oid)
        if not cpr_oid:
            __child_parent_relation(mo_oid, ch_oid, chip_id)

    return True

###########################
# Write Dat File in GridFS
def __grid_fs_file(i_file_json, i_file_path, i_filename, i_hash=''):
    logger.debug('\t\t\tWrite File by GridFS')
   
    if not i_file_path=='': 
        with open(i_file_path, 'rb') as f:
            binary = f.read()
    else:
        binary = json.dumps(i_file_json, indent=4).encode('utf-8')

    if i_hash=='': 
        oid = str(localfs.put( binary, filename=i_filename, dbVersion=__global.db_version ))
    else: 
        oid = str(localfs.put( binary, filename=i_filename, hash=i_hash, dbVersion=__global.db_version ))

    return oid

######################
### Check Function ###
######################

#############################
# Check User
# * userName 
# * institution
# * description
# * site document ID 
# * USER
# * HOSTNAME
# If there are matching data,
# return the document ID
def __check_user(i_json):
    logger.debug('\tCheck User')
    logger.debug('\t- User name: {}'.format(i_json['userName']))
    logger.debug('\t- Institution: {}'.format(i_json['institution']))
    logger.debug('\t- description: {}'.format(i_json['description']))
    logger.debug('\t- USER: {}'.format(i_json['USER']))
    logger.debug('\t- HOSTNAME: {}'.format(i_json['HOSTNAME']))

    oid = None

    query = {
        'userName'   : i_json['userName'],
        'institution': i_json['institution'],
        'description': i_json['description'],
        'USER'       : i_json['USER'],
        'HOSTNAME'   : i_json['HOSTNAME']
    }
    this_user = localdb.user.find_one(query)
    if this_user: oid = str(this_user['_id'])

    return oid

#############################
# Check Site
# * address 
# * hostname
# * institution
# If there are matching data,
# return the document ID
def __check_site(i_json):
    logger.debug('\tCheck Site')
    logger.debug('\t- Address: {}'.format(i_json['address']))
    logger.debug('\t- HOSTNAME: {}'.format(i_json['HOSTNAME']))
    logger.debug('\t- Institution: {}'.format(i_json['institution'])) 

    oid = None

    query = {
        'address'    : i_json['address'],
        'HOSTNAME'   : i_json['HOSTNAME'],
        'institution': i_json['institution']
    }

    this_site = localdb.institution.find_one(query)
    if this_site: oid = str(this_site['_id'])

    return oid

#############################
# Check Component-TestRun
# * chip document ID
# * testRun document ID
# * tx channel
# * rx channel
# If there are matching data,
# return the document ID
def __check_component_test_run(i_json, i_tr_oid):
    logger.debug('\tCheck Component-TestRun')
    logger.debug('\t- chip: {}'.format(i_json.get('chip','module')))
    logger.debug('\t- component: {}'.format(i_json['component']))
    logger.debug('\t- testrun: {}'.format(i_tr_oid))
    logger.debug('\t- tx: {}'.format(i_json.get('tx',-1)))
    logger.debug('\t- rx: {}'.format(i_json.get('rx',-1)))

    oid = None

    query = {
        'chip'     : i_json.get('chip','module'),
        'component': i_json['component'],
        'testRun'  : i_tr_oid,
        'tx'       : i_json.get('tx',-1),
        'rx'       : i_json.get('rx',-1)
    }
    this_ctr = localdb.componentTestRun.find_one(query)
    if this_ctr: oid = str(this_ctr['_id'])

    return oid

############################################################
# Check TestRun
# * start timestamp
# * site document ID
# * user document ID
# If there are matching data,
# return { '_id': documentID, 'passed': document['passed'] }
def __check_test_run(i_scanlog_json, i_conn_json):
    logger.debug('\tCheck TestRun')
    logger.debug('\t- timestamp: {}'.format(i_scanlog_json['startTime']))
    logger.debug('\t- site: {}'.format(__global.site_oid))
    logger.debug('\t- user: {}'.format(__global.user_oid))

    status = {
        '_id': None,
        'passed': False
    }
    start_time = datetime.utcfromtimestamp(i_scanlog_json['startTime'])
    query = {
        'startTime': start_time,
        'address'  : __global.site_oid,
        'user_id'  : __global.user_oid
    }
    run_entries = localdb.testRun.find(query).sort([('$natural', -1)])
    for this_run in run_entries:
        chip_ids = []
        for chip_json in i_conn_json['chips']:
            if chip_json.get('enable', 1)==0: continue
            chip_ids.append({ 'chip': chip_json['chip'] })
        query = {
            'testRun': str(this_run['_id']),
            '$or'    : chip_ids
        }
        ctr_entries = localdb.componentTestRun.find(query)
        if not ctr_entries.count()==0:
            status['_id'] = str(this_run['_id'])
            status['passed'] = this_run['passed']

    return status

#############################
# Check Chip
# * chip's name
# * chip's ID
# * component document ID
# * site document ID
# * user document ID
# If there are matching data,
# return document ID
def __check_chip(i_json):
    logger.debug('\tCheck Chip data:')
    logger.debug('\t- name: {}'.format(i_json['name']))
    logger.debug('\t- chipId: {}'.format(i_json['chipId']))
    logger.debug('\t- chipType: {}'.format(__global.chip_type))

    oid = None

    if i_json.get('enable', 1)==0: oid = '...'
    else:
        query = {
            'name'     : i_json['name'],
            'chipId'   : i_json['chipId'],
            'chipType' : __global.chip_type,
        }
        this_chip = localdb.chip.find_one(query)
        if this_chip: oid = str(this_chip['_id'])

    return oid

#############################################################
# Check Component
# * serialNumber
# If there are matching data,
# return document ID
# TODO If there are no matching data in QC scan, alert(error)
def __check_component(i_serial_number, register=False):
    logger.debug('\tCheck Component data:') 
    logger.debug('\t- Serial Number: {}'.format(i_serial_number))
    logger.debug('\t- chipType: {}'.format(__global.chip_type))

    doc_value = { 
        'serialNumber': i_serial_number, 
        'chipType' : __global.chip_type,
    }
    this_cmp = localdb.component.find_one(doc_value)
    oid = '...'
    if this_cmp: oid = str(this_cmp['_id'])
    elif register==False:
        message = 'Not registered the component "{}"'.format(i_serial_number)
        alert(message)

    return oid

#############################
# Check Child Parent Relation
# * parent document ID 
# * chip document ID 
# * status: active
# If there are matching data,
# return document ID
def __check_child_parent_relation(i_parent_oid, i_child_oid):
    logger.debug('\tCheck Child Parent Relation:')
    logger.debug('\t- parent: {}'.format(i_parent_oid)) 
    logger.debug('\t- child: {}'.format(i_child_oid))

    oid = None

    query = {
        'parent': i_parent_oid,
        'child': i_child_oid,
        'status': 'active'
    }
    this_cpr = localdb.childParentRelation.find_one(query)
    if this_cpr: oid = str(this_cpr['_id'])

    return oid

################################################
# Check Config
# * title
# * test document ID (testRun, componentTestRun) 
# * chip document ID (componentTestRun)
# If the config file has not registered,
# return document ID of testRun/componentTestRun
def __check_config(i_title, i_col, i_tr_oid, i_chip_oid):
    logger.debug('\tCheck Config Json:') 
    logger.debug('\t- title: {}'.format(i_title)) 
    logger.debug('\t- collection: {}'.format(i_col))
    logger.debug('\t- testRun: {}'.format(i_tr_oid))
    logger.debug('\t- chip: {}'.format(i_chip_oid))

    oid = None

    if i_col=='testRun': 
        query = { '_id': ObjectId(i_tr_oid) }
    elif i_col=='componentTestRun': 
        query = {
            'testRun': i_tr_oid,
            'chip'   : i_chip_oid
        }
    this = localdb[i_col].find_one(query)
    if this.get(i_title, '...')=='...': oid = str(this['_id'])

    return oid

########################################
# Check Attachment
# * histo name
# * test document ID
# * chip document ID
# If the dat file has not registered,
# return document ID of componentTestRun
def __check_attachment(i_histo_name, i_tr_oid, i_chip_oid):
    logger.debug('\tCheck Attachment:') 
    logger.debug('\t- histo name: {}'.format(i_histo_name)) 
    logger.debug('\t- testRun: {}'.format(i_tr_oid))
    logger.debug('\t- chip: {}'.format(i_chip_oid))

    oid = None

    query = {
        'testRun': i_tr_oid,
        'chip'   : i_chip_oid
    }
    this_ctr = localdb.componentTestRun.find_one(query)
    titles = []
    for attachment in this_ctr.get('attachments', []):
        titles.append(attachment['title'])
    if not i_histo_name in titles: oid = str(this_ctr['_id'])

    return oid

##################################
# Check Json File by Hash
# * hash value
# If the json file has registered,
# return document ID
def __check_gridfs(i_hash_code):
    logger.debug('\t\t\tCheck Json File by Hash')

    oid = None

    query = { 'hash': i_hash_code }
    this_cfg = localdb.fs.files.find_one( query, { '_id': 1 } )
    if this_cfg:
        oid = str(this_cfg['_id'])

    return oid 

############################################################
# Check Site Config
# Display site information in the console
def __check_site_data(i_oid):
    logger.debug('\tCheck Site Data for registration:') 

    query = { '_id': ObjectId(i_oid) }
    this_site = localdb.institution.find_one(query)
   
    logger.info('Site Data:')
    logger.info('    MAC address: {}'.format(this_site['address']))
    logger.info('    Institution: {}'.format(this_site['institution']))
    logger.info('    HOSTNAME: {}'.format(this_site['HOSTNAME']))

############################################################
# Check User Config
# Display user information in the console
def __check_user_data(i_oid):
    logger.debug('\tCheck User Data for registration:') 

    query = { '_id': ObjectId(i_oid) }
    this_user = localdb.user.find_one(query)
    
    logger.info('User Data:')
    logger.info('    User Name: {}'.format(this_user['userName']))
    logger.info('    Institution: {}'.format(this_user['institution']))
    logger.info('    Description: {}'.format(this_user['description']))
    logger.info('    USER: {}'.format(this_user['USER']))
    logger.info('    HOSTNAME: {}'.format(this_user['HOSTNAME']))

############################################################
# Check Component Connectivity
# If the components written in the file have not registered,
# Display component information in the console
def __check_conn_cfg(i_conn_path):
    logger.debug('\tCheck Connectivity config for registration:') 
    logger.debug('\t- connectivity: {}'.format(i_conn_path))

    conn_json = toJson(i_conn_path)

    chip_type = conn_json['chipType']
    if chip_type=='FEI4B': chip_type = 'FE-I4B'

    # module
    if not 'module' in conn_json:
        message = 'No module data in {}, Set module.serialNumber and module.componentType'.format(i_conn_path)
        alert(message)
    if not 'serialNumber' in conn_json['module']:
        message = 'No module data in {}, Set module.serialNumber'.format(i_conn_path)
        alert(message)
    if not 'componentType' in conn_json['module']:
        message = 'No module data in {}, Set module.componentType'.format(i_conn_path)
        alert(message)
    mo_serial_number = conn_json["module"]["serialNumber"]
    mo_component_type = conn_json["module"]["componentType"]
    __check_list(mo_component_type, 'component')
    # chip
    chips = []
    chipids = []
    for i, chip_conn_json in enumerate(conn_json['chips']):
        if not 'serialNumber' in chip_conn_json:
            message = 'No chip data in {0}, Set chips.{1}.serialNumber'.format(i_conn_path, i)
            alert(message)
        if not 'componentType' in chip_conn_json:
            message = 'No chip data in {0}, Set chips.{1}.componentType'.format(i_conn_path, i)
            alert(message)
        if not 'chipId' in chip_conn_json:
            message = 'No chip data in {0}, Set chips.{1}.chipId'.format(i_conn_path, i)
            alert(message)
        ch_serial_number = chip_conn_json['serialNumber']
        ch_component_type = chip_conn_json['componentType']
        ch_id = chip_conn_json['chipId']
        __check_list(ch_component_type, 'component')
        if ch_id in chipids:
            message = 'Conflict chip ID {0} in the same module {1}'.format(ch_id)
            alert(message)
        chips.append({'serialNumber': ch_serial_number, 'componentType': ch_component_type, 'chipId': ch_id})
        chipids.append(ch_id)
    
    logger.info('Component Data:')
    logger.info('    Chip Type: {}'.format(chip_type))
    logger.info('    Module:')
    logger.info('        serial number: {}'.format(mo_serial_number))
    logger.info('        component type: {}'.format(mo_component_type))
    logger.info('        chips: {}'.format(len(chips)))
    for i, chip in enumerate(chips):
        logger.info('    Chip ({}):'.format(i+1))
        logger.info('        serial number: {}'.format(chip['serialNumber']))
        logger.info('        component type: {}'.format(chip['componentType']))
        logger.info('        chip ID: {}'.format(chip['chipId']))

########################################
# Check DCS data
# * test document ID
# * chip name
# * DCS data key
# * DCS data number
# * DCS data description
# If the specified DCS data has not registered,
# return document ID(s) of componentTestRun
def __check_dcs(i_ctr_oid, i_key, i_num, i_description):
    logger.debug('\tCheck DCS')
    logger.debug('\t- componentTestRun: {}'.format(i_ctr_oid))
    logger.debug('\t- DCS key: {}'.format(i_key))
    logger.debug('\t- DCS num: {}'.format(i_num))
    logger.debug('\t- DCS description: {}'.format(i_description))

    ctr_oid = i_ctr_oid
    query = { '_id': ObjectId(i_ctr_oid) }
    this_ctr = localdb.componentTestRun.find_one(query)
    if not this_ctr.get('environment', '...')=='...':
        query = { '_id': ObjectId(this_ctr['environment']) }
        this_dcs = localdb.environment.find_one(query)
        for this_data in this_dcs.get(i_key,[]):
            if str(this_data['num'])==str(i_num) and this_data['description']==i_description:
                ctr_oid = None
                break

    return ctr_oid

##############################
# Check if the value is listed
def __check_list(i_value, i_name):
    logger.debug('\tCheck List:')
    logger.debug('\t- value: {}'.format(i_value)) 
    logger.debug('\t- list: {}'.format(i_name))
    if not i_value.lower().replace(' ','_') in __global.db_list[i_name]:
        message = 'Not registered {0} in the {1} list in database config file'.format(i_value, i_name)
        alert(massage)
    return 

###################################
# Check if the key of json is empty
def __check_empty(i_json, i_key, i_filename):
    logger.debug('\tCheck Empty:') 
    logger.debug('\t- key: {}'.format(i_key)) 
    logger.debug('\t- file: {}'.format(i_filename))

    if not i_key in i_json:
        message = []
        message.append('Found an empty field in json file.')
        message.append('\tfile: {0}  key: {1}'.format(i_filename, i_key))
        alert(message)
    return

########################
### setting function ###
########################

##############
# Set Local DB
# * localdb
# * localfs
def __set_localdb(i_localdb):
    logger.debug('Set Local DB') 

    global localdb
    global localfs
    localdb = i_localdb
    localfs = gridfs.GridFS(localdb)

######################################################
# Set connectivity config
# Chip Data can be registered in this function
# Component Data cannot be registered in this function
def __set_conn_cfg(i_conn_json, i_cache_dir):
    logger.debug('Set Connectivity Config')

    if i_conn_json=={}: 
        message = 'No connectivity config provided.'
        alert(message, 'warning')
        return i_conn_json

    # chip type
    __check_empty(i_conn_json, 'chipType', 'connectivity config')
    __global.chip_type = i_conn_json['chipType']
    if __global.chip_type=='FEI4B': __global.chip_type = 'FE-I4B'

    # module
    conn_json = { 'module': {}, 'chips':[], 'stage': i_conn_json.get('stage','...') }
    conn_json['module']['component'] = __check_component(i_conn_json.get('module', {}).get('serialNumber','NONAME'), True)
    conn_json['module']['name'] = i_conn_json.get('module', {}).get('serialNumber','UnnamedModule')
    # chip
    for i, chip_json in enumerate(i_conn_json['chips']):
        chip_json['config'] = chip_json['config'].split('/')[len(chip_json['config'].split('/'))-1]
        if chip_json.get('enable', 1)==0:  # disabled chip
            chip_json['name'] = 'DisabledChip_{}'.format(i)
            chip_json['chipId'] = -1
        else:  # enabled chip
            chip_cfg_json = toJson('{0}/{1}.before'.format(i_cache_dir, chip_json['config']))
            if not __global.chip_type in chip_cfg_json:
                message = 'Not found {0} in chip config file: {1}/{2}.before'.format(__global.chip_type, i_cache_dir, chip_json['config'])
                alert(message)
            if 'name' in chip_cfg_json[__global.chip_type]: # for FEI4B 
                chip_json['name'] = chip_cfg_json[__global.chip_type]['name']
                chip_json['chipId'] = chip_cfg_json[__global.chip_type]['Parameter']['chipId']
            elif 'Name' in chip_cfg_json[__global.chip_type]['Parameter']: # for RD53A
                chip_json['name'] = chip_cfg_json[__global.chip_type]['Parameter']['Name']
                chip_json['chipId'] = chip_cfg_json[__global.chip_type]['Parameter']['ChipId']
            else:
                chip_json['name'] = 'UnnamedChip_{}'.format(i)
                chip_json['chipId'] = 0 #TODO not sure the best
        if not chip_json.get('serialNumber', '')==chip_json['name']: chip_json['serialNumber']=chip_json['name']

        chip_json['component'] = __check_component(chip_json['serialNumber'], True) #TODO for registered component
        query = { 'parent': conn_json['module']['component'], 'child': chip_json['component'] }
        this_cpr = localdb.childParentRelation.find_one(query)
        if not this_cpr: conn_json['module']['component'] = '...'

        chip_oid = __check_chip(chip_json)
        if not chip_oid: chip_oid = __chip(chip_json)
        chip_json['geomId'] = chip_json.get('geomId', i)
        chip_json['chip'] = chip_oid
        del chip_json['serialNumber']
        del chip_json['chipId']
        conn_json['chips'].append(chip_json)
        
    return conn_json

##############################################
# Set User
# User Data can be registered in this function
def __set_user(i_json):
    logger.debug('Set User') 

    user_json = {
        'userName'    : os.environ['USER'],
        'institution' : os.environ['HOSTNAME'],
        'description' : 'default',
        'USER'        : os.environ['USER'],
        'HOSTNAME'    : os.environ['HOSTNAME']
    }

    if not i_json=={}:
        user_json.update(i_json)

    user_json['userName'] = user_json['userName'].lower().replace(' ','_')
    user_json['institution'] = user_json['institution'].lower().replace(' ','_')

    user_oid = __check_user(user_json)
    if not user_oid: user_oid = __user(user_json) 

    __global.user_oid = user_oid

    return user_oid

##############################################
# Set Site
# Site Data can be registered in this function
def __set_site(i_json):
    logger.debug('Set Site') 

    site_json = {
        'address'    : ':'.join(['{:02x}'.format((uuid.getnode() >> ele) & 0xff)for ele in range(0,8*6,8)][::-1]),
        'HOSTNAME'   : os.environ['HOSTNAME'],
        'institution': os.environ['HOSTNAME']
    }
    if not i_json=={}:
        site_json.update(i_json)

    site_json['institution'] = site_json['institution'].lower().replace(' ','_')

    site_oid = __check_site(site_json)
    if not site_oid: site_oid = __site(site_json)

    __global.site_oid = site_oid;

    return site_oid

##############################################
# Set TestRun (Start)
# Test Data can be registered in this function
def __set_test_run_start(i_scanlog_json, i_conn_jsons):
    logger.debug('Set Test Run (start)')

    for i, conn_json in enumerate(i_conn_jsons):
        status = __check_test_run(i_scanlog_json, conn_json)
        if status['passed']==False:
            tr_oid = __test_run(i_scanlog_json, conn_json.get('stage', '...'), status['_id'])
        else:
            message = 'Already registered test run data in DB'
            alert(message, 'warning')
            tr_oid = status['_id']
        conn_json['testRun'] = tr_oid 

        if not conn_json['module']['component']=='...':
            ctr_oid = __check_component_test_run(conn_json['module'], tr_oid)
            if not ctr_oid: __component_test_run(conn_json['module'], tr_oid)
        for chip_json in conn_json['chips']:
            ctr_oid = __check_component_test_run(chip_json, tr_oid)
            if not ctr_oid: __component_test_run(chip_json, tr_oid)

        i_conn_jsons[i] = conn_json

    return i_conn_jsons

############################################
# Set TestRun (Finish)
# Test Data can be modified in this function
def __set_test_run_finish(i_scanlog_json, i_conn_jsons):
    logger.debug('Set Test Run (finish)')

    for conn_json in i_conn_jsons:
        tr_oid = conn_json['testRun']
        query = { '_id': ObjectId(tr_oid) }
        this_run = localdb.testRun.find_one(query)

        doc_value = {}
        histo_names = list(set(__global.histo_names))
        if not list(set(this_run['plots']))==list(set(histo_names)):
            doc_value.update({ 'plots': histo_names })
        if not this_run['passed']==True:
            doc_value.update({ 'passed': True })

        if not doc_value=={}:
            localdb.testRun.update_one(
                { '_id': ObjectId(tr_oid) },
                { '$set': doc_value }
            )
            updateSys(tr_oid, 'testRun')

        __global.tr_oids.append(tr_oid)

    return

##############################################
# Set Config
# Config Data can be modified in this function
def __set_config(i_config_json, i_filename, i_title, i_col, i_chip_json, i_conn_json):
    logger.debug('Set Config Json')

    if i_config_json=={}: return

    oid = __check_config(i_title, i_col, i_conn_json['testRun'], i_chip_json.get('chip','...'))
    if oid: __config(i_config_json, i_filename, i_title, i_col, oid)

    return

###########################################
# Set Attachment
# Dat Data can be modified in this function
def __set_attachment(i_file_path, i_histo_name, i_chip_json, i_conn_json):
    logger.debug('Set Attachment')

    if not os.path.isfile(i_file_path): return
    __global.histo_names.append(i_histo_name);

    oid = __check_attachment(i_histo_name, i_conn_json['testRun'], i_chip_json.get('chip','...'))
    if oid: __attachment(i_file_path, i_histo_name, oid)

    return

##############
# Set DCS Data
def __set_dcs(i_tr_oid, i_env_json):
    logger.debug('\t\tRegister Environment')
    
    doc_value = { '_id': ObjectId(i_tr_oid) }
    this_run = localdb.testRun.find_one(doc_value)

    starttime = this_run['startTime'].timestamp()
    finishtime = this_run['finishTime'].timestamp()

    for env_j in i_env_json:
        if env_j['status']!='enabled': continue
        env_key = env_j['key'].lower().replace(' ','_')
        query = { 'testRun': i_tr_oid }
        if env_j.get('chip', None):
            query.update({ 'name': env_j['chip'] })
        ctr_entries = localdb.componentTestRun.find(query)
        for this_ctr in ctr_entries:
            ctr_oid = __check_dcs(str(this_ctr['_id']), env_key, env_j['num'], env_j['description'])
            if ctr_oid: __dcs(i_tr_oid, ctr_oid, env_key, env_j)

    return

########################
# Set specific name list
def __set_list(i_list, i_name):
    if not i_name in __global.db_list:
        __global.db_list.update({ i_name: [] })
        for value in i_list:
            __global.db_list[i_name].append(value.lower().replace(' ','_'))

#######################
### Global Variable ###
#######################
class __global:
    chip_type = ''
    user_oid = ''
    site_oid = ''
    tr_oids = []
    histo_names = []
    db_version = 1.01
    db_list = {}
    dir_path = ''
    option = ''
    updated = {}
    #force = False
    def clean():
        chip_type = ''
        user_oid = ''
        site_oid = ''
        tr_oids = []
        histo_names = []
        db_version = 1.01
        db_list = {}
        dir_path = ''
        option = ''
        updated = {}
        #force = False
