#ifndef _SPECDRIVER_INT_H 
#define _SPECDRIVER_INT_H

int specdriver_probe_irq(specdriver_privdata_t *privdata);
void specdriver_remove_irq(specdriver_privdata_t *privdata);
void specdriver_irq_unmap_bars(specdriver_privdata_t *privdata);
int specdriver_ack_irq(specdriver_privdata_t *privdata);
irqreturn_t specdriver_irq_handler(int irq, void *dev_id);

#endif
