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
--  /   /        Filename           : sim_tb_top.vhd
-- /___/   /\    Date Last Modified : $Date: 2011/06/02 07:16:58 $
-- \   \  /  \   Date Created       : Jul 03 2009
--  \___\/\___\
--
-- Device      : Spartan-6
-- Design Name : DDR/DDR2/DDR3/LPDDR
-- Purpose     : This is the simulation testbench which is used to verify the
--               design. The basic clocks and resets to the interface are
--               generated here. This also connects the memory interface to the
--               memory model.
--*****************************************************************************

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

library unisim;
use unisim.vcomponents.all;

entity sim_tb_top is

end entity sim_tb_top;

architecture arch of sim_tb_top is



-- ========================================================================== --
-- Parameters                                                                 --
-- ========================================================================== --
   constant DEBUG_EN              : integer :=0;
   
   constant C1_HW_TESTING      : string := "FALSE";
 
function c1_sim_hw (val1:std_logic_vector( 31 downto 0); val2: std_logic_vector( 31 downto 0) )  return  std_logic_vector is
   begin
   if (C1_HW_TESTING = "FALSE") then
     return val1;
   else
     return val2;
   end if;
   end function;		

   constant  C1_MEMCLK_PERIOD : integer    := 3000;
   constant C1_RST_ACT_LOW : integer := 0;
   constant C1_INPUT_CLK_TYPE : string := "SINGLE_ENDED";
   constant C1_CLK_PERIOD_NS   : real := 3000.0 / 1000.0;
   constant C1_TCYC_SYS        : real := C1_CLK_PERIOD_NS/2.0;
   constant C1_TCYC_SYS_DIV2   : time := C1_TCYC_SYS * 1 ns;
   constant C1_NUM_DQ_PINS        : integer := 16;
   constant C1_MEM_ADDR_WIDTH     : integer := 14;
   constant C1_MEM_BANKADDR_WIDTH : integer := 3;   
   constant C1_MEM_ADDR_ORDER     : string := "ROW_BANK_COLUMN"; 
      constant C1_P0_MASK_SIZE : integer      := 4;
   constant C1_P0_DATA_PORT_SIZE : integer := 32;  
   constant C1_P1_MASK_SIZE   : integer    := 4;
   constant C1_P1_DATA_PORT_SIZE  : integer := 32;
   constant C1_MEM_BURST_LEN	  : integer := 8;
   constant C1_MEM_NUM_COL_BITS   : integer := 10;
   constant C1_SIMULATION      : string := "TRUE";
   constant C1_CALIB_SOFT_IP      : string := "TRUE";
   constant C1_p0_BEGIN_ADDRESS                   : std_logic_vector(31 downto 0)  := c1_sim_hw (x"00000100", x"01000000");
   constant C1_p0_DATA_MODE                       : std_logic_vector(3 downto 0)  := "0010";
   constant C1_p0_END_ADDRESS                     : std_logic_vector(31 downto 0)  := c1_sim_hw (x"000002ff", x"02ffffff");
   constant C1_p0_PRBS_EADDR_MASK_POS             : std_logic_vector(31 downto 0)  := c1_sim_hw (x"fffffc00", x"fc000000");
   constant C1_p0_PRBS_SADDR_MASK_POS             : std_logic_vector(31 downto 0)  := c1_sim_hw (x"00000100", x"01000000");
   constant C1_p1_BEGIN_ADDRESS                   : std_logic_vector(31 downto 0)  := c1_sim_hw (x"00000300", x"03000000");
   constant C1_p1_DATA_MODE                       : std_logic_vector(3 downto 0)  := "0010";
   constant C1_p1_END_ADDRESS                     : std_logic_vector(31 downto 0)  := c1_sim_hw (x"000004ff", x"04ffffff");
   constant C1_p1_PRBS_EADDR_MASK_POS             : std_logic_vector(31 downto 0)  := c1_sim_hw (x"fffff800", x"f8000000");
   constant C1_p1_PRBS_SADDR_MASK_POS             : std_logic_vector(31 downto 0)  := c1_sim_hw (x"00000300", x"03000000");

-- ========================================================================== --
-- Component Declarations
-- ========================================================================== --

component ddr3_ctrl_vfc_bank1_32b_32b is
generic 
(
            C1_P0_MASK_SIZE         : integer;
    C1_P0_DATA_PORT_SIZE    : integer;
    C1_P1_MASK_SIZE         : integer;
    C1_P1_DATA_PORT_SIZE    : integer;
    
    C1_MEMCLK_PERIOD        : integer; 
    C1_RST_ACT_LOW          : integer;
    C1_INPUT_CLK_TYPE       : string;
    DEBUG_EN                : integer;

    C1_CALIB_SOFT_IP        : string;
    C1_SIMULATION           : string;
    C1_MEM_ADDR_ORDER       : string;
    C1_NUM_DQ_PINS          : integer; 
    C1_MEM_ADDR_WIDTH       : integer; 
    C1_MEM_BANKADDR_WIDTH   : integer
);  
  port
  (
        mcb1_dram_dq                            : inout  std_logic_vector(C1_NUM_DQ_PINS-1 downto 0);
   mcb1_dram_a                             : out std_logic_vector(C1_MEM_ADDR_WIDTH-1 downto 0);
   mcb1_dram_ba                            : out std_logic_vector(C1_MEM_BANKADDR_WIDTH-1 downto 0);
   mcb1_dram_ras_n                         : out std_logic;
   mcb1_dram_cas_n                         : out std_logic;
   mcb1_dram_we_n                          : out std_logic;
   mcb1_dram_odt                           : out std_logic;
   mcb1_dram_cke                           : out std_logic;
   mcb1_dram_dm                            : out std_logic;
      mcb1_rzq    				   : inout  std_logic;
        
   
        c1_sys_clk                            : in  std_logic;
   c1_sys_rst_i                            : in  std_logic;
	
   c1_calib_done                           : out std_logic;
        c1_clk0                                 : out std_logic;
   c1_rst0                                 : out std_logic;
 
   mcb1_dram_dqs                           : inout  std_logic;
   mcb1_dram_dqs_n                         : inout  std_logic;
   mcb1_dram_ck                            : out std_logic;
         mcb1_dram_udqs                           : inout  std_logic;
   mcb1_dram_udqs_n                         : inout  std_logic;
   mcb1_dram_udm                            : out std_logic;
   mcb1_dram_ck_n                          : out std_logic;
   mcb1_dram_reset_n                       : out std_logic;   c1_p0_cmd_clk                           : in std_logic;
   c1_p0_cmd_en                            : in std_logic;
   c1_p0_cmd_instr                         : in std_logic_vector(2 downto 0);
   c1_p0_cmd_bl                            : in std_logic_vector(5 downto 0);
   c1_p0_cmd_byte_addr                     : in std_logic_vector(29 downto 0);
   c1_p0_cmd_empty                         : out std_logic;
   c1_p0_cmd_full                          : out std_logic;
   c1_p0_wr_clk                            : in std_logic;
   c1_p0_wr_en                             : in std_logic;
   c1_p0_wr_mask                           : in std_logic_vector(C1_P0_MASK_SIZE - 1 downto 0);
   c1_p0_wr_data                           : in std_logic_vector(C1_P0_DATA_PORT_SIZE - 1 downto 0);
   c1_p0_wr_full                           : out std_logic;
   c1_p0_wr_empty                          : out std_logic;
   c1_p0_wr_count                          : out std_logic_vector(6 downto 0);
   c1_p0_wr_underrun                       : out std_logic;
   c1_p0_wr_error                          : out std_logic;
   c1_p0_rd_clk                            : in std_logic;
   c1_p0_rd_en                             : in std_logic;
   c1_p0_rd_data                           : out std_logic_vector(C1_P0_DATA_PORT_SIZE - 1 downto 0);
   c1_p0_rd_full                           : out std_logic;
   c1_p0_rd_empty                          : out std_logic;
   c1_p0_rd_count                          : out std_logic_vector(6 downto 0);
   c1_p0_rd_overflow                       : out std_logic;
   c1_p0_rd_error                          : out std_logic;
   c1_p1_cmd_clk                           : in std_logic;
   c1_p1_cmd_en                            : in std_logic;
   c1_p1_cmd_instr                         : in std_logic_vector(2 downto 0);
   c1_p1_cmd_bl                            : in std_logic_vector(5 downto 0);
   c1_p1_cmd_byte_addr                     : in std_logic_vector(29 downto 0);
   c1_p1_cmd_empty                         : out std_logic;
   c1_p1_cmd_full                          : out std_logic;
   c1_p1_wr_clk                            : in std_logic;
   c1_p1_wr_en                             : in std_logic;
   c1_p1_wr_mask                           : in std_logic_vector(C1_P1_MASK_SIZE - 1 downto 0);
   c1_p1_wr_data                           : in std_logic_vector(C1_P1_DATA_PORT_SIZE - 1 downto 0);
   c1_p1_wr_full                           : out std_logic;
   c1_p1_wr_empty                          : out std_logic;
   c1_p1_wr_count                          : out std_logic_vector(6 downto 0);
   c1_p1_wr_underrun                       : out std_logic;
   c1_p1_wr_error                          : out std_logic;
   c1_p1_rd_clk                            : in std_logic;
   c1_p1_rd_en                             : in std_logic;
   c1_p1_rd_data                           : out std_logic_vector(C1_P1_DATA_PORT_SIZE - 1 downto 0);
   c1_p1_rd_full                           : out std_logic;
   c1_p1_rd_empty                          : out std_logic;
   c1_p1_rd_count                          : out std_logic_vector(6 downto 0);
   c1_p1_rd_overflow                       : out std_logic;
   c1_p1_rd_error                          : out std_logic
  );
end component;


        component ddr3_model_c1 is
    port (
      ck      : in    std_logic;
      ck_n    : in    std_logic;
      cke     : in    std_logic;
      cs_n    : in    std_logic;
      ras_n   : in    std_logic;
      cas_n   : in    std_logic;
      we_n    : in    std_logic;
      dm_tdqs : inout std_logic_vector((C1_NUM_DQ_PINS/16) downto 0);
      ba      : in    std_logic_vector((C1_MEM_BANKADDR_WIDTH - 1) downto 0);
      addr    : in    std_logic_vector((C1_MEM_ADDR_WIDTH  - 1) downto 0);
      dq      : inout std_logic_vector((C1_NUM_DQ_PINS - 1) downto 0);
      dqs     : inout std_logic_vector((C1_NUM_DQ_PINS/16) downto 0);
      dqs_n   : inout std_logic_vector((C1_NUM_DQ_PINS/16) downto 0);
      tdqs_n  : out   std_logic_vector((C1_NUM_DQ_PINS/16) downto 0);
	  odt     : in    std_logic;
      rst_n   : in    std_logic
      );
  end component;
component memc1_tb_top is
generic
  (
      C_P0_MASK_SIZE                   : integer := 4;
      C_P0_DATA_PORT_SIZE              : integer := 32;
      C_P1_MASK_SIZE                   : integer := 4;
      C_P1_DATA_PORT_SIZE              : integer := 32;
      C_MEM_BURST_LEN                  : integer := 8;
      C_MEM_NUM_COL_BITS               : integer := 11;
      C_NUM_DQ_PINS                    : integer := 8;
	        C_p0_BEGIN_ADDRESS                      : std_logic_vector(31 downto 0)  := X"00000100";
      C_p0_DATA_MODE                          : std_logic_vector(3 downto 0)  := "0010";
      C_p0_END_ADDRESS                        : std_logic_vector(31 downto 0)  := X"000002ff";
      C_p0_PRBS_EADDR_MASK_POS                : std_logic_vector(31 downto 0)  := X"fffffc00";
      C_p0_PRBS_SADDR_MASK_POS                : std_logic_vector(31 downto 0)  := X"00000100";
      C_p1_BEGIN_ADDRESS                      : std_logic_vector(31 downto 0)  := X"00000300";
      C_p1_DATA_MODE                          : std_logic_vector(3 downto 0)  := "0010";
      C_p1_END_ADDRESS                        : std_logic_vector(31 downto 0)  := X"000004ff";
      C_p1_PRBS_EADDR_MASK_POS                : std_logic_vector(31 downto 0)  := X"fffff800";
      C_p1_PRBS_SADDR_MASK_POS                : std_logic_vector(31 downto 0)  := X"00000300"

  );
port
(

   clk0            : in std_logic;
   rst0            : in std_logic;
   calib_done      : in std_logic;

         p0_mcb_cmd_en_o                           : out std_logic;
      p0_mcb_cmd_instr_o                        : out std_logic_vector(2 downto 0);
      p0_mcb_cmd_bl_o                           : out std_logic_vector(5 downto 0);
      p0_mcb_cmd_addr_o                         : out std_logic_vector(29 downto 0);
      p0_mcb_cmd_full_i                         : in std_logic;

      p0_mcb_wr_en_o                            : out std_logic;
      p0_mcb_wr_mask_o                          : out std_logic_vector(C_P0_MASK_SIZE - 1 downto 0);
      p0_mcb_wr_data_o                          : out std_logic_vector(C_P0_DATA_PORT_SIZE - 1 downto 0);
      p0_mcb_wr_full_i                          : in std_logic;
      p0_mcb_wr_fifo_counts                     : in std_logic_vector(6 downto 0);

      p0_mcb_rd_en_o                            : out std_logic;
      p0_mcb_rd_data_i                          : in std_logic_vector(C_P0_DATA_PORT_SIZE - 1 downto 0);
      p0_mcb_rd_empty_i                         : in std_logic;
      p0_mcb_rd_fifo_counts                     : in std_logic_vector(6 downto 0);

      p1_mcb_cmd_en_o                           : out std_logic;
      p1_mcb_cmd_instr_o                        : out std_logic_vector(2 downto 0);
      p1_mcb_cmd_bl_o                           : out std_logic_vector(5 downto 0);
      p1_mcb_cmd_addr_o                         : out std_logic_vector(29 downto 0);
      p1_mcb_cmd_full_i                         : in std_logic;

      p1_mcb_wr_en_o                            : out std_logic;
      p1_mcb_wr_mask_o                          : out std_logic_vector(C_P1_MASK_SIZE - 1 downto 0);
      p1_mcb_wr_data_o                          : out std_logic_vector(C_P1_DATA_PORT_SIZE - 1 downto 0);
      p1_mcb_wr_full_i                          : in std_logic;
      p1_mcb_wr_fifo_counts                     : in std_logic_vector(6 downto 0);

      p1_mcb_rd_en_o                            : out std_logic;
      p1_mcb_rd_data_i                          : in std_logic_vector(C_P1_DATA_PORT_SIZE - 1 downto 0);
      p1_mcb_rd_empty_i                         : in std_logic;
      p1_mcb_rd_fifo_counts                     : in std_logic_vector(6 downto 0);



   vio_modify_enable   : in std_logic;
   vio_data_mode_value : in std_logic_vector(2 downto 0);
   vio_addr_mode_value : in std_logic_vector(2 downto 0);
   cmp_error       : out std_logic;
   error           : out std_logic;
   error_status    : out std_logic_vector(127 downto 0)
);
end component;

-- ========================================================================== --
-- Signal Declarations                                                        --
-- ========================================================================== --

-- Clocks
					-- Clocks
   signal  c1_sys_clk     : std_logic := '0';
   signal  c1_sys_clk_p   : std_logic;
   signal  c1_sys_clk_n   : std_logic;
-- System Reset
   signal  c1_sys_rst   : std_logic := '0';
   signal  c1_sys_rst_i     : std_logic;



-- Design-Top Port Map
   signal  c1_error  : std_logic;
   signal  c1_calib_done : std_logic;
   signal  c1_error_status : std_logic_vector(127 downto 0); 
   signal  mcb1_dram_a : std_logic_vector(C1_MEM_ADDR_WIDTH-1 downto 0);
   signal  mcb1_dram_ba : std_logic_vector(C1_MEM_BANKADDR_WIDTH-1 downto 0);  
   signal  mcb1_dram_ck : std_logic;  
   signal  mcb1_dram_ck_n : std_logic;  
   signal  mcb1_dram_dq : std_logic_vector(C1_NUM_DQ_PINS-1 downto 0);   
   signal  mcb1_dram_dqs   : std_logic;    
   signal  mcb1_dram_dqs_n : std_logic;  
   signal  mcb1_dram_dm    : std_logic;   
   signal  mcb1_dram_ras_n : std_logic;   
   signal  mcb1_dram_cas_n : std_logic;   
   signal  mcb1_dram_we_n  : std_logic;    
   signal  mcb1_dram_cke   : std_logic;   
   signal  mcb1_dram_odt   : std_logic;  
   signal  mcb1_dram_reset_n : std_logic; 
      signal  mcb1_dram_udqs   : std_logic;
   signal  mcb1_dram_udqs_n : std_logic;
   signal mcb1_dram_dqs_vector : std_logic_vector(1 downto 0);
   signal mcb1_dram_dqs_n_vector : std_logic_vector(1 downto 0);
      signal   mcb1_dram_udm :std_logic;     -- for X16 parts
   signal mcb1_dram_dm_vector : std_logic_vector(1 downto 0);
   
   
-- User design  Sim
   signal  c1_clk0 : std_logic;
   signal  c1_rst0 : std_logic;  
   signal  c1_cmp_error : std_logic; 
   signal  c1_vio_modify_enable : std_logic;
   signal  c1_vio_data_mode_value : std_logic_vector(2 downto 0);
   signal  c1_vio_addr_mode_value : std_logic_vector(2 downto 0);
   signal mcb1_command               : std_logic_vector(2 downto 0);
   signal mcb1_enable1                : std_logic;
   signal mcb1_enable2              : std_logic;

     signal  c1_p0_cmd_en                             : std_logic;
  signal  c1_p0_cmd_instr                          : std_logic_vector(2 downto 0);
  signal  c1_p0_cmd_bl                             : std_logic_vector(5 downto 0);
  signal  c1_p0_cmd_byte_addr                      : std_logic_vector(29 downto 0);
  signal  c1_p0_cmd_empty                          : std_logic;
  signal  c1_p0_cmd_full                           : std_logic;
  signal  c1_p0_wr_en                              : std_logic;
  signal  c1_p0_wr_mask                            : std_logic_vector(C1_P0_MASK_SIZE - 1 downto 0);
  signal  c1_p0_wr_data                            : std_logic_vector(C1_P0_DATA_PORT_SIZE - 1 downto 0);
  signal  c1_p0_wr_full                            : std_logic;
  signal  c1_p0_wr_empty                           : std_logic;
  signal  c1_p0_wr_count                           : std_logic_vector(6 downto 0);
  signal  c1_p0_wr_underrun                        : std_logic;
  signal  c1_p0_wr_error                           : std_logic;
  signal  c1_p0_rd_en                              : std_logic;
  signal  c1_p0_rd_data                            : std_logic_vector(C1_P0_DATA_PORT_SIZE - 1 downto 0);
  signal  c1_p0_rd_full                            : std_logic;
  signal  c1_p0_rd_empty                           : std_logic;
  signal  c1_p0_rd_count                           : std_logic_vector(6 downto 0);
  signal  c1_p0_rd_overflow                        : std_logic;
  signal  c1_p0_rd_error                           : std_logic;

  signal  c1_p1_cmd_en                             : std_logic;
  signal  c1_p1_cmd_instr                          : std_logic_vector(2 downto 0);
  signal  c1_p1_cmd_bl                             : std_logic_vector(5 downto 0);
  signal  c1_p1_cmd_byte_addr                      : std_logic_vector(29 downto 0);
  signal  c1_p1_cmd_empty                          : std_logic;
  signal  c1_p1_cmd_full                           : std_logic;
  signal  c1_p1_wr_en                              : std_logic;
  signal  c1_p1_wr_mask                            : std_logic_vector(C1_P1_MASK_SIZE - 1 downto 0);
  signal  c1_p1_wr_data                            : std_logic_vector(C1_P1_DATA_PORT_SIZE - 1 downto 0);
  signal  c1_p1_wr_full                            : std_logic;
  signal  c1_p1_wr_empty                           : std_logic;
  signal  c1_p1_wr_count                           : std_logic_vector(6 downto 0);
  signal  c1_p1_wr_underrun                        : std_logic;
  signal  c1_p1_wr_error                           : std_logic;
  signal  c1_p1_rd_en                              : std_logic;
  signal  c1_p1_rd_data                            : std_logic_vector(C1_P1_DATA_PORT_SIZE - 1 downto 0);
  signal  c1_p1_rd_full                            : std_logic;
  signal  c1_p1_rd_empty                           : std_logic;
  signal  c1_p1_rd_count                           : std_logic_vector(6 downto 0);
  signal  c1_p1_rd_overflow                        : std_logic;
  signal  c1_p1_rd_error                           : std_logic;

  signal  c1_selfrefresh_enter                     : std_logic;
  signal  c1_selfrefresh_mode                      : std_logic;


   signal  rzq1     : std_logic;
      

   signal   calib_done  : std_logic;
   signal   error  : std_logic;


function vector (asi:std_logic) return std_logic_vector is
  variable v : std_logic_vector(0 downto 0) ; 
begin
  v(0) := asi;
  return(v); 
end function vector; 

begin
-- ========================================================================== --
-- Clocks Generation                                                          --
-- ========================================================================== --


  process
  begin
    c1_sys_clk <= not c1_sys_clk;
    wait for (C1_TCYC_SYS_DIV2);
  end process;

  c1_sys_clk_p <= c1_sys_clk;
  c1_sys_clk_n <= not c1_sys_clk;

-- ========================================================================== --
-- Reset Generation                                                           --
-- ========================================================================== --
 
 process
  begin
    c1_sys_rst <= '0';
    wait for 200 ns;
    c1_sys_rst <= '1';
    wait;
  end process;

    c1_sys_rst_i <= c1_sys_rst when (C1_RST_ACT_LOW = 1) else (not c1_sys_rst);


error <= c1_error;
calib_done <= c1_calib_done;

   


   rzq_pulldown1 : PULLDOWN port map(O => rzq1);
      

-- ========================================================================== --
-- DESIGN TOP INSTANTIATION                                                    --
-- ========================================================================== --

design_top : ddr3_ctrl_vfc_bank1_32b_32b generic map
(
  
C1_P0_MASK_SIZE  =>     C1_P0_MASK_SIZE,
C1_P0_DATA_PORT_SIZE  => C1_P0_DATA_PORT_SIZE,
C1_P1_MASK_SIZE       => C1_P1_MASK_SIZE,
C1_P1_DATA_PORT_SIZE  => C1_P1_DATA_PORT_SIZE, 
	C1_MEMCLK_PERIOD  =>       C1_MEMCLK_PERIOD,
C1_RST_ACT_LOW    =>     C1_RST_ACT_LOW,
C1_INPUT_CLK_TYPE =>     C1_INPUT_CLK_TYPE, 
DEBUG_EN              =>     DEBUG_EN,

C1_MEM_ADDR_ORDER     => C1_MEM_ADDR_ORDER,
C1_NUM_DQ_PINS        => C1_NUM_DQ_PINS,
C1_MEM_ADDR_WIDTH     => C1_MEM_ADDR_WIDTH,
C1_MEM_BANKADDR_WIDTH => C1_MEM_BANKADDR_WIDTH,

C1_SIMULATION   =>      C1_SIMULATION,

C1_CALIB_SOFT_IP      => C1_CALIB_SOFT_IP
) 
port map ( 

    c1_sys_clk  =>         c1_sys_clk,
  c1_sys_rst_i    =>       c1_sys_rst_i,                        

  mcb1_dram_dq       =>    mcb1_dram_dq,  
  mcb1_dram_a        =>    mcb1_dram_a,  
  mcb1_dram_ba       =>    mcb1_dram_ba,
  mcb1_dram_ras_n    =>    mcb1_dram_ras_n,                        
  mcb1_dram_cas_n    =>    mcb1_dram_cas_n,                        
  mcb1_dram_we_n     =>    mcb1_dram_we_n,                          
  mcb1_dram_odt    =>      mcb1_dram_odt,
  mcb1_dram_cke      =>    mcb1_dram_cke,                          
  mcb1_dram_ck       =>    mcb1_dram_ck,                          
  mcb1_dram_ck_n     =>    mcb1_dram_ck_n,       
  mcb1_dram_dqs      =>    mcb1_dram_dqs,                          
  mcb1_dram_dqs_n    =>    mcb1_dram_dqs_n,
  mcb1_dram_reset_n =>     mcb1_dram_reset_n,
  mcb1_dram_udqs  =>       mcb1_dram_udqs,    -- for X16 parts           
    mcb1_dram_udqs_n  =>       mcb1_dram_udqs_n,    -- for X16 parts
  mcb1_dram_udm  =>        mcb1_dram_udm,     -- for X16 parts
  mcb1_dram_dm  =>       mcb1_dram_dm,
    c1_clk0	=>	        c1_clk0,
  c1_rst0		=>        c1_rst0,
	
 
  c1_calib_done      =>    c1_calib_done,
     mcb1_rzq         =>            rzq1,
        
  
     c1_p0_cmd_clk                           =>  (c1_clk0),
   c1_p0_cmd_en                            =>  c1_p0_cmd_en,
   c1_p0_cmd_instr                         =>  c1_p0_cmd_instr,
   c1_p0_cmd_bl                            =>  c1_p0_cmd_bl,
   c1_p0_cmd_byte_addr                     =>  c1_p0_cmd_byte_addr,
   c1_p0_cmd_empty                         =>  c1_p0_cmd_empty,
   c1_p0_cmd_full                          =>  c1_p0_cmd_full,
   c1_p0_wr_clk                            =>  (c1_clk0),
   c1_p0_wr_en                             =>  c1_p0_wr_en,
   c1_p0_wr_mask                           =>  c1_p0_wr_mask,
   c1_p0_wr_data                           =>  c1_p0_wr_data,
   c1_p0_wr_full                           =>  c1_p0_wr_full,
   c1_p0_wr_empty                          =>  c1_p0_wr_empty,
   c1_p0_wr_count                          =>  c1_p0_wr_count,
   c1_p0_wr_underrun                       =>  c1_p0_wr_underrun,
   c1_p0_wr_error                          =>  c1_p0_wr_error,
   c1_p0_rd_clk                            =>  (c1_clk0),
   c1_p0_rd_en                             =>  c1_p0_rd_en,
   c1_p0_rd_data                           =>  c1_p0_rd_data,
   c1_p0_rd_full                           =>  c1_p0_rd_full,
   c1_p0_rd_empty                          =>  c1_p0_rd_empty,
   c1_p0_rd_count                          =>  c1_p0_rd_count,
   c1_p0_rd_overflow                       =>  c1_p0_rd_overflow,
   c1_p0_rd_error                          =>  c1_p0_rd_error,
   c1_p1_cmd_clk                           =>  (c1_clk0),
   c1_p1_cmd_en                            =>  c1_p1_cmd_en,
   c1_p1_cmd_instr                         =>  c1_p1_cmd_instr,
   c1_p1_cmd_bl                            =>  c1_p1_cmd_bl,
   c1_p1_cmd_byte_addr                     =>  c1_p1_cmd_byte_addr,
   c1_p1_cmd_empty                         =>  c1_p1_cmd_empty,
   c1_p1_cmd_full                          =>  c1_p1_cmd_full,
   c1_p1_wr_clk                            =>  (c1_clk0),
   c1_p1_wr_en                             =>  c1_p1_wr_en,
   c1_p1_wr_mask                           =>  c1_p1_wr_mask,
   c1_p1_wr_data                           =>  c1_p1_wr_data,
   c1_p1_wr_full                           =>  c1_p1_wr_full,
   c1_p1_wr_empty                          =>  c1_p1_wr_empty,
   c1_p1_wr_count                          =>  c1_p1_wr_count,
   c1_p1_wr_underrun                       =>  c1_p1_wr_underrun,
   c1_p1_wr_error                          =>  c1_p1_wr_error,
   c1_p1_rd_clk                            =>  (c1_clk0),
   c1_p1_rd_en                             =>  c1_p1_rd_en,
   c1_p1_rd_data                           =>  c1_p1_rd_data,
   c1_p1_rd_full                           =>  c1_p1_rd_full,
   c1_p1_rd_empty                          =>  c1_p1_rd_empty,
   c1_p1_rd_count                          =>  c1_p1_rd_count,
   c1_p1_rd_overflow                       =>  c1_p1_rd_overflow,
   c1_p1_rd_error                          =>  c1_p1_rd_error
);      

-- user interface

memc1_tb_top_inst :  memc1_tb_top generic map
 (
   C_NUM_DQ_PINS       =>     C1_NUM_DQ_PINS,
   C_MEM_BURST_LEN     =>     C1_MEM_BURST_LEN,
   C_MEM_NUM_COL_BITS  =>     C1_MEM_NUM_COL_BITS,
   C_P0_MASK_SIZE      =>     C1_P0_MASK_SIZE,
   C_P0_DATA_PORT_SIZE =>     C1_P0_DATA_PORT_SIZE,        
   C_P1_MASK_SIZE      =>     C1_P1_MASK_SIZE,        
   C_P1_DATA_PORT_SIZE =>     C1_P1_DATA_PORT_SIZE,        
   C_p0_BEGIN_ADDRESS                      => C1_p0_BEGIN_ADDRESS,
   C_p0_DATA_MODE                          => C1_p0_DATA_MODE,
   C_p0_END_ADDRESS                        => C1_p0_END_ADDRESS,
   C_p0_PRBS_EADDR_MASK_POS                => C1_p0_PRBS_EADDR_MASK_POS,
   C_p0_PRBS_SADDR_MASK_POS                => C1_p0_PRBS_SADDR_MASK_POS,
   C_p1_BEGIN_ADDRESS                      => C1_p1_BEGIN_ADDRESS,
   C_p1_DATA_MODE                          => C1_p1_DATA_MODE,
   C_p1_END_ADDRESS                        => C1_p1_END_ADDRESS,
   C_p1_PRBS_EADDR_MASK_POS                => C1_p1_PRBS_EADDR_MASK_POS,
   C_p1_PRBS_SADDR_MASK_POS                => C1_p1_PRBS_SADDR_MASK_POS
   )
port map
(
   clk0			         => c1_clk0,
   rst0			         => c1_rst0,
   calib_done            => c1_calib_done, 
   cmp_error             => c1_cmp_error,
   error                 => c1_error,
   error_status          => c1_error_status,
   vio_modify_enable     => c1_vio_modify_enable,
   vio_data_mode_value   => c1_vio_data_mode_value,
   vio_addr_mode_value   => c1_vio_addr_mode_value,
   p0_mcb_cmd_en_o                          =>  c1_p0_cmd_en,
   p0_mcb_cmd_instr_o                       =>  c1_p0_cmd_instr,
   p0_mcb_cmd_bl_o                          =>  c1_p0_cmd_bl,
   p0_mcb_cmd_addr_o                        =>  c1_p0_cmd_byte_addr,
   p0_mcb_cmd_full_i                        =>  c1_p0_cmd_full,
   p0_mcb_wr_en_o                           =>  c1_p0_wr_en,
   p0_mcb_wr_mask_o                         =>  c1_p0_wr_mask,
   p0_mcb_wr_data_o                         =>  c1_p0_wr_data,
   p0_mcb_wr_full_i                         =>  c1_p0_wr_full,
   p0_mcb_wr_fifo_counts                    =>  c1_p0_wr_count,
   p0_mcb_rd_en_o                           =>  c1_p0_rd_en,
   p0_mcb_rd_data_i                         =>  c1_p0_rd_data,
   p0_mcb_rd_empty_i                        =>  c1_p0_rd_empty,
   p0_mcb_rd_fifo_counts                    =>  c1_p0_rd_count,
   p1_mcb_cmd_en_o                          =>  c1_p1_cmd_en,
   p1_mcb_cmd_instr_o                       =>  c1_p1_cmd_instr,
   p1_mcb_cmd_bl_o                          =>  c1_p1_cmd_bl,
   p1_mcb_cmd_addr_o                        =>  c1_p1_cmd_byte_addr,
   p1_mcb_cmd_full_i                        =>  c1_p1_cmd_full,
   p1_mcb_wr_en_o                           =>  c1_p1_wr_en,
   p1_mcb_wr_mask_o                         =>  c1_p1_wr_mask,
   p1_mcb_wr_data_o                         =>  c1_p1_wr_data,
   p1_mcb_wr_full_i                         =>  c1_p1_wr_full,
   p1_mcb_wr_fifo_counts                    =>  c1_p1_wr_count,
   p1_mcb_rd_en_o                           =>  c1_p1_rd_en,
   p1_mcb_rd_data_i                         =>  c1_p1_rd_data,
   p1_mcb_rd_empty_i                        =>  c1_p1_rd_empty,
   p1_mcb_rd_fifo_counts                    =>  c1_p1_rd_count
  

  );

-- ========================================================================== --
-- Memory model instances                                                     -- 
-- ========================================================================== --

    mcb1_command <= (mcb1_dram_ras_n & mcb1_dram_cas_n & mcb1_dram_we_n);

    process(mcb1_dram_ck)
    begin
      if (rising_edge(mcb1_dram_ck)) then
        if (c1_sys_rst = '0') then
          mcb1_enable1   <= '0';
          mcb1_enable2 <= '0';
        elsif (mcb1_command = "100") then
          mcb1_enable2 <= '0';
        elsif (mcb1_command = "101") then
          mcb1_enable2 <= '1';
        else
          mcb1_enable2 <= mcb1_enable2;
        end if;
        mcb1_enable1     <= mcb1_enable2;
      end if;
    end process;

-----------------------------------------------------------------------------
--read
-----------------------------------------------------------------------------
    mcb1_dram_dqs_vector(1 downto 0)               <= (mcb1_dram_udqs & mcb1_dram_dqs)
                                                           when (mcb1_enable2 = '0' and mcb1_enable1 = '0')
							   else "ZZ";
    mcb1_dram_dqs_n_vector(1 downto 0)             <= (mcb1_dram_udqs_n & mcb1_dram_dqs_n)
                                                           when (mcb1_enable2 = '0' and mcb1_enable1 = '0')
							   else "ZZ";
    
-----------------------------------------------------------------------------
--write
-----------------------------------------------------------------------------
    mcb1_dram_dqs          <= mcb1_dram_dqs_vector(0)
                              when ( mcb1_enable1 = '1') else 'Z';

    mcb1_dram_udqs          <= mcb1_dram_dqs_vector(1)
                              when (mcb1_enable1 = '1') else 'Z';


     mcb1_dram_dqs_n        <= mcb1_dram_dqs_n_vector(0)
                              when (mcb1_enable1 = '1') else 'Z';
    mcb1_dram_udqs_n         <= mcb1_dram_dqs_n_vector(1)
                              when (mcb1_enable1 = '1') else 'Z';

   
   

mcb1_dram_dm_vector <= (mcb1_dram_udm & mcb1_dram_dm);

     u_mem_c1 : ddr3_model_c1 port map
	 (
      ck        => mcb1_dram_ck,
      ck_n      => mcb1_dram_ck_n,
      cke       => mcb1_dram_cke,
      cs_n      => '0',
      ras_n     => mcb1_dram_ras_n,
      cas_n     => mcb1_dram_cas_n,
      we_n      => mcb1_dram_we_n,
      dm_tdqs   => mcb1_dram_dm_vector,
      ba        => mcb1_dram_ba,
      addr      => mcb1_dram_a,
      dq        => mcb1_dram_dq,
      dqs       => mcb1_dram_dqs_vector,
      dqs_n     => mcb1_dram_dqs_n_vector,
      tdqs_n    => open,
      odt       => mcb1_dram_odt,
      rst_n     => mcb1_dram_reset_n
      );

-----------------------------------------------------------------------------     
-- Reporting the test case status 
-----------------------------------------------------------------------------
   Logging: process 
   begin
      wait for 200 us;
      if (calib_done = '1') then
         if (error = '0') then
   	    report ("****TEST PASSED****");
         else
            report ("****TEST FAILED: DATA ERROR****");
         end if;
      else
         report ("****TEST FAILED: INITIALIZATION DID NOT COMPLETE****");
      end if;
   end process;   

end architecture;
