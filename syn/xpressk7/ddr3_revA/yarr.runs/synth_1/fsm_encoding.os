
 add_fsm_encoding \
       {p2l_decoder.state_s} \
       { }  \
       {{000 000} {001 001} {010 010} {011 011} {100 100} }

 add_fsm_encoding \
       {dma_controller.dma_ctrl_current_state} \
       { }  \
       {{000 000} {001 001} {010 010} {011 011} {100 100} {101 110} {110 101} }

 add_fsm_encoding \
       {p2l_dma_master.p2l_dma_current_state} \
       { }  \
       {{000 000} {001 001} {010 010} {011 011} {100 100} }

 add_fsm_encoding \
       {l2p_dma_master.l2p_dma_current_state} \
       { }  \
       {{000 000} {001 001} {010 010} {011 011} {101 100} {110 110} {111 101} }
