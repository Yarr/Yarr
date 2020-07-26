#!/usr/bin/env python3
#################################
# Author: Arisa Kubota
# Email: arisa.kubota at cern.ch
# Date: July 2020
# Project: Local Database for YARR
#################################

import os
from copy import copy
from logging import getLogger, getLoggerClass, Formatter, FileHandler, StreamHandler, addLevelName, DEBUG, INFO, WARNING, ERROR, CRITICAL
import coloredlogs

_level = INFO
#_level = DEBUG
logger = getLogger("Log")
logger.setLevel(_level)

LevelNames = {
    'DEBUG'   : "[ debug  ]", # white
    'INFO'    : "[  info  ]", # cyan
    'WARNING' : "[warning ]", # yellow
    'ERROR'   : "[ error  ]", # red
    'CRITICAL': "[critical]", # white on red bg
}

LevelColors = {
    'DEBUG'   : 37, # white
    'INFO'    : 32, # green
    'WARNING' : 33, # yellow
    'ERROR'   : 31, # red
    'CRITICAL': 41, # white on red bg
}

class ColoredFormatter(Formatter):
    def __init__(self, patern):
        Formatter.__init__(self, patern, datefmt="%H:%M:%S")
    def format(self, record):
        colored_record = copy(record)
        levelname = colored_record.levelname
        color = LevelColors.get(levelname, 37)
        name  = LevelNames .get(levelname, "[unknown ]")
        colored_levelname = "\033[{0}m{1}[   Local DB    ]:\033[0m".format(color, name)
        colored_record.levelname = colored_levelname
        return Formatter.format(self, colored_record)

class LogFileFormatter(Formatter):
    def __init__(self, patern):
        Formatter.__init__(self, patern)
    def format(self, record):
        file_record = copy(record)
        levelname = file_record.levelname
        name  = LevelNames .get(levelname, "[unknown ]")
        file_levelname = "{0}[   Local DB    ]:".format(name)
        file_record.levelname = file_levelname
        return Formatter.format(self, file_record)

def setLog(level=_level):
    console = StreamHandler()
    console.setLevel(level)
    formatter = ColoredFormatter("[%(asctime)s:%(msecs)-3d]%(levelname)s %(message)s")
    console.setFormatter(formatter)
    logger.addHandler(console)

    logger.setLevel(level)
    logger.debug('Set log')

def setLogFile(filename, level=_level):
    dir_path = os.path.dirname(os.path.abspath(filename))
    os.makedirs(dir_path, exist_ok=True)
    handler = FileHandler(filename)
    handler.setLevel(level)
    formatter = ColoredFormatter("[%(asctime)s:%(msecs)-3d]%(levelname)s %(message)s")
    handler.setFormatter(formatter)
    logger.addHandler(handler)
    logger.setLevel(level)
    logger.debug('Set log file: {}'.format(filename))
