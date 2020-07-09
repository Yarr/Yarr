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

def readConfig(i_path):
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

def readJson(i_path):
    """
    This function reads JSON file and convert it to dict format.
    If a JSON parsing error occurs, handle it as an exception.
    """
    logger.debug('Read json file and convert it to dict: {}'.format(i_path))
    j = {}
    if i_path and os.path.isfile(i_path):
        try:
            with open(i_path, 'r') as f: j = json.load(f)
        except ValueError as e:
            message = 'Could not parse ' + i_path + '\n\twhat(): ' + f'{e}'
            raise Exception(message)
    return j





