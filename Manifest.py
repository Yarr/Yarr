# To list file
# ls -1 | xargs -I % echo \"%\",

#modules = {"local" : ["../../../rtl/"],}
modules = {
"local" : ["rtl"],
"local" : ["ip-cores"],
}

files = [
#TOP
"syn/xpressk7/top_level.vhd",
"syn/xpressk7/xpressk7.xdc",
"syn/xpressk7/xpressk7-ddr3.xdc",
# IP cores
#"ip-cores/fifo_256x16/fifo_256x16.xci",
#"ip-cores/fifo_27x16/fifo_27x16.xci",
#"ip-cores/fifo_315x16/fifo_315x16.xci",
#"ip-cores/fifo_32x512/fifo_32x512.xci",
#"ip-cores/fifo_4x16/fifo_4x16.xci",
#"ip-cores/fifo_64x512/fifo_64x512.xci",
#"ip-cores/fifo_96x512_1/fifo_96x512.xci",
#"ip-cores/ila_axis/ila_axis.xci",
#"ip-cores/ila_ddr/ila_ddr.xci",
#"ip-cores/ila_dma_ctrl_reg/ila_dma_ctrl_reg.xci",
#"ip-cores/ila_l2p_dma/ila_l2p_dma.xci",
#"ip-cores/ila_pd_pdm/ila_pd_pdm.xci",
#"ip-cores/ila_wsh_pipe/ila_wsh_pipe.xci",
#"ip-cores/l2p_fifo64/l2p_fifo64.xci",
#"ip-cores/mig_7series_0/mig_7series_0.xci",
#"ip-cores/mig_7series_0/mig_a.prj",
#"ip-cores/mig_7series_0/mig_b.prj",
#"ip-cores/pcie_7x_0/pcie_7x_0.xci",
]

library = "work"


target = "xilinx" 
action = "synthesis" 

syn_device = "xc7k160" 
syn_grade = "-2" 
syn_package = "tfbg676" 
syn_top = "top_level" 
syn_project = "yarr"
syn_tool = "vivado"
