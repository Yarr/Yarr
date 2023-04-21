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
from pymongo          import MongoClient, DESCENDING
from bson.objectid    import ObjectId
from datetime         import datetime, timezone, timedelta
import time
import pprint
import traceback
import inspect

# Log
from logging import getLogger
logger = getLogger('Log').getChild('Register')
from common import readJson
from pathlib import Path

def get_function_name():
    return traceback.extract_stack(None, 2)[0][2]

home = os.environ['HOME']
if not 'HOSTNAME' in os.environ:
    hostname = 'default_host'
else:
    hostname = os.environ['HOSTNAME']

class RegisterError(Exception):
    pass

class ValidationError(Exception):
    pass

class DcsDataError(Exception):
    pass

class RegisterData():

    def __init__(self):
        self.logger = getLogger('Log').getChild('Register')
        self.logger.info(f'RegisterData.{get_function_name()}: Initialize register function')
        self.dbstatus = False
        self.updated = {}
        self.db_version = 1.01
        self.tr_oids = []

        self.chip_type = None
        self.user_json = {
            'userName'   : os.environ['USER'],
            'institution': hostname,
            'description': 'default',
            'USER'       : os.environ['USER'],
            'HOSTNAME'   : hostname,
        }
        self.site_json = {
            'address'    : ':'.join(['{:02x}'.format((uuid.getnode() >> ele) & 0xff)for ele in range(0,8*6,8)][::-1]),
            'HOSTNAME'   : hostname,
            'institution': hostname,
        }
        self.conns = []


    def setDb(self, i_cfg, i_localdb, i_toolsdb):
        self.db_cfg = i_cfg
        self.localdb = i_localdb
        self.toolsdb = i_toolsdb
        self.localfs = gridfs.GridFS(self.localdb)

        self.localdb['fs.files'].create_index([('hash', DESCENDING), ('_id', DESCENDING)])
        self.localdb['component'].create_index([('serialNumber', DESCENDING)])
        self.localdb['testRun'].create_index([('startTime', DESCENDING), ('user_id', DESCENDING), ('address', DESCENDING)])
        self.localdb['componentTestRun'].create_index([('name', DESCENDING), ('testRun', DESCENDING)])
        self.dbstatus = True

        self.db_list = {}
        keys = [ 'environment' ]
        for key in keys:
            self.db_list.update({ key: [] })
            for value in i_cfg.get(key, []):
                self.db_list[key].append(value.lower().replace(' ','_'))


    def setUser(self, i_json):
        self.logger.info(f'RegisterData.{get_function_name()}: Set User')
        self.user_json.update(i_json)

    def setSite(self, i_json):
        self.logger.info(f'RegisterData.{get_function_name()}: Set Site')
        self.site_json.update(i_json)

    def setConnCfg(self, conn_dir, i_conn, i_cache_dir=''):
        self.logger.info(f'RegisterData.{get_function_name()}: Set Connectivity Config')
        if i_conn=={}:
            return i_conn

        # chip type
        self._check_empty(i_conn, 'chipType', 'connectivity config')
        self.chip_type = i_conn['chipType']
        if self.chip_type=='FEI4B': self.chip_type = 'FE-I4B'

        conn = { 'module': {}, 'chips': [] }

        # module
        if 'module' in i_conn:
            self._check_empty(i_conn['module'], 'serialNumber', 'connectivity.module')
            conn['module'] = i_conn['module']
            conn['module']['name'] = conn['module']['serialNumber']
            conn['module']['componentType'] = conn['module'].get('componentType', 'module')
        # chips
        for i, chip_json in enumerate(i_conn['chips']):
            if chip_json.get('enable', 1)==0: # disabled chip #TODO
                if not 'name' in chip_json:   chip_json['name'] = chip_json.get('serialNumber', 'DisabledChip_{}'.format(i))
                if not 'chipId' in chip_json: chip_json['chipId'] = -1
                chip_json['component'] = '...'
            else: # enabled chip
                if not i_cache_dir=='':
                    chip_json['config'] = chip_json['config'].split('/')[len(chip_json['config'].split('/'))-1]
                    path = '{0}/{1}.before'.format(i_cache_dir, chip_json['config'])
                else:
                    path = chip_json['config']

                if chip_json['path'] == 'relToCon':
                    path = str(Path(conn_dir).parent / path)

                chip_cfg_json = readJson(path)
                if not self.chip_type in chip_cfg_json:
                    self.logger.error('Not found {0} in chip config file: {1}'.format(self.chip_type, path))
                    raise RegisterError
                if 'name' in chip_cfg_json[self.chip_type]: # for FEI4B
                    chip_json['name'] = chip_cfg_json[self.chip_type]['name']
                    chip_json['chipId'] = chip_cfg_json[self.chip_type]['Parameter']['chipId']
                elif 'Name' in chip_cfg_json[self.chip_type].get('Parameter',{}): # for RD53A
                    chip_json['name'] = chip_cfg_json[self.chip_type]['Parameter']['Name']
                    chip_json['chipId'] = chip_cfg_json[self.chip_type]['Parameter']['ChipId']
                else: # TODO
                    chip_json['name'] = 'UnnamedChip_{}'.format(i)
                    chip_json['chipId'] = -1
            chip_json['serialNumber'] = chip_json['name']
            chip_json['componentType'] = chip_json.get('componentType', 'front-end_chip')
            chip_json['geomId'] = chip_json.get('geomId', i)
            conn['chips'].append(chip_json)
        self.conns.append(conn)
        return conn


    def _update_sys(self, i_oid, i_col):
        if i_oid in self.updated.get(i_col, []): return
        self.logger.info(f'RegisterData.{get_function_name()}: \t\t\tUpdate system information: {i_oid} in {i_col}')
        query = { '_id': ObjectId(i_oid), 'dbVersion': self.db_version }
        this = self.localdb[i_col].find_one(query)
        now = datetime.utcnow()
        if not this: return
        if this.get('sys',{})=={}:
            doc_value = { '$set': { 'sys': { 'cts': now,                        'mts': now, 'rev': 0 }}}
        else:
            doc_value = { '$set': { 'sys': { 'cts': this['sys'].get('cts',now), 'mts': now, 'rev': this['sys'].get('rev',0)+1 }}}
        self.localdb[i_col].update_one( query, doc_value )

        if not i_col in self.updated:
            self.updated.update({ i_col: [] })
        self.updated[i_col].append(i_oid)


    def _add_value(self, i_oid, i_col, i_key, i_value, i_type='string'):
        self.logger.info(f'RegisterData.{get_function_name()}: \t\t\tAdd document: {i_key} to {i_col}')
        if i_type=='string': value = str(i_value)
        elif i_type=='bool': value = i_value.lower()=='true'
        elif i_type=='int':  value = int(i_value)
        query = { '_id': ObjectId(i_oid) }
        doc_value = { '$set': { i_key: i_value }}
        self.localdb[i_col].update_one( query, doc_value )


    def _get_hash(self, i_file_data, i_type='json'):
        self.logger.info(f'RegisterData.{get_function_name()}: \t\t\tGet Hash Code from File')
        if i_type=='json':
            shaHashed = hashlib.sha256(json.dumps(i_file_data, indent=4).encode('utf-8')).hexdigest()
        elif i_type=='dat':
            with open(i_file_data, 'rb') as f:
                binary = f.read()
            shaHashed = hashlib.sha256(binary).hexdigest()
        return shaHashed


    def _check_empty(self, i_json, i_key, i_filename):
        self.logger.info(f'RegisterData.{get_function_name()}: \tCheck Empty:')
        self.logger.info(f'RegisterData.{get_function_name()}: \t- key: {i_key}')
        self.logger.info(f'RegisterData.{get_function_name()}: \t- file: {i_filename}')
        if type(i_key)==type([]):
            if set(i_key)&set(i_json)==set({}):
                self.logger.error('Found an empty field in json file.')
                self.logger.error('\tfile: {0}  key: {1}'.format(i_filename, ' or '.join(map(str, A))))
                raise RegisterError
        else:
            if not i_key in i_json:
                self.logger.error('Found an empty field in json file.')
                self.logger.error('\tfile: {0}  key: {1}'.format(i_filename, i_key))
                raise RegisterError
        return


    def _check_number(self, i_json, i_key, i_filename):
        self.logger.info(f'RegisterData.{get_function_name()}: \tCheck Number:')
        self.logger.info(f'RegisterData.{get_function_name()}: \t- key: {i_key}')
        self.logger.info(f'RegisterData.{get_function_name()}: \t- file: {i_filename}')
        try:
            float(i_json.get(i_key, ''))
        except ValueError:
            self.logger.error('This field must be the number.')
            self.logger.error('\tfile: {0}  key: {1}'.format(i_filename, i_key))
            raise RegisterError
        return


    def _check_user(self, i_register=True):
        """
        This function checks user data
        If there is a matching data, return oid
        If there is not a matching data, register user_json and return oid
        """
        self.logger.info(f'RegisterData.{get_function_name()}: \tCheck User')
        oid = 'null'
        query = {
            'userName'   : os.environ['USER'],
            'institution': hostname,
            'description': 'default',
            'USER'       : os.environ['USER'],
            'HOSTNAME'   : hostname,
            'dbVersion'  : self.db_version
        }
        if not self.user_json=={}: query.update(self.user_json)
        query['userName'] = query['userName'].lower().replace(' ','_')
        query['institution'] = query['institution'].lower().replace(' ','_')
        self.user_json = query
        this_user = self.localdb.user.find_one(query)
        if this_user: oid = str(this_user['_id'])
        elif i_register: oid = self.__register_user()
        return oid


    def _check_site(self, i_register=True):
        """
        This function checks site data
        If there is a matching data, return oid
        If there is not a matching data, register site_json and return oid
        """
        self.logger.info(f'RegisterData.{get_function_name()}: \tCheck Site')
        oid = 'null'
        query = {
            'address'    : ':'.join(['{:02x}'.format((uuid.getnode() >> ele) & 0xff)for ele in range(0,8*6,8)][::-1]),
            'HOSTNAME'   : hostname,
            'institution': hostname,
            'dbVersion'  : self.db_version
        }
        if not self.site_json=={}: query.update(self.site_json)
        query['institution'] = query['institution'].lower().replace(' ','_')
        self.site_json = query
        this_site = self.localdb.institution.find_one(query)
        if this_site: oid = str(this_site['_id'])
        elif i_register: oid = self.__register_site()
        return oid


    def _check_component(self, i_json, i_qc=False):
        """
        This function checks component data
        If there is a matching data, return oid
        If there is not a matching data, return '...'
        """
        self.logger.info(f'RegisterData.{get_function_name()}: \tCheck Component: query = {i_json}')
        oid = '...'
        if not 'serialNumber' in i_json:
            return oid

        if not 'componentType' in i_json:
            return oid


        cptTypeString = i_json.get('componentType').lower().replace(' ', '_')

        if cptTypeString == 'module':
            query = {
                'serialNumber' : i_json['serialNumber'],
                'componentType': i_json['componentType'].lower().replace(' ','_'),
            }

        else:
            # FE chip
            query = {
                'serialNumber' : self._get_chip_serial_number( i_json['serialNumber'] ),
                'componentType': i_json['componentType'].lower().replace(' ','_'),
            }

        this_cmp = self.localdb.component.find_one(query)
        oid = str(this_cmp['_id'])

        return oid


    def _get_chip_serial_number( self, hexstr ):
        try:
            return '20UPGFC' + str( int(hexstr, 16) ).zfill(7)
        except:
            return hexstr

    def _check_child_parent_relation(self, i_mo_oid, i_ch_oid):
        """
        This function checks childParentRelation data
        If there is a matching data, return oid
        If there is not a matching data, return None
        """
        self.logger.info(f'RegisterData.{get_function_name()}: \tCheck Child Parent Relation: module doc {i_mo_oid}, fe_chip doc {i_ch_oid}')
        oid = None
        if not i_mo_oid=='...' and not i_ch_oid=='...':
            query = {
                'parent'   : i_mo_oid,
                'child'    : i_ch_oid,
                'status'   : 'active',
                'dbVersion': self.db_version
            }
            this_cpr = self.localdb.childParentRelation.find_one(query)
            if this_cpr: oid = str(this_cpr['_id'])
        return oid


    def _check_chip(self, i_json, i_register=True):
        """
        This function checks chip data
        If there is a matching data, return oid
        If there is not a matching data, register chip data and return oid
        If chip is disabled, return '...'
        """
        self.logger.info(f'RegisterData.{get_function_name()}: \tCheck Chip data:')
        oid = '...'
        if not i_json.get('enable', 1)==0:
            query = {
                'serialNumber' : i_json['name'],
                'chipId'   : i_json['chipId']
            }

            this_chip = self.localdb.component.find_one(query)

            if this_chip == None:
                try:
                    query = {
                        'serialNumber' : self._get_chip_serial_number( i_json['name'] )
                    }
                    this_chip = self.localdb.component.find_one(query)
                except:
                    pass

            self.logger.info( f'RegisterData._check_chip(): \tFound chip in localdb: ObjectId = {this_chip["_id"]}')

            if this_chip: oid = str(this_chip['_id'])
            elif i_register: oid = self.__register_chip(i_json)
        return oid


    def _check_test_run(self, i_tr_oid='', i_conn={}, i_timestamp=None):
        """
        This function checks test run data
        """
        def __run_exist(s, i_run):
            s['_id'].append(str(i_run['_id']))
            s['passed'].append(i_run.get('passed',False))
        self.logger.info(f'RegisterData.{get_function_name()}: \tCheck TestRun: i_tr_oid = {i_tr_oid}, i_timestamp = {i_timestamp}')
        status = {
            '_id'   : [],
            'passed': []
        }
        if not i_tr_oid=='' or i_tr_oid ==None:
            query = {
                '_id'      : ObjectId(i_tr_oid),
                'dbVersion': self.db_version
            }
            this_run = self.localdb.testRun.find_one(query)
            if this_run: __run_exist(status, this_run)
        elif i_timestamp:
            query = {
                'address'  : self.site_oid,
                'user_id'  : self.user_oid,
                'startTime': datetime.utcfromtimestamp(i_timestamp),
                'dbVersion': self.db_version
            }
            run_entries = self.localdb.testRun.find(query).sort([('$natural', -1)])
            for this_run in run_entries:
                try:
                    if not i_conn=={}:
                        chip_ids = []
                        for chip_json in i_conn['chips']:
                            if chip_json.get('enable', 1)==0: continue
                            chip_ids.append({ 'chip': chip_json['chip'] })
                        query = {
                            'testRun'  : str(this_run['_id']),
                            'dbVersion': self.db_version,
                            '$or'      : chip_ids
                        }
                        ctr_entries = self.localdb.componentTestRun.find(query)
                        if not ctr_entries.count()==0:
                            __run_exist(status, this_run)
                            break
                    else:
                        __run_exist(status, this_run)
                except Exception as e:
                    self.logger.warning( str(e) )


        return status


    def _check_list(self, i_value, i_name):
        """
        This function checks if the value is listed
        """
        self.logger.info(f'RegisterData.{get_function_name()}: \tCheck List:')
        self.logger.info(f'RegisterData.{get_function_name()}: \t- value: {i_value}')
        self.logger.info(f'RegisterData.{get_function_name()}: \t- list: {i_name}')
        if not i_value.lower().replace(' ','_') in self.db_list[i_name]:
            self.logger.error('Not found {0} in the {1} list in database config file.'.format(i_value, i_name))
            raise RegisterError
        return


    def _verify_user(self, i_qc=False):
        """
        This function verifies user data
        If there is not a matching data, raise ValidationError
        """
        self.logger.info(f'RegisterData.{get_function_name()}: \tVerify User')
        self.logger.info(f'RegisterData.{get_function_name()}: Loading user information ...')
        self.logger.info(f'RegisterData.{get_function_name()}: ~~~ {{')
        self.logger.info(f'RegisterData.{get_function_name()}: ~~~     "name": "\033[1;33m{self.user_json["userName"]}\033[0m",')
        self.logger.info(f'RegisterData.{get_function_name()}: ~~~     "institution": "\033[1;33m{self.user_json["institution"]}\033[0m"')
        self.logger.info(f'RegisterData.{get_function_name()}: ~~~ }}')


    def _verify_site(self, i_qc=False):
        """
        This function verifies site data
        If there is not a matching data, raise ValidationError
        """
        self.logger.info(f'RegisterData.{get_function_name()}: \tVerify Site')
        self.logger.info(f'RegisterData.{get_function_name()}: Loading site information ...')
        self.logger.info(f'RegisterData.{get_function_name()}: ~~~ {{')
        self.logger.info(f'RegisterData.{get_function_name()}: ~~~     "institution": "\033[1;33m{self.site_json["institution"]}\033[0m"')
        self.logger.info(f'RegisterData.{get_function_name()}: ~~~ }}')


    def __register_user(self):
        """
        This function registeres user data
        All the information in self.user_json is registered
        """
        self.logger.info(f'RegisterData.{get_function_name()}: \t\tRegister User')
        doc = self.user_json
        doc.update({
            'sys'      : {},
            'userType' : 'readWrite',
            'dbVersion': self.db_version
        })
        oid = str(self.localdb.user.insert_one(doc).inserted_id)
        self._update_sys(oid, 'user')
        self.logger.info(f'RegisterData.{get_function_name()}: \t\tdoc   : {oid}')
        return oid


    def __register_site(self):
        """
        All the information in self.site_json is registered.
        """
        self.logger.info(f'RegisterData.{get_function_name()}: \t\tRegister Site')
        doc = self.site_json
        doc.update({
            'sys': {},
            'dbVersion': self.db_version
        })
        oid = str(self.localdb.institution.insert_one(doc).inserted_id)
        self._update_sys(oid, 'institution')
        self.logger.info(f'RegisterData.{get_function_name()}: \t\tdoc   : {oid}')
        return oid


    def __register_chip(self, i_json):
        """
        chip data written in i_json is registered.
        """
        self.logger.warning('\t\tRegister Chip')
        doc = {
            'sys'          : {},
            'name'         : i_json.get('name','...'),
            'chipId'       : i_json.get('chipId',0),
            'chipType'     : self.chip_type,
            'componentType': 'front-end_chip',
            'dbVersion'    : self.db_version
        }
        oid = str(self.localdb.component.insert_one(doc).inserted_id)
        self.logger.warning( f'\tregistered a custom FE chip as a component in LocalDB: {doc}' )
        self._update_sys(oid, 'chip')
        self.logger.info(f'RegisterData.{get_function_name()}: \t\tdoc   : {oid}')
        return oid

class ScanData(RegisterData):

    def __init__(self):
        super().__init__()

    ##########
    # public #
    ##########

    def setTestRun(self, i_log):
        self.logger.info(f'RegisterData.{get_function_name()}: Set TestRun')
        # user
        self.user_oid = self._check_user()

        # site
        self.site_oid = self._check_site()

        # connectivity
        self.__check_conn()

        # testRun and componentTestRun
        self.histo_names = []
        for i, conn in enumerate(self.conns):
            status = self._check_test_run('', conn, i_log['startTime'])
            status_id = None
            if not status['_id']==[]: status_id = status['_id'][0]
            if not status['passed']==[] and status['passed'][0]:
                self.logger.warning('Already registered test run data in DB')
                tr_oid = status_id
            else:
                tr_oid = self.__register_test_run(i_log, conn.get('stage', '...'), status_id)
            conn['testRun'] = tr_oid
            if not conn['module'].get('component','...')=='...':
                ctr_oid = self.__check_component_test_run(conn['module'], tr_oid)

            for chip_json in conn['chips']:
                if 'name' in chip_json:
                    chip_json['name'] = self._get_chip_serial_number( chip_json['name'] )
                ctr_oid = self.__check_component_test_run(chip_json, tr_oid)
            self.conns[i] = conn
            query = { '_id': ObjectId(tr_oid) }
            this_run = self.localdb.testRun.find_one(query)
            if this_run and not this_run.get('plots',[])==[]: self.histo_names = this_run['plots']
        return self.conns


    def completeTestRun(self, i_scanlog_json, i_conns):
        self.logger.info(f'RegisterData.{get_function_name()}: Set Test Run (finish)')
        tr_oids = []
        for conn in i_conns:
            tr_oid = conn['testRun']
            query = {
                '_id'       : ObjectId(tr_oid),
                'dbVersion' : self.db_version
            }
            this_run = self.localdb.testRun.find_one(query)
            doc = {}
            histo_names = list(set(self.histo_names))
            if not list(set(this_run['plots']))==histo_names:
                doc.update({ 'plots': histo_names })
            if not this_run['passed']==True:
                doc.update({ 'passed': True })
            if not doc=={}:
                self.localdb.testRun.update_one(
                    { '_id': ObjectId(tr_oid) },
                    { '$set': doc }
                )
                self._update_sys(tr_oid, 'testRun')
            tr_oids.append(tr_oid)
        self.tr_oids = tr_oids


    def setConfig(self, i_config_json, i_filename, i_title, i_col, i_chip_json, i_conn):
        self.logger.info(f'RegisterData.{get_function_name()}: Set Config Json')
        if i_config_json=={}: return
        oid = self.__check_config(i_title, i_col, i_conn['testRun'], i_chip_json.get('chip','...'))
        if oid: self.__register_config(i_config_json, i_filename, i_title, i_col, oid)

        return


    def setAttachment(self,i_file_path, i_histo_name, i_chip_json, i_conn):
        def is_dat(b):
            try:
                this = bool('Histo' in b.decode('utf-8').split('\n')[0][0:7])
            except:
                this = False
            return this
        def is_json(b):
            try:
                json_data = json.loads(b.decode('utf-8'))
                if 'Histo' in json_data.get('Type',''):
                    return True
                else:
                    return False
            except:
                return False
        if not os.path.isfile(i_file_path):
            self.logger.warning( f'{i_file_path} was not identified as a file' )
            return

        with open(i_file_path, 'rb') as f:
            binary_data = f.read()
            if is_dat(binary_data):
                data_type = 'dat'
            elif is_json(binary_data):
                data_type = 'json'
            else:
                return
        self.histo_names.append(i_histo_name)
        oid = self.__check_attachment(i_histo_name, i_conn['testRun'], i_chip_json.get('chip','...'), data_type)
        if oid: self.__register_attachment(i_file_path, i_histo_name, oid, data_type)

        return


    def verifyCfg(self, i_qc):
        self._verify_user(i_qc)
        self._verify_site(i_qc)
        self.__verify_conn_cfg()


    def __verify_conn_cfg(self):
        """
        This function verifies component data
        If there is not a matching data, raise ValidationError
        """
        self.logger.info(f'RegisterData.{get_function_name()}: \tVerify Component')
        if not self.conns==[]:
            self.logger.info(f'RegisterData.{get_function_name()}: Loading component information ...')
        if True:
            conns = self.conns
            self.conns = []
            for conn in conns:
                # module
                if conn['module']=={}:
                    self.logger.error('Found an empty field in connectivity config file.')
                    self.logger.error('Please set "module.serialNumber"')
                    raise ValidationError
                mo_oid = self._check_component(conn['module'], True)
                if mo_oid=='...':
                    self.logger.error('Not found component data {{ "serialNumber": "{0}", "componentType": "{1}" }} registered in Local DB.'.format(conn['module']['serialNumber'], conn['module']['componentType']))
                    self.logger.error('Please set the serial number of the QC parent component correctly in ')
                    self.logger.error('{{ "module": {{ "serialNumber": "xxx" }} }} in connectivity file.')
                    raise ValidationError
                conn['module']['component'] = mo_oid
                # chips
                chips_json = conn['chips']
                conn['chips'] = []
                for i, chip_json in enumerate(chips_json):
                    if not 'serialNumber' in chip_json and not 'name' in chip_json:
                        self.logger.error('Found an empty field in connectivity config file.')
                        self.logger.error('Please set "chip.{}.serialNumber"'.format(i))
                        raise ValidationError
                    if not 'serialNumber' in chip_json: chip_json['serialNumber'] = chip_json['name']
                    chip_json['name'] = chip_json['serialNumber']
                    ch_oid = self._check_component(chip_json, True)
                    if ch_oid=='...':
                        self.logger.error('Not found component data {{ "serialNumber": "{0}", "componentType": "{1}" }} registered in Local DB.'.format(chip_json['serialNumber'], chip_json['componentType']))
                        self.logger.error('Please set the serial number of the QC child component correctly in ')
                        self.logger.error('{{ "serialNumber": "xxx" }} in connectivity file.')
                        raise ValidationError
                    chip_json['component'] = ch_oid
                    cpr_oid = self._check_child_parent_relation(mo_oid, ch_oid)
                    if not cpr_oid:
                        self.logger.error('Not found childParentRelation data for {{ "module": "{0}", "FE chip": "{1}" }} registered in Local DB.'.format(conn['module']['serialNumber'], self._get_chip_serial_number(chip_json['serialNumber'])))
                        self.logger.error('Please check the parent and children are set in the correct relationship.')
                        raise ValidationError
                    chip_json['cpr'] = cpr_oid
                    conn['chips'].append(chip_json)
                # stage -- fetch the stage from LocalDB
                stage = self.__check_stage(mo_oid)
                conn['stage'] = stage
                self.conns.append(conn)
        for conn in self.conns:
            self.logger.info(f'RegisterData.{get_function_name()}: ~~~ {{')
            if not conn['module']=={}:
                self.logger.info(f'RegisterData.{get_function_name()}: ~~~     "parent": {{')
                self.logger.info(f'RegisterData.{get_function_name()}: ~~~         "serialNumber": "\033[1;33m{{}}\033[0m",'.format(conn['module']['serialNumber']))
                self.logger.info(f'RegisterData.{get_function_name()}: ~~~         "componentType": "\033[1;33m{{}}\033[0m"'.format(conn['module']['componentType']))
                self.logger.info(f'RegisterData.{get_function_name()}: ~~~     }},')
            self.logger.info(f'RegisterData.{get_function_name()}: ~~~     "children": [{{')
            for i, chip in enumerate(conn['chips']):
                if not i==0:
                    self.logger.info(f'RegisterData.{get_function_name()}: ~~~     }},{{')
                self.logger.info(f'RegisterData.{get_function_name()}: ~~~         "serialNumber": "\033[1;33m{0}\033[0m",\033[1;33m{1}\033[0m'.format(chip['serialNumber'], ' (disabled)' if chip.get('enable',1)==0 else ''))
                self.logger.info(f'RegisterData.{get_function_name()}: ~~~         "componentType": "\033[1;33m{{}}\033[0m",'.format(chip['componentType']))
                self.logger.info(f'RegisterData.{get_function_name()}: ~~~         "chipId": "\033[1;33m{{}}\033[0m",'.format(chip['chipId']))
            self.logger.info(f'RegisterData.{get_function_name()}: ~~~     }}],')
            self.logger.info(f'RegisterData.{get_function_name()}: ~~~     "stage": "\033[1;33m{{}}\033[0m"'.format(conn.get('stage','...')))
            self.logger.info(f'RegisterData.{get_function_name()}: ~~~ }}')


    def __check_conn(self):
        """
        This function checks connectivity data
        """
        self.logger.info(f'RegisterData.__check_conn(): \tCheck Conn')
        conns = self.conns
        self.logger.info(f'RegisterData.__check_conn(): input conns = ' + pprint.pformat( conns ) )

        self.conns = []
        for conn in conns:
            # module
            if not conn['module']=={}:
                if conn['module'].get('component','...')=='...':
                    mo_oid = self._check_component(conn['module'])
                    conn['module']['component'] = mo_oid
                if 'serialNumber'  in conn['module']: del conn['module']['serialNumber']
                if 'componentType' in conn['module']: del conn['module']['componentType']
            # chips
            chips_json = conn['chips']
            conn['chips'] = []
            for i, chip_json in enumerate(chips_json):
                if not conn['module'].get('component','...')=='...' and not chip_json.get('cpr'):
                    cpr_oid = self._check_child_parent_relation(mo_oid, ch_oid)
                    chip_json['cpr'] = cpr_oid
                if not chip_json.get('cpr'):
                    conn['module'] = {}
                chip_json['chip'] = self._check_chip(chip_json)
                chip_json['component'] = chip_json['chip']
                if 'serialNumber'  in chip_json: del chip_json['serialNumber']
                if 'componentType' in chip_json: del chip_json['componentType']
                if 'chipId'        in chip_json: del chip_json['chipId']
                if 'cpr'           in chip_json: del chip_json['cpr']
                conn['chips'].append(chip_json)
            self.conns.append(conn)

        self.logger.info(f'RegisterData.__check_conn(): output conns = ' + pprint.pformat( self.conns ) )



    def __check_stage(self, i_mo_oid):
        """
        This function checks current stage
        If there is a matching data, return stage
        If there is not a matching data, return '...'
        """
        self.logger.info(f'RegisterData.{get_function_name()}: \tCheck Stage')
        stage = '...'
        if i_mo_oid:
            query = { 'component': i_mo_oid }
            this = self.localdb.QC.module.status.find_one(query)
            if this: stage = this['currentStage']
        return stage


    def __check_component_test_run(self, i_json, i_tr_oid):
        """
        This function checks test run data
        """
        self, logger.info(f'RegisterData.{get_function_name()}: \tCheck Component-TestRun')
        oid = None
        query = {
            'chip'       : i_json.get('chip','module'),
            'component'  : i_json.get('component','...'),
            'testRun'    : i_tr_oid,
            'tx'         : i_json.get('tx',-1),
            'rx'         : i_json.get('rx',-1),
            'dbVersion'  : self.db_version
        }
        this_ctr = self.localdb.componentTestRun.find_one(query)
        if this_ctr: oid = str(this_ctr['_id'])
        else:
            i_json.update(query)
            oid = self.__register_component_test_run(i_json, i_tr_oid)
        return oid


    def __check_config(self, i_title, i_col, i_tr_oid, i_chip_oid):
        self.logger.info(f'RegisterData.{get_function_name()}: \tCheck Config Json:')
        oid = None
        if i_col=='testRun':
            query = {
                '_id'      : ObjectId(i_tr_oid),
                'dbVersion': self.db_version
            }
        elif i_col=='componentTestRun':
            query = {
                'testRun'  : i_tr_oid,
                'chip'     : i_chip_oid,
                'dbVersion': self.db_version
            }
        this = self.localdb[i_col].find_one(query)
        if this.get(i_title, '...')=='...': oid = str(this['_id'])

        return oid


    def __check_attachment(self, i_histo_name, i_tr_oid, i_chip_oid, i_type):
        self.logger.info(f'RegisterData.{get_function_name()}: \tCheck Attachment:')
        oid = None
        query = {
            'testRun'  : i_tr_oid,
            'chip'     : i_chip_oid,
            'dbVersion': self.db_version
        }
        this_ctr = self.localdb.componentTestRun.find_one(query)
        filenames = []
        for attachment in this_ctr.get('attachments', []):
            filenames.append(attachment['filename'])
        if not '{0}.{1}'.format(i_histo_name, i_type) in filenames: oid = str(this_ctr['_id'])
        return oid


    def __check_gridfs(self, i_hash_code):
        self.logger.info(f'RegisterData.{get_function_name()}: \t\t\tCheck Json File by Hash')
        oid = None
        query = {
           'hash'     : i_hash_code,
           'dbVersion': self.db_version
        }
        this_file = self.localdb.fs.files.find_one( query, { '_id': 1 } )
        if this_file:
            oid = str(this_file['_id'])
        return oid


    def __register_test_run(self, i_json, i_stage, i_tr_oid):
        """
        Almost all the information in i_json is registered.
        """
        self.logger.info(f'RegisterData.{get_function_name()}: \t\tRegister Test Run')
        doc = {}
        for key in i_json:
            if not key=='connectivity' and not 'Cfg' in key and not key=='startTime' and not key=='finishTime':
                doc[key] = i_json[key]
        doc.update({
            'testType' : i_json.get('testType', '...'),
            'runNumber': i_json.get('runNumber', -1),
            'stage'    : i_stage,
            'chipType' : self.chip_type,
            'address'  : self.site_oid,
            'user_id'  : self.user_oid,
            'dbVersion': self.db_version
        })
        if not i_tr_oid:
            doc.update({
                'sys'        : {},
                'environment': False,
                'plots'      : [],
                'passed'     : False,
                'startTime'  : datetime.utcfromtimestamp(i_json['startTime']),
                'finishTime' : datetime.utcfromtimestamp(i_json['finishTime']),
            })
            oid = str(self.localdb.testRun.insert_one(doc).inserted_id)
        else:
            oid = i_tr_oid
            query = { '_id': ObjectId(i_tr_oid) }
            self.localdb.testRun.update_one( query, {'$set': doc} )
        self._update_sys(oid, 'testRun')
        self.logger.info(f'RegisterData.{get_function_name()}: \t\tdoc   : {{}}'.format(oid))
        return oid


    def __register_component_test_run(self, i_json, i_tr_oid):
        """
        Almost all the information in i_json is registered.
        """
        self.logger.info(f'RegisterData.{get_function_name()}: \t\tRegister Component-TestRun')
        doc = i_json
        doc.update({
            'sys'        : {},
            'attachments': [],
            'environment': '...',
            'dbVersion'  : self.db_version
        })
        oid = str(self.localdb.componentTestRun.insert_one(doc).inserted_id)
        self._update_sys(oid, 'componentTestRun')
        self.logger.info(f'RegisterData.{get_function_name()}: \t\tdoc   : {{}}'.format(oid))
        return oid


    def __register_config(self, i_file_json, i_filename, i_title, i_col, i_oid):
        self.logger.info(f'RegisterData.{get_function_name()}: \t\tRegister Config Json')
        hash_code = self._get_hash(i_file_json)
        data_id = self.__check_gridfs(hash_code)
        if not data_id: data_id = self.__register_grid_fs_file(i_file_json, '', i_filename+'.json', hash_code)
        doc = {
            'sys'      : {},
            'filename' : i_filename+'.json',
            'chipType' : self.chip_type,
            'title'    : i_title,
            'format'   : 'fs.files',
            'data_id'  : data_id,
            'dbVersion': self.db_version
        }
        oid = str(self.localdb.config.insert_one(doc).inserted_id)
        self._update_sys(oid, 'config')
        self._add_value(i_oid, i_col, i_title, oid)
        self._update_sys(i_oid, i_col)
        self.logger.info(f'RegisterData.{get_function_name()}: \t\tdoc   : {{}}'.format(i_oid))
        self.logger.info(f'RegisterData.{get_function_name()}: \t\tconfig: {{}}'.format(oid))
        self.logger.info(f'RegisterData.{get_function_name()}: \t\tdata  : {{}}'.format(data_id))
        return oid


    def __register_attachment(self, i_file_path, i_histo_name, i_oid, i_type):
        self.logger.info(f'RegisterData.{get_function_name()}: \t\tRegister Attachment')
        if i_type=='json':
            with open(i_file_path, 'rb') as f:
                binary_data = f.read()
                json_data = json.loads(binary_data.decode('utf-8'))
            hash_code = self._get_hash(json_data)
            oid = self.__check_gridfs(hash_code)
            if not oid: oid = self.__register_grid_fs_file(json_data, '', '{0}.{1}'.format(i_histo_name, i_type), hash_code)
        else:
            hash_code = self._get_hash(i_file_path)
            oid = self.__check_gridfs(hash_code)
            if not oid: oid = self.__register_grid_fs_file({}, i_file_path, '{0}.{1}'.format(i_histo_name, i_type))
        self.localdb.componentTestRun.update_one(
            { '_id': ObjectId(i_oid) },
            { '$push': {
                'attachments': {
                    'code'       : oid,
                    'dateTime'   : datetime.utcnow(),
                    'title'      : i_histo_name,
                    'description': 'describe',
                    'contentType': i_type,
                    'filename'   : '{0}.{1}'.format(i_histo_name, i_type)
                }
            }}
        )
        self._update_sys(i_oid, 'componentTestRun')
        self.logger.info(f'RegisterData.{get_function_name()}: \t\tdoc   : {{}}'.format(i_oid))
        self.logger.info(f'RegisterData.{get_function_name()}: \t\tdata  : {{}}'.format(oid))
        return oid


    def __register_grid_fs_file(self, i_file_json, i_file_path, i_filename, i_hash=''):
        self.logger.info(f'RegisterData.{get_function_name()}: \t\t\tWrite File by GridFS')
        if not i_file_path=='':
            with open(i_file_path, 'rb') as f:
                binary = f.read()
        else:
            binary = json.dumps(i_file_json, indent=4).encode('utf-8')
        if i_hash=='':
            oid = str(self.localfs.put( binary, filename=i_filename, dbVersion=self.db_version ))
        else:
            oid = str(self.localfs.put( binary, filename=i_filename, hash=i_hash, dbVersion=self.db_version ))
        self._update_sys(oid, 'fs.files')
        return oid

class DcsData(RegisterData):

    def __init__(self):
        super().__init__()
        self.ctr_oids = []


    def verifyCfg(self, i_log):
        self.__verify_dcs_log_format(i_log)
        self.__verify_test_data(i_log)


    def __verify_dcs_log_format(self, i_log):
        """
        This function verifies DCS log file
        If the format is unreadable, raise RegisterError
        """
        self.logger.info(f'RegisterData.{get_function_name()}: \tVerify DCS Log')
        for i, env_j in enumerate(i_log['environments']):
            filename = 'environments.{} in DCS log file'.format(i)
            self._check_empty(env_j, 'status', filename)
            if env_j['status']!='enabled': continue
            self._check_empty(env_j, 'key', filename)
            self._check_list(env_j['key'], 'environment')
            self._check_empty(env_j, 'description', filename)
            self._check_empty(env_j, 'num', filename)
            self._check_number(env_j, 'num', filename)
            self._check_empty(env_j, ['path','value'], filename)
            if 'path' in env_j:
                try:
                    self.__read_dcs_data(env_j['path'], env_j['key'], env_j['num'])
                except:
                    raise RegisterError
                if 'margin' in env_j:
                    self._check_number(env_j, 'margin', filename)


    def __verify_test_data(self, i_log):
        """
        This function verifies scan data
        If the data is not registered, raise ValidationError
        """
        self.user_oid = self._check_user(False)
        self.site_oid = self._check_site(False)
        self.__check_conn()
        tr_oids = []
        if self.conns==[]: self.conns=[{}]
        for conn in self.conns:
            status = self._check_test_run(i_log.get('id',''), conn, i_log['timestamp'])
            for i, tr_oid in enumerate(status['_id']):
                if status['passed'][i]: tr_oids.append(tr_oid)
        if tr_oids==[]:
            self.logger.error('Not found relational test run data in DB')
            self.logger.error('The scan data may not have been uploaded.')
            self.logger.error('Please make sure it is uploaded and try to upload DCS data again.')
            raise ValidationError
        self.dcs_tr_oids = tr_oids


    def verifyDcsData(self, i_env):
        """
        This function verifies DCS data associated to scan data
        If the data is already registered, return warning
        """
        ctr_oids = []
        registered_oids = []
        chips = []
        registered_chips = []
        if i_env['status']=='enabled':
            env_key = i_env['key'].lower().replace(' ','_')
            for tr_oid in self.dcs_tr_oids:
                query = {
                    'testRun'  : tr_oid,
                    'dbVersion': self.db_version
                }
                if i_env.get('chip', None): query.update({ 'name': i_env['chip'] })
                ctr_entries = self.localdb.componentTestRun.find(query)
                for this_ctr in ctr_entries:
                    ctr_oid = self._check_dcs(str(this_ctr['_id']), env_key, i_env['num'], i_env['description'])
                    if ctr_oid:
                        registered_oids.append(str(this_ctr['_id']))
                        registered_chips.append('"\033[1;33m{}\033[0m"'.format(this_ctr['name']))
                    else:
                        ctr_oids.append(str(this_ctr['_id']))
                        chips.append('\033[1;33m"{}"\033[0m'.format(this_ctr['name']))
        i_env.update({ 'registered_oids': registered_oids, 'ctr_oids': ctr_oids, 'chips': chips, 'registered_chips': registered_chips })
        return i_env


    def confirmDcsData(self, i_env):
        """
        This function display DCS configuration
        """
        self.logger.info(f'RegisterData.{get_function_name()}: ~~~ {{')
        self.logger.info(f'RegisterData.{get_function_name()}: ~~~     "key": "\033[1;33m{{}}\033[0m",'.format(i_env['key']))
        self.logger.info(f'RegisterData.{get_function_name()}: ~~~     "description": "\033[1;33m{{}}\033[0m",'.format(i_env['description']))
        self.logger.info(f'RegisterData.{get_function_name()}: ~~~     "num": "\033[1;33m{{}}\033[0m",'.format(i_env['num']))
        if 'path' in i_env:
            self.logger.info(f'RegisterData.{get_function_name()}: ~~~     "path": "\033[1;33m{{}}\033[0m",'.format(i_env['path']))
            if 'margin' in i_env: self.logger.info(f'RegisterData.{get_function_name()}: ~~~     "margin": "\033[1;33m{{}}\033[0m",'.format(i_env['margin']))
        elif 'value' in i_env: self.logger.info(f'RegisterData.{get_function_name()}: ~~~     "value": "\033[1;33m{{}}\033[0m",'.format(i_env['value']))
        if not i_env.get('chips',[])==[]: self.logger.info(f'RegisterData.{get_function_name()}: ~~~     "chips": {{}}'.format(', '.join(i_env['chips'])))
        if not i_env.get('registered_chips',[])==[]: self.logger.info(f'RegisterData.{get_function_name()}: ~~~     "\033[1;31mchips with registered DCS data\033[0m": {{}}'.format(', '.join(i_env['registered_chips'])))
        self.logger.info(f'RegisterData.{get_function_name()}: ~~~ }}')


    def setDcs(self):
        self.logger.info(f'RegisterData.{get_function_name()}: \t\tSet DCS')
        environments = self.environments
        for env_j in environments:
            env_key = env_j['key'].lower().replace(' ','_')
            ctr_oids = env_j.get('registered_oids',[])
            for ctr_oid in ctr_oids:
                self.ctr_oids.append({ 'ctr_oid': ctr_oid, 'key': env_key, 'num': env_j['num'], 'description': env_j['description'] })
            ctr_oids = env_j.get('ctr_oids',[])
            if 'ctr_oids' in env_j:         del env_j['ctr_oids']
            if 'registered_oids' in env_j:  del env_j['registered_oids']
            if 'chips' in env_j:            del env_j['chips']
            if 'registered_chips' in env_j: del env_j['registered_chips']
            for ctr_oid in ctr_oids:
                self.__register_dcs(ctr_oid, env_key, env_j)
                self.ctr_oids.append({ 'ctr_oid': ctr_oid, 'key': env_key, 'num': env_j['num'], 'description': env_j['description'] })


    def __check_conn(self):
        """
        This function checks connectivity data
        """
        self.logger.info(f'RegisterData.{get_function_name()}: \tCheck Conn')
        conns = self.conns
        self.conns = []
        for conn in conns:
            # chips
            chips_json = conn['chips']
            conn['chips'] = []
            for i, chip_json in enumerate(chips_json):
                chip_oid = self._check_chip(chip_json, False)
                chip_json['chip'] = chip_oid
                conn['chips'].append(chip_json)
            self.conns.append(conn)


    def _check_dcs(self, i_ctr_oid, i_key, i_num, i_description):
        self.logger.info(f'RegisterData.{get_function_name()}: \tCheck DCS')
        ctr_oid = None
        query = {
            '_id'      : ObjectId(i_ctr_oid),
            'dbVersion': self.db_version
        }
        this_ctr = self.localdb.componentTestRun.find_one(query)
        if not this_ctr.get('environment', '...')=='...':
            query = {
                '_id'      : ObjectId(this_ctr['environment']),
                'dbVersion': self.db_version
            }
            this_dcs = self.localdb.environment.find_one(query)
            for this_data in this_dcs.get(i_key,[]):
                if str(this_data['num'])==str(i_num) and this_data['description']==i_description:
                    ctr_oid = i_ctr_oid
                    break
        return ctr_oid


    def __register_dcs(self, i_ctr_oid, i_env_key, i_env_j):
        self.logger.info(f'RegisterData.{get_function_name()}: \t\tRegister DCS')
        query = {
            '_id'       : ObjectId(i_ctr_oid),
            'dbVersion' : self.db_version
        }
        this_ctr = self.localdb.componentTestRun.find_one(query)
        tr_oid = this_ctr['testRun']
        query = {
            '_id'       : ObjectId(tr_oid),
            'dbVersion' : self.db_version
        }
        this_tr = self.localdb.testRun.find_one(query)
        array = []
        if i_env_j.get('path','null')!='null':
            starttime  = None
            finishtime = None
            if 'margin' in i_env_j:
                starttime  = this_tr['startTime'].timestamp()  - i_env_j['margin']
                finishtime = this_tr['finishTime'].timestamp() + i_env_j['margin']
            try:
                array = self.__read_dcs_data(i_env_j['path'], i_env_key, i_env_j['num'], starttime, finishtime)
            except DcsDataError:
                return
        else:
            array.append({
                'date' : this_tr['startTime'],
                'value': i_env_j['value']
            })
        i_env_j.update({ 'data': array })
        if this_ctr.get('environment', '...')=='...':
            doc_value = {
                'sys'      : {},
                i_env_key  : [ i_env_j ],
                'dbVersion': self.db_version
            }
            oid = str(self.localdb.environment.insert_one(doc_value).inserted_id)
            self._add_value(i_ctr_oid, 'componentTestRun', 'environment', oid)
            self._update_sys(i_ctr_oid, 'componentTestRun');
        else:
            oid = this_ctr['environment']
            doc_value = { '$push': { i_env_key: i_env_j } }
            query = { '_id': ObjectId(oid) }
            self.localdb.environment.update_one( query, doc_value )

        self._update_sys(oid, 'environment');
        self._add_value(tr_oid, 'testRun', 'environment', True)
        self._update_sys(tr_oid, 'testRun')
        self.tr_oids.append(tr_oid)


    def __read_dcs_data(self, i_path, i_key, i_num, i_start=None, i_finish=None):
        self.logger.info(f'RegisterData.{get_function_name()}: \t\tRead DCS data')
        env_key = i_key.lower().replace(' ','_')
        extension = i_path.split('.')[len(i_path.split('.'))-1]
        if extension=='dat':
            separator = ' '
        elif extension=='csv':
            separator = ','
        else:
            self.logger.error('No supported DCS data.')
            self.logger.error('\tfile: {0}  extension: {1}'.format(i_path, extension))
            self.logger.error('\tSet to "dat" or "csv".')
            raise DcsDataError
        if os.path.isfile(i_path):
            data = open(i_path, 'r')
        else:
            self.logger.error('Not found DCS data file.')
            self.logger.error('\tfile: {0}'.format(i_path))
            raise DcsDataError

        # key and num
        key_lines = data.readline().splitlines()
        if key_lines==[]:
            self.logger.error('Not found DCS keys in the 1st line.')
            self.logger.error('\tfile: {0}'.format(i_path))
            raise DcsDataError

        num_lines = data.readline().splitlines()
        if num_lines==[]:
            self.logger.error('Not found DCS nums in the 2nd line.')
            self.logger.error('\tfile: {0}'.format(i_path))
            raise DcsDataError

        setting_lines = data.readline().splitlines()
        if setting_lines==[]:
            self.logger.error('Not found DCS nums in the 3nd line.')
            self.logger.error('\tfile: {0}'.format(i_path))
            raise DcsDataError

        key = -1
        for j, tmp_key in enumerate(key_lines[0].split(separator)):
            tmp_key = tmp_key.lower().replace(' ','_')
            tmp_num = num_lines[0].split(separator)[j]
            if str(env_key)==str(tmp_key) and str(i_num)==str(tmp_num):
                key = j
                break
        if key==-1:
            self.logger.error('Not found specified DCS data.')
            self.logger.error('\tfile: {0}'.format(i_path))
            self.logger.error('\tkey: {0}  num: {1}'.format(env_key, i_num))
            self.logger.error('Please check the key and num given in the DCS config file are set in the DCS data file.')
            raise DcsDataError

        # value
        env_lines = data.readline().splitlines()
        if env_lines==[]:
            self.logger.error('Not found DCS values from the 4rd line.')
            self.logger.error('\tfile: {0}'.format(i_path))
            raise DcsDataError
        l = 3
        array = []

        while env_lines:
            if len(env_lines[0].split(separator)) < key: break
            try:
                date = int(env_lines[0].split(separator)[1])
            except:
                self.logger.error('Invalid value: Unixtime must be "int"')
                self.logger.error('\tfile: {0}  line: {1}  text: {2}'.format(i_path, l, date))
                raise DcsDataError
            value = env_lines[0].split(separator)[key]
            if not value=='null':
                try:
                    value = float(value)
                except:
                    self.logger.error('Invalid value: DCS data must be "float" or "null"')
                    self.logger.error('\tfile: {0}  line: {1}  text: {2}'.format(i_path, l, value))
                    raise DcsDataError
                if i_start and date<i_start: pass
                elif i_finish and data>i_finish: pass
                else:
                    array.append({
                        'date' : datetime.utcfromtimestamp(date),
                        'value': value
                    })
            env_lines = data.readline().splitlines()
            l = l+1
        return array

class CompData(RegisterData):

    def __init__(self):
        super().__init__()


    def verifyCfg(self):
        self._verify_user()
        self._verify_site()


    def checkConnCfg(self, i_path):
        """
        Check Component Connectivity
        If the components written in the file have not registered,
        Display component information in the console
        """
        self.logger.info(f'RegisterData.{get_function_name()}: \tCheck Connectivity config for registration:')
        conn = readJson(i_path)
        self._check_empty(conn, 'chipType', 'connectivity config')
        if conn['chipType']=='FEI4B': conn['chipType'] = 'FE-I4B'
        self.chip_type = conn['chipType']

        # module
        if 'module' in conn:
            self._check_empty(conn['module'], 'serialNumber', 'connectivity.module')
            conn['module']['componentType'] = conn['module'].get('componentType', 'module')
            conn['module']['status'] = self.__check_component(conn['module'])
            if conn['module']['status']==2:
                self.logger.error('Already registered QC component data: {0} ({1})'.format(conn['module']['serialNumber'], conn['module']['componentType']))
                self.logger.error('QC components cannot be registered by overwriting.')
                raise RegisterError
        # chip
        chips = []
        chipids = []
        for i, chip_conn in enumerate(conn['chips']):
            self._check_empty(chip_conn, 'serialNumber',  'connectivity.chips.{}'.format(i))
            self._check_empty(chip_conn, 'chipId',        'connectivity.chips.{}'.format(i))
            chip_conn['componentType'] = chip_conn.get('componentType', 'front-end_chip')
            if chip_conn['chipId'] in chipids:
                self.logger.error('Conflict chip ID: {}'.format(chip_conn['chipId']))
                raise RegisterError
            chipids.append(chip_conn['chipId'])
            chip_conn['status'] = self.__check_component(chip_conn)
            if chip_conn['status']==2:
                self.logger.error('Already registered QC component data: {0} ({1})'.format(chip_conn['serialNumber'], chip_conn['componentType']))
                self.logger.error('QC components cannot be registered by overwriting.')
                raise RegisterError
            chips.append(chip_conn)
        conn['chips'] = chips
        if 'module' in conn: conn['module']['children'] = len(chips)

        self.logger.info(f'RegisterData.{get_function_name()}: Component Data:')
        self.logger.info(f'RegisterData.{get_function_name()}:     Chip Type: \033[1;33m{{}}\033[0m'.format(conn['chipType']))
        if 'module' in conn:
            self.logger.info(f'RegisterData.{get_function_name()}:     Module:')
            self.logger.info(f'RegisterData.{get_function_name()}:         serial number: \033[1;33m{0}\033[0m {1}'.format(conn['module']['serialNumber'], '\033[1;31m(data already registered in Local DB)\033[0m' if conn['module']['status']==1 else ''))
            self.logger.info(f'RegisterData.{get_function_name()}:         component type: \033[1;33m{{}}\033[0m'.format(conn['module']['componentType']))
            self.logger.info(f'RegisterData.{get_function_name()}:         # of chips: \033[1;33m{{}}\033[0m'.format(conn['module']['children']))
        for i, chip in enumerate(conn['chips']):
            self.logger.info(f'RegisterData.{get_function_name()}:     Chip ({{}}):'.format(i+1))
            self.logger.info(f'RegisterData.{get_function_name()}:         serial number: \033[1;33m{0}\033[0m {1}'.format(chip['serialNumber'], '\033[1;31m(data already registered in Local DB)\033[0m' if chip['status']==1 else ''))
            self.logger.info(f'RegisterData.{get_function_name()}:         component type: \033[1;33m{{}}\033[0m'.format(chip['componentType']))
            self.logger.info(f'RegisterData.{get_function_name()}:         chip ID: \033[1;33m{{}}\033[0m'.format(chip['chipId']))

        self.logger.warning('It will be override with the provided infomation if data already exists in Local DB.')
        self.conn = conn


    def setComponent(self):
        """
        This function registers component information from cnnectivity file
        """
        self.logger.info(f'RegisterData.{get_function_name()}: \t\tRegister from Connectivity')

        # user
        self.user_oid = self._check_user()

        # site
        self.site_oid = self._check_site()

        # components
        conn = self.conn
        # module
        if 'module' in conn:
            mo_oid = self._check_component(conn['module'])
            mo_oid = self.__register_component(conn['module'], mo_oid)
        # chips
        for i, chip_conn in enumerate(conn['chips']):
            ch_oid = self._check_component(chip_conn)
            ch_oid = self.__register_component(chip_conn, ch_oid)
            if 'module' in conn:
                chip_id = chip_conn['chipId']
                cpr_oid = self._check_child_parent_relation(mo_oid, ch_oid)
                self.__register_child_parent_relation(mo_oid, ch_oid, chip_id, -1, cpr_oid)

        self.logger.info(f'RegisterData.{get_function_name()}: Succeeded uploading component data')
        return True


    def __check_component(self, i_json):
        """
        This function checks component data
        """
        self.logger.info(f'RegisterData.{get_function_name()}: \tCheck Component')
        result = 0
        if not i_json=={} and 'serialNumber' in i_json and 'componentType' in i_json:
            query = {
                'serialNumber' : i_json['serialNumber'],
                'componentType': i_json['componentType'].lower().replace(' ','_'),
                'chipType'     : self.chip_type,
                'dbVersion'    : self.db_version
            }
            this_cmp = self.localdb.component.find_one(query)
            if not this_cmp: result = 0
            elif this_cmp.get('proDB', False): result = 2
            else: result = 1

        return result


    def __register_component(self, i_json, i_oid):
        """
        This function registers Component
        Almost all the information in i_json is registered.
        """
        self.logger.info(f'RegisterData.{get_function_name()}: \t\tRegister Component')
        doc = {
            'serialNumber' : i_json['serialNumber'],
            'componentType': i_json['componentType'].lower().replace(' ','_'),
            'chipType'     : self.chip_type,
            'name'         : i_json['serialNumber'],
            'chipId'       : i_json.get('chipId',-1),
            'children'     : i_json.get('children',-1),
            'proDB'        : False,
            'dbVersion'    : self.db_version
        }
        if i_oid=='...':
            doc.update({
                'sys'    : {},
                'address': self.site_oid,
                'user_id': self.user_oid
            })
            oid = str(self.localdb.component.insert_one(doc).inserted_id)
        else:
            oid = i_oid
            query = { '_id': ObjectId(oid) }
            self.localdb.component.update_one( query, {'$set': doc} )
        self._update_sys(oid, 'component')
        logger.info(f'RegisterData.{get_function_name()}: \t\tdoc   : {{}}'.format(oid))
        return oid


    def __register_child_parent_relation(self, i_parent_oid, i_child_oid, i_chip_id, i_geom_id, i_oid, i_active=True):
        """
        This function registeres ChildParentRelation
        """
        self.logger.info(f'RegisterData.{get_function_name()}: \t\tRegister Child Parent Relation.')
        doc = {
            'parent'   : i_parent_oid,
            'child'    : i_child_oid,
            'chipId'   : i_chip_id,
            'geomId'   : i_geom_id,
            'status'   : 'active' if i_active else 'dead',
            'dbVersion': self.db_version
        }
        if not i_oid:
            doc.update({ 'sys': {} })
            oid = str(self.localdb.childParentRelation.insert_one(doc).inserted_id)
        else:
            oid = i_oid
            query = { '_id': ObjectId(oid) }
            self.localdb.childParentRelation.update_one( query, {'$set': doc} )
        self._update_sys(oid, 'childParentRelation')
        self.logger.info(f'RegisterData.{get_function_name()}: \t\tdoc   : {{}}'.format(oid))
        return oid
