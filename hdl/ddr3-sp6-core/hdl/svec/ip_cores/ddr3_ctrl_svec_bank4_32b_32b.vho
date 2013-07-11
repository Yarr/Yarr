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
-- /___/  \  /   Vendor             : Xilinx
-- \   \   \/    Version            : 3.9
--  \   \        Application        : MIG
--  /   /        Filename           : ddr3_ctrl_svec_bank4_32b_32b.vho
-- /___/   /\    Date Last Modified : $Date: 2011/06/02 07:19:03 $
-- \   \  /  \   Date Created       : Fri Aug 7 2009
--  \___\/\___\
--
-- Purpose     : Template file containing code that can be used as a model
--               for instantiating a CORE Generator module in a HDL design.
-- Revision History:
--*****************************************************************************

-- The following code must appear in the VHDL architecture header:

------------- Begin Cut here for COMPONENT Declaration ------ COMP_TAG

component ddr3_ctrl_svec_bank4_32b_32b
 generic(
    C4_P0_MASK_SIZE           : integer := 4;
    C4_P0_DATA_PORT_SIZE      : integer := 32;
    C4_P1_MASK_SIZE           : integer := 4;
    C4_P1_DATA_PORT_SIZE      : integer := 32;
    C4_MEMCLK_PERIOD          : integer := 3000;
    C4_RST_ACT_LOW            : integer := 0;
    C4_INPUT_CLK_TYPE         : string := "SINGLE_ENDED";
    C4_CALIB_SOFT_IP          : string := "TRUE";
    C4_SIMULATION             : string := "FALSE";
    DEBUG_EN                  : integer := 0;
    C4_MEM_ADDR_ORDER         : string := "ROW_BANK_COLUMN";
    C4_NUM_DQ_PINS            : integer := 16;
    C4_MEM_ADDR_WIDTH         : integer := 14;
    C4_MEM_BANKADDR_WIDTH     : integer := 3
);
    port (
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
end component;

-- COMP_TAG_END ------ End COMPONENT Declaration ------------

-- The following code must appear in the VHDL architecture
-- body. Substitute your own instance name and net names.

------------- Begin Cut here for INSTANTIATION Template ----- INST_TAG
  u_ddr3_ctrl_svec_bank4_32b_32b : ddr3_ctrl_svec_bank4_32b_32b
    generic map (
    C4_P0_MASK_SIZE => C4_P0_MASK_SIZE,
    C4_P0_DATA_PORT_SIZE => C4_P0_DATA_PORT_SIZE,
    C4_P1_MASK_SIZE => C4_P1_MASK_SIZE,
    C4_P1_DATA_PORT_SIZE => C4_P1_DATA_PORT_SIZE,
    C4_MEMCLK_PERIOD => C4_MEMCLK_PERIOD,
    C4_RST_ACT_LOW => C4_RST_ACT_LOW,
    C4_INPUT_CLK_TYPE => C4_INPUT_CLK_TYPE,
    C4_CALIB_SOFT_IP => C4_CALIB_SOFT_IP,
    C4_SIMULATION => C4_SIMULATION,
    DEBUG_EN => DEBUG_EN,
    C4_MEM_ADDR_ORDER => C4_MEM_ADDR_ORDER,
    C4_NUM_DQ_PINS => C4_NUM_DQ_PINS,
    C4_MEM_ADDR_WIDTH => C4_MEM_ADDR_WIDTH,
    C4_MEM_BANKADDR_WIDTH => C4_MEM_BANKADDR_WIDTH
)
    port map (

    c4_sys_clk  =>         c4_sys_clk,
  c4_sys_rst_i    =>       c4_sys_rst_i,                        

  mcb4_dram_dq       =>    mcb4_dram_dq,  
  mcb4_dram_a        =>    mcb4_dram_a,  
  mcb4_dram_ba       =>    mcb4_dram_ba,
  mcb4_dram_ras_n    =>    mcb4_dram_ras_n,                        
  mcb4_dram_cas_n    =>    mcb4_dram_cas_n,                        
  mcb4_dram_we_n     =>    mcb4_dram_we_n,                          
  mcb4_dram_odt    =>      mcb4_dram_odt,
  mcb4_dram_cke      =>    mcb4_dram_cke,                          
  mcb4_dram_ck       =>    mcb4_dram_ck,                          
  mcb4_dram_ck_n     =>    mcb4_dram_ck_n,       
  mcb4_dram_dqs      =>    mcb4_dram_dqs,                          
  mcb4_dram_dqs_n    =>    mcb4_dram_dqs_n,
  mcb4_dram_reset_n =>     mcb4_dram_reset_n,
  mcb4_dram_udqs  =>       mcb4_dram_udqs,    -- for X16 parts           
    mcb4_dram_udqs_n  =>       mcb4_dram_udqs_n,    -- for X16 parts
  mcb4_dram_udm  =>        mcb4_dram_udm,     -- for X16 parts
  mcb4_dram_dm  =>       mcb4_dram_dm,
    c4_clk0	=>	        c4_clk0,
  c4_rst0		=>        c4_rst0,
	
 
  c4_calib_done      =>    c4_calib_done,
     mcb4_rzq         =>            rzq4,
        
  
     c4_p0_cmd_clk                           =>  c4_p0_cmd_clk,
   c4_p0_cmd_en                            =>  c4_p0_cmd_en,
   c4_p0_cmd_instr                         =>  c4_p0_cmd_instr,
   c4_p0_cmd_bl                            =>  c4_p0_cmd_bl,
   c4_p0_cmd_byte_addr                     =>  c4_p0_cmd_byte_addr,
   c4_p0_cmd_empty                         =>  c4_p0_cmd_empty,
   c4_p0_cmd_full                          =>  c4_p0_cmd_full,
   c4_p0_wr_clk                            =>  c4_p0_wr_clk,
   c4_p0_wr_en                             =>  c4_p0_wr_en,
   c4_p0_wr_mask                           =>  c4_p0_wr_mask,
   c4_p0_wr_data                           =>  c4_p0_wr_data,
   c4_p0_wr_full                           =>  c4_p0_wr_full,
   c4_p0_wr_empty                          =>  c4_p0_wr_empty,
   c4_p0_wr_count                          =>  c4_p0_wr_count,
   c4_p0_wr_underrun                       =>  c4_p0_wr_underrun,
   c4_p0_wr_error                          =>  c4_p0_wr_error,
   c4_p0_rd_clk                            =>  c4_p0_rd_clk,
   c4_p0_rd_en                             =>  c4_p0_rd_en,
   c4_p0_rd_data                           =>  c4_p0_rd_data,
   c4_p0_rd_full                           =>  c4_p0_rd_full,
   c4_p0_rd_empty                          =>  c4_p0_rd_empty,
   c4_p0_rd_count                          =>  c4_p0_rd_count,
   c4_p0_rd_overflow                       =>  c4_p0_rd_overflow,
   c4_p0_rd_error                          =>  c4_p0_rd_error,
   c4_p1_cmd_clk                           =>  c4_p1_cmd_clk,
   c4_p1_cmd_en                            =>  c4_p1_cmd_en,
   c4_p1_cmd_instr                         =>  c4_p1_cmd_instr,
   c4_p1_cmd_bl                            =>  c4_p1_cmd_bl,
   c4_p1_cmd_byte_addr                     =>  c4_p1_cmd_byte_addr,
   c4_p1_cmd_empty                         =>  c4_p1_cmd_empty,
   c4_p1_cmd_full                          =>  c4_p1_cmd_full,
   c4_p1_wr_clk                            =>  c4_p1_wr_clk,
   c4_p1_wr_en                             =>  c4_p1_wr_en,
   c4_p1_wr_mask                           =>  c4_p1_wr_mask,
   c4_p1_wr_data                           =>  c4_p1_wr_data,
   c4_p1_wr_full                           =>  c4_p1_wr_full,
   c4_p1_wr_empty                          =>  c4_p1_wr_empty,
   c4_p1_wr_count                          =>  c4_p1_wr_count,
   c4_p1_wr_underrun                       =>  c4_p1_wr_underrun,
   c4_p1_wr_error                          =>  c4_p1_wr_error,
   c4_p1_rd_clk                            =>  c4_p1_rd_clk,
   c4_p1_rd_en                             =>  c4_p1_rd_en,
   c4_p1_rd_data                           =>  c4_p1_rd_data,
   c4_p1_rd_full                           =>  c4_p1_rd_full,
   c4_p1_rd_empty                          =>  c4_p1_rd_empty,
   c4_p1_rd_count                          =>  c4_p1_rd_count,
   c4_p1_rd_overflow                       =>  c4_p1_rd_overflow,
   c4_p1_rd_error                          =>  c4_p1_rd_error
);

-- INST_TAG_END ------ End INSTANTIATION Template ------------

-- You must compile the wrapper file ddr3_ctrl_svec_bank4_32b_32b.vhd when simulating
-- the core, ddr3_ctrl_svec_bank4_32b_32b. When compiling the wrapper file, be sure to
-- reference the XilinxCoreLib VHDL simulation library. For detailed
-- instructions, please refer to the "CORE Generator Help".

