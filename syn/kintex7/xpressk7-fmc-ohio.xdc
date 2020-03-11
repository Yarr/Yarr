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
set_property PACKAGE_PIN J13 [get_ports {fe_clk_p[3]}]
set_property PACKAGE_PIN H13 [get_ports {fe_clk_n[3]}]

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

#set_property LOC [get_ports {N22 clk0_c2m_n}]       
#set_property LOC [get_ports {N21 clk0_c2m_p}]       
#set_property LOC [get_ports {P21 clk0_m2c_n}]       
#set_property LOC [get_ports {R21 clk0_m2c_p}]       

#clk1_c2m
#set_property LOC [get_ports {AA22 clk1_c2m_n}]      
#set_property LOC [get_ports {Y22 clk1_c2m_p}]       

#clk1_m2c
#set_property LOC AA24 [get_ports {eudet_clk_n}]      
#set_property LOC Y23 [get_ports {eudet_clk_p}]       
set_property LOC AA24 [get_ports {ext_busy_o_n}]       
set_property LOC Y23 [get_ports {ext_busy_o_p}]       

# MGTs
#set_property LOC H1 [get_ports {dp0_c2m_n}]         
#set_property LOC H2 [get_ports {dp0_c2m_p}]         
#set_property LOC J3 [get_ports {dp0_m2c_n}]         
#set_property LOC J4 [get_ports {dp0_m2c_p}]         
#set_property LOC K1 [get_ports {dp1_c2m_n}]         
#set_property LOC K2 [get_ports {dp1_c2m_p}]         
#set_property LOC L3 [get_ports {dp1_m2c_n}]         
#set_property LOC L4 [get_ports {dp1_m2c_p}]         
#set_property LOC M1 [get_ports {dp2_c2m_n}]         
#set_property LOC M2 [get_ports {dp2_c2m_p}]         
#set_property LOC N3 [get_ports {dp2_m2c_n}]         
#set_property LOC N4 [get_ports {dp2_m2c_p}]         
#set_property LOC P1 [get_ports {dp3_c2m_n}]         
#set_property LOC P2 [get_ports {dp3_c2m_p}]         
#set_property LOC R3 [get_ports {dp3_m2c_n}]         
#set_property LOC R4 [get_ports {dp3_m2c_p}]         

#set_property LOC H5 [get_ports {gbtclk0_m2c_n}]     
#set_property LOC H6 [get_ports {gbtclk0_m2c_p}]     

#ha00
#set_property LOC AB24 [get_ports {eudet_busy_n}]       
#set_property LOC AA23 [get_ports {eudet_busy_p}]       

#ha01
#set_property LOC AC24 [get_ports {ha01_cc_n}]       
#set_property LOC AC23 [get_ports {ha01_cc_p}]       

#ha02
#set_property LOC P26  [get_ports {eudet_rst_n}]          
#set_property LOC R26  [get_ports {eudet_rst_p}]          

#ha03
#set_property LOC AD24 [get_ports {ha03_n}]          
#set_property LOC AD23 [get_ports {ha03_p}]          

#ha04
#set_property LOC AC22 [get_ports {ha04_n}]          
#set_property LOC AB22 [get_ports {ha04_p}]          

#ha05
#set_property LOC AC21 [get_ports {fe_data_n[19]}]          
#set_property LOC AB21 [get_ports {fe_data_p[19]}]          

#ha06
set_property LOC L25  [get_ports {ext_trig_i_n[0]}]          
set_property LOC M25  [get_ports {ext_trig_i_p[0]}]          
#set_property LOC L25  [get_ports {eudet_trig_n}]          
#set_property LOC M25  [get_ports {eudet_trig_p}]          

#ha07
#set_property LOC AB25 [get_ports {ha07_n}]          
#set_property LOC AA25 [get_ports {ha07_p}]          

#ha08
#set_property LOC W24  [get_ports {fe_data_n[17]}]          
#set_property LOC W23  [get_ports {fe_data_p[17]}]          

#ha09
#set_property LOC Y26  [get_ports {fe_data_n[22]}]          
#set_property LOC Y25  [get_ports {fe_data_p[22]}]          

#ha10
#set_property LOC F24  [get_ports {fe_data_n[18]}]          
#set_property LOC G24  [get_ports {fe_data_p[18]}]          

#ha11
#set_property LOC V26  [get_ports {ha11_n}]          
#set_property LOC U26  [get_ports {ha11_p}]          

#ha12
#set_property LOC U25  [get_ports {fe_data_n[21]}]          
#set_property LOC U24  [get_ports {fe_data_p[21]}]          

#ha13
#set_property LOC W21  [get_ports {fe_data_n[16]}]          
#set_property LOC V21  [get_ports {fe_data_p[16]}]          

#ha14
#set_property LOC M22  [get_ports {ha14_n}]          
#set_property LOC M21  [get_ports {ha14_p}]          

#ha15
#set_property LOC U20  [get_ports {fe_cmd_n[4]}]          
#set_property LOC U19  [get_ports {fe_cmd_p[4]}]          

#ha16
#set_property LOC P25  [get_ports {fe_cmd_n[5]}]          
#set_property LOC R25  [get_ports {fe_cmd_p[5]}]          

#ha17
#set_property LOC E23  [get_ports {fe_data_n[23]}]       
#set_property LOC F22  [get_ports {fe_data_p[23]}]       

#ha18
#set_property LOC N24  [get_ports {ha18_n}]          
#set_property LOC P24  [get_ports {ha18_p}]          

#ha19
#set_property LOC N17  [get_ports {fe_clk_n[4]}]          
#set_property LOC P16  [get_ports {fe_clk_p[4]}]          

#ha20
#set_property LOC T23  [get_ports {fe_data_n[20]}]          
#set_property LOC T22  [get_ports {fe_data_p[20]}]          

#ha21
#set_property LOC P20  [get_ports {fe_clk_p[5]}]          
#set_property LOC P19  [get_ports {fe_clk_n[5]}]          

#ha22
#set_property LOC B16  [get_ports {ha22_n}]          
#set_property LOC C16  [get_ports {ha22_p}]          

#ha23
#set_property LOC B19  [get_ports {ha23_n}]          
#set_property LOC C19  [get_ports {ha23_p}]          

#hb00
#set_property LOC H18  [get_ports {hb00_cc_n}]       
#set_property LOC H17  [get_ports {hb00_cc_p}]       

#hb01
#set_property LOC A19  [get_ports {hb01_n}]          
#set_property LOC A18  [get_ports {hb01_p}]          

#hb02
#set_property LOC R17  [get_ports {hb02_n}]          
#set_property LOC R16  [get_ports {hb02_p}]          

#hb03
#set_property LOC T17  [get_ports {fe_clk_n[6]}]          
#set_property LOC U17  [get_ports {fe_clk_p[6]}]          

#hb04
#set_property LOC G16  [get_ports {hb04_n}]          
#set_property LOC H16  [get_ports {hb04_p}]          

#hb05
#set_property LOC E16  [get_ports {fe_clk_n[7]}]          
#set_property LOC E15  [get_ports {fe_clk_p[7]}]          

#hb06
#set_property LOC C11  [get_ports {fe_data_n[26]}]       
#set_property LOC C12  [get_ports {fe_data_p[26]}]       

#hb07
#set_property LOC J16  [get_ports {hb07_n}]          
#set_property LOC J15  [get_ports {hb07_p}]          

#hb08
#set_property LOC F15  [get_ports {scl2_o}]          
#set_property LOC G15  [get_ports {sda2_o}]          

#hb09
#set_property LOC A14  [get_ports {fe_data_n[25]}]          
#set_property LOC B14  [get_ports {fe_data_p[25]}]          

#hb10
#set_property LOC D10  [get_ports {fe_data_n[24]}]          
#set_property LOC E10  [get_ports {fe_data_p[24]}]          

#hb11
#set_property LOC C13  [get_ports {hb11_n}]          
#set_property LOC C14  [get_ports {hb11_p}]          

#hb12
#set_property LOC F13  [get_ports {latch2_o}]          
#set_property LOC F14  [get_ports {hb12_p}]          

#hb13
#set_property LOC G14  [get_ports {fe_cmd_n[6]}]          
#set_property LOC H14  [get_ports {fe_cmd_p[6]}]          

#hb14
#set_property LOC A12  [get_ports {fe_data_n[30]}] 
#set_property LOC A13  [get_ports {fe_data_p[30]}]          

#hb15
#set_property LOC B9   [get_ports {hb15_n}]          
#set_property LOC C9   [get_ports {hb15_p}]          

#hb16
#set_property LOC J10  [get_ports {fe_data_n[27]}]          
#set_property LOC J11  [get_ports {fe_data_p[27]}]          

#hb17
#set_property LOC F10  [get_ports {fe_data_n[31]}]       
#set_property LOC G11  [get_ports {fe_data_p[31]}]       

#hb18
#set_property LOC H8   [get_ports {hb18_n}]          
#set_property LOC H9   [get_ports {hb18_p}]          

#hb19
#set_property LOC B11  [get_ports {fe_data_n[29]}]          
#set_property LOC B12  [get_ports {fe_data_p[29]}]          

#hb20
#set_property LOC A10  [get_ports {fe_data_n[28]}]          
#set_property LOC B10  [get_ports {fe_data_p[28]}]  

#hb21
#set_property LOC G9   [get_ports {fe_cmd_n[7]}]          
#set_property LOC G10  [get_ports {fe_cmd_p[7]}]  


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
set_property PACKAGE_PIN G22 [get_ports {fe_data_p[7]}]
set_property PACKAGE_PIN F23 [get_ports {fe_data_n[7]}]
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

#set_property IOSTANDARD DIFF_SSTL18_II [get_ports fe_data_*]
set_property IOSTANDARD LVDS_25 [get_ports fe_data_*]
set_property DIFF_TERM TRUE [get_ports fe_data_*]
set_property IBUF_LOW_PWR FALSE [get_ports fe_data_*]

set_property IOSTANDARD LVDS_25 [get_ports fe_cmd_*]
set_property SLEW FAST [get_ports fe_clk*]

set_property IOSTANDARD LVDS_25 [get_ports fe_clk_*]
set_property SLEW FAST [get_ports fe_clk*]

#set_property IOSTANDARD DIFF_SSTL18_II [get_ports eudet_trig*]
#set_property IOSTANDARD DIFF_SSTL18_II [get_ports eudet_clk*]
#set_property IOSTANDARD DIFF_SSTL18_II [get_ports eudet_busy*]
#set_property IOSTANDARD DIFF_SSTL18_II [get_ports eudet_rst*]

set_property IOSTANDARD LVDS_25 [get_ports ext_trig_*]
set_property DIFF_TERM TRUE [get_ports ext_trig_*]
set_property IOSTANDARD LVDS_25 [get_ports ext_busy_*]

set_property IOSTANDARD LVCMOS25 [get_ports {scl_o}]
set_property IOSTANDARD LVCMOS25 [get_ports {sda_o}]
set_property IOSTANDARD LVCMOS25 [get_ports {latch_o}]
set_property IOSTANDARD LVCMOS25 [get_ports {sdi_i}]

#set_property IOSTANDARD LVCMOS18 [get_ports {sda_io}]
#set_property IOSTANDARD LVCMOS18 [get_ports {scl_io}]

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











