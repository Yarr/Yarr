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
from dateutil.tz      import tzlocal
from tzlocal          import get_localzone
import pytz
import time

DB_DEBUG = False

global localdb
global localfs
max_server_delay = 1
localdb = MongoClient("mongodb://127.0.0.1:27017", serverSelectionTimeoutMS=max_server_delay)['localdb']
global m_stage_list
global m_env_list
global m_cmp_list

# local function
def setTime(i_date):
    local_tz = get_localzone() 
    converted_time = i_date.replace(tzinfo=timezone.utc).astimezone(local_tz)
    time = converted_time.strftime('%Y/%m/%d %H:%M:%S')
    return time

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
    if DB_DEBUG: print('\t\tLocal DB: Add document: {0} to {1}'.format(i_key, i_col))
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
    if DB_DEBUG: print('\t\tLocal DB: Add user and institution')

    localdb[i_col].update(
        { 
            '_id': ObjectId(i_oid) 
        },{
            '$set': { 
                'address': __global.m_site_oid,
                'user_id': __global.m_user_oid
            }
        }
    )

def alert(i_message, i_type='error'):
    if DB_DEBUG: print('Local DB: Alert "{}"'.format(i_type))

    if i_type=='error':
        alert_message = '#DB ERROR#'
    elif i_type=='warning':
        alert_message = '#DB WARNING#'

    print('{0} {1}'.format(alert_message, i_message))
# TODO logging
#    now = datetime.now().timestamp()
#
#    m_cache_dir+"/var/log/localdb/"+timestamp+"_error.log"
#    std::string log_path = m_cache_dir+"/var/log/"+timestamp+"_error.log";
#    std::ofstream file_ofs(log_path, std::ios::app);
#    strftime(tmp, 20, "%F_%H:%M:%S", lt);
#    timestamp=tmp;
#    file_ofs << timestamp << " " << alert_message << " [" << m_option << "] " << i_function << std::endl;
#    file_ofs << "Message: " << i_message << std::endl;
#    file_ofs << "Log: " << m_log_path << std::endl;
#    file_ofs << "Cache: " << m_cache_path << std::endl;
#    file_ofs << "--------------------" << std::endl;

    if i_type=='error': sys.exit()

def toJson(i_file_path):
    if DB_DEBUG: print('\t\tLocal DB: Convert to json code from: {}'.format(i_file_path))

    file_json = {}
    if os.path.isfile(i_file_path):
        try:
            with open(i_file_path, 'r') as f: file_json = json.load(f)
        except ValueError as e:
            message = 'Could not parse {0}\n\twhat(): {1}'.format(i_file_path, e)
            alert(message)

    return file_json

def writeJson(i_key, i_value, i_file_path, i_file_json):
    if DB_DEBUG: print('\tLocal DB: Write Json file: {}'.format(i_file_path))

    i_file_json[i_key] = i_value
    with open(i_file_path, 'w') as f:
        json.dump(i_file_json, f, indent=4)

def getComponent(i_serial_number):
    if DB_DEBUG: print('\tLocal DB: Get component data: "Serial Number": {}'.format(i_serial_numebr))

    doc_value = { 'serialNumber': i_serial_number }
    this_cmp = localdb.component.find_one(doc_value)
    oid = ''
    if this_cmp:
        oid = str(this_cmp['_id'])
    return oid

def getValue(i_col, i_mem_key, i_mem_val, i_mem_type, i_key, i_type):
    if DB_DEBUG: print('\tLocal DB: Get Value of Key: "{0}" from: "{1}" query by Key: "{2}" and Value: "{3}"'.format(i_key, i_col, i_mem_key, i_mem_val))

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

def getHash(i_file_path):
    if DB_DEBUG: print('\tLocal DB: Get Hash Code from File: {}'.format(i_file_path))

    with open(i_file_path, 'rb') as f:
        binary = f.read()
        shaHashed = hashlib.sha256(binary).hexdigest()

    return shaHashed

# register function
def __user(i_user_name, i_institution, i_user_identity):
    if DB_DEBUG: print('\tLocal DB: Register user \n\tUser name: {0} \n\tInstitution: {1} \n\tUser identity: {2}'.format(i_user_name, i_institution, i_user_identity))
 
    doc_value = {
        'userName'    : i_user_name,
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
            'dbVersion': __global.m_db_version
        })
        user_oid = localdb.user.insert(doc_value)
        addSys(str(user_oid), 'user')

    return str(user_oid)

def __site(i_address, i_hostname, i_site):
    if DB_DEBUG: print('\tLocal DB: Register site \n\tAddress: {0} \n\tName: {1} \n\tInstitution: {2}'.format(i_address, i_hostname, i_site)) 
 
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
            'dbVersion': __global.m_db_version
        })
        site_oid = localdb.institution.insert(doc_value)
        addSys(str(site_oid), 'institution')

    return str(site_oid)

def __component(i_serial_number, i_component_type, i_chip_id, i_chips):
    if DB_DEBUG: print('\tLocal DB: Register Component: {}'.format(i_serial_number))

    doc_value = {
        'serialNumber': i_serial_number
    }
    this_cmp = localdb.component.find_one(doc_value)
    if this_cmp: 
        cmp_oid = this_cmp['_id']
    else:
        doc_value.update({
            'sys': {},
            'chipType': __global.m_chip_type,
            'componentType': i_component_type,
            'children': i_chips,
            'chipId': i_chip_id,
            'dbVersion': __global.m_db_version,
            'address': '...',
            'user_id': '...'
        })
        cmp_oid = localdb.component.insert(doc_value)
        addSys(str(cmp_oid), 'component')
        #addUser(str(cmp_oid), 'component');

    return str(cmp_oid);

def __child_parent_relation(i_parent_oid, i_child_oid, i_chip_id):
    if DB_DEBUG: print('\tLocal DB: Register Child Parent Relation.')
    
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
            'dbVersion': __global.m_db_version
        })
        cpr_oid = localdb.childParentRelation.insert(doc_value)
        addSys(str(cpr_oid), 'childParentRelation')

    return

def __conn_cfg(i_conn_paths):
    if DB_DEBUG: print('Local DB: Register Component') 

    for conn_path in i_conn_paths:
        if DB_DEBUG: print('\tLocal DB: Register from connectivity: {}'.format(conn_path))

        conn_json = toJson(conn_path)
        mo_serial_number = conn_json["module"]["serialNumber"]

        __global.m_chip_type = conn_json['chipType']
        if __global.m_chip_type=='FEI4B': __global.m_chip_type = 'FE-I4B'

        module_is_exist = False
        chip_is_exist = False
        cpr_is_fine = True

        mo_oid_str = getComponent(mo_serial_number) 
        if mo_oid_str!='': module_is_exist = True
        chips = 0
        for i, chip_conn_json in enumerate(conn_json['chips']):
            ch_serial_number = chip_conn_json['serialNumber']
            chip_oid_str = getComponent(ch_serial_number)
            if chip_oid_str!='':
                chip_is_exist = True
                doc_value = {
                    'parent': mo_oid_str,
                    'child' : chip_oid_str,
                    'status': 'active'
                }
                this_cpr = localdb.childParentRelation.find_one(doc_value)
                if not this_cpr: cpr_is_fine = False
            chips+=1
        if module_is_exist and not chip_is_exist:
            message = 'There are registered module in connectivity: {}'.format(conn_path)
            alert(message)
        elif not module_is_exist and chip_is_exist:
            message = 'There are registered chips in connectivity: {}'.format(conn_path)
            alert(message)
        elif not cpr_is_fine:
            message = 'There are wrong relationship between module and chips in connectivity: {}'.format(conn_path)
            alert(message)
        elif module_is_exist and chip_is_exist and cpr_is_fine:
            return False
        mo_component_type = conn_json['module']['componentType']
        mo_oid_str = __component(mo_serial_number, mo_component_type, -1, chips)
        
        for i, chip_conn_json in enumerate(conn_json['chips']):
            ch_serial_number = chip_conn_json['serialNumber']
            ch_component_type = chip_conn_json['componentType']
            chip_id = chip_conn_json['chipId']
            ch_oid_str = __component(ch_serial_number, ch_component_type, chip_id, -1)
            __child_parent_relation(mo_oid_str, ch_oid_str, chip_id)

    return True

def __config(i_serial_number, i_file_path, i_filename, i_title, i_col):
    if DB_DEBUG: pring('\tLocal DB: Register Config Json: {}'.format(i_file_path))

    ctr_oid = ''
    if i_serial_number!='':
        doc_value = { 'serialNumber': i_serial_number }
        this_cmp = localdb.component.find_one(doc_value)
        if this_cmp:
            cmp_oid = str(this_cmp['_id'])
        else:
            cmp_oid = i_serial_number

        for tr_oid in __global.m_tr_oids:
            doc_value = { 
                'component': cmp_oid,
                'testRun': tr_oid
            }
            this_ctr = localdb.componentTestRun.find_one(doc_value)
            if this_ctr:
                ctr_oid = str(this_ctr['_id'])

    if i_col=='testRun':
        cfg_oid = __json_code(i_file_path, i_filename+'.json', i_title, 'gj')
        for tr_oid in __global.m_tr_oids:
            if tr_oid=='': continue
            addValue(tr_oid, i_col, i_title, cfg_oid)
    if i_col=='componentTestRun':
        if not ctr_oid=='': 
            cfg_oid = __json_code(i_file_path, i_title+'.json', i_title, 'gj')
            addValue(ctr_oid, i_col, i_filename, cfg_oid)

    return

def __json_code(i_file_path, i_filename, i_title, i_type):
    if DB_DEBUG: print('\tLocal DB: Upload Json File')

    if i_type=='gj':
        data_id = __json_code_gridfs(i_file_path, i_filename, i_title)
        type_doc = 'fs.files'
    #elif i_type=='m': #TODO to be added
    else:
        message = 'Unknown type "{}" to upload json file'.format(i_type)
        alert(message)

    doc_value = {
        'sys': {},
        'filename': i_filename,
        'chipType': __global.m_chip_type,
        'title': i_title,
        'format': type_doc,
        'data_id': data_id,
        'dbVersion': __global.m_db_version
    }

    oid = localdb.config.insert(doc_value)
    addSys(str(oid), 'config')

    return str(oid)

def __json_code_gridfs(i_file_path, i_filename, i_title):
    if DB_DEBUG: print('\tLocal DB: Write Json File: {}'.format(i_file_path))

    hash_code = getHash(i_file_path)
    doc_value = { 'hash': hash_code }
    this_cfg = localdb.fs.files.find_one( doc_value, { '_id': 1 } )
    if this_cfg:
        code = str(this_cfg['_id'])
    else:
        code = __grid_fs_file(i_file_path, i_filename, hash_code) 

    return code

def __grid_fs_file(i_file_path, i_filename, i_hash=''):
    if DB_DEBUG: pring('Local DB: Write Attachment: {}'.format(i_file_path))
    
    with open(i_file_path, 'rb') as f:
        binary = f.read()
        shaHashed = hashlib.sha256(binary).hexdigest()

    if i_hash=='': code = localfs.put( binary, filename=i_filename, dbVersion=__global.m_db_version ) 
    else: code = localfs.put( binary, filename=i_filename, hash=i_hash, dbVersion=__global.m_db_version )

    return str(code)

def __attachment(i_serial_number, i_file_path, i_histo_name):
    if DB_DEBUG: print('\tLocal DB: Register Attachment: {}'.format(i_file_path))
    
    if i_serial_number!='':
        doc_value = { 'serialNumber': i_serial_number } 
        this_cmp = localdb.component.find_one(doc_value)
        if this_cmp: 
            cmp_oid = str(this_cmp['_id'])
        else:
            cmp_oid = i_serial_number

        for tr_oid in __global.m_tr_oids:
            doc_value = { 
                'component': cmp_oid,
                'testRun': tr_oid
            }
            this_ctr = localdb.componentTestRun.find_one(doc_value)
            if this_ctr:
                ctr_oid = str(this_ctr['_id'])

    if not ctr_oid=='':
        if os.path.isfile(i_file_path):
            code = __grid_fs_file(i_file_path, '{}.dat'.format(i_histo_name)) 
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

def __dcs(i_dcs_path, i_tr_path):
    if DB_DEBUG: print('Local DB: Register Environment: {}'.format(i_dcs_path))

    
    dcs_json = toJson(i_dcs_path)
    tr_json = toJson(i_tr_path)

    if __global.m_tr_oid=='':
        message = 'Not found relational test run data in DB'
        alert(message)
    doc_value = { '_id': ObjectId(__global.m_tr_oid) }
    this_run = localdb.testRun.find_one(doc_value)
    if this_run.get('environment','...')!='...':
        message = 'Already registered dcs data to this test run data in DB'
        alert(message)
    if not 'environments' in dcs_json: return

    starttime = this_run['startTime'].timestamp()
    finishtime = this_run['finishTime'].timestamp()

    env_json = dcs_json['environments']
    doc_value = {
        'dbVersion': __global.m_db_version,
        'sys': {}
    }
    for env_j in env_json:
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
            env_file = open('{0}/{1}'.format(m_cache_json['logPath'],env_j['path']),'r')
            key_values = []
            dates = []
            data_num = 0
            # key and num
            env_key_line = env_file.readline()
            env_num_line = env_file.readline()
            for j, tmp_key in enumerate(env_key_line.split(separator)):
                if env_key==tmp_key and env_j['num']==env_num_line.split(separator)[j]:
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
                if env_line.split(separator) < key: break
                datetime = int(env_line.split(separator)[1])
                value = float(env_line.split(separator)[key])
                if 'margin' in env_j:
                    if starttime-datetime<env_j['margin'] and finishtime-datetime>env_j['margin']:
                        array.append({
                            'date': datetime.utcfromtimestamp(datetime),
                            'value': value
                        })
                else:
                    array.append({
                        'date': datetime.utcfromtimestamp(datetime),
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
    addValue(__global.m_tr_oid, 'testRun', 'environment', str(oid))

def __component_test_run(i_conn_path, i_tr_oid, i_test_type, i_run_number):
    if DB_DEBUG: print('\tLocal DB: Register Com-Test Run')

    conn_json = toJson(i_conn_path)
    mo_serial_number = conn_json["module"]["serialNumber"];
    
    cmp_oids = []
    if conn_json["dummy"]:
        cmp_oids.append(mo_serial_number)
        for chip_json in conn_json['chips']:
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
        if conn_json["dummy"]:
            serial_number = cmp_oid
        else:
            serial_number = getValue('component', '_id', cmp_oid, 'oid', 'serialNumber')
        chip_tx = -1
        chip_rx = -1
        chip_id = -1
        geom_id = -1
        for chip_json in conn_json['chips']:
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
            'dbVersion': __global.m_db_version
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
            'command': '...',
            'comments': [],
            'defects': [],
            'finishTime': start_time,
            'plots': [],
            'chipType': __global.m_chip_type,
            'stage': '...',
            'ctrlCfg': '...',    
            'scanCfg': '...',    
            'environment': '...',
            'address': '...',
            'user_id': '...',
            'dbVersion': __global.m_db_version
        })
        oid = localdb.testRun.insert(doc_value)
        addSys(str(oid), 'testRun')
        addUser(str(oid), 'testRun')
    if i_type=='finish':
        timestamp = i_time;
        finish_time = datetime.utcfromtimestamp(timestamp)
        array = []
        for histo_name in __global.m_histo_names: 
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

    return oid

def __set_conn_cfg(i_conn_paths):
    if DB_DEBUG: pring('Local DB: Set Connectivity Config')

    for conn_path in i_conn_paths:
        conn_json = __check_conn_cfg(conn_path)
        __global.m_chip_type = conn_json['chipType']
        if __global.m_chip_type=='FEI4B': __global.m_chip_type = 'FE-I4B'

def __check_conn_cfg(i_conn_path):
    if DB_DEBUG: pring('\t\tLocal DB: Check Connectivity Config: {}'.format(i_conn_path))
    conn_json = toJson(i_conn_path);
    #this->checkEmpty(conn_json["chipType"].empty(), "chipType", i_conn_path);
    if 'module' in conn_json:
        #this->checkEmpty(conn_json["module"]["serialNumber"].empty(), "module.serialNumber", i_conn_path); 
        is_listed = False
        for chip_json in conn_json['chips']:
            print('A')
            #this->checkEmpty(conn_json["chips"][i]["chipId"].empty(), "chips."+std::to_string(i)+".chipId", i_conn_path);
            #this->checkEmpty(conn_json["chips"][i]["serialNumber"].empty(), "chips."+std::to_string(i)+".serialNumber", i_conn_path);
            #this->checkEmpty(conn_json["chips"][i]["geomId"].empty(), "chips."+std::to_string(i)+".geomId", i_conn_path);
    if 'stage' in conn_json:
        stage = conn_json['stage']
        #checkList(m_stage_list, stage, m_db_cfg_path, i_conn_path)

    __global.m_conn_jsons.append(conn_json)
    return conn_json;

def __set_localdb(i_localdb):
    global localdb
    global localfs
    localdb = i_localdb
    localfs = gridfs.GridFS(localdb)

def __set_stage_list(i_stage_list):
    global m_stage_list
    m_stage_list = i_stage_list

def __set_env_list(i_env_list):
    global m_env_list
    m_env_list = i_env_list

def __set_cmp_list(i_cmp_list):
    global m_cmp_list
    m_cmp_list = i_cmp_list

def __set_user(user_path):
    if DB_DEBUG: print('DBHandler: Set user: {}'.format(user_path)) 

    if user_path == '':
        user_name = os.environ['USER']
        institution = os.environ['HOSTNAME']
        user_identity = 'default'
    else:
        user_json = toJson(user_path) 
        user_name = user_json.get('userName', os.environ['USER'])
        institution = user_json.get('institution', os.environ['HOSTNAME'])
        user_identity = user_json.get('userIdentity', 'default')

    __global.m_user_oid = __user(user_name, institution, user_identity)

def __set_site(site_path):
    if DB_DEBUG: print('DBHandler: Set site: {}'.format(site_path)) 

    if site_path == '':
        adderss = os.environ['HOSTNAME']
        hostname = os.environ['HOSTNAME']
        site = 'null'
    else:
        site_json = toJson(site_path)
        address = site_json['macAddress']
        hostname = site_json['hostname']
        site = site_json['institution']

    __global.m_site_oid = __site(address, hostname, site);

def __set_conn(conn_paths):
    if DB_DEBUG: print('DBHandler: Set connectivity config') 

    for conn_path in conn_paths:
        if DB_DEBUG: print('\tDBHandler: setting connectivity config file: {}'.format(conn_path))
        with open(conn_path, 'r') as f: conn_json = json.load(f)
        __global.m_chip_type = conn_json['chipType']
        if __global.m_chip_type=='FEI4B': __global.m_chip_type = 'FE-I4B'

def __set_test_run_start(i_test_type, i_conn_paths, i_run_number, i_target_charge, i_target_tot, i_timestamp, i_command):
    if DB_DEBUG: print('Local DB: Write Test Run (start)')

    for conn_path in i_conn_paths:
        conn_json = toJson(conn_path)
        chip_type = conn_json['chipType']
        mo_serial_number = conn_json["module"]["serialNumber"]
        tr_oid = __test_run(i_test_type, i_run_number, i_target_charge, i_target_tot, i_timestamp, mo_serial_number, 'start')
        addValue(tr_oid, 'testRun', 'command', i_command)
        if 'stage' in conn_json:
            addValue(tr_oid, 'testRun', 'stage', stage)
        if conn_json['dummy']==True:
            addValue(tr_oid, 'testRun', 'dummy', 'true', 'bool')
        __global.m_tr_oids.append(tr_oid)
        __component_test_run(conn_path, tr_oid, i_test_type, i_run_number)
        __global.m_serial_numbers.append(mo_serial_number)

    return

def __set_test_run_finish(i_test_type, i_conn_paths, i_run_number, i_target_charge, i_target_tot, i_timestamp, i_command):
    if DB_DEBUG: print('Local DB: Write Test Run (finish)')

    __global.m_histo_names = list(set(__global.m_histo_names))
    for tr_oid in __global.m_tr_oids:
        __test_run(i_test_type, i_run_number, i_target_charge, i_target_tot, i_timestamp, '', 'finish', tr_oid)
    
    return

def __set_config(i_tx, i_rx, i_file_path, i_filename, i_title, i_col, i_serial_number):
    if DB_DEBUG: print('Local DB: Write Config Json: {}'.format(i_file_path))

    if not os.path.isfile(i_file_path): return

    if i_serial_number!='': serial_number = i_serial_number
    else:
        for conn_json in m_conn_jsons:
            for chip_json in conn_json['chips']:
                if chip_json['tx']==i_tx and chip_json['rx']==i_rx: serial_number = chip_json['serialNumber']
    __config(serial_number, i_file_path, i_filename, i_title, i_col)

    return

def __set_attachment(i_tx, i_rx, i_file_path, i_histo_name, i_serial_number):
    if DB_DEBUG: print('Local DB: Write Attachment: {}'.format(i_file_path))

    if not os.path.isfile(i_file_path): return

    if i_serial_number!='': serial_number = i_serial_number
    else:
        for conn_json in m_conn_jsons:
            for chip_json in conn_json['chips']:
                if chip_json['tx']==i_tx and chip_json['rx']==i_rx: serial_number = chip_json['serialNumber']
    __attachment(serial_number, i_file_path, i_histo_name)

    __global.m_histo_names.append(i_histo_name);

    return

def test():
    __global.m_chip_type = 'test'

class __global:
    m_chip_type = ''
    m_user_oid = ''
    m_site_oid = ''
    m_tr_oids = []
    m_tr_oid = ''
    m_histo_names = []
    m_serial_numbers = []
    m_db_version = 1
    m_conn_jsons = []
