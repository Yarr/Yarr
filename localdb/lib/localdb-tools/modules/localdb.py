#!/usr/bin/env python3
#################################
# Author: Arisa Kubota
# Email: arisa.kubota at cern.ch
# Date: July 2020
# Project: Local Database for YARR
#################################

### Common
import os, sys, yaml, requests, logging, hashlib
from pymongo          import MongoClient, errors, DESCENDING
from getpass          import getpass
from bson.objectid    import ObjectId
from datetime         import datetime

class DBServiceError(Exception):
    pass
class DBConnectionError(Exception):
    pass
class DBAuthenticationFailure(Exception):
    pass
class ViewerConnectionError(Exception):
    pass

class LocalDb(object):
    def __init__(self):
        self.logger = logging.getLogger('Log').getChild('LocalDb')
        self.service = 'mongodb'
        self.username = None
        self.password = None
        self.auth = 'default'
        self.tls = False
        self.ssl = False
        self.__set_config()

    def setCfg(self, i_cfg):
        self.service = i_cfg.get('service', 'mongodb')

        if 'hostIp'   in i_cfg: self.ip       = i_cfg['hostIp']
        if 'hostPort' in i_cfg: self.port     = i_cfg['hostPort']
        if 'dbName'   in i_cfg: self.name     = i_cfg['dbName']
        if 'tls'      in i_cfg: self.tls      = i_cfg['tls'].get('enabled',False)
        if 'ssl'      in i_cfg: self.ssl      = i_cfg['ssl'].get('enabled',False)
        if 'auth'     in i_cfg: self.auth     = i_cfg['auth']
        if 'username' in i_cfg: self.username = i_cfg['username']
        if 'password' in i_cfg: self.password = i_cfg['password']
        if self.tls:
            self.cert = i_cfg['tls'].get('CertificateKeyFile',None)
            self.ca   = i_cfg['tls'].get('CAFile',None)
        if self.ssl:
            self.cert = i_cfg['ssl'].get('PEMKeyFile',None)
            self.ca   = i_cfg['ssl'].get('CAFile',None)

        return self.service

    def setUsername(self, i_username):
        self.username = i_username
    def setPassword(i_password):
        self.password = i_password

    def checkConnection(self):
        if self.service=='mongodb':
            url = 'mongodb://{0}:{1}'.format(self.ip, self.port)
            if (self.tls or self.ssl):
                url +='/?ssl=true'
                if (self.ca and self.cert):
                    url+='&ssl_ca_certs={0}&ssl_certfile={1}&ssl_match_hostname=false'.format(self.ca, self.cert)
            if self.auth=='x509':
                url+='&authMechanism=MONGODB-X509'
                self.authSource = "$external"
            self.url = url
            self.__check_connection_mongo()
        elif self.service=='viewer':
            url = 'http://{0}:{1}/localdb/'.format(self.ip, self.port)
            self.url = url
            self.__check_connection_viewer()
        return self.url

    def getClient(self):
        return self.client

    def getLocalDb(self):
        localdb = self.client[self.name]
        return localdb

    def getLocalDbTools(self):
        localdbtools = self.client['{}tools'.format(self.name)]
        return localdbtools

    def __set_config(self):
        if self.service=='mongodb':
            self.ip   = '127.0.0.1'
            self.port = 27017
            self.name = 'localdb'
            self.authSource = 'localdb'
        elif self.service=='viewer':
            self.ip   = '127.0.0.1'
            self.port = 5000
        else:
            raise DBServiceError

    def __check_connection_mongo(self):
        self.logger.info('Checking connection to DB Server: {0}/{1} ...'.format(self.url, self.name))
        max_server_delay = 1000
        username = None
        password = None
        try:
            client = MongoClient(
            self.url,
            serverSelectionTimeoutMS=max_server_delay,
            authSource=self.authSource
            )
        except Exception as e:
            print("failed with {0}".format(e))
        localdb = client[self.name]
        try:
            localdb.list_collection_names()
            self.__connection_succeeded()
        except errors.ServerSelectionTimeoutError as err:
            self.__connection_failed('to', err)
        except errors.OperationFailure as err:
            self.logger.info('Local DB is locked.')
            ### Need user authentication
            if self.username:
                username = self.username
            elif os.environ.get('username',None):
                username = hashlib.md5(os.environ['username'].encode('utf-8')).hexdigest()
            else:
                username = ''

            if self.password:
                password = self.password
            elif os.environ.get('password',None):
                password = os.environ['password']
            else:
                password = ''

            if username and password:
                try:
                    localdb.authenticate(username, password)
                    self.__connection_succeeded('Authentication success.')
                except errors.OperationFailure as err:
                    self.__connection_failed('auth', err)
            else:
                self.__connection_failed('auth', 'No username and password given')
        client = MongoClient(
            self.url,
            username=username,
            password=password,
            authSource=self.authSource
        )
        self.client = client
        return True

    def __check_connection_viewer(self):
        self.logger.info('Checking connection to Viewer: {} ...'.format(self.url))
        try:
            response = requests.get(self.url)
            if response.status_code==200:
                self.__connection_succeeded()
            else:
                self.__connection_failed('code', response.status_code)
        except Exception as err:
            self.__connection_failed('', err)
        return True

    def __connection_succeeded(self, message='Good connection!'):
        self.logger.info('---> {}'.format(message))

    def __connection_failed(self, err='to', message=''):
        if self.service=='mongodb':
            if err=='to':
                self.logger.error('---> Bad connection.')
                self.logger.error('     ' + f'{message}')
                raise DBConnectionError
            elif err=='auth':
                self.logger.error('---> ' + f'{message}')
                self.logger.error('     Please login Local DB with correct username and password by')
                self.logger.error('        $ source path/to/YARR/localdb/login_mongodb.sh')
                raise DBAuthenticationFailure
        elif self.service=='viewer':
            if err=='code':
                self.logger.error('---> Bad connection.')
                self.logger.error('     http response status code: ' + f'{message}')
                raise ViewerConnectionError
            else:
                self.logger.error('---> Bad connection.')
                self.logger.error('     ' + f'{message}')
                raise ViewerConnectionError
