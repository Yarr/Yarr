#la32_n
#la32_p
set_property PACKAGE_PIN F9 [get_ports {eudet_trig_p}]
set_property PACKAGE_PIN F8 [get_ports {eudet_trig_n}]
#la30_n
#la30_p
set_property PACKAGE_PIN H12 [get_ports {fe_clk_p[0]}]
set_property PACKAGE_PIN H11 [get_ports {fe_clk_n[0]}]
#la28_n
#la28_p
#set_property PACKAGE_PIN G12 [get_ports {fe_clk_p[2]}]
#set_property PACKAGE_PIN F12 [get_ports {fe_clk_n[2]}]
#la26_n
#la26_p
#set_property PACKAGE_PIN J13 [get_ports {fe_clk_p[3]}]
#set_property PACKAGE_PIN H13 [get_ports {fe_clk_n[3]}]
#la00_cc_n
#la00_cc_p
set_property PACKAGE_PIN R22 [get_ports {fe_data_p[1]}]
set_property PACKAGE_PIN R23 [get_ports {fe_data_n[1]}]
#la01_cc_n
#la01_cc_p
set_property PACKAGE_PIN P23 [get_ports {fe_cmd_p[0]}]
set_property PACKAGE_PIN N23 [get_ports {fe_cmd_n[0]}]
#la02_n
#la02_p
set_property PACKAGE_PIN AB26 [get_ports {fe_data_p[0]}]
set_property PACKAGE_PIN AC26 [get_ports {fe_data_n[0]}]
#la03_n
#la03_p
set_property PACKAGE_PIN N26 [get_ports {fe_data_p[2]}]
set_property PACKAGE_PIN M26 [get_ports {fe_data_n[2]}]
#la04_n
#la04_p
set_property PACKAGE_PIN W25 [get_ports {fe_data_p[3]}]
set_property PACKAGE_PIN W26 [get_ports {fe_data_n[3]}]
#la06_n
#la06_p
set_property PACKAGE_PIN W20 [get_ports {ext_trig_i_p[0]}]
set_property PACKAGE_PIN Y21 [get_ports {ext_trig_i_n[0]}]
#la05_n
#la05_p
#set_property PACKAGE_PIN M24 [get_ports {fe_data_p[3]}]
#set_property PACKAGE_PIN L24 [get_ports {fe_data_n[3]}]



#la33_n
#la33_p
set_property PACKAGE_PIN D8 [get_ports {eudet_rst_p}]
set_property PACKAGE_PIN D9 [get_ports {eudet_rst_n}]
#la31_n
#la31_p
set_property PACKAGE_PIN A9 [get_ports {eudet_clk_p}]
set_property PACKAGE_PIN A8 [get_ports {eudet_clk_n}]
#la29_n
#la29_p
set_property PACKAGE_PIN E11 [get_ports {eudet_busy_p}]
set_property PACKAGE_PIN D11 [get_ports {eudet_busy_n}]
#la27_n
#la27_p
#set_property PACKAGE_PIN B15 [get_ports {fe_cmd_p[3]}]
#set_property PACKAGE_PIN A15 [get_ports {fe_cmd_n[3]}]
#la07_n
#la07_p
#set_property PACKAGE_PIN V23 [get_ports {fe_cmd_p[7]}]
#set_property PACKAGE_PIN V24 [get_ports {fe_cmd_n[7]}]



#la24_n
#la24_p
#set_property PACKAGE_PIN D14 [get_ports {fe_data_p[0]}]
#set_property PACKAGE_PIN D13 [get_ports {fe_data_n[0]}]
#la23_n
#la23_p
#set_property PACKAGE_PIN G17 [get_ports {fe_data_p[1]}]
#set_property PACKAGE_PIN F18 [get_ports {fe_data_n[1]}]
#la11_n
#la11_p
#set_property PACKAGE_PIN T24 [get_ports {fe_data_p[2]}]
#set_property PACKAGE_PIN T25 [get_ports {fe_data_n[2]}]
#la10_n
#la10_p
set_property PACKAGE_PIN U22 [get_ports {ext_trig_i_p[3]}]
set_property PACKAGE_PIN V22 [get_ports {ext_trig_i_n[3]}]
#la21_n
#la21_p
set_property PACKAGE_PIN D15 [get_ports {scl_o}]
set_property PACKAGE_PIN D16 [get_ports {sda_o}]
#la22_n
#la22_p
set_property PACKAGE_PIN C17 [get_ports {latch_o}]
set_property PACKAGE_PIN C18 [get_ports {sdi_i}]
#la08_n
#la08_p
set_property PACKAGE_PIN K25 [get_ports {ext_trig_i_p[1]}]
set_property PACKAGE_PIN K26 [get_ports {ext_trig_i_n[1]}]
#la09_n
#la09_p
set_property PACKAGE_PIN G22 [get_ports {ext_trig_i_p[2]}]
set_property PACKAGE_PIN F23 [get_ports {ext_trig_i_n[2]}]

#la12_n
#set_property PACKAGE_PIN D24 [get_ports {io[0]}]
#la14_n
#set_property PACKAGE_PIN T19 [get_ports {io[1]}]
#la16_n
#set_property PACKAGE_PIN M20 [get_ports {io[2]}]

set_property IOSTANDARD LVDS_25 [get_ports fe_cmd_*]
set_property SLEW FAST [get_ports fe_cmd*]

set_property IOSTANDARD LVDS_25 [get_ports fe_data_*]
set_property DIFF_TERM TRUE [get_ports fe_data_*]
set_property IBUF_LOW_PWR FALSE [get_ports fe_data_*]

set_property IOSTANDARD LVDS_25 [get_ports fe_clk_*]
set_property SLEW FAST [get_ports fe_clk*]

set_property IOSTANDARD LVDS_25 [get_ports ext_trig_*]
set_property DIFF_TERM TRUE [get_ports ext_trig_*]

set_property IOSTANDARD LVDS_25 [get_ports eudet_*]

set_property IOSTANDARD LVCMOS25 [get_ports {scl_o}]
set_property IOSTANDARD LVCMOS25 [get_ports {sda_o}]
set_property IOSTANDARD LVCMOS25 [get_ports {latch_o}]
set_property IOSTANDARD LVCMOS25 [get_ports {sdi_i}]


#  Rising Edge Source Synchronous Outputs 
#
#  Source synchronous output interfaces can be constrained either by the max data skew
#  relative to the generated clock or by the destination device setup/hold requirements.
#
#  Max Skew Case:
#  The skew requirements for FPGA are known from system level analysis.
#
# forwarded                _____________        
# clock        ___________|             |_________
#                         |                        
#                 bre_skew|are_skew          
#                 <------>|<------>        
#           ______        |        ____________    
# data      ______XXXXXXXXXXXXXXXXX____________XXXXX
#
# Example of creating generated clock at clock output port
# create_generated_clock -name <gen_clock_name> -multiply_by 1 -source [get_pins <source_pin>] [get_ports <output_clock_port>]
# gen_clock_name is the name of forwarded clock here. It should be used below for defining "fwclk".	

set fwclk       	clk_160_s;	# forwarded clock name (generated using create_generated_clock at output clock port)
set fwclk_period 	6.25;	# forwarded clock period
set bre_skew 		-0.050;			# skew requirement before rising edge
set are_skew 		0.050;			# skew requirement after rising edge
set output_ports 	fe_cmd_*;	# list of output ports

# Output Delay Constraints
set_output_delay -clock $fwclk -max [expr $fwclk_period - $are_skew] [get_ports $output_ports];
set_output_delay -clock $fwclk -min $bre_skew                        [get_ports $output_ports];

# Report Timing Template
# report_timing -to [get_ports $output_ports] -max_paths 20 -nworst 1 -delay_type min_max -name src_sync_pos_out -file src_sync_pos_out.txt;
