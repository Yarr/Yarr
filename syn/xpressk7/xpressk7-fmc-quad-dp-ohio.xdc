#Fixed polarity
#la25_p
#la25_n
#set_property PACKAGE_PIN E13 [get_ports {ext_trig_i_p[2]}] 
#set_property PACKAGE_PIN E12 [get_ports {ext_trig_i_n[2]}] 
set_property PACKAGE_PIN E13 [get_ports {fe_data_p[6]}] 
set_property PACKAGE_PIN E12 [get_ports {fe_data_n[6]}] 

#la29_p
#la29_n
#set_property PACKAGE_PIN E11 [get_ports {ext_trig_i_p[1]}] 
#set_property PACKAGE_PIN D11 [get_ports {ext_trig_i_n[1]}] 
set_property PACKAGE_PIN E11 [get_ports {fe_data_p[5]}] 
set_property PACKAGE_PIN D11 [get_ports {fe_data_n[5]}] 


#la22_p
#la22_n
#set_property PACKAGE_PIN C17 [get_ports {ext_trig_i_p[3]}] 
#set_property PACKAGE_PIN C18 [get_ports {ext_trig_i_n[3]}] 
set_property PACKAGE_PIN C17 [get_ports {fe_data_p[7]}] 
set_property PACKAGE_PIN C18 [get_ports {fe_data_n[7]}] 

#la21_p
#la21_n
#set_property PACKAGE_PIN D15 [get_ports {ext_trig_i_p[0]}] 
#set_property PACKAGE_PIN D16 [get_ports {ext_trig_i_n[0]}] 
set_property PACKAGE_PIN D15 [get_ports {fe_data_p[4]}] 
set_property PACKAGE_PIN D16 [get_ports {fe_data_n[4]}] 

#la15_p
#la15_n
set_property PACKAGE_PIN R18 [get_ports {fe_data_p[9]}] 
set_property PACKAGE_PIN P18 [get_ports {fe_data_n[9]}] 

#Wrong polarity in comment
#la32_n
#la32_p
set_property PACKAGE_PIN F9 [get_ports {fe_data_p[1]}]
set_property PACKAGE_PIN F8 [get_ports {fe_data_n[1]}]

#la30_n
#la30_p
set_property PACKAGE_PIN H12 [get_ports {fe_data_p[2]}]
set_property PACKAGE_PIN H11 [get_ports {fe_data_n[2]}]

#la28_n
#la28_p
set_property PACKAGE_PIN G12 [get_ports {fe_data_p[3]}]
set_property PACKAGE_PIN F12 [get_ports {fe_data_n[3]}]

#la26_n
#la26_p
#set_property PACKAGE_PIN J13 [get_ports {fe_clk_p[3]}]
#set_property PACKAGE_PIN H13 [get_ports {fe_clk_n[3]}]

#la00_cc_n
#la00_cc_p
#set_property PACKAGE_PIN R22 [get_ports {eudet_rst_p}]
#set_property PACKAGE_PIN R23 [get_ports {eudet_rst_n}]
set_property PACKAGE_PIN R22 [get_ports {fe_data_p[15]}]
set_property PACKAGE_PIN R23 [get_ports {fe_data_n[15]}]

#la01_cc_n
#la01_cc_p
#set_property PACKAGE_PIN P23 [get_ports {fe_cmd_p[0]}]
#set_property PACKAGE_PIN N23 [get_ports {fe_cmd_n[0]}]

#la02_n
#la02_p
#set_property PACKAGE_PIN AB26 [get_ports {eudet_clk_p}]
#set_property PACKAGE_PIN AC26 [get_ports {eudet_clk_n}]
#set_property PACKAGE_PIN AB26 [get_ports {ext_busy_o_p}]
#set_property PACKAGE_PIN AC26 [get_ports {ext_busy_o_n}]
set_property PACKAGE_PIN AB26 [get_ports {fe_data_p[12]}]
set_property PACKAGE_PIN AC26 [get_ports {fe_data_p[12]}]

#la03_n
#la03_p
#set_property PACKAGE_PIN N26 [get_ports {eudet_trig_p}]
#set_property PACKAGE_PIN M26 [get_ports {eudet_trig_n}]
#set_property PACKAGE_PIN N26 [get_ports {ext_trig_i_p[0]}]
#set_property PACKAGE_PIN M26 [get_ports {ext_trig_i_n[0]}]
set_property PACKAGE_PIN N26 [get_ports {fe_data_p[14]}]
set_property PACKAGE_PIN M26 [get_ports {fe_data_n[14]}]

#la04_n
#la04_p
set_property PACKAGE_PIN W25 [get_ports {fe_cmd_p[3]}]
set_property PACKAGE_PIN W26 [get_ports {fe_cmd_n[3]}]

#la06_n
#la06_p
set_property PACKAGE_PIN W20 [get_ports {fe_clk_p[1]}]
set_property PACKAGE_PIN Y21 [get_ports {fe_clk_n[1]}]
#la33_n
#la33_p
set_property PACKAGE_PIN D9 [get_ports {fe_cmd_p[0]}]
set_property PACKAGE_PIN D8 [get_ports {fe_cmd_n[0]}]
#la05_n
#la05_p
set_property PACKAGE_PIN M24 [get_ports {latch_o}]
set_property PACKAGE_PIN L24 [get_ports {sdi_i}]




#la31_n
#la31_p
set_property PACKAGE_PIN A9 [get_ports {fe_data_p[0]}]
set_property PACKAGE_PIN A8 [get_ports {fe_data_n[0]}]
#la27_n
#la27_p
set_property PACKAGE_PIN B15 [get_ports {fe_clk_p[0]}]
set_property PACKAGE_PIN A15 [get_ports {fe_clk_n[0]}]
#la07_n
#la07_p
set_property PACKAGE_PIN V23 [get_ports {fe_data_p[11]}]
set_property PACKAGE_PIN V24 [get_ports {fe_data_n[11]}]

#la24_n
#la24_p
set_property PACKAGE_PIN D14 [get_ports {fe_cmd_p[1]}]
set_property PACKAGE_PIN D13 [get_ports {fe_cmd_n[1]}]

#la23_n
#la23_p
set_property PACKAGE_PIN G17 [get_ports {fe_clk_p[2]}]
set_property PACKAGE_PIN F18 [get_ports {fe_clk_n[2]}]

#la11_n
#la11_p
set_property PACKAGE_PIN T24 [get_ports {fe_data_p[10]}]
set_property PACKAGE_PIN T25 [get_ports {fe_data_n[10]}]

#la10_n
#la10_p
set_property PACKAGE_PIN U22 [get_ports {fe_clk_p[3]}]
set_property PACKAGE_PIN V22 [get_ports {fe_clk_n[3]}]

#la08_n
#la08_p
#set_property PACKAGE_PIN K25 [get_ports {eudet_busy_p}]
#set_property PACKAGE_PIN K26 [get_ports {eudet_busy_n}]
set_property PACKAGE_PIN K25 [get_ports {fe_data_p[13]}]
set_property PACKAGE_PIN K26 [get_ports {fe_data_n[13]}]

#la09_n
#la09_p
#set_property PACKAGE_PIN G22 [get_ports {fe_data_p[7]}]
#set_property PACKAGE_PIN F23 [get_ports {fe_data_n[7]}]
#la13_n
#la13_p
set_property PACKAGE_PIN T20 [get_ports {sda_o}]
set_property PACKAGE_PIN R20 [get_ports {scl_o}]

#la12_p
#la12_n
set_property PACKAGE_PIN D24 [get_ports {fe_data_n[8]}]
set_property PACKAGE_PIN D23 [get_ports {fe_data_p[8]}]

#la14_n
#set_property PACKAGE_PIN T19 [get_ports {io[1]}]

#la16_n
#la16_p
set_property PACKAGE_PIN N19 [get_ports {fe_cmd_p[2]}]
set_property PACKAGE_PIN M20 [get_ports {fe_cmd_n[2]}]

#set_property IOSTANDARD LVDS_25 [get_ports fe_cmd_*]
#set_property SLEW FAST [get_ports fe_cmd*]

#hb09_p 
#hb09_n
#set_property PACKAGE_PIN B14 [get_ports {ext_trig_i_p[0]}]
#set_property PACKAGE_PIN A14 [get_ports {ext_trig_i_n[0]}]

#ha03_p
#ha03_n
#set_property PACKAGE_PIN U17 [get_ports {ext_busy_o_p}]
#set_property PACKAGE_PIN T17 [get_ports {ext_busy_o_n}]

set_property PACKAGE_PIN N16 [get_ports {sda_io}]
set_property PACKAGE_PIN J14 [get_ports {scl_io}]

set_property IOSTANDARD LVDS_25 [get_ports fe_data_*]
set_property DIFF_TERM TRUE [get_ports fe_data_*]
set_property IBUF_LOW_PWR FALSE [get_ports fe_data_*]

set_property IOSTANDARD LVDS_25 [get_ports fe_clk_*]
set_property SLEW FAST [get_ports fe_clk*]

set_property IOSTANDARD LVDS_25 [get_ports eudet_*]
set_property IOSTANDARD LVDS_25 [get_ports ext_trig_*]
set_property DIFF_TERM TRUE [get_ports ext_trig_*]
set_property IOSTANDARD LVDS_25 [get_ports ext_busy_*]

set_property IOSTANDARD LVCMOS25 [get_ports {scl_o}]
set_property IOSTANDARD LVCMOS25 [get_ports {sda_o}]
set_property IOSTANDARD LVCMOS25 [get_ports {latch_o}]
set_property IOSTANDARD LVCMOS25 [get_ports {sdi_i}]

set_property IOSTANDARD LVCMOS25 [get_ports {sda_io}]
set_property IOSTANDARD LVCMOS25 [get_ports {scl_io}]

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











