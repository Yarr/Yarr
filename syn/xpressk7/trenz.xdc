#Clocks
set_property IOSTANDARD DIFF_SSTL15 [get_ports {clk200_*}]
set_property PACKAGE_PIN AC9 [get_ports clk200_p]
set_property PACKAGE_PIN AD9 [get_ports clk200_n]

#PCIe Clock
set_property PACKAGE_PIN K6 [get_ports pcie_clk_p]
set_property PACKAGE_PIN K5 [get_ports pcie_clk_n]


#On-board switch reset
#set_property PACKAGE_PIN Y20 [get_ports rst_n_i]
#set_property IOSTANDARD LVCMOS25 [get_ports rst_n_i]

#PCIe reset
set_property IOSTANDARD LVCMOS25 [get_ports sys_rst_n_i]
set_property PACKAGE_PIN B20 [get_ports sys_rst_n_i]

#PCIe signals
set_property LOC GTXE2_CHANNEL_X0Y7 [get_cells {pcie_0/U0/inst/gt_top_i/pipe_wrapper_i/pipe_lane[0].gt_wrapper_i/gtx_channel.gtxe2_channel_i}]
set_property PACKAGE_PIN J3 [get_ports {pci_exp_rxn[0]}]
set_property PACKAGE_PIN J4 [get_ports {pci_exp_rxp[0]}]
set_property LOC GTXE2_CHANNEL_X0Y6 [get_cells {pcie_0/U0/inst/gt_top_i/pipe_wrapper_i/pipe_lane[1].gt_wrapper_i/gtx_channel.gtxe2_channel_i}]
set_property PACKAGE_PIN L3 [get_ports {pci_exp_rxn[1]}]
set_property PACKAGE_PIN L4 [get_ports {pci_exp_rxp[1]}]
set_property LOC GTXE2_CHANNEL_X0Y5 [get_cells {pcie_0/U0/inst/gt_top_i/pipe_wrapper_i/pipe_lane[2].gt_wrapper_i/gtx_channel.gtxe2_channel_i}]
set_property PACKAGE_PIN N3 [get_ports {pci_exp_rxn[2]}]
set_property PACKAGE_PIN N4 [get_ports {pci_exp_rxp[2]}]
set_property LOC GTXE2_CHANNEL_X0Y4 [get_cells {pcie_0/U0/inst/gt_top_i/pipe_wrapper_i/pipe_lane[3].gt_wrapper_i/gtx_channel.gtxe2_channel_i}]
set_property PACKAGE_PIN R3 [get_ports {pci_exp_rxn[3]}]
set_property PACKAGE_PIN R4 [get_ports {pci_exp_rxp[3]}]

# On-board LEDs
set_property PACKAGE_PIN B21 [get_ports {usr_led_o[0]}]
set_property IOSTANDARD LVCMOS25 [get_ports {usr_led_o[0]}]

set_property BITSTREAM.GENERAL.COMPRESS TRUE [current_design]
set_property BITSTREAM.CONFIG.CONFIGRATE 66 [current_design]
set_property CONFIG_VOLTAGE 1.8 [current_design]
set_property CFGBVS GND [current_design]
set_property CONFIG_MODE SPIx4 [current_design]
set_property BITSTREAM.CONFIG.SPI_32BIT_ADDR YES [current_design]
set_property BITSTREAM.CONFIG.SPI_BUSWIDTH 4 [current_design]
set_property BITSTREAM.CONFIG.M1PIN PULLNONE [current_design]
set_property BITSTREAM.CONFIG.M2PIN PULLNONE [current_design]
set_property BITSTREAM.CONFIG.M0PIN PULLNONE [current_design]
 
set_property BITSTREAM.CONFIG.USR_ACCESS TIMESTAMP [current_design]
