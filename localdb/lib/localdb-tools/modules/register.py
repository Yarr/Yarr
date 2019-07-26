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
import hashlib
import json

import gridfs             
from pymongo          import MongoClient
from bson.objectid    import ObjectId 
from datetime         import datetime, timezone, timedelta
import time

# log
from logging import getLogger
logger = getLogger("Log").getChild("sub")

DB_DEBUG = False

global localdb
global localfs
home = os.environ['HOME']
max_server_delay = 1
localdb = MongoClient("mongodb://127.0.0.1:27017", serverSelectionTimeoutMS=max_server_delay)['localdb']
# local function
def addSys(i_oid, i_col):
    now = datetime.utcnow()
    localdb[i_col].update(
        { 
            '_id': ObjectId(i_oid) 
        },{
            '$set': { 
                'sys': {
                    'cts': now,
                    'mts': now,
                    'rev': 0
                }
            }
        }
    )

def addValue(i_oid, i_col, i_key, i_value, i_type='string'):
    logger.debug('\t\tLocal DB: Add document: {0} to {1}'.format(i_key, i_col))
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

def addUser(i_oid, i_col):
    logger.debug('\t\tLocal DB: Add user and institution')

    localdb[i_col].update(
        { 
            '_id': ObjectId(i_oid) 
        },{
            '$set': { 
                'address': __global.site_oid,
                'user_id': __global.user_oid
            }
        }
    )

def alert(i_message, i_type='error'):
    logger.debug('Local DB: Alert "{}"'.format(i_type))

    if i_type=='error':
        logger.error(i_message)
    elif i_type=='warning':
        logger.warning(i_message)

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
        sys.exit()

def toJson(i_file_path):
    logger.debug('\t\tLocal DB: Convert to json code from: {}'.format(i_file_path))

    file_json = {}
    if i_file_path:
        if os.path.isfile(i_file_path):
            try:
                with open(i_file_path, 'r') as f: file_json = json.load(f)
            except ValueError as e:
                message = 'Could not parse {0}\n\twhat(): {1}'.format(i_file_path, e)
                alert(message)

    return file_json

def writeJson(i_key, i_value, i_file_path, i_file_json):
    logger.debug('\tLocal DB: Write Json file: {}'.format(i_file_path))

    i_file_json[i_key] = i_value
    with open(i_file_path, 'w') as f:
        json.dump(i_file_json, f, indent=4)

def getComponent(i_serial_number, register=False):
    logger.debug('\tLocal DB: Get component data: "Serial Number": {}'.format(i_serial_number))

    doc_value = { 'serialNumber': i_serial_number }
    this_cmp = localdb.component.find_one(doc_value)
    oid = ''
    if this_cmp:
        oid = str(this_cmp['_id'])
    elif register==False:
        message = 'Not registered the component "{}"'.format(i_serial_number)
        alert(message)
    return oid

def getValue(i_col, i_mem_key, i_mem_val, i_mem_type, i_key, i_type='str'):
    logger.debug('\tLocal DB: Get Value of Key: "{0}" from: "{1}" query by Key: "{2}" and Value: "{3}"'.format(i_key, i_col, i_mem_key, i_mem_val))

    if i_mem_type=='oid': query_doc = { i_mem_key: ObjectId(i_mem_val) }
    else: query_doc = { i_mem_key: i_mem_val }
    result = localdb[i_col].find_one(query_doc)
    if result:
        if i_type=='oid': return str(result['_id'])
        elif i_type=='sys_cts': return result['sys']['cts'].strftime('%Y-%m-%d %H:%M:%S')
        elif i_type=='sys_rev': return int(result['sys']['rev']) 
        elif i_type=='sys_mts': return result['sys']['mts'].strftime('%Y-%m-%d %H:%M:%S')
        elif i_type=='int': return int(result[i_key])
        else: 
            if i_key in result: return str(result[i_key])
            else: return 'ERROR'
    else: 
        message = 'Cannot find "{0}" from member "{1}": "{2}" in collection name: "{3}"'.format(i_key, i_mem_key, i_mem_val, i_col)
        alert(message)

def getHash(i_file_json):
    logger.debug('\tLocal DB: Get Hash Code from File')

    shaHashed = hashlib.sha256(json.dumps(i_file_json, indent=4).encode('utf-8')).hexdigest()

    return shaHashed

# register function
def __user(i_user_name, i_institution, i_user_identity):
    logger.debug('\tLocal DB: Register user \n\tUser name: {0} \n\tInstitution: {1} \n\tUser identity: {2}'.format(i_user_name, i_institution, i_user_identity))
 
    doc_value = {
        'userName'    : i_user_name,
        'machine'     : __global.site_oid,
        'machineUser' : os.environ['USER'],
        'institution' : i_institution,
        'userIdentity': i_user_identity
    }
    this_user = localdb.user.find_one(doc_value)
    if this_user: 
        user_oid = this_user['_id']
    else:
        doc_value.update({
            'sys'      : {},
            'userType' : 'readWrite',
            'dbVersion': __global.db_version
        })
        user_oid = localdb.user.insert(doc_value)
        addSys(str(user_oid), 'user')

    return str(user_oid)

def __site(i_address, i_hostname, i_site):
    logger.debug('\tLocal DB: Register site \n\tAddress: {0} \n\tName: {1} \n\tInstitution: {2}'.format(i_address, i_hostname, i_site)) 
 
    doc_value = {
        'address'    : i_address,
        'institution': i_site
    }
    this_site = localdb.institution.find_one(doc_value)
    if this_site: 
        site_oid = this_site['_id']
    else: 
        doc_value.update({
            'sys'      : {},
            'name'     : i_hostname,
            'dbVersion': __global.db_version
        })
        site_oid = localdb.institution.insert(doc_value)
        addSys(str(site_oid), 'institution')

    return str(site_oid)

def __component(i_serial_number, i_component_type, i_chip_id, i_chips):
    logger.debug('\tLocal DB: Register Component: {}'.format(i_serial_number))

    doc_value = {
        'serialNumber': i_serial_number
    }
    this_cmp = localdb.component.find_one(doc_value)
    if this_cmp: 
        cmp_oid = this_cmp['_id']
    else:
        doc_value.update({
            'sys': {},
            'chipType': __global.chip_type,
            'componentType': i_component_type,
            'children': i_chips,
            'chipId': i_chip_id,
            'dbVersion': __global.db_version,
            'address': '...',
            'user_id': '...'
        })
        cmp_oid = localdb.component.insert(doc_value)
        addSys(str(cmp_oid), 'component')
        addUser(str(cmp_oid), 'component');

    return str(cmp_oid);

def __child_parent_relation(i_parent_oid, i_child_oid, i_chip_id):
    logger.debug('\tLocal DB: Register Child Parent Relation.')
    
    doc_value = {
        'parent': i_parent_oid,
        'child': i_child_oid,
        'status': 'active'
    }
    this_cpr = localdb.childParentRelation.find_one(doc_value)
    if not this_cpr:
        doc_value.update({
            'sys': {},
            'parent': i_parent_oid,
            'child': i_child_oid,
            'chipId': i_chip_id,
            'status': 'active',
            'dbVersion': __global.db_version
        })
        cpr_oid = localdb.childParentRelation.insert(doc_value)
        addSys(str(cpr_oid), 'childParentRelation')

    return

def __conn_cfg(i_conn_path):
    logger.debug('\tLocal DB: Register from connectivity: {}'.format(i_conn_path))

    conn_json = toJson(i_conn_path)

    __global.chip_type = conn_json['chipType']
    if __global.chip_type=='FEI4B': __global.chip_type = 'FE-I4B'

    module_is_exist = False
    chip_is_exist = False
    cpr_is_fine = True

    # module
    mo_serial_number = conn_json["module"]["serialNumber"]
    mo_oid = getComponent(mo_serial_number, True) 
    if mo_oid!='': module_is_exist = True
    # chips
    chips = 0
    for i, chip_conn_json in enumerate(conn_json['chips']):
        ch_serial_number = chip_conn_json['serialNumber']
        chip_oid = getComponent(ch_serial_number, True)
        if chip_oid!='':
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
    mo_oid = __component(mo_serial_number, mo_component_type, -1, chips)
    
    for i, chip_conn_json in enumerate(conn_json['chips']):
        ch_serial_number = chip_conn_json['serialNumber']
        ch_component_type = chip_conn_json['componentType']
        chip_id = chip_conn_json['chipId']
        ch_oid = __component(ch_serial_number, ch_component_type, chip_id, -1)
        __child_parent_relation(mo_oid, ch_oid, chip_id)

    return True

def __config(i_serial_number, i_file_json, i_filename, i_title, i_col):
    if DB_DEBUG: pring('\tLocal DB: Register Config Json: {}'.format(i_filename))

    ctr_oid = ''
    if i_serial_number!='':
        doc_value = { 'serialNumber': i_serial_number }
        this_cmp = localdb.component.find_one(doc_value)
        if this_cmp:
            cmp_oid = str(this_cmp['_id'])
        else:
            cmp_oid = i_serial_number

        for tr_oid in __global.tr_oids:
            doc_value = { 
                'component': cmp_oid,
                'testRun': tr_oid
            }
            this_ctr = localdb.componentTestRun.find_one(doc_value)
            if this_ctr:
                ctr_oid = str(this_ctr['_id'])

    if i_col=='testRun':
        cfg_oid = __json_code(i_file_json, i_filename+'.json', i_title, 'gj')
        for tr_oid in __global.tr_oids:
            if tr_oid=='': continue
            addValue(tr_oid, i_col, i_title, cfg_oid)
    if i_col=='componentTestRun':
        if not ctr_oid=='': 
            cfg_oid = __json_code(i_file_json, i_title+'.json', i_title, 'gj')
            addValue(ctr_oid, i_col, i_filename, cfg_oid)

    return

def __json_code(i_file_json, i_filename, i_title, i_type):
    logger.debug('\tLocal DB: Upload Json File')

    if i_type=='gj':
        data_id = __json_code_gridfs(i_file_json, i_filename, i_title)
        type_doc = 'fs.files'
    #elif i_type=='m': #TODO to be added
    else:
        message = 'Unknown type "{}" to upload json file'.format(i_type)
        alert(message)

    doc_value = {
        'sys': {},
        'filename': i_filename,
        'chipType': __global.chip_type,
        'title': i_title,
        'format': type_doc,
        'data_id': data_id,
        'dbVersion': __global.db_version
    }

    oid = localdb.config.insert(doc_value)
    addSys(str(oid), 'config')

    return str(oid)

def __json_code_gridfs(i_file_json, i_filename, i_title):
    logger.debug('\tLocal DB: Write Json File: {}'.format(i_filename))

    hash_code = getHash(i_file_json)
    doc_value = { 'hash': hash_code }
    this_cfg = localdb.fs.files.find_one( doc_value, { '_id': 1 } )
    if this_cfg:
        code = str(this_cfg['_id'])
    else:
        code = __grid_fs_file(i_file_json, '', i_filename, hash_code) 

    return code

def __grid_fs_file(i_file_json, i_file_path, i_filename, i_hash=''):
    if DB_DEBUG: pring('Local DB: Write Attachment: {}'.format(i_filename))
   
    if not i_file_path=='': 
        with open(i_file_path, 'rb') as f:
            binary = f.read()
    else:
        binary = json.dumps(i_file_json, indent=4).encode('utf-8')

    if i_hash=='': code = localfs.put( binary, filename=i_filename, dbVersion=__global.db_version ) 
    else: code = localfs.put( binary, filename=i_filename, hash=i_hash, dbVersion=__global.db_version )

    return str(code)

def __attachment(i_serial_number, i_file_path, i_histo_name):
    logger.debug('\tLocal DB: Register Attachment: {}'.format(i_file_path))
    
    if i_serial_number!='':
        doc_value = { 'serialNumber': i_serial_number } 
        this_cmp = localdb.component.find_one(doc_value)
        if this_cmp: 
            cmp_oid = str(this_cmp['_id'])
        else:
            cmp_oid = i_serial_number

        for tr_oid in __global.tr_oids:
            doc_value = { 
                'component': cmp_oid,
                'testRun': tr_oid
            }
            this_ctr = localdb.componentTestRun.find_one(doc_value)
            if this_ctr:
                ctr_oid = str(this_ctr['_id'])
                break

    if not ctr_oid=='':
        if os.path.isfile(i_file_path):
            code = __grid_fs_file({}, i_file_path, '{}.dat'.format(i_histo_name)) 
            localdb.componentTestRun.update(
                { '_id': ObjectId(ctr_oid) },
                { '$push': {
                    'attachments': {
                        'code': code,
                        'dateTime': datetime.utcnow(),
                        'title': i_histo_name,
                        'description': 'describe',
                        'contentType': 'dat',
                        'filename': '{}.dat'.format(i_histo_name)
                    }
                }}
            )

def __dcs(i_tr_oid, i_env_json):
    logger.debug('Local DB: Register Environment')
    
    doc_value = { '_id': ObjectId(i_tr_oid) }
    this_run = localdb.testRun.find_one(doc_value)

    starttime = this_run['startTime'].timestamp()
    finishtime = this_run['finishTime'].timestamp()

    doc_value = {
        'dbVersion': __global.db_version,
        'sys': {}
    }
    for env_j in i_env_json:
        if env_j['status']!='enabled': continue
        if not env_j['key'] in doc_value:
            doc_value.update({ env_j['key']: [] })
        array = []
        env_mode = 'null'
        env_setting = 'null'
        if env_j['path']!='null':
            extension = env_j['path'].split('.')[len(env_j['path'].split('.'))-1]
            if extension=='dat': separator = ' '
            elif extenstion=='csv': saparator = ','
            env_file = open(env_j['path'],'r')
            key_values = []
            dates = []
            data_num = 0
            # key and num
            env_key_line = env_file.readline().splitlines()[0]
            env_num_line = env_file.readline().splitlines()[0]
            for j, tmp_key in enumerate(env_key_line.split(separator)):
                if str(env_j['key'])==str(tmp_key) and int(env_j['num'])==int(env_num_line.split(separator)[j]):
                    key = j
                    break
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
                if 'margin' in env_j:
                    if starttime-date<env_j['margin'] and finishtime-date>env_j['margin']:
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
                'value': env_j['value']
            })
        doc_value[env_j['key']].append({
            'data': array,
            'description': env_j['description'],
            'mode': env_mode,
            'setting': env_setting,
            'num': env_j['num']
        })
    oid = localdb.environment.insert(doc_value)
    addSys(str(oid), "environment");
    addValue(i_tr_oid, 'testRun', 'environment', str(oid))

def __component_test_run(i_conn_json, i_tr_oid, i_test_type, i_run_number):
    logger.debug('\tLocal DB: Register Com-Test Run')

    mo_serial_number = i_conn_json["module"]["serialNumber"];
    
    cmp_oids = []
    if i_conn_json["dummy"]==True:
        cmp_oids.append(mo_serial_number)
        for chip_json in i_conn_json['chips']:
            ch_serial_number = chip_json['serialNumber']
            cmp_oids.append(ch_serial_number)
    else:
        mo_oid = getComponent(mo_serial_number)
        if mo_oid!='':
            cmp_oids.append(mo_oid)
            doc_value = {
                'parent': mo_oid,
                'status': 'active'
            }
            child_entries = localdb.childParentRelation.find(doc_value)
            for child in child_entries:
                chip_oid = child['child']
                cmp_oids.append(chip_oid)
        else:
            message = 'This Module "{0}" is not registered : {1}'.format(mo_serial_number, i_conn_path)
            alert(message)
    for cmp_oid in cmp_oids:
        if i_conn_json["dummy"]==True:
            serial_number = cmp_oid
        else:
            serial_number = getValue('component', '_id', cmp_oid, 'oid', 'serialNumber')
        chip_tx = -1
        chip_rx = -1
        chip_id = -1
        geom_id = -1
        for chip_json in i_conn_json['chips']:
            if chip_json['serialNumber']==serial_number:
                chip_tx = chip_json['tx']
                chip_rx = chip_json['rx']
                geom_id = chip_json['geomId']
                chip_id = chip_json['chipId']
                break                
        doc_value = {
            'sys': {},
            'component': cmp_oid,
            'state': '...',
            'testType': i_test_type,
            'testRun': i_tr_oid,
            'qaTest': False,
            'runNumber': i_run_number,
            'passed': True,
            'problems': True,
            'attachments': [],
            'tx': chip_tx,
            'rx': chip_rx,
            'chipId': chip_id,
            'beforeCfg': '...',
            'afterCfg': '...',
            'geomId': geom_id,
            'dbVersion': __global.db_version
        }
        oid = localdb.componentTestRun.insert(doc_value)
        addSys(str(oid), 'componentTestRun')

def __test_run(i_test_type, i_run_number, i_target_charge, i_target_tot, i_time, i_serial_number, i_type, i_tr_oid=''):
    if DB_DEBUG: pring('\tLocal DB: Register Test Run')
   
    if i_type=='start':
        timestamp = i_time
        start_time = datetime.utcfromtimestamp(timestamp)
        doc_value = {
            'startTime': start_time,
            'serialNumber': i_serial_number
        }
        this_run = localdb.testRun.find_one(doc_value)
        if this_run: return str(this_run['_id'])
        doc_value.update({
            'sys': {},
            'testType': i_test_type,
            'runNumber': i_run_number,
            'passed': False,
            'problems':False,
            'summary': False,
            'dummy': False,
            'state': 'ready',
            'targetCharge': i_target_charge,
            'targetTot': i_target_tot,
            'exec': '...',
            'comments': [],
            'defects': [],
            'finishTime': start_time,
            'plots': [],
            'chipType': __global.chip_type,
            'stage': '...',
            'ctrlCfg': '...',    
            'scanCfg': '...',    
            'environment': '...',
            'address': '...',
            'user_id': '...',
            'dbVersion': __global.db_version
        })
        oid = localdb.testRun.insert(doc_value)
        addSys(str(oid), 'testRun')
        addUser(str(oid), 'testRun')
    if i_type=='finish':
        timestamp = i_time;
        finish_time = datetime.utcfromtimestamp(timestamp)
        array = []
        __global.histo_names = list(set(__global.histo_names))
        for histo_name in __global.histo_names: 
            array.append(histo_name)
        doc_value = {
            'passed': True,
            'problems': True,
            'finishTime': finish_time,
            'plots': array
        }
        localdb.testRun.update(
            { '_id': ObjectId(i_tr_oid) },
            { '$set': doc_value }
        )
        oid = i_tr_oid

    return str(oid)

def __set_conn_cfg(i_conn_jsons, i_cache_dir, db_support):
    if DB_DEBUG: pring('\t\tLocal DB: Check Connectivity Config')

    if not type(i_conn_jsons)==type([]): i_conn_jsons=[i_conn_jsons]
    conn_jsons = []
    for i, conn_json in enumerate(i_conn_jsons):
        # chip type
        __check_empty(conn_json, 'chipType', 'connectivity config')
        __global.chip_type = conn_json['chipType']
        if __global.chip_type=='FEI4B': __global.chip_type = 'FE-I4B'
           
        # module
        mo_oid = None
        if 'module' in conn_json and db_support==True:
            __check_empty(conn_json['module'], 'serialNumber', 'connectivity config')
            mo_oid = getComponent(conn_json['module']['serialNumber'])
            conn_json['dummy'] = False
        else:
            if not 'module' in conn_json or not 'serialNumber' in conn_json['module']:
                conn_json['module'] = { 'serialNumber': 'DUMMY_{}'.format(i) }
            conn_json['dummy'] = True

        # chip
        for j, chip_json in enumerate(conn_json['chips']):
            chip_cfg_path = chip_json['config'].split('/')[len(chip_json['config'].split('/'))-1]
            chip_cfg_json = toJson('{0}/{1}.before'.format(i_cache_dir, chip_cfg_path))
            if not 'chipId' in chip_json:
                if 'chipId' in chip_cfg_json[__global.chip_type]['Parameter']: 
                    conn_json['chips'][j]['chipId'] = chip_cfg_json[__global.chip_type]['Parameter']['chipId']
                elif 'ChipId' in chip_cfg_json[__global.chip_type]['Parameter']: 
                    conn_json['chips'][j]['chipId'] = chip_cfg_json[__global.chip_type]['Parameter']['ChipId']
                else:
                    message = 'There are no chip Id in config file'
                    alert(message) 

            if mo_oid:
                __check_empty(chip_json, 'serialNumber', 'connectivity config')
                ch_oid = getComponent(chip_json['serialNumber'])
                query = {'parent': mo_oid, 'child': ch_oid}
                this_cpr = localdb.childParentRelation.find_one(query)
                if not this_cpr:
                    message = 'Not registered Chip {0} on the Module {1}'.format(chip_json['serialNumber'], conn_json['module']['serialNumber'])
                    alert(message)
            else:
                if not 'serialNumber' in chip_json:
                    conn_json['chips'][j]['serialNumber'] = '{0}_CHIP_{1}'.format(conn_json['module']['serialNumber'], j)

            conn_json['chips'][j]['geomId'] = chip_json.get('geomId', j)

            if 'name' in chip_cfg_json[__global.chip_type]: 
                conn_json['chips'][j]['name'] = chip_cfg_json[__global.chip_type]['name']
            elif 'Name' in chip_cfg_json[__global.chip_type]['Parameter']:
                conn_json['chips'][j]['name'] = chip_cfg_json[__global.chip_type]['Parameter']['Name']
            else:
                conn_json['chips'][j]['name'] = 'NONE'
            
        conn_jsons.append(conn_json)
        if 'stage' in conn_json:
            stage = conn_json['stage']

    return conn_jsons

def __set_localdb(i_localdb):
    global localdb
    global localfs
    localdb = i_localdb
    localfs = gridfs.GridFS(localdb)

def __set_user(i_json):
    logger.debug('Local DB: Set user') 

    if 'userCfg' in i_json:
        user_json = i_json['userCfg']
        user_name = user_json['userName']
        institution = user_json['institution']
        user_identity = user_json.get('description', 'default')
    elif os.path.isfile('{}/.yarr/localdb/user.json'.format(os.environ['HOME'])):
        user_json = toJson('{}/.yarr/localdb/user.json'.format(os.environ['HOME']))
        user_name = user_json.get('userName', os.environ['USER'])
        institution = user_json.get('institution', os.environ['HOSTNAME'])
        user_identity = user_json.get('description', 'default')
    else:
        user_name = os.environ['USER']
        institution = os.environ['HOSTNAME']
        user_identity = 'default'

    user_name = user_name.lower().replace(' ','').replace('_','')
    institution = institution.lower().replace(' ','').replace('_','')

    __global.user_oid = __user(user_name, institution, user_identity)

def __set_site(i_json):
    logger.debug('Local DB: Set site') 

    if 'siteCfg' in i_json:
        site_json = i_json['siteCfg']
        address = site_json['macAddress']
        hostname = site_json.get('hostname', os.environ['HOSTNAME'])
        site = site_json['institution']
    elif os.path.isfile('{}/.yarr/localdb/site.json'.format(os.environ['HOME'])):
        site_json = toJson('{}/.yarr/localdb/site.json'.format(os.environ['HOME']))
        address = site_json.get('macAddress', '{0}_{1}'.format(os.environ['HOSTNAME'], os.environ['USER']))
        hostname = site_json.get('hostname', os.environ['HOSTNAME'])
        site = site_json.get('institution', 'null')
    else:
        address = '{0}_{1}'.format(os.environ['HOSTNAME'], os.environ['USER'])
        hostname = os.environ['HOSTNAME']
        site = 'null'

    site = site.lower().replace(' ','').replace('_','')

    __global.site_oid = __site(address, hostname, site);

def __set_test_run_start(i_test_type, i_conn_jsons, i_run_number, i_target_charge, i_target_tot, i_timestamp, i_command):
    logger.debug('Local DB: Write Test Run (start)')

    for conn_json in i_conn_jsons:
        mo_serial_number = conn_json["module"]["serialNumber"]
        tr_oid = __test_run(i_test_type, i_run_number, i_target_charge, i_target_tot, i_timestamp, mo_serial_number, 'start')
        addValue(tr_oid, 'testRun', 'exec', i_command)
        if 'stage' in conn_json:
            addValue(tr_oid, 'testRun', 'stage', conn_json['stage'])
        if conn_json['dummy']==True:
            addValue(tr_oid, 'testRun', 'dummy', 'true', 'bool')
        __global.tr_oids.append(tr_oid)
        __component_test_run(conn_json, tr_oid, i_test_type, i_run_number)

    return

def __set_test_run_finish(i_test_type, i_conn_jsons, i_run_number, i_target_charge, i_target_tot, i_timestamp, i_command, i_scan_log):
    logger.debug('Local DB: Write Test Run (finish)')

    for tr_oid in __global.tr_oids:
        __test_run(i_test_type, i_run_number, i_target_charge, i_target_tot, i_timestamp, '', 'finish', tr_oid)

        for key in i_scan_log:
            if not key=='connectivity' and not key=='ctrlCfg' and not key=='dbCfg':
                query = { '_id': ObjectId(tr_oid) }
                this_run = localdb.testRun.find_one(query)
                if not key in this_run:
                    localdb['testRun'].update_one(
                        { '_id': ObjectId(tr_oid) },
                        { '$set': {
                            key: i_scan_log[key]
                        }}
                    )
    return

def __set_config(i_tx, i_rx, i_file_json, i_filename, i_title, i_col, i_serial_number, i_conn_jsons):
    logger.debug('Local DB: Write Config Json: {}'.format(i_filename))

    if i_file_json=={}: return

    if i_serial_number!='': serial_number = i_serial_number
    else:
        for conn_json in i_conn_jsons:
            for chip_json in conn_json['chips']:
                if chip_json['tx']==i_tx and chip_json['rx']==i_rx: serial_number = chip_json['serialNumber']
    __config(serial_number, i_file_json, i_filename, i_title, i_col)

    return

def __set_attachment(i_tx, i_rx, i_file_path, i_histo_name, i_serial_number, i_conn_jsons):
    logger.debug('Local DB: Write Attachment: {}'.format(i_file_path))

    if not os.path.isfile(i_file_path): return

    if i_serial_number!='': serial_number = i_serial_number
    else:
        for conn_json in i_conn_jsons:
            for chip_json in conn_json['chips']:
                if chip_json['tx']==i_tx and chip_json['rx']==i_rx: serial_number = chip_json['serialNumber']
    __attachment(serial_number, i_file_path, i_histo_name)

    __global.histo_names.append(i_histo_name);

    return

def __check_conn_cfg(i_conn_path):
    logger.debug('\tLocal DB: Check connectivity config for registration: {}'.format(i_conn_path))

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
    __check_list(mo_component_type, __global.cmp_list, 'component')
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
        __check_list(ch_component_type, __global.cmp_list, 'component')
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

def __check_list(i_value, i_list, i_name):
    if not i_value in i_list:
        message = 'Not registered {0} in the {1} list in database config file'.format(i_value, i_name)
        alert(massage)
    return 

def __check_empty(i_json, i_key, i_filename):
    logger.debug('\t\tLocal DB: Check Empty: {0} in {1}'.format(i_key, i_filename))

    if not i_key in i_json:
        message = 'Found an empty field in json file.\n\tfile: {0}  key: {1}'.format(i_filename, i_key)
        alert(message)
    return

class __global:
    chip_type = ''
    user_oid = ''
    site_oid = ''
    tr_oids = []
    tr_oid = ''
    histo_names = []
    db_version = 1
    stage_list = []
    env_list = []
    cmp_list = []
    dir_path = ''
    option = ''
    def clean():
        chip_type = ''
        user_oid = ''
        site_oid = ''
        tr_oids = []
        tr_oid = ''
        histo_names = []
        db_version = 1
        stage_list = []
        env_list = []
        cmp_list = []
        dir_path = ''
        option = ''
