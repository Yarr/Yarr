/**
 *
 * @file ioctl.c
 * @author Guillermo Marcus
 * @date 2009-04-05
 * @brief Contains the functions handling the different ioctl calls.
 *
 */
#include <linux/version.h>
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
#include <linux/sched.h>
#include <linux/completion.h>

#include "config.h" 			/* Configuration for the driver */
#include "compat.h" 			/* Compatibility functions/definitions */
#include "specDriver.h" 			/* External interface for the driver */
#include "common.h" 			/* Internal definitions for all parts */
#include "kmem.h" 			/* Internal definitions for kernel memory */
#include "umem.h" 			/* Internal definitions for user space memory */
#include "ioctl.h"			/* Internal definitions for the ioctl part */

/** Declares a variable of the given type with the given name and copies it from userspace */
#define READ_FROM_USER(type, name) \
	type name; \
	if ((ret = copy_from_user(&name, (type*)arg, sizeof(name))) != 0) \
		return -EFAULT;

/** Writes back the given variable with the given type to userspace */
#define WRITE_TO_USER(type, name) \
	if ((ret = copy_to_user((type*)arg, &name, sizeof(name))) != 0) \
		return -EFAULT;

/**
 *
 * Sets the mmap mode for following mmap() calls.
 *
 * @param arg Not a pointer, but either SPECDRIVER_MMAP_PCI or SPECDRIVER_MMAP_KMEM
 *
 */
static int ioctl_mmap_mode(specdriver_privdata_t *privdata, unsigned long arg)
{
	if ((arg != SPECDRIVER_MMAP_PCI) && (arg != SPECDRIVER_MMAP_KMEM))
		return -EINVAL;

	/* change the mode */
	privdata->mmap_mode = arg;

	return 0;
}

/**
 *
 * Sets the mmap area (BAR) for following mmap() calls.
 *
 */
static int ioctl_mmap_area(specdriver_privdata_t *privdata, unsigned long arg)
{
	/* validate input */
	if ((arg < SPECDRIVER_BAR0) || (arg > SPECDRIVER_BAR5))
		return -EINVAL;

	/* change the PCI area to mmap */
	privdata->mmap_area = arg;

	return 0;
}

/**
 *
 * Reads/writes a byte/word/dword of the device's PCI config.
 *
 * @see specdriver_pci_read
 * @see specdriver_pci_write
 *
 */
static int ioctl_pci_config_read_write(specdriver_privdata_t *privdata, unsigned int cmd, unsigned long arg)
{
	int ret;
	READ_FROM_USER(pci_cfg_cmd, pci_cmd);

	if (cmd == SPECDRIVER_IOC_PCI_CFG_RD) {
		switch (pci_cmd.size) {
			case SPECDRIVER_PCI_CFG_SZ_BYTE:
				ret = pci_read_config_byte( privdata->pdev, pci_cmd.addr, &(pci_cmd.val.byte) );
				break;
			case SPECDRIVER_PCI_CFG_SZ_WORD:
				ret = pci_read_config_word( privdata->pdev, pci_cmd.addr, &(pci_cmd.val.word) );
				break;
			case SPECDRIVER_PCI_CFG_SZ_DWORD:
				ret = pci_read_config_dword( privdata->pdev, pci_cmd.addr, &(pci_cmd.val.dword) );
				break;
			default:
				return -EINVAL;		/* Wrong size setting */
		}
	} else {
		switch (pci_cmd.size) {
			case SPECDRIVER_PCI_CFG_SZ_BYTE:
				ret = pci_write_config_byte( privdata->pdev, pci_cmd.addr, pci_cmd.val.byte );
				break;
			case SPECDRIVER_PCI_CFG_SZ_WORD:
				ret = pci_write_config_word( privdata->pdev, pci_cmd.addr, pci_cmd.val.word );
				break;
			case SPECDRIVER_PCI_CFG_SZ_DWORD:
				ret = pci_write_config_dword( privdata->pdev, pci_cmd.addr, pci_cmd.val.dword );
				break;
			default:
				return -EINVAL;		/* Wrong size setting */
				break;
		}
	}

	WRITE_TO_USER(pci_cfg_cmd, pci_cmd);

	return 0;
}

/**
 *
 * Gets the PCI information for the device.
 *
 * @see specdriver_pci_info
 *
 */
static int ioctl_pci_info(specdriver_privdata_t *privdata, unsigned long arg)
{
	int ret;
	int bar;
	READ_FROM_USER(pci_board_info, pci_info);

	pci_info.vendor_id = privdata->pdev->vendor;
	pci_info.device_id = privdata->pdev->device;
	pci_info.bus = privdata->pdev->bus->number;
	pci_info.slot = PCI_SLOT(privdata->pdev->devfn);
	pci_info.devfn = privdata->pdev->devfn;


	if ((ret = pci_read_config_byte(privdata->pdev, PCI_INTERRUPT_PIN, &(pci_info.interrupt_pin))) != 0)
		return ret;

	if ((ret = pci_read_config_byte(privdata->pdev, PCI_INTERRUPT_LINE, &(pci_info.interrupt_line))) != 0)
		return ret;


	for (bar = 0; bar < 6; bar++) {
		pci_info.bar_start[bar] = pci_resource_start(privdata->pdev, bar);
		pci_info.bar_length[bar] = pci_resource_len(privdata->pdev, bar);
	}

	WRITE_TO_USER(pci_board_info, pci_info);

	return 0;
}

/**
 *
 * Allocates kernel memory.
 *
 * @see specdriver_kmem_alloc
 *
 */
static int ioctl_kmem_alloc(specdriver_privdata_t *privdata, unsigned long arg)
{
	int ret;
	READ_FROM_USER(kmem_handle_t, khandle);

	if ((ret = specdriver_kmem_alloc(privdata, &khandle)) != 0)
		return ret;

	WRITE_TO_USER(kmem_handle_t, khandle);

	return 0;
}

/**
 *
 * Frees kernel memory.
 *
 * @see specdriver_kmem_free
 *
 */
static int ioctl_kmem_free(specdriver_privdata_t *privdata, unsigned long arg)
{
	int ret;
	READ_FROM_USER(kmem_handle_t, khandle);

	if ((ret = specdriver_kmem_free(privdata, &khandle)) != 0)
		return ret;

	return 0;
}

/**
 *
 * Syncs kernel memory.
 *
 * @see specdriver_kmem_sync
 *
 */
static int ioctl_kmem_sync(specdriver_privdata_t *privdata, unsigned long arg)
{
	int ret;
	READ_FROM_USER(kmem_sync_t, ksync);

	return specdriver_kmem_sync(privdata, &ksync);
}

/*
 *
 * Maps the given scatter/gather list from memory to PCI bus addresses.
 *
 * @see specdriver_umem_sgmap
 *
 */
static int ioctl_umem_sgmap(specdriver_privdata_t *privdata, unsigned long arg)
{
	int ret;
	READ_FROM_USER(umem_handle_t, uhandle);

	if ((ret = specdriver_umem_sgmap(privdata, &uhandle)) != 0)
		return ret;

	WRITE_TO_USER(umem_handle_t, uhandle);

	return 0;
}

/**
 *
 * Unmaps the given scatter/gather list.
 *
 * @see specdriver_umem_sgunmap
 *
 */
static int ioctl_umem_sgunmap(specdriver_privdata_t *privdata, unsigned long arg)
{
	int ret;
	specdriver_umem_entry_t *umem_entry;
	READ_FROM_USER(umem_handle_t, uhandle);

	/* Find the associated umem_entry for this buffer,
	 * return -EINVAL if the specified handle id is invalid */
	if ((umem_entry = specdriver_umem_find_entry_id(privdata, uhandle.handle_id)) == NULL)
		return -EINVAL;

	if ((ret = specdriver_umem_sgunmap(privdata, umem_entry)) != 0)
		return ret;

	return 0;
}

/**
 *
 * Copies the scatter/gather list from kernelspace to userspace.
 *
 * @see specdriver_umem_sgget
 *
 */
static int ioctl_umem_sgget(specdriver_privdata_t *privdata, unsigned long arg)
{
	int ret;
	READ_FROM_USER(umem_sglist_t, usglist);

	/* The umem_sglist_t has a pointer to the scatter/gather list itself which
	 * needs to be copied separately. The number of elements is stored in ->nents.
	 * As the list can get very big, we need to use vmalloc. */
	if ((usglist.sg = vmalloc(usglist.nents * sizeof(umem_sgentry_t))) == NULL)
		return -ENOMEM;

	/* copy array to kernel structure */
	ret = copy_from_user(usglist.sg, ((umem_sglist_t *)arg)->sg, (usglist.nents)*sizeof(umem_sgentry_t));
	if (ret) return -EFAULT;

	if ((ret = specdriver_umem_sgget(privdata, &usglist)) != 0)
		return ret;

	/* write data to user space */
	ret = copy_to_user(((umem_sglist_t *)arg)->sg, usglist.sg, (usglist.nents)*sizeof(umem_sgentry_t));
	if (ret) return -EFAULT;

	/* free array memory */
	vfree(usglist.sg);

	/* restore sg pointer to vma address in user space before copying */
	usglist.sg = ((umem_sglist_t *)arg)->sg;

	WRITE_TO_USER(umem_sglist_t, usglist);

	return 0;
}

/**
 *
 * Syncs user memory.
 *
 * @see specdriver_umem_sync
 *
 */
static int ioctl_umem_sync(specdriver_privdata_t *privdata, unsigned long arg)
{
	int ret;
	READ_FROM_USER(umem_handle_t, uhandle);

	return specdriver_umem_sync( privdata, &uhandle );
}

/**
 *
 * Waits for an interrupt
 *
 * @param arg Not a pointer, but the irq source to wait for (unsigned int)
 *
 */
static int ioctl_wait_interrupt(specdriver_privdata_t *privdata, unsigned long arg)
{
	unsigned int irq_source;
	int temp;
    int ret;

	if (arg >= SPECDRIVER_INT_MAXSOURCES)
		return -1;						/* User tried to overrun the IRQ_SOURCES array */

	irq_source = arg;

	/* Thanks to Joern for the correction and tips! */
	/* done this way to avoid wrong behaviour (endless loop) of the compiler in AMD platforms */
	temp=1;
    //mod_info("Waiting for interrupt ...");
	while (temp) {
		/* We wait here with an interruptible timeout. This will be interrupted
                 * by int.c:check_acknowledge_channel() as soon as in interrupt for
                 * the specified source arrives. */
		ret = wait_event_interruptible_timeout( (privdata->irq_queues[irq_source]), (atomic_read(&(privdata->irq_outstanding[irq_source])) > 0), (500*HZ/1000) ); // Timeout after 100ms
        if (ret == 0)
            mod_info("Wait for interrupt timed out!");

		//if (atomic_add_negative( -1, &(privdata->irq_outstanding[irq_source])) )
		//	atomic_inc( &(privdata->irq_outstanding[irq_source]) );
		//else
        if (ret != 0) atomic_dec(&(privdata->irq_outstanding[irq_source]));
	    temp =0;
	}
    //mod_info("Done waiting for interrupt!");
	return ret;
}

/**
 *
 * Clears the interrupt wait queue.
 *
 * @param arg Not a pointer, but the irq source (unsigned int)
 * @returns -EFAULT if the user specified an irq source out of range
 *
 */
static int ioctl_clear_ioq(specdriver_privdata_t *privdata, unsigned long arg)
{
	unsigned int irq_source;

	if (arg >= SPECDRIVER_INT_MAXSOURCES)
		return -EFAULT;

	irq_source = arg;
	atomic_set(&(privdata->irq_outstanding[irq_source]), 0);

	return 0;
}

/**
 *
 * This function handles all ioctl file operations.
 * Generally, the data of the ioctl is copied from userspace to kernelspace, a separate
 * function is called to handle the ioctl itself, then the data is copied back to userspace.
 *
 * @returns -EFAULT when an invalid memory pointer is passed
 *
 */
long specdriver_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	specdriver_privdata_t *privdata = filp->private_data;

	/* Select the appropiate command */
	switch (cmd) {
		case SPECDRIVER_IOC_MMAP_MODE:
			return ioctl_mmap_mode(privdata, arg);

		case SPECDRIVER_IOC_MMAP_AREA:
			return ioctl_mmap_area(privdata, arg);

		case SPECDRIVER_IOC_PCI_CFG_RD:
		case SPECDRIVER_IOC_PCI_CFG_WR:
			return ioctl_pci_config_read_write(privdata, cmd, arg);

//		case SPECDRIVER_IOC_PCI_INFO:
//			return ioctl_pci_info(privdata, arg);
//			return ioctl_pci_config_read_write(privdata, cmd, arg);

		case SPECDRIVER_IOC_PCI_INFO:
			return ioctl_pci_info(privdata, arg);

		case SPECDRIVER_IOC_KMEM_ALLOC:
			return ioctl_kmem_alloc(privdata, arg);

		case SPECDRIVER_IOC_KMEM_FREE:
			return ioctl_kmem_free(privdata, arg);

		case SPECDRIVER_IOC_KMEM_SYNC:
			return ioctl_kmem_sync(privdata, arg);

		case SPECDRIVER_IOC_UMEM_SGMAP:
			return ioctl_umem_sgmap(privdata, arg);

		case SPECDRIVER_IOC_UMEM_SGUNMAP:
			return ioctl_umem_sgunmap(privdata, arg);

		case SPECDRIVER_IOC_UMEM_SGGET:
			return ioctl_umem_sgget(privdata, arg);

		case SPECDRIVER_IOC_UMEM_SYNC:
			return ioctl_umem_sync(privdata, arg);

		case SPECDRIVER_IOC_WAITI:
			return ioctl_wait_interrupt(privdata, arg);

		case SPECDRIVER_IOC_CLEAR_IOQ:
			return ioctl_clear_ioq(privdata, arg);

		default:
			return -EINVAL;
	}
}
