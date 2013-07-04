// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Simple PCie Carrier Kernel driver
// # Comment: Original driver taken from Marcus Guillermo
// #          Modified for SPEC card
// ################################

#ifndef _SPECDRIVER_CONFIG_H
#define _SPECDRIVER_CONFIG_H

/*******************************/
/* Configuration of the driver */
/*******************************/

/* Debug messages */
//#define DEBUG

/* The name of the module */
#define MODNAME "specDriver"

/* Major number is allocated dynamically */
/* Minor number */
#define MINORNR 0

/* Node name of the char device */
#define NODENAME "spec"
#define NODENAMEFMT "spec%d"

/* Maximum number of devices*/
#define MAXDEVICES 4

#endif
