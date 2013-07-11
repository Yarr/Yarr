--*****************************************************************************
-- (c) Copyright 2009 Xilinx, Inc. All rights reserved.
--
-- This file contains confidential and proprietary information
-- of Xilinx, Inc. and is protected under U.S. and
-- international copyright and other intellectual property
-- laws.
--
-- DISCLAIMER
-- This disclaimer is not a license and does not grant any
-- rights to the materials distributed herewith. Except as
-- otherwise provided in a valid license issued to you by
-- Xilinx, and to the maximum extent permitted by applicable
-- law: (1) THESE MATERIALS ARE MADE AVAILABLE "AS IS" AND
-- WITH ALL FAULTS, AND XILINX HEREBY DISCLAIMS ALL WARRANTIES
-- AND CONDITIONS, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
-- BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, NON-
-- INFRINGEMENT, OR FITNESS FOR ANY PARTICULAR PURPOSE; and
-- (2) Xilinx shall not be liable (whether in contract or tort,
-- including negligence, or under any other theory of
-- liability) for any loss or damage of any kind or nature
-- related to, arising under or in connection with these
-- materials, including for any direct, or any indirect,
-- special, incidental, or consequential loss or damage
-- (including loss of data, profits, goodwill, or any type of
-- loss or damage suffered as a result of any action brought
-- by a third party) even if such damage or loss was
-- reasonably foreseeable or Xilinx had been advised of the
-- possibility of the same.
--
-- CRITICAL APPLICATIONS
-- Xilinx products are not designed or intended to be fail-
-- safe, or for use in any application requiring fail-safe
-- performance, such as life-support or safety devices or
-- systems, Class III medical devices, nuclear facilities,
-- applications related to the deployment of airbags, or any
-- other applications that could lead to death, personal
-- injury, or severe property or environmental damage
-- (individually and collectively, "Critical
-- Applications"). Customer assumes the sole risk and
-- liability of any use of Xilinx products in Critical
-- Applications, subject only to applicable laws and
-- regulations governing limitations on product liability.
--
-- THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS
-- PART OF THIS FILE AT ALL TIMES.
--
--*****************************************************************************
--   ____  ____
--  /   /\/   /
-- /___/  \  /    Vendor             : Xilinx
-- \   \   \/     Version            : 3.9
--  \   \         Application        : MIG
--  /   /         Filename           : ddr3_ctrl_svec_bank4_64b_32b.vhd
-- /___/   /\     Date Last Modified : $Date: 2011/06/02 07:16:59 $
-- \   \  /  \    Date Created       : Jul 03 2009
--  \___\/\___\
--
--Device           : Spartan-6
--Design Name      : DDR/DDR2/DDR3/LPDDR 
--Purpose          : This is the design top level. which instantiates top wrapper,
--                   test bench top and infrastructure modules.
--Reference        :
--Revision History :
--*****************************************************************************
library ieee;
use ieee.std_logic_1164.all;
entity ddr3_ctrl_svec_bank4_64b_32b is
generic
  (
            C4_P0_MASK_SIZE           : integer := 8;
          C4_P0_DATA_PORT_SIZE      : integer := 64;
          C4_P1_MASK_SIZE           : integer := 4;
          C4_P1_DATA_PORT_SIZE      : integer := 32;
    C4_MEMCLK_PERIOD        : integer := 3000; 
                                       -- Memory data transfer clock period.
    C4_RST_ACT_LOW          : integer := 0; 
                                       -- # = 1 for active low reset,
                                       -- # = 0 for active high reset.
    C4_INPUT_CLK_TYPE       : string := "SINGLE_ENDED"; 
                                       -- input clock type DIFFERENTIAL or SINGLE_ENDED.
    C4_CALIB_SOFT_IP        : string := "TRUE"; 
                                       -- # = TRUE, Enables the soft calibration logic,
                                       -- # = FALSE, Disables the soft calibration logic.
    C4_SIMULATION           : string := "FALSE"; 
                                       -- # = TRUE, Simulating the design. Useful to reduce the simulation time,
                                       -- # = FALSE, Implementing the design.
    DEBUG_EN                : integer := 0; 
                                       -- # = 1, Enable debug signals/controls,
                                       --   = 0, Disable debug signals/controls.
    C4_MEM_ADDR_ORDER       : string := "ROW_BANK_COLUMN"; 
                                       -- The order in which user address is provided to the memory controller,
                                       -- ROW_BANK_COLUMN or BANK_ROW_COLUMN.
    C4_NUM_DQ_PINS          : integer := 16; 
                                       -- External memory data width.
    C4_MEM_ADDR_WIDTH       : integer := 14; 
                                       -- External memory address width.
    C4_MEM_BANKADDR_WIDTH   : integer := 3 
                                       -- External memory bank address width.
  );
   
  port
  (

   mcb4_dram_dq                            : inout  std_logic_vector(C4_NUM_DQ_PINS-1 downto 0);
   mcb4_dram_a                             : out std_logic_vector(C4_MEM_ADDR_WIDTH-1 downto 0);
   mcb4_dram_ba                            : out std_logic_vector(C4_MEM_BANKADDR_WIDTH-1 downto 0);
   mcb4_dram_ras_n                         : out std_logic;
   mcb4_dram_cas_n                         : out std_logic;
   mcb4_dram_we_n                          : out std_logic;
   mcb4_dram_odt                           : out std_logic;
   mcb4_dram_reset_n                       : out std_logic;
   mcb4_dram_cke                           : out std_logic;
   mcb4_dram_dm                            : out std_logic;
   mcb4_dram_udqs                          : inout  std_logic;
   mcb4_dram_udqs_n                        : inout  std_logic;
   mcb4_rzq                                : inout  std_logic;
   mcb4_dram_udm                           : out std_logic;
   c4_sys_clk                              : in  std_logic;
   c4_sys_rst_i                            : in  std_logic;
   c4_calib_done                           : out std_logic;
   c4_clk0                                 : out std_logic;
   c4_rst0                                 : out std_logic;
   mcb4_dram_dqs                           : inout  std_logic;
   mcb4_dram_dqs_n                         : inout  std_logic;
   mcb4_dram_ck                            : out std_logic;
   mcb4_dram_ck_n                          : out std_logic;
   c4_p0_cmd_clk                           : in std_logic;
   c4_p0_cmd_en                            : in std_logic;
   c4_p0_cmd_instr                         : in std_logic_vector(2 downto 0);
   c4_p0_cmd_bl                            : in std_logic_vector(5 downto 0);
   c4_p0_cmd_byte_addr                     : in std_logic_vector(29 downto 0);
   c4_p0_cmd_empty                         : out std_logic;
   c4_p0_cmd_full                          : out std_logic;
   c4_p0_wr_clk                            : in std_logic;
   c4_p0_wr_en                             : in std_logic;
   c4_p0_wr_mask                           : in std_logic_vector(C4_P0_MASK_SIZE - 1 downto 0);
   c4_p0_wr_data                           : in std_logic_vector(C4_P0_DATA_PORT_SIZE - 1 downto 0);
   c4_p0_wr_full                           : out std_logic;
   c4_p0_wr_empty                          : out std_logic;
   c4_p0_wr_count                          : out std_logic_vector(6 downto 0);
   c4_p0_wr_underrun                       : out std_logic;
   c4_p0_wr_error                          : out std_logic;
   c4_p0_rd_clk                            : in std_logic;
   c4_p0_rd_en                             : in std_logic;
   c4_p0_rd_data                           : out std_logic_vector(C4_P0_DATA_PORT_SIZE - 1 downto 0);
   c4_p0_rd_full                           : out std_logic;
   c4_p0_rd_empty                          : out std_logic;
   c4_p0_rd_count                          : out std_logic_vector(6 downto 0);
   c4_p0_rd_overflow                       : out std_logic;
   c4_p0_rd_error                          : out std_logic;
   c4_p1_cmd_clk                           : in std_logic;
   c4_p1_cmd_en                            : in std_logic;
   c4_p1_cmd_instr                         : in std_logic_vector(2 downto 0);
   c4_p1_cmd_bl                            : in std_logic_vector(5 downto 0);
   c4_p1_cmd_byte_addr                     : in std_logic_vector(29 downto 0);
   c4_p1_cmd_empty                         : out std_logic;
   c4_p1_cmd_full                          : out std_logic;
   c4_p1_wr_clk                            : in std_logic;
   c4_p1_wr_en                             : in std_logic;
   c4_p1_wr_mask                           : in std_logic_vector(C4_P1_MASK_SIZE - 1 downto 0);
   c4_p1_wr_data                           : in std_logic_vector(C4_P1_DATA_PORT_SIZE - 1 downto 0);
   c4_p1_wr_full                           : out std_logic;
   c4_p1_wr_empty                          : out std_logic;
   c4_p1_wr_count                          : out std_logic_vector(6 downto 0);
   c4_p1_wr_underrun                       : out std_logic;
   c4_p1_wr_error                          : out std_logic;
   c4_p1_rd_clk                            : in std_logic;
   c4_p1_rd_en                             : in std_logic;
   c4_p1_rd_data                           : out std_logic_vector(C4_P1_DATA_PORT_SIZE - 1 downto 0);
   c4_p1_rd_full                           : out std_logic;
   c4_p1_rd_empty                          : out std_logic;
   c4_p1_rd_count                          : out std_logic_vector(6 downto 0);
   c4_p1_rd_overflow                       : out std_logic;
   c4_p1_rd_error                          : out std_logic
  );
end ddr3_ctrl_svec_bank4_64b_32b;

architecture arc of ddr3_ctrl_svec_bank4_64b_32b is

 
component memc4_infrastructure is
    generic (
      C_RST_ACT_LOW        : integer;
      C_INPUT_CLK_TYPE     : string;
      C_CLKOUT0_DIVIDE     : integer;
      C_CLKOUT1_DIVIDE     : integer;
      C_CLKOUT2_DIVIDE     : integer;
      C_CLKOUT3_DIVIDE     : integer;
      C_CLKFBOUT_MULT      : integer;
      C_DIVCLK_DIVIDE      : integer;
      C_INCLK_PERIOD       : integer

      );
    port (
      sys_clk_p                              : in    std_logic;
      sys_clk_n                              : in    std_logic;
      sys_clk                                : in    std_logic;
      sys_rst_i                              : in    std_logic;
      clk0                                   : out   std_logic;
      rst0                                   : out   std_logic;
      async_rst                              : out   std_logic;
      sysclk_2x                              : out   std_logic;
      sysclk_2x_180                          : out   std_logic;
      pll_ce_0                               : out   std_logic;
      pll_ce_90                              : out   std_logic;
      pll_lock                               : out   std_logic;
      mcb_drp_clk                            : out   std_logic

      );
  end component;


component memc4_wrapper is
    generic (
      C_MEMCLK_PERIOD      : integer;
      C_CALIB_SOFT_IP      : string;
      C_SIMULATION         : string;
      C_P0_MASK_SIZE       : integer;
      C_P0_DATA_PORT_SIZE   : integer;
      C_P1_MASK_SIZE       : integer;
      C_P1_DATA_PORT_SIZE   : integer;
      C_ARB_NUM_TIME_SLOTS   : integer;
      C_ARB_TIME_SLOT_0    : bit_vector(5 downto 0);
      C_ARB_TIME_SLOT_1    : bit_vector(5 downto 0);
      C_ARB_TIME_SLOT_2    : bit_vector(5 downto 0);
      C_ARB_TIME_SLOT_3    : bit_vector(5 downto 0);
      C_ARB_TIME_SLOT_4    : bit_vector(5 downto 0);
      C_ARB_TIME_SLOT_5    : bit_vector(5 downto 0);
      C_ARB_TIME_SLOT_6    : bit_vector(5 downto 0);
      C_ARB_TIME_SLOT_7    : bit_vector(5 downto 0);
      C_ARB_TIME_SLOT_8    : bit_vector(5 downto 0);
      C_ARB_TIME_SLOT_9    : bit_vector(5 downto 0);
      C_ARB_TIME_SLOT_10   : bit_vector(5 downto 0);
      C_ARB_TIME_SLOT_11   : bit_vector(5 downto 0);
      C_MEM_TRAS           : integer;
      C_MEM_TRCD           : integer;
      C_MEM_TREFI          : integer;
      C_MEM_TRFC           : integer;
      C_MEM_TRP            : integer;
      C_MEM_TWR            : integer;
      C_MEM_TRTP           : integer;
      C_MEM_TWTR           : integer;
      C_MEM_ADDR_ORDER     : string;
      C_NUM_DQ_PINS        : integer;
      C_MEM_TYPE           : string;
      C_MEM_DENSITY        : string;
      C_MEM_BURST_LEN      : integer;
      C_MEM_CAS_LATENCY    : integer;
      C_MEM_ADDR_WIDTH     : integer;
      C_MEM_BANKADDR_WIDTH   : integer;
      C_MEM_NUM_COL_BITS   : integer;
      C_MEM_DDR1_2_ODS     : string;
      C_MEM_DDR2_RTT       : string;
      C_MEM_DDR2_DIFF_DQS_EN   : string;
      C_MEM_DDR2_3_PA_SR   : string;
      C_MEM_DDR2_3_HIGH_TEMP_SR   : string;
      C_MEM_DDR3_CAS_LATENCY   : integer;
      C_MEM_DDR3_ODS       : string;
      C_MEM_DDR3_RTT       : string;
      C_MEM_DDR3_CAS_WR_LATENCY   : integer;
      C_MEM_DDR3_AUTO_SR   : string;
      C_MEM_MOBILE_PA_SR   : string;
      C_MEM_MDDR_ODS       : string;
      C_MC_CALIB_BYPASS    : string;
      C_MC_CALIBRATION_MODE   : string;
      C_MC_CALIBRATION_DELAY   : string;
      C_SKIP_IN_TERM_CAL   : integer;
      C_SKIP_DYNAMIC_CAL   : integer;
      C_LDQSP_TAP_DELAY_VAL   : integer;
      C_LDQSN_TAP_DELAY_VAL   : integer;
      C_UDQSP_TAP_DELAY_VAL   : integer;
      C_UDQSN_TAP_DELAY_VAL   : integer;
      C_DQ0_TAP_DELAY_VAL   : integer;
      C_DQ1_TAP_DELAY_VAL   : integer;
      C_DQ2_TAP_DELAY_VAL   : integer;
      C_DQ3_TAP_DELAY_VAL   : integer;
      C_DQ4_TAP_DELAY_VAL   : integer;
      C_DQ5_TAP_DELAY_VAL   : integer;
      C_DQ6_TAP_DELAY_VAL   : integer;
      C_DQ7_TAP_DELAY_VAL   : integer;
      C_DQ8_TAP_DELAY_VAL   : integer;
      C_DQ9_TAP_DELAY_VAL   : integer;
      C_DQ10_TAP_DELAY_VAL   : integer;
      C_DQ11_TAP_DELAY_VAL   : integer;
      C_DQ12_TAP_DELAY_VAL   : integer;
      C_DQ13_TAP_DELAY_VAL   : integer;
      C_DQ14_TAP_DELAY_VAL   : integer;
      C_DQ15_TAP_DELAY_VAL   : integer
      );
    port (
      mcb4_dram_dq                           : inout  std_logic_vector((C_NUM_DQ_PINS-1) downto 0);
      mcb4_dram_a                            : out  std_logic_vector((C_MEM_ADDR_WIDTH-1) downto 0);
      mcb4_dram_ba                           : out  std_logic_vector((C_MEM_BANKADDR_WIDTH-1) downto 0);
      mcb4_dram_ras_n                        : out  std_logic;
      mcb4_dram_cas_n                        : out  std_logic;
      mcb4_dram_we_n                         : out  std_logic;
      mcb4_dram_odt                          : out  std_logic;
      mcb4_dram_reset_n                      : out  std_logic;
      mcb4_dram_cke                          : out  std_logic;
      mcb4_dram_dm                           : out  std_logic;
      mcb4_dram_udqs                         : inout  std_logic;
      mcb4_dram_udqs_n                       : inout  std_logic;
      mcb4_rzq                               : inout  std_logic;
      mcb4_dram_udm                          : out  std_logic;
      calib_done                             : out  std_logic;
      async_rst                              : in  std_logic;
      sysclk_2x                              : in  std_logic;
      sysclk_2x_180                          : in  std_logic;
      pll_ce_0                               : in  std_logic;
      pll_ce_90                              : in  std_logic;
      pll_lock                               : in  std_logic;
      mcb_drp_clk                            : in  std_logic;
      mcb4_dram_dqs                          : inout  std_logic;
      mcb4_dram_dqs_n                        : inout  std_logic;
      mcb4_dram_ck                           : out  std_logic;
      mcb4_dram_ck_n                         : out  std_logic;
      p0_cmd_clk                            : in std_logic;
      p0_cmd_en                             : in std_logic;
      p0_cmd_instr                          : in std_logic_vector(2 downto 0);
      p0_cmd_bl                             : in std_logic_vector(5 downto 0);
      p0_cmd_byte_addr                      : in std_logic_vector(29 downto 0);
      p0_cmd_empty                          : out std_logic;
      p0_cmd_full                           : out std_logic;
      p0_wr_clk                             : in std_logic;
      p0_wr_en                              : in std_logic;
      p0_wr_mask                            : in std_logic_vector(C_P0_MASK_SIZE - 1 downto 0);
      p0_wr_data                            : in std_logic_vector(C_P0_DATA_PORT_SIZE - 1 downto 0);
      p0_wr_full                            : out std_logic;
      p0_wr_empty                           : out std_logic;
      p0_wr_count                           : out std_logic_vector(6 downto 0);
      p0_wr_underrun                        : out std_logic;
      p0_wr_error                           : out std_logic;
      p0_rd_clk                             : in std_logic;
      p0_rd_en                              : in std_logic;
      p0_rd_data                            : out std_logic_vector(C_P0_DATA_PORT_SIZE - 1 downto 0);
      p0_rd_full                            : out std_logic;
      p0_rd_empty                           : out std_logic;
      p0_rd_count                           : out std_logic_vector(6 downto 0);
      p0_rd_overflow                        : out std_logic;
      p0_rd_error                           : out std_logic;
      p1_cmd_clk                            : in std_logic;
      p1_cmd_en                             : in std_logic;
      p1_cmd_instr                          : in std_logic_vector(2 downto 0);
      p1_cmd_bl                             : in std_logic_vector(5 downto 0);
      p1_cmd_byte_addr                      : in std_logic_vector(29 downto 0);
      p1_cmd_empty                          : out std_logic;
      p1_cmd_full                           : out std_logic;
      p1_wr_clk                             : in std_logic;
      p1_wr_en                              : in std_logic;
      p1_wr_mask                            : in std_logic_vector(C_P1_MASK_SIZE - 1 downto 0);
      p1_wr_data                            : in std_logic_vector(C_P1_DATA_PORT_SIZE - 1 downto 0);
      p1_wr_full                            : out std_logic;
      p1_wr_empty                           : out std_logic;
      p1_wr_count                           : out std_logic_vector(6 downto 0);
      p1_wr_underrun                        : out std_logic;
      p1_wr_error                           : out std_logic;
      p1_rd_clk                             : in std_logic;
      p1_rd_en                              : in std_logic;
      p1_rd_data                            : out std_logic_vector(C_P1_DATA_PORT_SIZE - 1 downto 0);
      p1_rd_full                            : out std_logic;
      p1_rd_empty                           : out std_logic;
      p1_rd_count                           : out std_logic_vector(6 downto 0);
      p1_rd_overflow                        : out std_logic;
      p1_rd_error                           : out std_logic;
      selfrefresh_enter                     : in std_logic;
      selfrefresh_mode                      : out std_logic

      );
  end component;






   constant C4_CLKOUT0_DIVIDE       : integer := 1; 
   constant C4_CLKOUT1_DIVIDE       : integer := 1; 
   constant C4_CLKOUT2_DIVIDE       : integer := 16; 
   constant C4_CLKOUT3_DIVIDE       : integer := 8; 
   constant C4_CLKFBOUT_MULT        : integer := 2; 
   constant C4_DIVCLK_DIVIDE        : integer := 1; 
   constant C4_INCLK_PERIOD         : integer := ((C4_MEMCLK_PERIOD * C4_CLKFBOUT_MULT) / (C4_DIVCLK_DIVIDE * C4_CLKOUT0_DIVIDE * 2)); 
   constant C4_ARB_NUM_TIME_SLOTS   : integer := 12; 
   constant C4_ARB_TIME_SLOT_0      : bit_vector(5 downto 0) := o"02"; 
   constant C4_ARB_TIME_SLOT_1      : bit_vector(5 downto 0) := o"20"; 
   constant C4_ARB_TIME_SLOT_2      : bit_vector(5 downto 0) := o"02"; 
   constant C4_ARB_TIME_SLOT_3      : bit_vector(5 downto 0) := o"20"; 
   constant C4_ARB_TIME_SLOT_4      : bit_vector(5 downto 0) := o"02"; 
   constant C4_ARB_TIME_SLOT_5      : bit_vector(5 downto 0) := o"20"; 
   constant C4_ARB_TIME_SLOT_6      : bit_vector(5 downto 0) := o"02"; 
   constant C4_ARB_TIME_SLOT_7      : bit_vector(5 downto 0) := o"20"; 
   constant C4_ARB_TIME_SLOT_8      : bit_vector(5 downto 0) := o"02"; 
   constant C4_ARB_TIME_SLOT_9      : bit_vector(5 downto 0) := o"20"; 
   constant C4_ARB_TIME_SLOT_10     : bit_vector(5 downto 0) := o"02"; 
   constant C4_ARB_TIME_SLOT_11     : bit_vector(5 downto 0) := o"20"; 
   constant C4_MEM_TRAS             : integer := 36000; 
   constant C4_MEM_TRCD             : integer := 13500; 
   constant C4_MEM_TREFI            : integer := 7800000; 
   constant C4_MEM_TRFC             : integer := 160000; 
   constant C4_MEM_TRP              : integer := 13500; 
   constant C4_MEM_TWR              : integer := 15000; 
   constant C4_MEM_TRTP             : integer := 7500; 
   constant C4_MEM_TWTR             : integer := 7500; 
   constant C4_MEM_TYPE             : string := "DDR3"; 
   constant C4_MEM_DENSITY          : string := "2Gb"; 
   constant C4_MEM_BURST_LEN        : integer := 8; 
   constant C4_MEM_CAS_LATENCY      : integer := 6; 
   constant C4_MEM_NUM_COL_BITS     : integer := 10; 
   constant C4_MEM_DDR1_2_ODS       : string := "FULL"; 
   constant C4_MEM_DDR2_RTT         : string := "50OHMS"; 
   constant C4_MEM_DDR2_DIFF_DQS_EN  : string := "YES"; 
   constant C4_MEM_DDR2_3_PA_SR     : string := "FULL"; 
   constant C4_MEM_DDR2_3_HIGH_TEMP_SR  : string := "NORMAL"; 
   constant C4_MEM_DDR3_CAS_LATENCY  : integer := 6; 
   constant C4_MEM_DDR3_ODS         : string := "DIV6"; 
   constant C4_MEM_DDR3_RTT         : string := "DIV4"; 
   constant C4_MEM_DDR3_CAS_WR_LATENCY  : integer := 5; 
   constant C4_MEM_DDR3_AUTO_SR     : string := "ENABLED"; 
   constant C4_MEM_MOBILE_PA_SR     : string := "FULL"; 
   constant C4_MEM_MDDR_ODS         : string := "FULL"; 
   constant C4_MC_CALIB_BYPASS      : string := "NO"; 
   constant C4_MC_CALIBRATION_MODE  : string := "CALIBRATION"; 
   constant C4_MC_CALIBRATION_DELAY  : string := "HALF"; 
   constant C4_SKIP_IN_TERM_CAL     : integer := 1; 
   constant C4_SKIP_DYNAMIC_CAL     : integer := 0; 
   constant C4_LDQSP_TAP_DELAY_VAL  : integer := 0; 
   constant C4_LDQSN_TAP_DELAY_VAL  : integer := 0; 
   constant C4_UDQSP_TAP_DELAY_VAL  : integer := 0; 
   constant C4_UDQSN_TAP_DELAY_VAL  : integer := 0; 
   constant C4_DQ0_TAP_DELAY_VAL    : integer := 0; 
   constant C4_DQ1_TAP_DELAY_VAL    : integer := 0; 
   constant C4_DQ2_TAP_DELAY_VAL    : integer := 0; 
   constant C4_DQ3_TAP_DELAY_VAL    : integer := 0; 
   constant C4_DQ4_TAP_DELAY_VAL    : integer := 0; 
   constant C4_DQ5_TAP_DELAY_VAL    : integer := 0; 
   constant C4_DQ6_TAP_DELAY_VAL    : integer := 0; 
   constant C4_DQ7_TAP_DELAY_VAL    : integer := 0; 
   constant C4_DQ8_TAP_DELAY_VAL    : integer := 0; 
   constant C4_DQ9_TAP_DELAY_VAL    : integer := 0; 
   constant C4_DQ10_TAP_DELAY_VAL   : integer := 0; 
   constant C4_DQ11_TAP_DELAY_VAL   : integer := 0; 
   constant C4_DQ12_TAP_DELAY_VAL   : integer := 0; 
   constant C4_DQ13_TAP_DELAY_VAL   : integer := 0; 
   constant C4_DQ14_TAP_DELAY_VAL   : integer := 0; 
   constant C4_DQ15_TAP_DELAY_VAL   : integer := 0; 
   constant C4_SMALL_DEVICE         : string := "FALSE"; -- The parameter is set to TRUE for all packages of xc6slx9 device
                                                         -- as most of them cannot fit the complete example design when the
                                                         -- Chip scope modules are enabled

  signal  c4_sys_clk_p                             : std_logic;
  signal  c4_sys_clk_n                             : std_logic;
  signal  c4_async_rst                             : std_logic;
  signal  c4_sysclk_2x                             : std_logic;
  signal  c4_sysclk_2x_180                         : std_logic;
  signal  c4_pll_ce_0                              : std_logic;
  signal  c4_pll_ce_90                             : std_logic;
  signal  c4_pll_lock                              : std_logic;
  signal  c4_mcb_drp_clk                           : std_logic;
  signal  c4_cmp_error                             : std_logic;
  signal  c4_cmp_data_valid                        : std_logic;
  signal  c4_vio_modify_enable                     : std_logic;
  signal  c4_error_status                          : std_logic_vector(191 downto 0);
  signal  c4_vio_data_mode_value                   : std_logic_vector(2 downto 0);
  signal  c4_vio_addr_mode_value                   : std_logic_vector(2 downto 0);
  signal  c4_cmp_data                              : std_logic_vector(31 downto 0);
  signal  c4_selfrefresh_enter                     : std_logic;
  signal  c4_selfrefresh_mode                      : std_logic;



begin
 

c4_sys_clk_p <= '0';
c4_sys_clk_n <= '0';
c4_selfrefresh_enter <= '0';
memc4_infrastructure_inst : memc4_infrastructure

generic map
 (
   C_RST_ACT_LOW                     => C4_RST_ACT_LOW,
   C_INPUT_CLK_TYPE                  => C4_INPUT_CLK_TYPE,
   C_CLKOUT0_DIVIDE                  => C4_CLKOUT0_DIVIDE,
   C_CLKOUT1_DIVIDE                  => C4_CLKOUT1_DIVIDE,
   C_CLKOUT2_DIVIDE                  => C4_CLKOUT2_DIVIDE,
   C_CLKOUT3_DIVIDE                  => C4_CLKOUT3_DIVIDE,
   C_CLKFBOUT_MULT                   => C4_CLKFBOUT_MULT,
   C_DIVCLK_DIVIDE                   => C4_DIVCLK_DIVIDE,
   C_INCLK_PERIOD                    => C4_INCLK_PERIOD
   )
port map
 (
   sys_clk_p                       => c4_sys_clk_p,
   sys_clk_n                       => c4_sys_clk_n,
   sys_clk                         => c4_sys_clk,
   sys_rst_i                       => c4_sys_rst_i,
   clk0                            => c4_clk0,
   rst0                            => c4_rst0,
   async_rst                       => c4_async_rst,
   sysclk_2x                       => c4_sysclk_2x,
   sysclk_2x_180                   => c4_sysclk_2x_180,
   pll_ce_0                        => c4_pll_ce_0,
   pll_ce_90                       => c4_pll_ce_90,
   pll_lock                        => c4_pll_lock,
   mcb_drp_clk                     => c4_mcb_drp_clk
   );

 
-- wrapper instantiation
 memc4_wrapper_inst : memc4_wrapper

generic map
 (
   C_MEMCLK_PERIOD                   => C4_MEMCLK_PERIOD,
   C_CALIB_SOFT_IP                   => C4_CALIB_SOFT_IP,
   C_SIMULATION                      => C4_SIMULATION,
   C_P0_MASK_SIZE                    => C4_P0_MASK_SIZE,
   C_P0_DATA_PORT_SIZE               => C4_P0_DATA_PORT_SIZE,
   C_P1_MASK_SIZE                    => C4_P1_MASK_SIZE,
   C_P1_DATA_PORT_SIZE               => C4_P1_DATA_PORT_SIZE,
   C_ARB_NUM_TIME_SLOTS              => C4_ARB_NUM_TIME_SLOTS,
   C_ARB_TIME_SLOT_0                 => C4_ARB_TIME_SLOT_0,
   C_ARB_TIME_SLOT_1                 => C4_ARB_TIME_SLOT_1,
   C_ARB_TIME_SLOT_2                 => C4_ARB_TIME_SLOT_2,
   C_ARB_TIME_SLOT_3                 => C4_ARB_TIME_SLOT_3,
   C_ARB_TIME_SLOT_4                 => C4_ARB_TIME_SLOT_4,
   C_ARB_TIME_SLOT_5                 => C4_ARB_TIME_SLOT_5,
   C_ARB_TIME_SLOT_6                 => C4_ARB_TIME_SLOT_6,
   C_ARB_TIME_SLOT_7                 => C4_ARB_TIME_SLOT_7,
   C_ARB_TIME_SLOT_8                 => C4_ARB_TIME_SLOT_8,
   C_ARB_TIME_SLOT_9                 => C4_ARB_TIME_SLOT_9,
   C_ARB_TIME_SLOT_10                => C4_ARB_TIME_SLOT_10,
   C_ARB_TIME_SLOT_11                => C4_ARB_TIME_SLOT_11,
   C_MEM_TRAS                        => C4_MEM_TRAS,
   C_MEM_TRCD                        => C4_MEM_TRCD,
   C_MEM_TREFI                       => C4_MEM_TREFI,
   C_MEM_TRFC                        => C4_MEM_TRFC,
   C_MEM_TRP                         => C4_MEM_TRP,
   C_MEM_TWR                         => C4_MEM_TWR,
   C_MEM_TRTP                        => C4_MEM_TRTP,
   C_MEM_TWTR                        => C4_MEM_TWTR,
   C_MEM_ADDR_ORDER                  => C4_MEM_ADDR_ORDER,
   C_NUM_DQ_PINS                     => C4_NUM_DQ_PINS,
   C_MEM_TYPE                        => C4_MEM_TYPE,
   C_MEM_DENSITY                     => C4_MEM_DENSITY,
   C_MEM_BURST_LEN                   => C4_MEM_BURST_LEN,
   C_MEM_CAS_LATENCY                 => C4_MEM_CAS_LATENCY,
   C_MEM_ADDR_WIDTH                  => C4_MEM_ADDR_WIDTH,
   C_MEM_BANKADDR_WIDTH              => C4_MEM_BANKADDR_WIDTH,
   C_MEM_NUM_COL_BITS                => C4_MEM_NUM_COL_BITS,
   C_MEM_DDR1_2_ODS                  => C4_MEM_DDR1_2_ODS,
   C_MEM_DDR2_RTT                    => C4_MEM_DDR2_RTT,
   C_MEM_DDR2_DIFF_DQS_EN            => C4_MEM_DDR2_DIFF_DQS_EN,
   C_MEM_DDR2_3_PA_SR                => C4_MEM_DDR2_3_PA_SR,
   C_MEM_DDR2_3_HIGH_TEMP_SR         => C4_MEM_DDR2_3_HIGH_TEMP_SR,
   C_MEM_DDR3_CAS_LATENCY            => C4_MEM_DDR3_CAS_LATENCY,
   C_MEM_DDR3_ODS                    => C4_MEM_DDR3_ODS,
   C_MEM_DDR3_RTT                    => C4_MEM_DDR3_RTT,
   C_MEM_DDR3_CAS_WR_LATENCY         => C4_MEM_DDR3_CAS_WR_LATENCY,
   C_MEM_DDR3_AUTO_SR                => C4_MEM_DDR3_AUTO_SR,
   C_MEM_MOBILE_PA_SR                => C4_MEM_MOBILE_PA_SR,
   C_MEM_MDDR_ODS                    => C4_MEM_MDDR_ODS,
   C_MC_CALIB_BYPASS                 => C4_MC_CALIB_BYPASS,
   C_MC_CALIBRATION_MODE             => C4_MC_CALIBRATION_MODE,
   C_MC_CALIBRATION_DELAY            => C4_MC_CALIBRATION_DELAY,
   C_SKIP_IN_TERM_CAL                => C4_SKIP_IN_TERM_CAL,
   C_SKIP_DYNAMIC_CAL                => C4_SKIP_DYNAMIC_CAL,
   C_LDQSP_TAP_DELAY_VAL             => C4_LDQSP_TAP_DELAY_VAL,
   C_LDQSN_TAP_DELAY_VAL             => C4_LDQSN_TAP_DELAY_VAL,
   C_UDQSP_TAP_DELAY_VAL             => C4_UDQSP_TAP_DELAY_VAL,
   C_UDQSN_TAP_DELAY_VAL             => C4_UDQSN_TAP_DELAY_VAL,
   C_DQ0_TAP_DELAY_VAL               => C4_DQ0_TAP_DELAY_VAL,
   C_DQ1_TAP_DELAY_VAL               => C4_DQ1_TAP_DELAY_VAL,
   C_DQ2_TAP_DELAY_VAL               => C4_DQ2_TAP_DELAY_VAL,
   C_DQ3_TAP_DELAY_VAL               => C4_DQ3_TAP_DELAY_VAL,
   C_DQ4_TAP_DELAY_VAL               => C4_DQ4_TAP_DELAY_VAL,
   C_DQ5_TAP_DELAY_VAL               => C4_DQ5_TAP_DELAY_VAL,
   C_DQ6_TAP_DELAY_VAL               => C4_DQ6_TAP_DELAY_VAL,
   C_DQ7_TAP_DELAY_VAL               => C4_DQ7_TAP_DELAY_VAL,
   C_DQ8_TAP_DELAY_VAL               => C4_DQ8_TAP_DELAY_VAL,
   C_DQ9_TAP_DELAY_VAL               => C4_DQ9_TAP_DELAY_VAL,
   C_DQ10_TAP_DELAY_VAL              => C4_DQ10_TAP_DELAY_VAL,
   C_DQ11_TAP_DELAY_VAL              => C4_DQ11_TAP_DELAY_VAL,
   C_DQ12_TAP_DELAY_VAL              => C4_DQ12_TAP_DELAY_VAL,
   C_DQ13_TAP_DELAY_VAL              => C4_DQ13_TAP_DELAY_VAL,
   C_DQ14_TAP_DELAY_VAL              => C4_DQ14_TAP_DELAY_VAL,
   C_DQ15_TAP_DELAY_VAL              => C4_DQ15_TAP_DELAY_VAL
   )
port map
(
   mcb4_dram_dq                         => mcb4_dram_dq,
   mcb4_dram_a                          => mcb4_dram_a,
   mcb4_dram_ba                         => mcb4_dram_ba,
   mcb4_dram_ras_n                      => mcb4_dram_ras_n,
   mcb4_dram_cas_n                      => mcb4_dram_cas_n,
   mcb4_dram_we_n                       => mcb4_dram_we_n,
   mcb4_dram_odt                        => mcb4_dram_odt,
   mcb4_dram_reset_n                    => mcb4_dram_reset_n,
   mcb4_dram_cke                        => mcb4_dram_cke,
   mcb4_dram_dm                         => mcb4_dram_dm,
   mcb4_dram_udqs                       => mcb4_dram_udqs,
   mcb4_dram_udqs_n                     => mcb4_dram_udqs_n,
   mcb4_rzq                             => mcb4_rzq,
   mcb4_dram_udm                        => mcb4_dram_udm,
   calib_done                      => c4_calib_done,
   async_rst                       => c4_async_rst,
   sysclk_2x                       => c4_sysclk_2x,
   sysclk_2x_180                   => c4_sysclk_2x_180,
   pll_ce_0                        => c4_pll_ce_0,
   pll_ce_90                       => c4_pll_ce_90,
   pll_lock                        => c4_pll_lock,
   mcb_drp_clk                     => c4_mcb_drp_clk,
   mcb4_dram_dqs                        => mcb4_dram_dqs,
   mcb4_dram_dqs_n                      => mcb4_dram_dqs_n,
   mcb4_dram_ck                         => mcb4_dram_ck,
   mcb4_dram_ck_n                       => mcb4_dram_ck_n,
   p0_cmd_clk                           =>  c4_p0_cmd_clk,
   p0_cmd_en                            =>  c4_p0_cmd_en,
   p0_cmd_instr                         =>  c4_p0_cmd_instr,
   p0_cmd_bl                            =>  c4_p0_cmd_bl,
   p0_cmd_byte_addr                     =>  c4_p0_cmd_byte_addr,
   p0_cmd_empty                         =>  c4_p0_cmd_empty,
   p0_cmd_full                          =>  c4_p0_cmd_full,
   p0_wr_clk                            =>  c4_p0_wr_clk,
   p0_wr_en                             =>  c4_p0_wr_en,
   p0_wr_mask                           =>  c4_p0_wr_mask,
   p0_wr_data                           =>  c4_p0_wr_data,
   p0_wr_full                           =>  c4_p0_wr_full,
   p0_wr_empty                          =>  c4_p0_wr_empty,
   p0_wr_count                          =>  c4_p0_wr_count,
   p0_wr_underrun                       =>  c4_p0_wr_underrun,
   p0_wr_error                          =>  c4_p0_wr_error,
   p0_rd_clk                            =>  c4_p0_rd_clk,
   p0_rd_en                             =>  c4_p0_rd_en,
   p0_rd_data                           =>  c4_p0_rd_data,
   p0_rd_full                           =>  c4_p0_rd_full,
   p0_rd_empty                          =>  c4_p0_rd_empty,
   p0_rd_count                          =>  c4_p0_rd_count,
   p0_rd_overflow                       =>  c4_p0_rd_overflow,
   p0_rd_error                          =>  c4_p0_rd_error,
   p1_cmd_clk                           =>  c4_p1_cmd_clk,
   p1_cmd_en                            =>  c4_p1_cmd_en,
   p1_cmd_instr                         =>  c4_p1_cmd_instr,
   p1_cmd_bl                            =>  c4_p1_cmd_bl,
   p1_cmd_byte_addr                     =>  c4_p1_cmd_byte_addr,
   p1_cmd_empty                         =>  c4_p1_cmd_empty,
   p1_cmd_full                          =>  c4_p1_cmd_full,
   p1_wr_clk                            =>  c4_p1_wr_clk,
   p1_wr_en                             =>  c4_p1_wr_en,
   p1_wr_mask                           =>  c4_p1_wr_mask,
   p1_wr_data                           =>  c4_p1_wr_data,
   p1_wr_full                           =>  c4_p1_wr_full,
   p1_wr_empty                          =>  c4_p1_wr_empty,
   p1_wr_count                          =>  c4_p1_wr_count,
   p1_wr_underrun                       =>  c4_p1_wr_underrun,
   p1_wr_error                          =>  c4_p1_wr_error,
   p1_rd_clk                            =>  c4_p1_rd_clk,
   p1_rd_en                             =>  c4_p1_rd_en,
   p1_rd_data                           =>  c4_p1_rd_data,
   p1_rd_full                           =>  c4_p1_rd_full,
   p1_rd_empty                          =>  c4_p1_rd_empty,
   p1_rd_count                          =>  c4_p1_rd_count,
   p1_rd_overflow                       =>  c4_p1_rd_overflow,
   p1_rd_error                          =>  c4_p1_rd_error,
   selfrefresh_enter                    =>  c4_selfrefresh_enter,
   selfrefresh_mode                     =>  c4_selfrefresh_mode
);

 
 
  

 end  arc;
