// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Simple PCie Carrier Kernel driver
// # Comment: Original driver taken from Marcus Guillermo
// ################################

#ifndef _SPECDRIVER_BASE_H
#define _SPECDRIVER_BASE_H

#include "sysfs.h"

/* prototypes for file_operations */
static struct file_operations specdriver_fops;
int specdriver_mmap( struct file *filp, struct vm_area_struct *vmap );
int specdriver_open(struct inode *inode, struct file *filp );
int specdriver_release(struct inode *inode, struct file *filp);

/* prototypes for device operations */
static struct pci_driver specdriver_driver;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0)
static int specdriver_probe(struct pci_dev *pdev, const struct pci_device_id *id);
static void specdriver_remove(struct pci_dev *pdev);
#else
static int __devinit specdriver_probe(struct pci_dev *pdev, const struct pci_device_id *id);
static void __devexit specdriver_remove(struct pci_dev *pdev);
#endif

/* prototypes for module operations */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0)
static int specdriver_init(void);
#else
static int __init specdriver_init(void);
#endif
static void specdriver_exit(void);

/*
 * This is the table of PCI devices handled by this driver by default
 * If you want to add devices dynamically to this list, do:
 *
 *   echo "vendor device" > /sys/bus/pci/drivers/pciDriver/new_id
 * where vendor and device are in hex, without leading '0x'.
 *
 * The IDs themselves can be found in common.h
 *
 * For more info, see <kernel-source>/Documentation/pci.txt
 *
 * __devinitdata is applied because the kernel does not need those
 * tables any more after boot is finished on systems which don't
 * support hotplug.
 *
 */

//static const __devinitdata struct pci_device_id specdriver_ids[] = {
static const struct pci_device_id specdriver_ids[] = {
	{ PCI_DEVICE( PCIE_SPEC_VENDOR_ID, PCIE_SPEC_DEVICE_ID ) },     // PCI-E SPEC card
	{0,0,0,0},
};

/* prototypes for internal driver functions */
int specdriver_pci_read( specdriver_privdata_t *privdata, pci_cfg_cmd *pci_cmd );
int specdriver_pci_write( specdriver_privdata_t *privdata, pci_cfg_cmd *pci_cmd );
int specdriver_pci_info( specdriver_privdata_t *privdata, pci_board_info *pci_info );

int specdriver_mmap_pci( specdriver_privdata_t *privdata, struct vm_area_struct *vmap , int bar );
int specdriver_mmap_kmem( specdriver_privdata_t *privdata, struct vm_area_struct *vmap );

/*************************************************************************/
/* Static data */
/* Hold the allocated major & minor numbers */
static dev_t specdriver_devt;

/* Number of devices allocated */
static atomic_t specdriver_deviceCount;

/* Sysfs attributes */
static DEVICE_ATTR(mmap_mode, (S_IRUGO | S_IWUGO), specdriver_show_mmap_mode, specdriver_store_mmap_mode);
static DEVICE_ATTR(mmap_area, (S_IRUGO | S_IWUGO), specdriver_show_mmap_area, specdriver_store_mmap_area);
static DEVICE_ATTR(kmem_count, S_IRUGO, specdriver_show_kmem_count, NULL);
static DEVICE_ATTR(kbuffers, S_IRUGO, specdriver_show_kbuffers, NULL);
static DEVICE_ATTR(kmem_alloc, S_IWUGO, NULL, specdriver_store_kmem_alloc);
static DEVICE_ATTR(kmem_free, S_IWUGO, NULL, specdriver_store_kmem_free);
static DEVICE_ATTR(umappings, S_IRUGO, specdriver_show_umappings, NULL);
static DEVICE_ATTR(umem_unmap, S_IWUGO, NULL, specdriver_store_umem_unmap);
static DEVICE_ATTR(irq_count, S_IRUGO, specdriver_show_irq_count, NULL);
static DEVICE_ATTR(irq_queues, S_IRUGO, specdriver_show_irq_queues, NULL);

#endif
