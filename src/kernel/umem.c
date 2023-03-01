/**
 *
 * @file umem.c
 * @brief This file contains the functions handling user space memory.
 * @author Guillermo Marcus
 * @date 2009-04-05
 *
 */
#include <linux/version.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/list.h>
#include <linux/interrupt.h>
#include <linux/pci.h>
#include <linux/cdev.h>
#include <linux/wait.h>
#include <linux/mm.h>
#include <linux/pagemap.h>
#include <linux/sched.h>
// For Kernel 4.8
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,0,0)
    #include <linux/vmalloc.h>
#endif

#include "config.h"			/* compile-time configuration */
#include "compat.h"			/* compatibility definitions for older linux */
#include "specDriver.h"			/* external interface for the driver */
#include "common.h"		/* internal definitions for all parts */
#include "umem.h"		/* prototypes for kernel memory */
#include "sysfs.h"		/* prototypes for sysfs */

/**
 *
 * Reserve a new scatter/gather list and map it from memory to PCI bus addresses.
 *
 */
int specdriver_umem_sgmap(specdriver_privdata_t *privdata, umem_handle_t *umem_handle)
{
	int i = 0;
    int res = 0;
    int nr_pages = 0;
	struct page **pages = NULL;
	struct scatterlist *sg = NULL;
	specdriver_umem_entry_t *umem_entry = NULL;
	unsigned int nents = 0;
	unsigned long count = 0;
    unsigned long offset = 0;
    unsigned long length = 0;

	/*
	 * We do some checks first. Then, the following is necessary to create a
	 * Scatter/Gather list from a user memory area:
	 *  - Determine the number of pages
	 *  - Get the pages for the memory area
	 * 	- Lock them.
	 *  - Create a scatter/gather list of the pages
	 *  - Map the list from memory to PCI bus addresses
	 *
	 * Then, we:
	 *  - Create an entry on the umem list of the device, to cache the mapping.
	 *  - Create a sysfs attribute that gives easy access to the SG list
	 */

	/* zero-size?? */
	if (umem_handle->size == 0)
		return -EINVAL;

	/* Direction is better ignoring during mapping. */
	/* We assume bidirectional buffers always, except when sync'ing */

	/* calculate the number of pages */
	nr_pages = ((umem_handle->vma & ~PAGE_MASK) + umem_handle->size + ~PAGE_MASK) >> PAGE_SHIFT;

	mod_info_dbg("nr_pages computed: %u\n", nr_pages);

	/* Allocate space for the page information */
	/* This can be very big, so we use vmalloc */
	if ((pages = vmalloc(nr_pages * sizeof(*pages))) == NULL)
		return -ENOMEM;

	mod_info_dbg("allocated space for the pages.\n");

	/* Allocate space for the scatterlist */
	/* We do not know how many entries will be, but the maximum is nr_pages. */
	/* This can be very big, so we use vmalloc */
	if ((sg = vmalloc(nr_pages * sizeof(*sg))) == NULL)
		goto umem_sgmap_pages;

	sg_init_table(sg, nr_pages);

	mod_info_dbg("allocated space for the SG list.\n");

	/* Get the page information */
	mmap_read_lock(current->mm);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,6,0)
	res = get_user_pages_remote(
#else
	res = get_user_pages(
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,9,0) && LINUX_VERSION_CODE < KERNEL_VERSION(5,9,0)
				current,
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,9,0)
				current->mm,
				umem_handle->vma,
				nr_pages,
                FOLL_WRITE, //gup flags
				pages,
				NULL,
                NULL); // if there is an error here your version is between 4.9 and 4.10
#else
				current,
				current->mm,
				umem_handle->vma,
				nr_pages,
				1,
				0,  /* do not force, FIXME: shall I? */
				pages,
				NULL );
#endif
	mmap_read_unlock(current->mm);

	/* Error, not all pages mapped */
	if (res < (int)nr_pages) {
		mod_info("Could not map all user pages (%d of %d)\n", res, nr_pages);
		/* If only some pages could be mapped, we release those. If a real
		 * error occured, we set nr_pages to 0 */
		nr_pages = (res > 0 ? res : 0);
		goto umem_sgmap_unmap;
	}

	mod_info_dbg("Got the pages (%d).\n", res);

	/* Lock the pages, then populate the SG list with the pages */
	/* page0 is different */
	if ( !PageReserved(pages[0]) )
		compat_lock_page(pages[0]);

	offset = (umem_handle->vma & ~PAGE_MASK);
	length = (umem_handle->size > (PAGE_SIZE-offset) ? (PAGE_SIZE-offset) : umem_handle->size);

	sg_set_page(&sg[0], pages[0], length, offset);

	count = umem_handle->size - length;
	for(i=1;i<nr_pages;i++) {
		/* Lock page first */
		if ( !PageReserved(pages[i]) )
			compat_lock_page(pages[i]);

		/* Populate the list */
		sg_set_page(&sg[i], pages[i], ((count > PAGE_SIZE) ? PAGE_SIZE : count), 0);
		count -= sg[i].length;
	}

	/* Use the page list to populate the SG list */
	/* SG entries may be merged, res is the number of used entries */
	/* We have originally nr_pages entries in the sg list */
	if ((nents = pci_map_sg(privdata->pdev, sg, nr_pages, PCI_DMA_BIDIRECTIONAL)) == 0)
		goto umem_sgmap_unmap;

	mod_info_dbg("Mapped SG list (%d entries).\n", nents);

	/* Add an entry to the umem_list of the device, and update the handle with the id */
	/* Allocate space for the new umem entry */
	if ((umem_entry = kmalloc(sizeof(*umem_entry), GFP_KERNEL)) == NULL)
		goto umem_sgmap_entry;

	/* Fill entry to be added to the umem list */
	umem_entry->id = atomic_inc_return(&privdata->umem_count) - 1;
	umem_entry->nr_pages = nr_pages;	/* Will be needed when unmapping */
	umem_entry->pages = pages;
	umem_entry->nents = nents;
	umem_entry->sg = sg;

	if (specdriver_sysfs_initialize_umem(privdata, umem_entry->id, &(umem_entry->sysfs_attr)) != 0)
		goto umem_sgmap_name_fail;

	/* Add entry to the umem list */
	spin_lock( &(privdata->umemlist_lock) );
	list_add_tail( &(umem_entry->list), &(privdata->umem_list) );
	spin_unlock( &(privdata->umemlist_lock) );

	/* Update the Handle with the Handle ID of the entry */
	umem_handle->handle_id = umem_entry->id;

	return 0;

umem_sgmap_name_fail:
	kfree(umem_entry);
umem_sgmap_entry:
	pci_unmap_sg( privdata->pdev, sg, nr_pages, PCI_DMA_BIDIRECTIONAL );
umem_sgmap_unmap:
	/* release pages */
	if (nr_pages > 0) {
		for(i=0;i<nr_pages;i++) {
			if (PageLocked(pages[i]))
				compat_unlock_page(pages[i]);
			if (!PageReserved(pages[i]))
				set_page_dirty(pages[i]);
			// Kernel 4.8
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,0,0)
			put_page(pages[i]);
#else
			page_cache_release(pages[i]);
#endif
		}
	}
	vfree(sg);
umem_sgmap_pages:
	vfree(pages);
	return -ENOMEM;

}

/**
 *
 * Unmap a scatter/gather list
 *
 */
int specdriver_umem_sgunmap(specdriver_privdata_t *privdata, specdriver_umem_entry_t *umem_entry)
{
	int i = 0;
	specdriver_sysfs_remove(privdata, &(umem_entry->sysfs_attr));

	/* Unmap user memory */
	pci_unmap_sg( privdata->pdev, umem_entry->sg, umem_entry->nr_pages, PCI_DMA_BIDIRECTIONAL );

	/* Release the pages */
	if (umem_entry->nr_pages > 0) {
		for(i=0;i<(umem_entry->nr_pages);i++) {
			/* Mark pages as Dirty and unlock it */
			if ( !PageReserved( umem_entry->pages[i] )) {
				SetPageDirty( umem_entry->pages[i] );
				compat_unlock_page(umem_entry->pages[i]);
			}
			/* and release it from the cache */
			// Kernel 4.8
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,0,0)
			put_page( umem_entry->pages[i] );
#else
			page_cache_release( umem_entry->pages[i] );
#endif
		}
	}

	/* Remove the umem list entry */
	spin_lock( &(privdata->umemlist_lock) );
	list_del( &(umem_entry->list) );
	spin_unlock( &(privdata->umemlist_lock) );

	/* Release SG list and page list memory */
	/* These two are in the vm area of the kernel */
	vfree(umem_entry->pages);
	vfree(umem_entry->sg);

	/* Release umem_entry memory */
	kfree(umem_entry);

	return 0;
}

/**
 *
 * Unmap all scatter/gather lists.
 *
 */
int specdriver_umem_sgunmap_all(specdriver_privdata_t *privdata)
{
	struct list_head *ptr, *next;
	specdriver_umem_entry_t *umem_entry;

	/* iterate safely over the entries and delete them */
	list_for_each_safe( ptr, next, &(privdata->umem_list) ) {
		umem_entry = list_entry(ptr, specdriver_umem_entry_t, list );
		specdriver_umem_sgunmap( privdata, umem_entry ); 		/* spin lock inside! */
	}

	return 0;
}

/**
 *
 * Copies the scatter/gather list from kernelspace to userspace.
 *
 */
int specdriver_umem_sgget(specdriver_privdata_t *privdata, umem_sglist_t *umem_sglist)
{
	int i = 0;
	specdriver_umem_entry_t *umem_entry = NULL;
	struct scatterlist *sg = NULL;
	int idx = 0;
	dma_addr_t cur_addr = 0;
	unsigned int cur_size = 0;

	/* Find the associated umem_entry for this buffer */
	umem_entry = specdriver_umem_find_entry_id( privdata, umem_sglist->handle_id );
	if (umem_entry == NULL)
		return -EINVAL;					/* umem_handle is not valid */

	/* Check if passed SG list is enough */
	if (umem_sglist->nents < umem_entry->nents)
		return -EINVAL;					/* sg has not enough entries */

	/* Copy the SG list to the user format */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24)
	if (umem_sglist->type == SPECDRIVER_SG_MERGED) {
		for_each_sg(umem_entry->sg, sg, umem_entry->nents, i ) {
			if (i==0) {
				umem_sglist->sg[0].addr = sg_dma_address( sg );
				umem_sglist->sg[0].size = sg_dma_len( sg );
				idx = 0;
			}
			else {
				cur_addr = sg_dma_address( sg );
				cur_size = sg_dma_len( sg );

				/* Check if entry fits after current entry */
				if (cur_addr == (umem_sglist->sg[idx].addr + umem_sglist->sg[idx].size)) {
					umem_sglist->sg[idx].size += cur_size;
					continue;
				}

				/* Skip if the entry is zero-length (yes, it can happen.... at the end of the list) */
				if (cur_size == 0)
					continue;

				/* None of the above, add new entry */
				idx++;
				umem_sglist->sg[idx].addr = cur_addr;
				umem_sglist->sg[idx].size = cur_size;
			}
		}
		/* Set the used size of the SG list */
		umem_sglist->nents = idx+1;
	} else {
		for_each_sg(umem_entry->sg, sg, umem_entry->nents, i ) {
			mod_info_dbg("entry: %d\n",i);
			umem_sglist->sg[i].addr = sg_dma_address( sg );
			umem_sglist->sg[i].size = sg_dma_len( sg );
		}

		/* Set the used size of the SG list */
		/* Check if the last one is zero-length */
		if ( umem_sglist->sg[ umem_entry->nents - 1].size == 0)
			umem_sglist->nents = umem_entry->nents -1;
		else
			umem_sglist->nents = umem_entry->nents;
	}
#else
	if (umem_sglist->type == SPECDRIVER_SG_MERGED) {
		/* Merge entries that are contiguous into a single entry */
		/* Non-optimal but fast for most cases */
		/* First one always true */
		sg=umem_entry->sg;
		umem_sglist->sg[0].addr = sg_dma_address( sg );
		umem_sglist->sg[0].size = sg_dma_len( sg );
		sg++;
		idx = 0;

		/* Iterate over the SG entries */
		for(i=1; i< umem_entry->nents; i++, sg++ ) {
			cur_addr = sg_dma_address( sg );
			cur_size = sg_dma_len( sg );

			/* Check if entry fits after current entry */
			if (cur_addr == (umem_sglist->sg[idx].addr + umem_sglist->sg[idx].size)) {
				umem_sglist->sg[idx].size += cur_size;
				continue;
			}

			/* Skip if the entry is zero-length (yes, it can happen.... at the end of the list) */
			if (cur_size == 0)
				continue;

			/* None of the above, add new entry */
			idx++;
			umem_sglist->sg[idx].addr = cur_addr;
			umem_sglist->sg[idx].size = cur_size;
		}
		/* Set the used size of the SG list */
		umem_sglist->nents = idx+1;
	} else {
		/* Assume pci_map_sg made a good job (ehem..) and just copy it.
		 * actually, now I assume it just gives them plainly to me. */
		for(i=0, sg=umem_entry->sg ; i< umem_entry->nents; i++, sg++ ) {
			umem_sglist->sg[i].addr = sg_dma_address( sg );
			umem_sglist->sg[i].size = sg_dma_len( sg );
		}
		/* Set the used size of the SG list */
		/* Check if the last one is zero-length */
		if ( umem_sglist->sg[ umem_entry->nents - 1].size == 0)
			umem_sglist->nents = umem_entry->nents -1;
		else
			umem_sglist->nents = umem_entry->nents;
	}
#endif

	return 0;
}

/**
 *
 * Sync user space memory from/to device
 *
 */
int specdriver_umem_sync( specdriver_privdata_t *privdata, umem_handle_t *umem_handle )
{
	specdriver_umem_entry_t *umem_entry = NULL;

	/* Find the associated umem_entry for this buffer */
	umem_entry = specdriver_umem_find_entry_id( privdata, umem_handle->handle_id );
	if (umem_entry == NULL)
		return -EINVAL;					/* umem_handle is not valid */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,11)
	switch (umem_handle->dir) {
		case SPECDRIVER_DMA_TODEVICE:
			pci_dma_sync_sg_for_device( privdata->pdev, umem_entry->sg, umem_entry->nents, PCI_DMA_TODEVICE );
			break;
		case SPECDRIVER_DMA_FROMDEVICE:
			pci_dma_sync_sg_for_cpu( privdata->pdev, umem_entry->sg, umem_entry->nents, PCI_DMA_FROMDEVICE );
			break;
		case SPECDRIVER_DMA_BIDIRECTIONAL:
			pci_dma_sync_sg_for_device( privdata->pdev, umem_entry->sg, umem_entry->nents, PCI_DMA_BIDIRECTIONAL );
			pci_dma_sync_sg_for_cpu( privdata->pdev, umem_entry->sg, umem_entry->nents, PCI_DMA_BIDIRECTIONAL );
			break;
		default:
			return -EINVAL;				/* wrong direction parameter */
	}
#else
	switch (umem_handle->dir) {
		case SPECDRIVER_DMA_TODEVICE:
			pci_dma_sync_sg( privdata->pdev, umem_entry->sg, umem_entry->nents, PCI_DMA_TODEVICE );
			break;
		case SPECDRIVER_DMA_FROMDEVICE:
			pci_dma_sync_sg( privdata->pdev, umem_entry->sg, umem_entry->nents, PCI_DMA_FROMDEVICE );
			break;
		case SPECDRIVER_DMA_BIDIRECTIONAL:
			pci_dma_sync_sg( privdata->pdev, umem_entry->sg, umem_entry->nents, PCI_DMA_BIDIRECTIONAL );
			break;
		default:
			return -EINVAL;				/* wrong direction parameter */
	}
#endif

	return 0;
}

/*
 *
 * Get the specdriver_umem_entry_t structure for the given id.
 *
 * @param id ID of the umem entry to search for
 *
 */
specdriver_umem_entry_t *specdriver_umem_find_entry_id(specdriver_privdata_t *privdata, int id)
{
	struct list_head *ptr = NULL;
	specdriver_umem_entry_t *entry = NULL;

	spin_lock(&(privdata->umemlist_lock));
	list_for_each(ptr, &(privdata->umem_list)) {
		entry = list_entry(ptr, specdriver_umem_entry_t, list );

		if (entry->id == id) {
			spin_unlock( &(privdata->umemlist_lock) );
			return entry;
		}
	}

	spin_unlock(&(privdata->umemlist_lock));
	return NULL;
}
