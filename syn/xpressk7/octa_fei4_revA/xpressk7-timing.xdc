create_clock -period 10.000 -name pcie_clk -waveform {0.000 5.000} [get_ports *pcie_clk*]

set_max_delay -from [get_clocks -of_objects [get_pins app_0/clk_gen_cmp/inst/mmcm_adv_inst/CLKOUT1]] -to [get_clocks -of_objects [get_pins app_0/clk_gen_cmp/inst/mmcm_adv_inst/CLKOUT5]] 2.000
set_max_delay -from [get_clocks -of_objects [get_pins app_0/clk_gen_cmp/inst/mmcm_adv_inst/CLKOUT3]] -to [get_clocks -of_objects [get_pins app_0/clk_gen_cmp/inst/mmcm_adv_inst/CLKOUT5]] 2.000
set_max_delay -from [get_clocks -of_objects [get_pins app_0/clk_gen_cmp/inst/mmcm_adv_inst/CLKOUT3]] -to [get_clocks -of_objects [get_pins app_0/clk_gen_cmp/inst/mmcm_adv_inst/CLKOUT1]] 3.125
set_max_delay -from [get_clocks -of_objects [get_pins app_0/clk_gen_cmp/inst/mmcm_adv_inst/CLKOUT5]] -to [get_clocks -of_objects [get_pins app_0/clk_gen_cmp/inst/mmcm_adv_inst/CLKOUT1]] 3.125
set_max_delay -from [get_clocks -of_objects [get_pins app_0/clk_gen_cmp/inst/mmcm_adv_inst/CLKOUT1]] -to [get_clocks -of_objects [get_pins app_0/clk_gen_cmp/inst/mmcm_adv_inst/CLKOUT3]] 6.12
set_max_delay -from [get_clocks -of_objects [get_pins app_0/clk_gen_cmp/inst/mmcm_adv_inst/CLKOUT5]] -to [get_clocks -of_objects [get_pins app_0/clk_gen_cmp/inst/mmcm_adv_inst/CLKOUT3]] 6.12


set_false_path -to [get_pins -hierarchical *pcie_id_s*D*]

set_false_path -through [get_nets -hierarchical *cfg_interrupt_o*]

set_max_delay -through [get_nets {app_0/cfg_interrupt_rdy_s[0]}] 8.000

set_property C_CLK_INPUT_FREQ_HZ 300000000 [get_debug_cores dbg_hub]
set_property C_ENABLE_CLK_DIVIDER false [get_debug_cores dbg_hub]
set_property C_USER_SCAN_CHAIN 1 [get_debug_cores dbg_hub]
connect_debug_port dbg_hub/clk [get_nets aclk]
