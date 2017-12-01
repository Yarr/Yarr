# ------------------------------------------
#  FMC slot - VHDCI to FE65-P2 Adapter revA 
# ------------------------------------------


# LA00_P: DAC_LD
set_property PACKAGE_PIN R22 [get_ports {dac_ld}]
set_property IOSTANDARD LVCMOS25 [get_ports {dac_ld}]

# LA00_N: INJ_SW
set_property PACKAGE_PIN R23 [get_ports {inj_sw}]
set_property IOSTANDARD LVCMOS25 [get_ports {inj_sw}]

# LA01_P: DAC_DIN
set_property PACKAGE_PIN P23 [get_ports {dac_din}]
set_property IOSTANDARD LVCMOS25 [get_ports {dac_din}]

# LA01_P: DAC_CLK
set_property PACKAGE_PIN N23 [get_ports {dac_clk}]
set_property IOSTANDARD LVCMOS25 [get_ports {dac_clk}]

# LA02_N: DAC_CS
set_property PACKAGE_PIN AC26 [get_ports {dac_cs}]
set_property IOSTANDARD LVCMOS25 [get_ports {dac_cs}]

# LA03_P: TRIGGER_N
set_property PACKAGE_PIN N26 [get_ports {trigger_n}]
set_property IOSTANDARD LVDS_25 [get_ports {trigger_n}]

# LA03_N: TRIGGER_P
set_property PACKAGE_PIN M26 [get_ports {trigger_p}]
set_property IOSTANDARD LVDS_25 [get_ports {trigger_p}]

# LA05_P: CLK_DATA_P
set_property PACKAGE_PIN M24 [get_ports {clk_data_p}]
set_property IOSTANDARD LVDS_25 [get_ports {clk_data_p}]

# LA05_N: CLK_DATA_N
set_property PACKAGE_PIN L24 [get_ports {clk_data_n}]
set_property IOSTANDARD LVDS_25 [get_ports {clk_data_n}]

# LA07_P: RST_0_N
set_property PACKAGE_PIN V23 [get_ports {rst_0_n}]
set_property IOSTANDARD LVDS_25 [get_ports {rst_0_n}]

# LA07_N: RST_0_P
set_property PACKAGE_PIN V24 [get_ports {rst_0_p}]
set_property IOSTANDARD LVDS_25 [get_ports {rst_0_p}]

# LA09_P: CLK_CNFG_N
set_property PACKAGE_PIN G22 [get_ports {clk_cnfg_n}]
set_property IOSTANDARD LVDS_25 [get_ports {clk_cnfg_n}]

# LA09_N: CLK_CNFG_P
set_property PACKAGE_PIN F23 [get_ports {clk_cnfg_p}]
set_property IOSTANDARD LVDS_25 [get_ports {clk_cnfg_p}]

# LA11_P: PIX_D_CNFG_P
set_property PACKAGE_PIN T24 [get_ports {pix_d_cnfg_p}]
set_property IOSTANDARD LVDS_25 [get_ports {pix_d_cnfg_p}]

# LA11_N: PIX_D_CNFG_N
set_property PACKAGE_PIN T25 [get_ports {pix_d_cnfg_n}]
set_property IOSTANDARD LVDS_25 [get_ports {pix_d_cnfg_n}]

# LA13_P: LD_CNFG_P
set_property PACKAGE_PIN T20 [get_ports {ld_cnfg_p}]
set_property IOSTANDARD LVDS_25 [get_ports {ld_cnfg_p}]

# LA13_N: LD_CNFG_N
set_property PACKAGE_PIN R20 [get_ports {ld_cnfg_n}]
set_property IOSTANDARD LVDS_25 [get_ports {ld_cnfg_n}]

# LA15_P: CLK_BX_P
set_property PACKAGE_PIN R18 [get_ports {clk_bx_p}]
set_property IOSTANDARD LVDS_25 [get_ports {clk_bx_p}]

# LA15_N: CLK_BX_N
set_property PACKAGE_PIN P18 [get_ports {clk_bx_n}]
set_property IOSTANDARD LVDS_25 [get_ports {clk_bx_n}]

# LA17_P: RST_1_N
set_property PACKAGE_PIN E18 [get_ports {rst_1_n}]
set_property IOSTANDARD LVDS_25 [get_ports {rst_1_n}]

# LA17_N: RST_1_P
set_property PACKAGE_PIN D18 [get_ports {rst_1_p}]
set_property IOSTANDARD LVDS_25 [get_ports {rst_1_p}]

# LA19_P: EN_PIX_SR_CNFG_N
set_property PACKAGE_PIN B17 [get_ports {en_pix_sr_cnfg_n}]
set_property IOSTANDARD LVDS_25 [get_ports {en_pix_sr_cnfg_n}]

# LA19_N: EN_PIX_SR_CNFG_P
set_property PACKAGE_PIN A17 [get_ports {en_pix_sr_cnfg_p}]
set_property IOSTANDARD LVDS_25 [get_ports {en_pix_sr_cnfg_p}]

# LA21_P: SI_CNFG_P
set_property PACKAGE_PIN D15 [get_ports {si_cnfg_p}]
set_property IOSTANDARD LVDS_25 [get_ports {si_cnfg_p}]

# LA21_N: SI_CNFG_N
set_property PACKAGE_PIN D16 [get_ports {si_cnfg_n}]
set_property IOSTANDARD LVDS_25 [get_ports {si_cnfg_n}]

# LA27_P: HIT_OR_P
set_property PACKAGE_PIN B15 [get_ports {hit_or_p}]
set_property IOSTANDARD LVDS_25 [get_ports {hit_or_p}]

# LA27_N: HIT_OR_N
set_property PACKAGE_PIN A15 [get_ports {hit_or_n}]
set_property IOSTANDARD LVDS_25 [get_ports {hit_or_n}]

# LA28_P: EXT_4_P
set_property PACKAGE_PIN G12 [get_ports {ext_4_p}]
set_property IOSTANDARD LVDS_25 [get_ports {ext_4_p}]

# LA28_N: EXT_4_N
set_property PACKAGE_PIN F12 [get_ports {ext_4_n}]
set_property IOSTANDARD LVDS_25 [get_ports {ext_4_n}]

# LA29_P: EXT_3_P
set_property PACKAGE_PIN E11 [get_ports {ext_3_p}]
set_property IOSTANDARD LVDS_25 [get_ports {ext_3_p}]

# LA29_N: EXT_3_N
set_property PACKAGE_PIN D11 [get_ports {ext_3_n}]
set_property IOSTANDARD LVDS_25 [get_ports {ext_3_n}]

# LA30_P: EXT_2_P
set_property PACKAGE_PIN H12 [get_ports {ext_2_p}]
set_property IOSTANDARD LVDS_25 [get_ports {ext_2_p}]

# LA30_N: EXT_2_N
set_property PACKAGE_PIN H11 [get_ports {ext_2_n}]
set_property IOSTANDARD LVDS_25 [get_ports {ext_2_n}]

# LA31_P: OUT_DATA_P
set_property PACKAGE_PIN A9 [get_ports {out_data_p}]
set_property IOSTANDARD LVDS_25 [get_ports {out_data_p}]

# LA31_N: OUT_DATA_N
set_property PACKAGE_PIN A8 [get_ports {out_data_n}]
set_property IOSTANDARD LVDS_25 [get_ports {out_data_n}]

# LA32_P: EXT_1_P
set_property PACKAGE_PIN F9 [get_ports {ext_1_p}]
set_property IOSTANDARD LVDS_25 [get_ports {ext_1_p}]

# LA32_N: EXT_1_N
set_property PACKAGE_PIN F8 [get_ports {ext_1_n}]
set_property IOSTANDARD LVDS_25 [get_ports {ext_1_n}]

# LA33_P: IO_0
set_property PACKAGE_PIN D9 [get_ports {IO_0}]
set_property IOSTANDARD LVCMOS25 [get_ports {IO_0}]

# LA33_N: IO_1
set_property PACKAGE_PIN D8 [get_ports {IO_1}]
set_property IOSTANDARD LVCMOS25 [get_ports {IO_1}]
