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

# Log
from logging import getLogger
logger = getLogger('Log').getChild('Register')
from common import readJson

home = os.environ['HOME']
if not 'HOSTNAME' in os.environ:
    hostname = 'default_host'
else:
    hostname = os.environ['HOSTNAME']

class RegisterError(Exception):
    pass

class ValidationError(Exception):
    pass

class RegisterData():
    def __init__(self):
        self.logger = getLogger('Log').getChild('Register')
        self.logger.debug('Initialize register function')
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
        self.qcTest=False

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
        self.logger.debug('Set User')
        self.user_json.update(i_json)
    def setSite(self, i_json):
        self.logger.debug('Set Site')
        self.site_json.update(i_json)
    def setConnCfg(self, i_conn, i_cache_dir=''):
        self.logger.debug('Set Connectivity Config')
        if i_conn=={}:
            return i_conn
        # chip type
        self._check_empty(i_conn, 'chipType', 'connectivity config')
        self.chip_type = i_conn['chipType']
        if self.chip_type=='FEI4B': self.chip_type = 'FE-I4B'

        conn = { 'module': {}, 'chips': [] }
        if 'stage' in i_conn: conn['stage'] = i_conn['stage']

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
                    chip_json['chipId'] = 0
            chip_json['serialNumber'] = chip_json['name']
            chip_json['componentType'] = chip_json.get('componentType', 'front-end_chip')
            chip_json['geomId'] = chip_json.get('geomId', i)
            conn['chips'].append(chip_json)
        self.conns.append(conn)
        return conn

    def _update_sys(self, i_oid, i_col):
        if i_oid in self.updated.get(i_col, []): return
        self.logger.debug('\t\t\tUpdate system information: {0} in {1}'.format(i_oid, i_col))
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
        self.logger.debug('\t\t\tAdd document: {0} to {1}'.format(i_key, i_col))
        if i_type=='string': value = str(i_value)
        elif i_type=='bool': value = i_value.lower()=='true'
        elif i_type=='int':  value = int(i_value)
        query = { '_id': ObjectId(i_oid) }
        doc_value = { '$set': { i_key: i_value }}
        self.localdb[i_col].update_one( query, doc_value )

    def _get_hash(self, i_file_data, i_type='json'):
        self.logger.debug('\t\t\tGet Hash Code from File')
        if i_type=='json':
            shaHashed = hashlib.sha256(json.dumps(i_file_data, indent=4).encode('utf-8')).hexdigest()
        elif i_type=='dat':
            with open(i_file_data, 'rb') as f:
                binary = f.read()
            shaHashed = hashlib.sha256(binary).hexdigest()
        return shaHashed

    def _check_empty(self, i_json, i_key, i_filename):
        self.logger.debug('\tCheck Empty:')
        self.logger.debug('\t- key: {}'.format(i_key))
        self.logger.debug('\t- file: {}'.format(i_filename))
        if not i_key in i_json:
            self.logger.error('Found an empty field in json file.')
            self.logger.error('\tfile: {0}  key: {1}'.format(i_filename, i_key))
            raise RegisterError
        return

    def _check_user(self):
        """
        This function checks user data
        If there is a matching data, return oid
        If there is not a matching data, register user_json and return oid
        """
        self.logger.debug('\tCheck User')
        oid = None
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
        else: oid = self._register_user()
        return oid

    def _check_site(self):
        """
        This function checks site data
        If there is a matching data, return oid
        If there is not a matching data, register site_json and return oid
        """
        self.logger.debug('\tCheck Site')
        oid = None
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
        else: oid = self._register_site()
        return oid

    def _check_component(self, i_json, i_qc=False):
        """
        This function checks component data
        If there is a matching data, return oid
        If there is not a matching data, return '...'
        """
        self.logger.debug('\tCheck Component')
        oid = '...'
        if not i_json=={} and 'serialNumber' in i_json and 'componentType' in i_json:
            query = {
                'serialNumber' : i_json['serialNumber'],
                'componentType': i_json['componentType'].lower().replace(' ','_'),
                'chipType'     : self.chip_type,
                'dbVersion'    : self.db_version
            }
            if i_qc: query.update({ 'proDB': True })
            this_cmp = self.localdb.component.find_one(query)
            if this_cmp:
                oid = str(this_cmp['_id'])
        return oid

    def _check_child_parent_relation(self, i_mo_oid, i_ch_oid):
        """
        This function checks childParentRelation data
        If there is a matching data, return oid
        If there is not a matching data, return None
        """
        self.logger.debug('\tCheck Child Parent Relation')
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

    def _check_test_run(self, i_tr_oid='', i_conn=None, i_timestamp=None):
        """
        This function checks test run data
        """
        def __run_exist(s, i_run):
            s['_id'].append(str(i_run['_id']))
            s['passed'].append(i_run.get('passed',False))
        self.logger.debug('\tCheck TestRun')
        status = {
            '_id'   : [],
            'passed': []
        }
        if not i_tr_oid=='':
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

        return status

    def __check_list(self, i_value, i_name):
        """
        This function checks if the value is listed
        """
        self.logger.debug('\tCheck List:')
        self.logger.debug('\t- value: {}'.format(i_value))
        self.logger.debug('\t- list: {}'.format(i_name))
        if not i_value.lower().replace(' ','_') in self.db_list[i_name]:
            self.logger.error('Not registered {0} in the {1} list in database config file.'.format(i_value, i_name))
            raise RegisterError
        return

    def _register_user(self):
        """
        This function registeres user data
        All the information in self.user_json is registered
        """
        self.logger.debug('\t\tRegister User')
        doc = self.user_json
        doc.update({
            'sys'      : {},
            'userType' : 'readWrite',
            'dbVersion': self.db_version
        })
        oid = str(self.localdb.user.insert_one(doc).inserted_id)
        self._update_sys(oid, 'user')
        self.logger.debug('\t\tdoc   : {}'.format(oid))
        return oid

    def _register_site(self):
        """
        All the information in self.site_json is registered.
        """
        self.logger.debug('\t\tRegister Site')
        doc = self.site_json
        doc.update({
            'sys': {},
            'dbVersion': self.db_version
        })
        oid = str(self.localdb.institution.insert_one(doc).inserted_id)
        self._update_sys(oid, 'institution')
        self.logger.debug('\t\tdoc   : {}'.format(oid))
        return oid


class ScanData(RegisterData):
    def __init__(self):
        super().__init__()

    ##########
    # public #
    ##########
    def setTestRun(self, i_log):
        self.logger.debug('Set TestRun')
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
                ctr_oid = self.__check_component_test_run(chip_json, tr_oid)
            self.conns[i] = conn
            query = { '_id': ObjectId(tr_oid) }
            this_run = self.localdb.testRun.find_one(query)
            if this_run and not this_run.get('plots',[])==[]: self.histo_names = this_run['plots']
        return self.conns

    def completeTestRun(self, i_scanlog_json, i_conns):
        self.logger.debug('Set Test Run (finish)')
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
        self.logger.debug('Set Config Json')
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
        self.logger.debug('Set Attachment')
        if not os.path.isfile(i_file_path): return
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
        self._verify_conn_cfg(i_qc)
        self.qcTest=i_qc

    def _verify_user(self, i_qc):
        """
        This function verifies user data
        If there is not a matching data, raise ValidationError
        """
        self.logger.debug('\tVerify User')
        self.logger.info('Loading user information ...')
        if i_qc:
            if 'viewerUser' in self.user_json:
                query = { 'username': self.user_json['viewerUser'] }
                this_user = self.toolsdb.viewer.user.find_one(query)
                if this_user:
                    self.user_json['userName'] = this_user['name']
                    self.user_json['institution'] = this_user['institution']
                    self.user_json['description'] = 'viewer'
                    self.user_json['viewerUser'] = this_user['username']
                else:
                    self.logger.error('Not found user data {} registered in Local DB.'.format(query))
                    self.logger.error('Please contact Local DB administrator to create your account.')
                    raise ValidationError
            else:
                self.logger.error('Not username of viewer user account provided.')
                self.logger.error('Please login Local DB with username and password')
                self.logger.error('and create user config file by:')
                self.logger.error('   $ source localdb/login_mongodb.sh -Q')
                raise ValidationError
        self.logger.info('~~~ {')
        if i_qc:
            self.logger.info('~~~     "viewerUser": "\033[1;33m{}\033[0m",'.format(self.user_json['viewerUser']))
        self.logger.info('~~~     "name": "\033[1;33m{}\033[0m",'.format(self.user_json['userName']))
        self.logger.info('~~~     "institution": "\033[1;33m{}\033[0m"'.format(self.user_json['institution']))
        self.logger.info('~~~ }')

    def _verify_site(self, i_qc):
        """
        This function verifies site data
        If there is not a matching data, raise ValidationError
        """
        self.logger.debug('\tVerify Site')
        self.logger.info('Loading site information ...')
        if i_qc:
            if 'institution' in self.site_json or 'code' in self.site_json:
                query = {}
                if 'code' in self.site_json:
                    query = { 'code': self.site_json['code'] }
                else:
                    query = { 'institution': self.site_json['institution'] }
                this_site = self.localdb.pd.institution.find_one(query)
                if this_site:
                    self.site_json['code'] = this_site['code']
                    self.site_json['institution'] = this_site['institution']
                    self.site_json['pdInstitution'] = str(this_site['_id'])
                else:
                    self.logger.error('Not found site data {} registered in Local DB.'.format(query))
                    self.logger.error('Please set your institution correctly in ')
                    self.logger.error('{{ "code": "xxx" }} or {{ "institution": "xxx" }} in {0}/.yarr/localdb/{1}_site.json'.format(home, hostname))
                    raise ValidationError
            else:
                self.logger.error('Not institution code provided.')
                self.logger.error('Please set your institution correctory in ')
                self.logger.error('{{ "code": "xxx" }} or {{ "institution": "xxx" }} in {0}/.yarr/localdb/{1}_site.json'.format(home, hostname))
                raise ValidationError
        self.logger.info('~~~ {')
        if i_qc:
            self.logger.info('~~~     "code": "\033[1;33m{}\033[0m",'.format(self.site_json['code']))
        self.logger.info('~~~     "institution": "\033[1;33m{}\033[0m"'.format(self.site_json['institution']))
        self.logger.info('~~~ }')

    def _verify_conn_cfg(self, i_qc):
        """
        This function verifies component data
        If there is not a matching data, raise ValidationError
        """
        self.logger.debug('\tVerify Component')
        if not self.conns==[]:
            self.logger.info('Loading component information ...')
        if i_qc:
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
                    self.logger.error('Not found data {{ "serialNumber": "{0}", "componentType": "{1}" }} registered in Local DB.'.format(conn['module']['serialNumber'], conn['module']['componentType']))
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
                        self.logger.error('Not found data {{ "serialNumber": "{0}", "componentType": "{1}" }} registered in Local DB.'.format(chip_json['serialNumber'], chip_json['componentType']))
                        raise ValidationError
                    chip_json['component'] = ch_oid
                    cpr_oid = self._check_child_parent_relation(mo_oid, ch_oid)
                    if not cpr_oid:
                        self.logger.error('Not found chipParentRelation data {{ "module": "{0}", "chip": "{1}" }} registered in Local DB.'.format(conn['module']['serialNumber'], chip_json['serialNumber']))
                        raise ValidationError
                    chip_json['cpr'] = cpr_oid
                    conn['chips'].append(chip_json)
                # stage
                stage = self.__check_stage(mo_oid)
                if 'stage' in conn and not conn['stage']==stage:
                    self.logger.error('Not match stage "{0}" written in connectivity config file and "{1}" registered in Local DB.'.format(conn['stage'], stage))
                    self.logger.error('Please set correct stage or remove stage name from connectivity config file.')
                    raise ValidationError
                conn['stage'] = stage
                self.conns.append(conn)
        for conn in self.conns:
            self.logger.info('~~~ {')
            if not conn['module']=={}:
                self.logger.info('~~~     "parent": {')
                self.logger.info('~~~         "serialNumber": "\033[1;33m{}\033[0m",'.format(conn['module']['serialNumber']))
                self.logger.info('~~~         "componentType": "\033[1;33m{}\033[0m"'.format(conn['module']['componentType']))
                self.logger.info('~~~     },')
            self.logger.info('~~~     "children": [{')
            for i, chip in enumerate(conn['chips']):
                if not i==0:
                    self.logger.info('~~~     },{')
                self.logger.info('~~~         "serialNumber": "\033[1;33m{0}\033[0m",\033[1;33m{1}\033[0m'.format(chip['serialNumber'], ' (disabled)' if chip.get('enable',1)==0 else ''))
                self.logger.info('~~~         "componentType": "\033[1;33m{}\033[0m",'.format(chip['componentType']))
                self.logger.info('~~~         "chipId": "\033[1;33m{}\033[0m",'.format(chip['chipId']))
            self.logger.info('~~~     }],')
            self.logger.info('~~~     "stage": "\033[1;33m{}\033[0m"'.format(conn['stage']))
            self.logger.info('~~~ }')

    def __check_conn(self):
        """
        This function checks connectivity data
        """
        self.logger.debug('\tCheck Conn')
        conns = self.conns
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
                if chip_json.get('component','...')=='...':
                    ch_oid = self._check_component(chip_json)
                    chip_json['component'] = ch_oid
                if not conn['module'].get('component','...')=='...' and not chip_json.get('cpr'):
                    cpr_oid = self._check_child_parent_relation(mo_oid, ch_oid)
                    chip_json['cpr'] = cpr_oid
                if not chip_json.get('cpr'):
                    conn['module'] = {}
                chip_oid = self.__check_chip(chip_json)
                chip_json['chip'] = chip_oid
                if 'serialNumber'  in chip_json: del chip_json['serialNumber']
                if 'componentType' in chip_json: del chip_json['componentType']
                if 'chipId'        in chip_json: del chip_json['chipId']
                if 'cpr'           in chip_json: del chip_json['cpr']
                conn['chips'].append(chip_json)
            self.conns.append(conn)

    def __check_chip(self, i_json):
        """
        This function checks chip data
        If there is a matching data, return oid
        If there is not a matching data, register chip data and return oid
        If chip is disabled, return '...'
        """
        self.logger.debug('\tCheck Chip data:')
        oid = '...'
        if not i_json.get('enable', 1)==0:
            query = {
                'name'     : i_json['name'],
                'chipId'   : i_json['chipId'],
                'chipType' : self.chip_type,
                'dbVersion': self.db_version
            }
            this_chip = self.localdb.chip.find_one(query)
            if this_chip: oid = str(this_chip['_id'])
            else: oid = self.__register_chip(i_json)
        return oid

    def __check_stage(self, i_mo_oid):
        """
        This function checks current stage
        If there is a matching data, return stage
        If there is not a matching data, return '...'
        """
        self.logger.debug('\tCheck Stage')
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
        self, logger.debug('\tCheck Component-TestRun')
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
        self.logger.debug('\tCheck Config Json:')
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
        self.logger.debug('\tCheck Attachment:')
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
        self.logger.debug('\t\t\tCheck Json File by Hash')
        oid = None
        query = {
           'hash'     : i_hash_code,
           'dbVersion': self.db_version
        }
        this_file = self.localdb.fs.files.find_one( query, { '_id': 1 } )
        if this_file:
            oid = str(this_file['_id'])
        return oid

    def __register_chip(self, i_json):
        """
        chip data written in i_json is registered.
        """
        self.logger.debug('\t\tRegister Chip')
        doc = {
            'sys'          : {},
            'name'         : i_json.get('name','...'),
            'chipId'       : i_json.get('chipId',0),
            'chipType'     : self.chip_type,
            'componentType': 'front-end_chip',
            'dbVersion'    : self.db_version
        }
        oid = str(self.localdb.chip.insert_one(doc).inserted_id)
        self._update_sys(oid, 'chip')
        self.logger.debug('\t\tdoc   : {}'.format(oid))
        return oid

    def __register_test_run(self, i_json, i_stage, i_tr_oid):
        """
        Almost all the information in i_json is registered.
        """
        self.logger.debug('\t\tRegister Test Run')
        doc = {}
        for key in i_json:
            if not key=='connectivity' and not 'Cfg' in key and not key=='startTime' and not key=='finishTime':
                doc[key] = i_json[key]
        doc.update({
            'testType' : i_json.get('testType', '...'),
            'runNumber': i_json.get('runNumber', -1),
            'stage'    : i_stage.replace(' ','_'),
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
                'qcTest'     : self.qcTest,
                'qaTest'     : False,
                'summary'    : False,
                'startTime'  : datetime.utcfromtimestamp(i_json['startTime']),
                'finishTime' : datetime.utcfromtimestamp(i_json['finishTime']),
            })
            oid = str(self.localdb.testRun.insert_one(doc).inserted_id)
        else:
            oid = i_tr_oid
            query = { '_id': ObjectId(i_tr_oid) }
            self.localdb.testRun.update_one( query, {'$set': doc} )
        self._update_sys(oid, 'testRun')
        self.logger.debug('\t\tdoc   : {}'.format(oid))
        return oid

    def __register_component_test_run(self, i_json, i_tr_oid):
        """
        Almost all the information in i_json is registered.
        """
        self.logger.debug('\t\tRegister Component-TestRun')
        doc = i_json
        doc.update({
            'sys'        : {},
            'attachments': [],
            'environment': '...',
            'dbVersion'  : self.db_version
        })
        oid = str(self.localdb.componentTestRun.insert_one(doc).inserted_id)
        self._update_sys(oid, 'componentTestRun')
        self.logger.debug('\t\tdoc   : {}'.format(oid))
        return oid

    def __register_config(self, i_file_json, i_filename, i_title, i_col, i_oid):
        self.logger.debug('\t\tRegister Config Json')
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
        self.logger.debug('\t\tdoc   : {}'.format(i_oid))
        self.logger.debug('\t\tconfig: {}'.format(oid))
        self.logger.debug('\t\tdata  : {}'.format(data_id))
        return oid

    def __register_attachment(self, i_file_path, i_histo_name, i_oid, i_type):
        self.logger.debug('\t\tRegister Attachment')
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
        self.logger.debug('\t\tdoc   : {}'.format(i_oid))
        self.logger.debug('\t\tdata  : {}'.format(oid))
        return oid

    def __register_grid_fs_file(self, i_file_json, i_file_path, i_filename, i_hash=''):
        self.logger.debug('\t\t\tWrite File by GridFS')
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

    def setConfig(self):
        self.logger.debug('Set Configs')
        # user
        self.user_oid = self._check_user()

        # site
        self.site_oid = self._check_site()

        # connectivity
        self.__check_conn()

    def setDcs(self, i_tr_oid, i_env_json):
        self.logger.debug('\t\tRegister Environment')
        doc = {
            '_id'      : ObjectId(i_tr_oid),
            'dbVersion': self.db_version
        }
        this_run = self.localdb.testRun.find_one(doc)
        starttime  = this_run['startTime'].timestamp()
        finishtime = this_run['finishTime'].timestamp()
        for env_j in i_env_json:
            if env_j['status']!='enabled': continue
            env_key = env_j['key'].lower().replace(' ','_')
            query = {
                'testRun'   : i_tr_oid,
                'dbVersion' : self.db_version
            }
            if env_j.get('chip', None):
                query.update({ 'name': env_j['chip'] })
            ctr_entries = self.localdb.componentTestRun.find(query)
            for this_ctr in ctr_entries:
                ctr_oid = self.__check_dcs(str(this_ctr['_id']), env_key, env_j['num'], env_j['description'])
                if ctr_oid: self.__register_dcs(i_tr_oid, ctr_oid, env_key, env_j)
        self.tr_oids.append(i_tr_oid)

        return

    def __check_conn(self):
        """
        This function checks connectivity data
        """
        self.logger.debug('\tCheck Conn')
        conns = self.conns
        self.conns = []
        for conn in conns:
            # chips
            chips_json = conn['chips']
            conn['chips'] = []
            for i, chip_json in enumerate(chips_json):
                chip_oid = self.__check_chip(chip_json)
                chip_json['chip'] = chip_oid
                conn['chips'].append(chip_json)
            self.conns.append(conn)

    def __check_dcs(self, i_ctr_oid, i_key, i_num, i_description):
        self.logger.debug('\tCheck DCS')
        ctr_oid = i_ctr_oid
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
                    ctr_oid = None
                    break
        return ctr_oid

    def __register_dcs(self, i_tr_oid, i_ctr_oid, i_env_key, i_env_j):
        self.logger.debug('\t\tRegister DCS')
        array = []
        if i_env_j['path']!='null':
            extension = i_env_j['path'].split('.')[len(i_env_j['path'].split('.'))-1]
            if extension=='dat':
                separator = ' '
            elif extenstion=='csv':
                separator = ','
            else:
                self.logger.warning('This file ({}) format is not supported by Local DB, set to "dat" or "csv"'.format(i_env_j['path']))
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
                self.logger.warning('Not found DCS data {{ key: {0}, num: {1} }} in data file: {2}'.format(i_env_key, i_env_j['num'], i_env_j['path']))
                return
            # value
            env_line = env_file.readline()

            while env_line:
                if len(env_line.split(separator)) < key: break
                date = int(env_line.split(separator)[1])
                value = env_line.split(separator)[key]
                if not value=='null':
                    try:
                        value = float(value)
                    except:
                        self.logger.error('Invalid value : {0} in data file: {1}'.format(value, i_env_j['path']))
                        raise RegisterError
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
        query = {
            '_id'       : ObjectId(i_ctr_oid),
            'dbVersion' : self.db_version
        }
        this_ctr = self.localdb.componentTestRun.find_one(query)
        if this_ctr.get('environment', '...')=='...':
            doc_value = {
                'sys': {},
                i_env_key: [ i_env_j ],
                'dbVersion': self.db_version
            }
            doc_value[i_env_key][0].update({ 'data': array })
            oid = str(self.localdb.environment.insert_one(doc_value).inserted_id)
            self._add_value(i_ctr_oid, 'componentTestRun', 'environment', oid)
            self._update_sys(i_ctr_oid, 'componentTestRun');
        else:
            oid = this_ctr['environment']
            doc_value = {
                '$push': {
                    i_env_key: i_env_j
                }
            }
            doc_value['$push'][i_env_key].update({ 'data': array })
            query = { '_id': ObjectId(oid) }
            self.localdb.environment.update_one( query, doc_value )

        self._update_sys(oid, 'environment');
        self._add_value(i_tr_oid, 'testRun', 'environment', True)
        self._update_sys(i_tr_oid, 'testRun')

        self.logger.debug('\t\tctr doc : {}'.format(i_ctr_oid))
        self.logger.debug('\t\tdcs data: {}'.format(oid))

class CompData(RegisterData):
    def __init__(self):
        super().__init__()

    def checkConnCfg(self, i_path):
        """
        Check Component Connectivity
        If the components written in the file have not registered,
        Display component information in the console
        """
        self.logger.debug('\tCheck Connectivity config for registration:')
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
        conn['module']['children'] = len(chips)

        self.logger.info('Component Data:')
        self.logger.info('    Chip Type: \033[1;33m{}\033[0m'.format(conn['chipType']))
        if 'module' in conn:
            self.logger.info('    Module:')
            self.logger.info('        serial number: \033[1;33m{0}\033[0m {1}'.format(conn['module']['serialNumber'], '\033[1;31m(data already registered in Local DB)\033[0m' if conn['module']['status']==1 else ''))
            self.logger.info('        component type: \033[1;33m{}\033[0m'.format(conn['module']['componentType']))
            self.logger.info('        # of chips: \033[1;33m{}\033[0m'.format(conn['module']['children']))
        for i, chip in enumerate(conn['chips']):
            self.logger.info('    Chip ({}):'.format(i+1))
            self.logger.info('        serial number: \033[1;33m{0}\033[0m {1}'.format(chip['serialNumber'], '\033[1;31m(data already registered in Local DB)\033[0m' if chip['status']==1 else ''))
            self.logger.info('        component type: \033[1;33m{}\033[0m'.format(chip['componentType']))
            self.logger.info('        chip ID: \033[1;33m{}\033[0m'.format(chip['chipId']))

        self.logger.warning('It will be override with the provided infomation if data already exists in Local DB.')
        self.conn = conn

    def setComponent(self):
        """
        This function registers component information from cnnectivity file
        """
        self.logger.debug('\t\tRegister from Connectivity')

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

        self.logger.info('Succeeded uploading component data')
        self.logger.info('')
        self.logger.info('You can retrieve config files from Local DB by')
        if 'module' in conn:
            self.logger.info('    localdbtool-retrieve pull --chip {}'.format(conn['module']['serialNumber']))
        else:
            for chip_conn in conn['chips']:
                self.logger.info('    localdbtool-retrieve pull --chip {}'.format(chip_conn['serialNumber']))

        return True

    def __check_component(self, i_json):
        """
        This function checks component data
        """
        self.logger.debug('\tCheck Component')
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
        self.logger.debug('\t\tRegister Component')
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
        logger.debug('\t\tdoc   : {}'.format(oid))
        return oid

    def __register_child_parent_relation(self, i_parent_oid, i_child_oid, i_chip_id, i_geom_id, i_oid, i_active=True):
        """
        This function registeres ChildParentRelation
        """
        self.logger.debug('\t\tRegister Child Parent Relation.')
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
        self.logger.debug('\t\tdoc   : {}'.format(oid))
        return oid
