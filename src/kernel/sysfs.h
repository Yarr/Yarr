#ifndef _PCIDRIVER_SYSFS_H
#define _PCIDRIVER_SYSFS_H

int specdriver_sysfs_initialize_kmem(specdriver_privdata_t *privdata, int id, struct class_device_attribute *sysfs_attr);
int specdriver_sysfs_initialize_umem(specdriver_privdata_t *privdata, int id, struct class_device_attribute *sysfs_attr);
void specdriver_sysfs_remove(specdriver_privdata_t *privdata, struct class_device_attribute *sysfs_attr);

/* prototypes for sysfs operations */
SYSFS_GET_FUNCTION(specdriver_show_irq_count);
SYSFS_GET_FUNCTION(specdriver_show_irq_queues);
SYSFS_GET_FUNCTION(specdriver_show_mmap_mode);
SYSFS_SET_FUNCTION(specdriver_store_mmap_mode);
SYSFS_GET_FUNCTION(specdriver_show_mmap_area);
SYSFS_SET_FUNCTION(specdriver_store_mmap_area);
SYSFS_GET_FUNCTION(specdriver_show_kmem_count);
SYSFS_GET_FUNCTION(specdriver_show_kbuffers);
SYSFS_SET_FUNCTION(specdriver_store_kmem_alloc);
SYSFS_SET_FUNCTION(specdriver_store_kmem_free);
SYSFS_GET_FUNCTION(specdriver_show_umappings);
SYSFS_SET_FUNCTION(specdriver_store_umem_unmap);

#endif
