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
import requests
import json
import signal

import argparse
import yaml     # Read YAML config file

# log
from logging import getLogger
logger = getLogger("Log").getChild("sub")

global url

####################
### Response to Json
### exist: return {json}
### not exist: return {}
### not json file: error
def getJson(viewer_url, params={}):
    response = requests.get(viewer_url, params=params)
    try:
        r_json = response.json()
    except:
        logger.error('Something wrong in url and could not get json data')
        sys.exit()
    if r_json.get('error'):
        logger.error(r_json['message'])
        sys.exit()
    return r_json

#########################
### Display test data log
### Searchable by
### - chip name (perfect match)
### - user name (partial match)
### - site name (partial match)
def __log(args, serialnumber=None):
    global url

    params = {}

    params.update({ 'chip': args.chip })
    params.update({ 'user': args.user })
    params.update({ 'site': args.site })

    viewer_url = '{0}/retrieve/log'.format(url)
    r_json = getJson(viewer_url, params)

    return r_json

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
    global url

    params = {}
    params.update({ 'chip': args.chip })
    params.update({ 'test': args.test })
    params.update({ 'dir' : dir_path })

    # get chip data
    viewer_url = '{0}/retrieve/data'.format(url)
    r_json = getJson(viewer_url, params)

    if r_json.get('warning',None):
        logger.warning(r_json['warning'])

    console_data = r_json['console_data']
    data_entries = []
    for entry in console_data['data']:
        if not entry['bool']:
            viewer_url = '{0}/retrieve/config?oid={1}&type={2}'.format(url, entry['data'], entry['type'])
            r_json = getJson(viewer_url)
            entry.update({ 'data': r_json['data'] })
        data_entries.append(entry)
    console_data['data'] = data_entries

    return console_data

#####################
### Display data list
### - component
### - user
### - site
def __list_component():
    global url
    viewer_url = '{0}/retrieve/list?opt=component'.format(url)
    r_json = getJson(viewer_url)

    return r_json

def __list_user():
    global url
    viewer_url = '{0}/retrieve/list?opt=user'.format(url)
    r_json = getJson(viewer_url)

    return r_json

def __list_site():
    global url

    viewer_url = '{0}/retrieve/list?opt={1}'.format(url, opt)
    r_json = getJson(viewer_url)

    return r_json
