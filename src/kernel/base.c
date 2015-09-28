// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Simple PCie Carrier Kernel driver
// # Comment: Original driver taken from Marcus Guillermo
// #          Modified for SPEC card
// ################################

#include <linux/version.h>

/* Check macros and kernel version first */
#ifndef KERNEL_VERSION
#error "No KERNEL_VERSION macro! Stopping."
#endif

#ifndef LINUX_VERSION_CODE
#error "No LINUX_VERSION_CODE macro! Stopping."
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,8)
#error "This driver has been tested only for Kernel 2.6.8 or above."
#endif

/* Required includes */
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/sysfs.h>
#include <asm/atomic.h>
#include <linux/pagemap.h>
#include <linux/spinlock.h>
#include <linux/list.h>
#include <asm/scatterlist.h>
#include <linux/vmalloc.h>
#include <linux/stat.h>
#include <linux/interrupt.h>
#include <linux/wait.h>

/* Configuration for the driver (what should be compiled in, module name, etc...) */
#include "config.h"

/* Compatibility functions/definitions (provides functions which are not available on older kernels) */
#include "compat.h"

/* External interface for the driver */
#include "specDriver.h"

/* Internal definitions for all parts (prototypes, data, macros) */
#include "common.h"

/* Internal definitions for the base part */
#include "base.h"

/* Internal definitions of the IRQ handling part */
#include "int.h"

/* Internal definitions for kernel memory */
#include "kmem.h"

/* Internal definitions for user space memory */
#include "umem.h"

/* IO functions for char dev */
#include "ioctl.h"

/*************************************************************************/
/* Module device table associated with this driver */
MODULE_DEVICE_TABLE(pci, specdriver_ids);

/* Module init and exit points */
module_init(specdriver_init);
module_exit(specdriver_exit);

/* Module info */
MODULE_AUTHOR("Timon Heim");
MODULE_DESCRIPTION("SPEC: Simple PCIe Carrier driver");
MODULE_LICENSE("GPL v2");

/* Module class */
static struct class_compat *specdriver_class;

/**
 *
 * Called when loading the driver
 *
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0)
static int specdriver_init(void)
#else
static int __init specdriver_init(void)
#endif
{
	int err;

	/* Initialize the device count */
	atomic_set(&specdriver_deviceCount, 0);

	/* Allocate character device region dynamically */
	if ((err = alloc_chrdev_region(&specdriver_devt, MINORNR, MAXDEVICES, NODENAME)) != 0) {
		mod_info("Couldn't allocate chrdev region. Module not loaded.\n");
		goto init_alloc_fail;
	}
	mod_info("Major %d allocated to nodename '%s'\n", MAJOR(specdriver_devt), NODENAME);

	/* Register driver class */
	specdriver_class = class_create(THIS_MODULE, NODENAME);

	if (IS_ERR(specdriver_class)) {
		mod_info("No sysfs support. Module not loaded.\n");
		goto init_class_fail;
	}

	/* Register PCI driver. This function returns the number of devices on some
	 * systems, therefore check for errors as < 0. */
	if ((err = pci_register_driver(&specdriver_driver)) < 0) {
		mod_info("Couldn't register PCI driver. Module not loaded.\n");
		goto init_pcireg_fail;
	}

	mod_info("Module loaded\n");

	return 0;

init_pcireg_fail:
	class_destroy(specdriver_class);
init_class_fail:
	unregister_chrdev_region(specdriver_devt, MAXDEVICES);
init_alloc_fail:
	return err;
}

/**
 *
 * Called when unloading the driver
 *
 */
static void specdriver_exit(void)
{

	pci_unregister_driver(&specdriver_driver);
	unregister_chrdev_region(specdriver_devt, MAXDEVICES);

	if (specdriver_class != NULL)
		class_destroy(specdriver_class);

	mod_info("Module unloaded\n");
}

/*************************************************************************/
/* Driver functions */

/**
 *
 * This struct defines the PCI entry points.
 * Will be registered at module init.
 *
 */
static struct pci_driver specdriver_driver = {
	.name = MODNAME,
	.id_table = specdriver_ids,
	.probe = specdriver_probe,
	.remove = specdriver_remove,
};

/**
 *
 * This function is called when installing the driver for a device
 * @param pdev Pointer to the PCI device
 *
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0)
static int specdriver_probe(struct pci_dev *pdev, const struct pci_device_id *id)
#else
static int __devinit specdriver_probe(struct pci_dev *pdev, const struct pci_device_id *id)
#endif
{
	int err;
	int devno;
	specdriver_privdata_t *privdata;
	int devid;

	/* Get our SPEC board or atleast GN4124 */
    if ((id->vendor == PCIE_SPEC_VENDOR_ID) &&
		(id->device == PCIE_SPEC_DEVICE_ID))
	{
        /* It is a SPEC card */
		mod_info("Found SPEC card at %s\n", dev_name(&pdev->dev));
	}
	else
	{
		/* It is something else */
		mod_info( "Found unknown board (%x:%x) at %s\n", id->vendor, id->device, dev_name(&pdev->dev));
	}

    /* Need bus master for DMA SG */
    pci_set_master(pdev);

	/* Enable the device */
	if ((err = pci_enable_device(pdev)) != 0) {
		mod_info("Couldn't enable device\n");
		goto probe_pcien_fail;
	}
    
    /* Setup DMA mask, no idea why ?? */
	if(pci_set_dma_mask(pdev, DMA_BIT_MASK(64)) == 0) {
		mod_info("64bits bus master DMA capable\n");
		if(pci_set_consistent_dma_mask(pdev, DMA_BIT_MASK(64)) < 0) {
			mod_info("Unable to perform 64bits consistent DMA mask set operation!\n");
		}
	} else if(pci_set_dma_mask(pdev, DMA_BIT_MASK(32)) == 0) {
		mod_info("32bits bus master DMA capable\n");
		if(pci_set_consistent_dma_mask(pdev, DMA_BIT_MASK(32)) < 0) {
		    mod_info("Unable to perform 64bits consistent DMA mask set operation!\n");
		}
	} else {
	    mod_info("Unable to perform DMA mask set operation!\n");
	}
    
    /* LEts use MSI interrupts */
    if (pci_enable_msi(pdev) != 0) 
        mod_info("Failed activating MSI!"); 

	/* Set Memory-Write-Invalidate support */
	if ((err = pci_set_mwi(pdev)) != 0)
		mod_info("MWI not supported. Continue without enabling MWI.\n");

	/* Get / Increment the device id */
	devid = atomic_inc_return(&specdriver_deviceCount) - 1;
	if (devid >= MAXDEVICES) {
		mod_info("Maximum number of devices reached! Increase MAXDEVICES.\n");
		err = -ENOMSG;
		goto probe_maxdevices_fail;
	}

	/* Allocate and initialize the private data for this device */
	if ((privdata = kcalloc(1, sizeof(*privdata), GFP_KERNEL)) == NULL) {
		err = -ENOMEM;
		goto probe_nomem;
	}

	INIT_LIST_HEAD(&(privdata->kmem_list));
	spin_lock_init(&(privdata->kmemlist_lock));
	atomic_set(&privdata->kmem_count, 0);

	INIT_LIST_HEAD(&(privdata->umem_list));
	spin_lock_init(&(privdata->umemlist_lock));
	atomic_set(&privdata->umem_count, 0);

	pci_set_drvdata( pdev, privdata );
	privdata->pdev = pdev;

	/* Device add to sysfs */
	devno = MKDEV(MAJOR(specdriver_devt), MINOR(specdriver_devt) + devid);
	privdata->devno = devno;
	if (specdriver_class != NULL) {
		/* FIXME: some error checking missing here */
		privdata->class_dev = class_device_create(specdriver_class, NULL, devno, &(pdev->dev), NODENAMEFMT, MINOR(specdriver_devt) + devid, privdata);
		class_set_devdata( privdata->class_dev, privdata );
		mod_info("Device /dev/%s%d added\n",NODENAME,MINOR(specdriver_devt) + devid);
	}

	/* Setup mmaped BARs into kernel space */
	if ((err = specdriver_probe_irq(privdata)) != 0)
		goto probe_irq_probe_fail;

	/* Populate sysfs attributes for the class device */
	/* TODO: correct errorhandling. ewww. must remove the files in reversed order :-( */
	#define sysfs_attr(name) do { \
			if (class_device_create_file(sysfs_attr_def_pointer, &sysfs_attr_def_name(name)) != 0) \
				goto probe_device_create_fail; \
			} while (0)
	sysfs_attr(irq_count);
	sysfs_attr(irq_queues);
	sysfs_attr(mmap_mode);
	sysfs_attr(mmap_area);
	sysfs_attr(kmem_count);
	sysfs_attr(kmem_alloc);
	sysfs_attr(kmem_free);
	sysfs_attr(kbuffers);
	sysfs_attr(umappings);
	sysfs_attr(umem_unmap);
	#undef sysfs_attr

	/* Register character device */
	cdev_init( &(privdata->cdev), &specdriver_fops );
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,35)
	privdata->cdev.owner = THIS_MODULE;
#endif
	privdata->cdev.ops = &specdriver_fops;
	err = cdev_add( &privdata->cdev, devno, 1 );
	if (err) {
		mod_info( "Couldn't add character device.\n" );
		goto probe_cdevadd_fail;
	}

	return 0;

probe_device_create_fail:
probe_cdevadd_fail:
probe_irq_probe_fail:
	specdriver_irq_unmap_bars(privdata);
	kfree(privdata);
probe_nomem:
	atomic_dec(&specdriver_deviceCount);
probe_maxdevices_fail:
	pci_disable_device(pdev);
probe_pcien_fail:
 	return err;
}

/**
 *
 * This function is called when disconnecting a device
 *
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0)
static void specdriver_remove(struct pci_dev *pdev)
#else
static void __devexit specdriver_remove(struct pci_dev *pdev)
#endif
{
	specdriver_privdata_t *privdata;
    
	/* Get private data from the device */
	privdata = pci_get_drvdata(pdev);

	/* Removing sysfs attributes from class device */
	#define sysfs_attr(name) do { \
			class_device_remove_file(sysfs_attr_def_pointer, &sysfs_attr_def_name(name)); \
			} while (0)
	sysfs_attr(irq_count);
	sysfs_attr(irq_queues);
	sysfs_attr(mmap_mode);
	sysfs_attr(mmap_area);
	sysfs_attr(kmem_count);
	sysfs_attr(kmem_alloc);
	sysfs_attr(kmem_free);
	sysfs_attr(kbuffers);
	sysfs_attr(umappings);
	sysfs_attr(umem_unmap);
	#undef sysfs_attr

	/* Free all allocated kmem buffers before leaving */
	specdriver_kmem_free_all( privdata );

	specdriver_remove_irq(privdata);

	/* Removing Character device */
	cdev_del(&(privdata->cdev));

	/* Removing the device from sysfs */
	class_device_destroy(specdriver_class, privdata->devno);

	/* Releasing privdata */
	kfree(privdata);
    
    /* Disable MSI IRQ */
    pci_disable_msi(pdev);

	/* Disabling PCI device */
	pci_disable_device(pdev);

	mod_info("Device at %s removed\n", dev_name(&pdev->dev));
}

/*************************************************************************/
/* File operations */
/*************************************************************************/

/**
 * This struct defines the file operation entry points.
 *
 * @see specdriver_ioctl
 * @see specdriver_mmap
 * @see specdriver_open
 * @see specdriver_release
 *
 */
static struct file_operations specdriver_fops = {
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,35)
	.owner = THIS_MODULE,
#endif
	.unlocked_ioctl = specdriver_ioctl,
	.mmap = specdriver_mmap,
	.open = specdriver_open,
	.release = specdriver_release,
};

/**
 *
 * Called when an application open()s a /dev/fpga*, attaches the private data
 * with the file pointer.
 *
 */
int specdriver_open(struct inode *inode, struct file *filp)
{
	specdriver_privdata_t *privdata;

	/* Set the private data area for the file */
	privdata = container_of( inode->i_cdev, specdriver_privdata_t, cdev);
	filp->private_data = privdata;

	return 0;
}

/**
 *
 * Called when the application close()s the file descriptor. Does nothing at
 * the moment.
 *
 */
int specdriver_release(struct inode *inode, struct file *filp)
{
	specdriver_privdata_t *privdata;

	/* Get the private data area */
	privdata = filp->private_data;

	return 0;
}

/**
 *
 * This function is the entry point for mmap() and calls either specdriver_mmap_pci
 * or specdriver_mmap_kmem
 *
 * @see specdriver_mmap_pci
 * @see specdriver_mmap_kmem
 *
 */
int specdriver_mmap(struct file *filp, struct vm_area_struct *vma)
{
	specdriver_privdata_t *privdata;
	int ret = 0, bar;

	mod_info_dbg("Entering mmap\n");

	/* Get the private data area */
	privdata = filp->private_data;

	/* Check the current mmap mode */
	switch (privdata->mmap_mode) {
		case SPECDRIVER_MMAP_PCI:
			/* Mmap a PCI region */
            /* 3 Bars for SPEC */
			switch (privdata->mmap_area) {
				case SPECDRIVER_BAR0:	bar = 0; break;
				case SPECDRIVER_BAR1:	bar = 1; break;
				case SPECDRIVER_BAR2:	bar = 2; break;
				case SPECDRIVER_BAR3:	bar = 3; break;
				case SPECDRIVER_BAR4:	bar = 4; break;
				case SPECDRIVER_BAR5:	bar = 5; break;
				default:
					mod_info("Attempted to mmap a PCI area with the wrong mmap_area value: %d\n",privdata->mmap_area);
					return -EINVAL;			/* invalid parameter */
					break;
			}
			ret = specdriver_mmap_pci(privdata, vma, bar);
			break;
		case SPECDRIVER_MMAP_KMEM:
			/* mmap a Kernel buffer */
			ret = specdriver_mmap_kmem(privdata, vma);
			break;
		default:
			mod_info( "Invalid mmap_mode value (%d)\n",privdata->mmap_mode );
			return -EINVAL;			/* Invalid parameter (mode) */
	}

	return ret;
}

/*************************************************************************/
/* Internal driver functions */
int specdriver_mmap_pci(specdriver_privdata_t *privdata, struct vm_area_struct *vmap, int bar)
{
	int ret = 0;
	unsigned long bar_addr;
	unsigned long bar_length, vma_size;
	unsigned long bar_flags;

	mod_info_dbg("Entering mmap_pci\n");

	/* Get info of the BAR to be mapped */
	bar_addr = pci_resource_start(privdata->pdev, bar);
	bar_length = pci_resource_len(privdata->pdev, bar);
	bar_flags = pci_resource_flags(privdata->pdev, bar);

	/* Check sizes */
	vma_size = (vmap->vm_end - vmap->vm_start);
	if ((vma_size != bar_length) &&
	   ((bar_length < PAGE_SIZE) && (vma_size != PAGE_SIZE))) {
		mod_info( "mmap size is not correct! bar: %lu - vma: %lu\n", bar_length, vma_size );
		return -EINVAL;
	}

	if (bar_flags & IORESOURCE_IO) {
		/* Unlikely case, we will mmap a IO region */

		/* IO regions are never cacheable */
#ifdef pgprot_noncached
		vmap->vm_page_prot = pgprot_noncached(vmap->vm_page_prot);
#endif

		/* Map the BAR */
		ret = io_remap_pfn_range_compat(
					vmap,
					vmap->vm_start,
					bar_addr,
					bar_length,
					vmap->vm_page_prot);
	} else {
		/* Normal case, mmap a memory region */

		/* Ensure this VMA is non-cached, if it is not flaged as prefetchable.
		 * If it is prefetchable, caching is allowed and will give better performance.
		 * This should be set properly by the BIOS, but we want to be sure. */
		/* adapted from drivers/char/mem.c, mmap function. */
#ifdef pgprot_noncached
/* Setting noncached disables MTRR registers, and we want to use them.
 * So we take this code out. This can lead to caching problems if and only if
 * the System BIOS set something wrong. Check LDDv3, page 425.
 */
//		if (!(bar_flags & IORESOURCE_PREFETCH))
//			vmap->vm_page_prot = pgprot_noncached(vmap->vm_page_prot);
#endif

		/* Map the BAR */
		ret = remap_pfn_range_compat(
					vmap,
					vmap->vm_start,
					bar_addr,
					bar_length,
					vmap->vm_page_prot);
	}

	if (ret) {
		mod_info("remap_pfn_range failed\n");
		return -EAGAIN;
	}

	return 0;	/* success */
}
