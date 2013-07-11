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
-- \   \   \/    Version            : 3.5
--  \   \        Application        : MIG
--  /   /        Filename           : ddr_controller_bank5.vho
-- /___/   /\    Date Last Modified : $Date: 2010/03/21 17:22:03 $
-- \   \  /  \   Date Created       : Fri Aug 7 2009
--  \___\/\___\
--
-- Purpose     : Template file containing code that can be used as a model
--               for instantiating a CORE Generator module in a HDL design.
-- Revision History:
--*****************************************************************************

-- The following code must appear in the VHDL architecture header:

------------- Begin Cut here for COMPONENT Declaration ------ COMP_TAG

component ddr_controller_bank5
 generic(
    C5_P0_MASK_SIZE           : integer := 4;
    C5_P0_DATA_PORT_SIZE      : integer := 32;
    C5_P1_MASK_SIZE           : integer := 4;
    C5_P1_DATA_PORT_SIZE      : integer := 32;
    C5_MEMCLK_PERIOD          : integer := 3200;
    C5_RST_ACT_LOW            : integer := 0;
    C5_INPUT_CLK_TYPE         : string := "SINGLE_ENDED";
    C5_CALIB_SOFT_IP          : string := "TRUE";
    C5_SIMULATION             : string := "FALSE";
    DEBUG_EN                  : integer := 0;
    C5_MEM_ADDR_ORDER         : string := "ROW_BANK_COLUMN";
    C5_NUM_DQ_PINS            : integer := 16;
    C5_MEM_ADDR_WIDTH         : integer := 14;
    C5_MEM_BANKADDR_WIDTH     : integer := 3
);
    port (
   mcb5_dram_dq                            : inout  std_logic_vector(C5_NUM_DQ_PINS-1 downto 0);
   mcb5_dram_a                             : out std_logic_vector(C5_MEM_ADDR_WIDTH-1 downto 0);
   mcb5_dram_ba                            : out std_logic_vector(C5_MEM_BANKADDR_WIDTH-1 downto 0);
   mcb5_dram_ras_n                         : out std_logic;
   mcb5_dram_cas_n                         : out std_logic;
   mcb5_dram_we_n                          : out std_logic;
   mcb5_dram_reset_n                       : out std_logic;
   mcb5_dram_cke                           : out std_logic;
   mcb5_dram_dm                            : out std_logic;
   mcb5_dram_udqs                          : inout  std_logic;
   mcb5_dram_udqs_n                        : inout  std_logic;
   mcb5_rzq                                : inout  std_logic;
   mcb5_dram_udm                           : out std_logic;
   c5_sys_clk                              : in  std_logic;
   c5_sys_rst_n                            : in  std_logic;
   c5_calib_done                           : out std_logic;
   c5_clk0                                 : out std_logic;
   c5_rst0                                 : out std_logic;
   mcb5_dram_dqs                           : inout  std_logic;
   mcb5_dram_dqs_n                         : inout  std_logic;
   mcb5_dram_ck                            : out std_logic;
   mcb5_dram_ck_n                          : out std_logic;
   c5_p0_cmd_clk                           : in std_logic;
   c5_p0_cmd_en                            : in std_logic;
   c5_p0_cmd_instr                         : in std_logic_vector(2 downto 0);
   c5_p0_cmd_bl                            : in std_logic_vector(5 downto 0);
   c5_p0_cmd_byte_addr                     : in std_logic_vector(29 downto 0);
   c5_p0_cmd_empty                         : out std_logic;
   c5_p0_cmd_full                          : out std_logic;
   c5_p0_wr_clk                            : in std_logic;
   c5_p0_wr_en                             : in std_logic;
   c5_p0_wr_mask                           : in std_logic_vector(C5_P0_MASK_SIZE - 1 downto 0);
   c5_p0_wr_data                           : in std_logic_vector(C5_P0_DATA_PORT_SIZE - 1 downto 0);
   c5_p0_wr_full                           : out std_logic;
   c5_p0_wr_empty                          : out std_logic;
   c5_p0_wr_count                          : out std_logic_vector(6 downto 0);
   c5_p0_wr_underrun                       : out std_logic;
   c5_p0_wr_error                          : out std_logic;
   c5_p0_rd_clk                            : in std_logic;
   c5_p0_rd_en                             : in std_logic;
   c5_p0_rd_data                           : out std_logic_vector(C5_P0_DATA_PORT_SIZE - 1 downto 0);
   c5_p0_rd_full                           : out std_logic;
   c5_p0_rd_empty                          : out std_logic;
   c5_p0_rd_count                          : out std_logic_vector(6 downto 0);
   c5_p0_rd_overflow                       : out std_logic;
   c5_p0_rd_error                          : out std_logic;
   c5_p1_cmd_clk                           : in std_logic;
   c5_p1_cmd_en                            : in std_logic;
   c5_p1_cmd_instr                         : in std_logic_vector(2 downto 0);
   c5_p1_cmd_bl                            : in std_logic_vector(5 downto 0);
   c5_p1_cmd_byte_addr                     : in std_logic_vector(29 downto 0);
   c5_p1_cmd_empty                         : out std_logic;
   c5_p1_cmd_full                          : out std_logic;
   c5_p1_wr_clk                            : in std_logic;
   c5_p1_wr_en                             : in std_logic;
   c5_p1_wr_mask                           : in std_logic_vector(C5_P1_MASK_SIZE - 1 downto 0);
   c5_p1_wr_data                           : in std_logic_vector(C5_P1_DATA_PORT_SIZE - 1 downto 0);
   c5_p1_wr_full                           : out std_logic;
   c5_p1_wr_empty                          : out std_logic;
   c5_p1_wr_count                          : out std_logic_vector(6 downto 0);
   c5_p1_wr_underrun                       : out std_logic;
   c5_p1_wr_error                          : out std_logic;
   c5_p1_rd_clk                            : in std_logic;
   c5_p1_rd_en                             : in std_logic;
   c5_p1_rd_data                           : out std_logic_vector(C5_P1_DATA_PORT_SIZE - 1 downto 0);
   c5_p1_rd_full                           : out std_logic;
   c5_p1_rd_empty                          : out std_logic;
   c5_p1_rd_count                          : out std_logic_vector(6 downto 0);
   c5_p1_rd_overflow                       : out std_logic;
   c5_p1_rd_error                          : out std_logic
);
end component;

-- COMP_TAG_END ------ End COMPONENT Declaration ------------

-- The following code must appear in the VHDL architecture
-- body. Substitute your own instance name and net names.

------------- Begin Cut here for INSTANTIATION Template ----- INST_TAG
  u_ddr_controller_bank5 : ddr_controller_bank5
    generic map (
    C5_P0_MASK_SIZE => C5_P0_MASK_SIZE,
    C5_P0_DATA_PORT_SIZE => C5_P0_DATA_PORT_SIZE,
    C5_P1_MASK_SIZE => C5_P1_MASK_SIZE,
    C5_P1_DATA_PORT_SIZE => C5_P1_DATA_PORT_SIZE,
    C5_MEMCLK_PERIOD => C5_MEMCLK_PERIOD,
    C5_RST_ACT_LOW => C5_RST_ACT_LOW,
    C5_INPUT_CLK_TYPE => C5_INPUT_CLK_TYPE,
    C5_CALIB_SOFT_IP => C5_CALIB_SOFT_IP,
    C5_SIMULATION => C5_SIMULATION,
    DEBUG_EN => DEBUG_EN,
    C5_MEM_ADDR_ORDER => C5_MEM_ADDR_ORDER,
    C5_NUM_DQ_PINS => C5_NUM_DQ_PINS,
    C5_MEM_ADDR_WIDTH => C5_MEM_ADDR_WIDTH,
    C5_MEM_BANKADDR_WIDTH => C5_MEM_BANKADDR_WIDTH
)
    port map (

    c5_sys_clk  =>         c5_sys_clk,
  c5_sys_rst_n    =>       c5_sys_rst_n,                        

  mcb5_dram_dq       =>    mcb5_dram_dq,  
  mcb5_dram_a        =>    mcb5_dram_a,  
  mcb5_dram_ba       =>    mcb5_dram_ba,
  mcb5_dram_ras_n    =>    mcb5_dram_ras_n,                        
  mcb5_dram_cas_n    =>    mcb5_dram_cas_n,                        
  mcb5_dram_we_n     =>    mcb5_dram_we_n,                          
  
  mcb5_dram_cke      =>    mcb5_dram_cke,                          
  mcb5_dram_ck       =>    mcb5_dram_ck,                          
  mcb5_dram_ck_n     =>    mcb5_dram_ck_n,       
  mcb5_dram_dqs      =>    mcb5_dram_dqs,                          
  mcb5_dram_dqs_n    =>    mcb5_dram_dqs_n,
  mcb5_dram_reset_n =>     mcb5_dram_reset_n,
  mcb5_dram_udqs  =>       mcb5_dram_udqs,    -- for X16 parts           
    mcb5_dram_udqs_n  =>       mcb5_dram_udqs_n,    -- for X16 parts
  mcb5_dram_udm  =>        mcb5_dram_udm,     -- for X16 parts
  mcb5_dram_dm  =>       mcb5_dram_dm,
    c5_clk0	=>	        c5_clk0,
  c5_rst0		=>        c5_rst0,
	
 
  c5_calib_done      =>    c5_calib_done,
     mcb5_rzq         =>            rzq5,
        
  
     c5_p0_cmd_clk                           =>  c5_p0_cmd_clk,
   c5_p0_cmd_en                            =>  c5_p0_cmd_en,
   c5_p0_cmd_instr                         =>  c5_p0_cmd_instr,
   c5_p0_cmd_bl                            =>  c5_p0_cmd_bl,
   c5_p0_cmd_byte_addr                     =>  c5_p0_cmd_byte_addr,
   c5_p0_cmd_empty                         =>  c5_p0_cmd_empty,
   c5_p0_cmd_full                          =>  c5_p0_cmd_full,
   c5_p0_wr_clk                            =>  c5_p0_wr_clk,
   c5_p0_wr_en                             =>  c5_p0_wr_en,
   c5_p0_wr_mask                           =>  c5_p0_wr_mask,
   c5_p0_wr_data                           =>  c5_p0_wr_data,
   c5_p0_wr_full                           =>  c5_p0_wr_full,
   c5_p0_wr_empty                          =>  c5_p0_wr_empty,
   c5_p0_wr_count                          =>  c5_p0_wr_count,
   c5_p0_wr_underrun                       =>  c5_p0_wr_underrun,
   c5_p0_wr_error                          =>  c5_p0_wr_error,
   c5_p0_rd_clk                            =>  c5_p0_rd_clk,
   c5_p0_rd_en                             =>  c5_p0_rd_en,
   c5_p0_rd_data                           =>  c5_p0_rd_data,
   c5_p0_rd_full                           =>  c5_p0_rd_full,
   c5_p0_rd_empty                          =>  c5_p0_rd_empty,
   c5_p0_rd_count                          =>  c5_p0_rd_count,
   c5_p0_rd_overflow                       =>  c5_p0_rd_overflow,
   c5_p0_rd_error                          =>  c5_p0_rd_error,
   c5_p1_cmd_clk                           =>  c5_p1_cmd_clk,
   c5_p1_cmd_en                            =>  c5_p1_cmd_en,
   c5_p1_cmd_instr                         =>  c5_p1_cmd_instr,
   c5_p1_cmd_bl                            =>  c5_p1_cmd_bl,
   c5_p1_cmd_byte_addr                     =>  c5_p1_cmd_byte_addr,
   c5_p1_cmd_empty                         =>  c5_p1_cmd_empty,
   c5_p1_cmd_full                          =>  c5_p1_cmd_full,
   c5_p1_wr_clk                            =>  c5_p1_wr_clk,
   c5_p1_wr_en                             =>  c5_p1_wr_en,
   c5_p1_wr_mask                           =>  c5_p1_wr_mask,
   c5_p1_wr_data                           =>  c5_p1_wr_data,
   c5_p1_wr_full                           =>  c5_p1_wr_full,
   c5_p1_wr_empty                          =>  c5_p1_wr_empty,
   c5_p1_wr_count                          =>  c5_p1_wr_count,
   c5_p1_wr_underrun                       =>  c5_p1_wr_underrun,
   c5_p1_wr_error                          =>  c5_p1_wr_error,
   c5_p1_rd_clk                            =>  c5_p1_rd_clk,
   c5_p1_rd_en                             =>  c5_p1_rd_en,
   c5_p1_rd_data                           =>  c5_p1_rd_data,
   c5_p1_rd_full                           =>  c5_p1_rd_full,
   c5_p1_rd_empty                          =>  c5_p1_rd_empty,
   c5_p1_rd_count                          =>  c5_p1_rd_count,
   c5_p1_rd_overflow                       =>  c5_p1_rd_overflow,
   c5_p1_rd_error                          =>  c5_p1_rd_error
);

-- INST_TAG_END ------ End INSTANTIATION Template ------------

-- You must compile the wrapper file ddr_controller_bank5.vhd when simulating
-- the core, ddr_controller_bank5. When compiling the wrapper file, be sure to
-- reference the XilinxCoreLib VHDL simulation library. For detailed
-- instructions, please refer to the "CORE Generator Help".

