#ifndef _SPECDRIVER_KMEM_H
#define _SPECDRIVER_KMEM_H

int specdriver_kmem_alloc( specdriver_privdata_t *privdata, kmem_handle_t *kmem_handle );
int specdriver_kmem_free(  specdriver_privdata_t *privdata, kmem_handle_t *kmem_handle );
int specdriver_kmem_sync(  specdriver_privdata_t *privdata, kmem_sync_t *kmem_sync );
int specdriver_kmem_free_all(  specdriver_privdata_t *privdata );
specdriver_kmem_entry_t *specdriver_kmem_find_entry( specdriver_privdata_t *privdata, kmem_handle_t *kmem_handle );
specdriver_kmem_entry_t *specdriver_kmem_find_entry_id( specdriver_privdata_t *privdata, int id );
int specdriver_kmem_free_entry( specdriver_privdata_t *privdata, specdriver_kmem_entry_t *kmem_entry );

#endif
