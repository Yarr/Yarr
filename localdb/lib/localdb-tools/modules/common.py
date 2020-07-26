#!/usr/bin/env python3
#################################
# Author: Arisa Kubota
# Email: arisa.kubota at cern.ch
# Date: July 2020
# Project: Local Database for YARR
#################################

import os, yaml, json
import logging
logger = logging.getLogger('Log').getChild('common')

class JsonParsingError(Exception):
    pass
class ConfigError(Exception):
    pass

home = os.environ['HOME']
if not "HOSTNAME" in os.environ:
    hostname = "default_host"
else:
    hostname = os.environ["HOSTNAME"]

def readCfg(i_path):
    """This function converts yaml config file to dict."""
    logger.debug('Read config file: {}'.format(i_path))
    conf = {}
    if os.path.isfile(i_path):
        f = open(i_path, 'r')
        conf = yaml.load(f, Loader=yaml.SafeLoader)
    return conf

def readKey(i_path):
    """This function read key file for authentication with Local DB."""
    logger.debug('Read key file: {}'.format(i_path))
    file_text = open(i_path, 'r')
    file_keys = file_text.read().split()
    keys = {
        'username': file_keys[0],
        'password': file_keys[1]
    }
    file_text.close()
    return keys

def writeJson(i_dict, i_path):
    """
    This function writes dict data into JSON file.
    If data is not dict, handle it as an exception.
    """
    logger.debug('Write data into JSON file: {}'.format(i_path))
    try:
        with open(i_path, 'w') as f: json.dump(i_dict, f, indent=4)
    except Exception as e:
        logger.error('Could not write JSON file: {}'.format(i_path))
        logger.error('\twhat(): '+f'{e}')
        raise JsonParsingError

def readJson(i_path):
    """
    This function reads JSON file and convert it to dict format.
    If a JSON parsing error occurs, handle it as an exception.
    """
    logger.debug('Read JSON file and convert it to dict: {}'.format(i_path))
    j = {}
    if i_path and os.path.isfile(i_path):
        try:
            with open(i_path, 'r') as f: j = json.load(f)
        except ValueError as e:
            logger.error('Could not parse {}'.format(i_path))
            logger.error('\twhat(): '+f'{e}')
            raise JsonParsingError
    return j

def readDbCfg(arg, log={}):
    """
    This function reads database config from
    1. argument: --database
    2. path written in scanLog.json: for upload tool
    3. default: $HOME/.yarr/localdb/$HOSTNAME_database.json
    Return empty if no db config
    """
    cfg = {}
    path = ''
    if arg:
        path = os.path.realpath(os.path.abspath(arg))
        cfg = readJson(path)
    elif not log=={}:
        path = 'scanLog.json'
        cfg = log
    else:
        path = '{0}/.yarr/localdb/{1}_database.json'.format(home, hostname)
        cfg = readJson(path)
        path = path + ' (default)'
    logger.info('-> Setting DB config: {}'.format(path))
    if cfg=={}:
        logger.error('\033[5mNot found database config.\033[0m')
        logger.error('Specify the correct path or create the default config by YARR/localdb/setup_db.sh')
        raise ConfigError
    return cfg

def readUserCfg(arg, log={}):
    """
    This function reads user config from
    1. argument: --user
    2. path written in scanLog.json: for upload tool
    3. default: $HOME/.yarr/localdb/user.json
    Return empty if no db config
    """
    cfg = log
    path = 'scanLog.json'
    if arg:
        path = os.path.realpath(os.path.abspath(arg))
        cfg.update(readJson(path))
    if cfg=={}:
        path = '{0}/.yarr/localdb/user.json'.format(home)
        cfg = readJson(path)
    logger.info('-> Setting user config: {}'.format(path))
    return cfg

def readSiteCfg(arg, log={}):
    """
    This function reads site config from
    1. argument: --site
    2. path written in scanLog.json: for upload tool
    3. default: $HOME/.yarr/localdb/$HOSTNAME_site.json
    Return empty if no db config
    """
    cfg = log
    path = 'scanLog.json'
    if arg:
        path = os.path.realpath(os.path.abspath(arg))
        cfg.update(readJson(path))
    if cfg=={}:
        path = '{0}/.yarr/localdb/{1}_site.json'.format(home, hostname)
        cfg = readJson(path)
    logger.info('-> Setting site config: {}'.format(path))
    return cfg

def addInstanceMethod(Class, method):
    setattr(Class, method.__name__, method)
