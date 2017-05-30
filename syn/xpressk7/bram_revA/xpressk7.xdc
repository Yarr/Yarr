
create_clock -period 10.000 -name pcie_clk -waveform {0.000 5.000} [get_ports *pcie_clk*]

set_property IOSTANDARD LVDS [get_ports clk200_n]
set_property IOSTANDARD LVDS [get_ports clk200_p]
set_property PACKAGE_PIN AB10 [get_ports clk200_n]
set_property PACKAGE_PIN AA10 [get_ports clk200_p]

#PCIe signals
set_property PACKAGE_PIN D5 [get_ports pcie_clk_n]
set_property PACKAGE_PIN D6 [get_ports pcie_clk_p]

set_property LOC GTXE2_CHANNEL_X0Y7 [get_cells {pcie_0/U0/inst/gt_top_i/pipe_wrapper_i/pipe_lane[0].gt_wrapper_i/gtx_channel.gtxe2_channel_i}]
set_property PACKAGE_PIN B6 [get_ports {pci_exp_rxp[0]}]
set_property PACKAGE_PIN B5 [get_ports {pci_exp_rxn[0]}]
set_property LOC GTXE2_CHANNEL_X0Y6 [get_cells {pcie_0/U0/inst/gt_top_i/pipe_wrapper_i/pipe_lane[1].gt_wrapper_i/gtx_channel.gtxe2_channel_i}]
set_property PACKAGE_PIN C4 [get_ports {pci_exp_rxp[1]}]
set_property PACKAGE_PIN C3 [get_ports {pci_exp_rxn[1]}]
set_property LOC GTXE2_CHANNEL_X0Y5 [get_cells {pcie_0/U0/inst/gt_top_i/pipe_wrapper_i/pipe_lane[2].gt_wrapper_i/gtx_channel.gtxe2_channel_i}]
set_property PACKAGE_PIN E4 [get_ports {pci_exp_rxp[2]}]
set_property PACKAGE_PIN E3 [get_ports {pci_exp_rxn[2]}]
set_property LOC GTXE2_CHANNEL_X0Y4 [get_cells {pcie_0/U0/inst/gt_top_i/pipe_wrapper_i/pipe_lane[3].gt_wrapper_i/gtx_channel.gtxe2_channel_i}]
set_property PACKAGE_PIN G3 [get_ports {pci_exp_rxn[3]}]
set_property PACKAGE_PIN G4 [get_ports {pci_exp_rxp[3]}]

#set_property PACKAGE_PIN AB11 [get_ports clk_i]
set_property PACKAGE_PIN Y20 [get_ports rst_n_i]

set_property IOSTANDARD LVCMOS15 [get_ports rst_n_i]
#set_property IOSTANDARD LVCMOS15 [get_ports clk_i]

# On-board LEDs
set_property PACKAGE_PIN W10 [get_ports {usr_led_o[0]}]
set_property PACKAGE_PIN V11 [get_ports {usr_led_o[1]}]
set_property PACKAGE_PIN Y10 [get_ports {usr_led_o[2]}]
#set_property PACKAGE_PIN W13 [get_ports {usr_led_o[3]}]
set_property IOSTANDARD LVCMOS15 [get_ports {usr_led_o[0]}]
set_property IOSTANDARD LVCMOS15 [get_ports {usr_led_o[1]}]
set_property IOSTANDARD LVCMOS15 [get_ports {usr_led_o[2]}]
#set_property IOSTANDARD LVCMOS15 [get_ports {usr_led_o[3]}]

# On-board Switches
set_property PACKAGE_PIN AA15 [get_ports {usr_sw_i[0]}]
set_property PACKAGE_PIN V8 [get_ports {usr_sw_i[1]}]
set_property PACKAGE_PIN Y8 [get_ports {usr_sw_i[2]}]
set_property IOSTANDARD LVCMOS15 [get_ports {usr_sw_i[0]}]
set_property IOSTANDARD LVCMOS15 [get_ports {usr_sw_i[1]}]
set_property IOSTANDARD LVCMOS15 [get_ports {usr_sw_i[2]}]


#set_property IOSTANDARD LVCMOS15 [get_ports {front_led_o[3]}]
#set_property IOSTANDARD LVCMOS15 [get_ports {front_led_o[2]}]
#set_property IOSTANDARD LVCMOS15 [get_ports {front_led_o[1]}]
#set_property IOSTANDARD LVCMOS15 [get_ports {front_led_o[0]}]
#set_property PACKAGE_PIN T7 [get_ports {front_led_o[3]}]
#set_property PACKAGE_PIN V7 [get_ports {front_led_o[2]}]
#set_property PACKAGE_PIN U4 [get_ports {front_led_o[1]}]
#set_property PACKAGE_PIN V2 [get_ports {front_led_o[0]}]




set_property IOSTANDARD LVCMOS15 [get_ports sys_rst_n_i]
set_property PACKAGE_PIN U16 [get_ports sys_rst_n_i]

#create_clock -period 8.000 -name user_clk_out -waveform {0.000 4.000} [get_nets user_clk_out]


#create_clock -period 5.000 -name clk_200 -waveform {0.000 2.500} [get_ports *clk200*]

#connect_debug_port dbg_hub/clk [get_nets clk200]


set_property C_CLK_INPUT_FREQ_HZ 300000000 [get_debug_cores dbg_hub]
set_property C_ENABLE_CLK_DIVIDER false [get_debug_cores dbg_hub]
set_property C_USER_SCAN_CHAIN 1 [get_debug_cores dbg_hub]
connect_debug_port dbg_hub/clk [get_nets clk]
