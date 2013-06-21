#ifndef _SPECDRIVER_UMEM_H
#define _SPECDRIVER_UMEM_H

int specdriver_umem_sgmap( specdriver_privdata_t *privdata, umem_handle_t *umem_handle );
int specdriver_umem_sgunmap( specdriver_privdata_t *privdata, specdriver_umem_entry_t *umem_entry );
int specdriver_umem_sgget( specdriver_privdata_t *privdata, umem_sglist_t *umem_sglist );
int specdriver_umem_sync( specdriver_privdata_t *privdata, umem_handle_t *umem_handle );
specdriver_umem_entry_t *specdriver_umem_find_entry_id( specdriver_privdata_t *privdata, int id );

#endif
