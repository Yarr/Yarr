create_clock -period 10.000 -name pcie_clk -waveform {0.000 5.000} [get_ports *pcie_clk*]

set_max_delay -from [get_clocks -of_objects [get_pins app_0/clk_gen_cmp/inst/mmcm_adv_inst/CLKOUT1]] -to [get_clocks -of_objects [get_pins app_0/clk_gen_cmp/inst/mmcm_adv_inst/CLKOUT5]] 3.000
set_max_delay -from [get_clocks -of_objects [get_pins app_0/clk_gen_cmp/inst/mmcm_adv_inst/CLKOUT3]] -to [get_clocks -of_objects [get_pins app_0/clk_gen_cmp/inst/mmcm_adv_inst/CLKOUT5]] 3.000
set_max_delay -from [get_clocks -of_objects [get_pins app_0/clk_gen_cmp/inst/mmcm_adv_inst/CLKOUT3]] -to [get_clocks -of_objects [get_pins app_0/clk_gen_cmp/inst/mmcm_adv_inst/CLKOUT1]] 3.125
set_max_delay -from [get_clocks -of_objects [get_pins app_0/clk_gen_cmp/inst/mmcm_adv_inst/CLKOUT5]] -to [get_clocks -of_objects [get_pins app_0/clk_gen_cmp/inst/mmcm_adv_inst/CLKOUT1]] 3.125
set_max_delay -from [get_clocks -of_objects [get_pins app_0/clk_gen_cmp/inst/mmcm_adv_inst/CLKOUT1]] -to [get_clocks -of_objects [get_pins app_0/clk_gen_cmp/inst/mmcm_adv_inst/CLKOUT3]] 6.120
set_max_delay -from [get_clocks -of_objects [get_pins app_0/clk_gen_cmp/inst/mmcm_adv_inst/CLKOUT5]] -to [get_clocks -of_objects [get_pins app_0/clk_gen_cmp/inst/mmcm_adv_inst/CLKOUT3]] 6.120


set_false_path -to [get_pins -hierarchical *pcie_id_s*D*]




#set_false_path -to [get_pins -hierarchical cfg_interrupt_s_reg/CLR]

#set_max_delay -from [get_pins app_0/wb_exp_comp/cfg_interrupt_s_reg/C] 2.000
set_false_path -from [get_pins app_0/wb_exp_comp/cfg_interrupt_s_reg/C]
set_false_path -from [get_ports rst_n_i]


#set_max_delay -through [get_nets -hierarchical {cfg_interrupt_rdy_s_reg[0]_C}] 2.000

set_max_delay -from [get_clocks -of_objects [get_pins pcie_0/U0/inst/gt_top_i/pipe_wrapper_i/pipe_clock_int.pipe_clock_i/mmcm_i/CLKOUT2]] -to [get_clocks -of_objects [get_pins app_0/clk_gen_cmp/inst/mmcm_adv_inst/CLKOUT5]] 3.000
set_property C_CLK_INPUT_FREQ_HZ 300000000 [get_debug_cores dbg_hub]
set_property C_ENABLE_CLK_DIVIDER false [get_debug_cores dbg_hub]
set_property C_USER_SCAN_CHAIN 1 [get_debug_cores dbg_hub]
connect_debug_port dbg_hub/clk [get_nets clk]
