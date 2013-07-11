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
   
   constant C4_HW_TESTING      : string := "FALSE";
 
function c4_sim_hw (val1:std_logic_vector( 31 downto 0); val2: std_logic_vector( 31 downto 0) )  return  std_logic_vector is
   begin
   if (C4_HW_TESTING = "FALSE") then
     return val1;
   else
     return val2;
   end if;
   end function;		

   constant  C4_MEMCLK_PERIOD : integer    := 3000;
   constant C4_RST_ACT_LOW : integer := 0;
   constant C4_INPUT_CLK_TYPE : string := "SINGLE_ENDED";
   constant C4_CLK_PERIOD_NS   : real := 3000.0 / 1000.0;
   constant C4_TCYC_SYS        : real := C4_CLK_PERIOD_NS/2.0;
   constant C4_TCYC_SYS_DIV2   : time := C4_TCYC_SYS * 1 ns;
   constant C4_NUM_DQ_PINS        : integer := 16;
   constant C4_MEM_ADDR_WIDTH     : integer := 14;
   constant C4_MEM_BANKADDR_WIDTH : integer := 3;
   constant C4_MEM_ADDR_ORDER     : string := "ROW_BANK_COLUMN"; 
      constant C4_P0_MASK_SIZE : integer      := 8;
   constant C4_P0_DATA_PORT_SIZE : integer := 64;  
   constant C4_P1_MASK_SIZE   : integer    := 4;
   constant C4_P1_DATA_PORT_SIZE  : integer := 32;
   constant C4_CALIB_SOFT_IP      : string := "TRUE";
   constant C4_SIMULATION      : string := "TRUE";
   

-- ========================================================================== --
-- Component Declarations
-- ========================================================================== --

component example_top is
generic 
(
            C4_P0_MASK_SIZE         : integer;
    C4_P0_DATA_PORT_SIZE    : integer;
    C4_P1_MASK_SIZE         : integer;
    C4_P1_DATA_PORT_SIZE    : integer;
    
    C4_MEMCLK_PERIOD        : integer; 
    C4_RST_ACT_LOW          : integer;
    C4_INPUT_CLK_TYPE       : string;
    DEBUG_EN                : integer;

    C4_CALIB_SOFT_IP        : string;
    C4_SIMULATION           : string;
    C4_HW_TESTING           : string;
    C4_MEM_ADDR_ORDER       : string;
    C4_NUM_DQ_PINS          : integer; 
    C4_MEM_ADDR_WIDTH       : integer; 
    C4_MEM_BANKADDR_WIDTH   : integer
);  
  port
  (
        calib_done                    : out std_logic;
   error                                 : out std_logic;
   mcb4_dram_dq                            : inout  std_logic_vector(C4_NUM_DQ_PINS-1 downto 0);
   mcb4_dram_a                             : out std_logic_vector(C4_MEM_ADDR_WIDTH-1 downto 0);
   mcb4_dram_ba                            : out std_logic_vector(C4_MEM_BANKADDR_WIDTH-1 downto 0);
   mcb4_dram_ras_n                         : out std_logic;
   mcb4_dram_cas_n                         : out std_logic;
   mcb4_dram_we_n                          : out std_logic;
   mcb4_dram_odt                           : out std_logic;
   mcb4_dram_cke                           : out std_logic;
   mcb4_dram_dm                            : out std_logic;
      mcb4_rzq    				   : inout  std_logic;
        
   
        c4_sys_clk                            : in  std_logic;
   c4_sys_rst_i                            : in  std_logic;
	
   mcb4_dram_dqs                           : inout  std_logic;
   mcb4_dram_dqs_n                         : inout  std_logic;
   mcb4_dram_ck                            : out std_logic;
   mcb4_dram_ck_n                          : out std_logic;
      mcb4_dram_udqs                           : inout  std_logic;
   mcb4_dram_udqs_n                         : inout  std_logic;
   mcb4_dram_udm                            : out std_logic;
   mcb4_dram_reset_n                       : out std_logic
  );
end component;


        component ddr3_model_c4 is
    port (
      ck      : in    std_logic;
      ck_n    : in    std_logic;
      cke     : in    std_logic;
      cs_n    : in    std_logic;
      ras_n   : in    std_logic;
      cas_n   : in    std_logic;
      we_n    : in    std_logic;
      dm_tdqs : inout std_logic_vector((C4_NUM_DQ_PINS/16) downto 0);
      ba      : in    std_logic_vector((C4_MEM_BANKADDR_WIDTH - 1) downto 0);
      addr    : in    std_logic_vector((C4_MEM_ADDR_WIDTH  - 1) downto 0);
      dq      : inout std_logic_vector((C4_NUM_DQ_PINS - 1) downto 0);
      dqs     : inout std_logic_vector((C4_NUM_DQ_PINS/16) downto 0);
      dqs_n   : inout std_logic_vector((C4_NUM_DQ_PINS/16) downto 0);
      tdqs_n  : out   std_logic_vector((C4_NUM_DQ_PINS/16) downto 0);
	  odt     : in    std_logic;
      rst_n   : in    std_logic
      );
  end component;

-- ========================================================================== --
-- Signal Declarations                                                        --
-- ========================================================================== --

 			-- Clocks
   signal  c4_sys_clk     : std_logic := '0';
   signal  c4_sys_clk_p   : std_logic;
   signal  c4_sys_clk_n   : std_logic;
-- System Reset
   signal  c4_sys_rst   : std_logic := '0';
   signal  c4_sys_rst_i     : std_logic;



-- Design-Top Port Map   
   signal mcb4_dram_a : std_logic_vector(C4_MEM_ADDR_WIDTH-1 downto 0);
   signal mcb4_dram_ba : std_logic_vector(C4_MEM_BANKADDR_WIDTH-1 downto 0);  
   signal  mcb4_dram_ck : std_logic;  
   signal  mcb4_dram_ck_n : std_logic;  
   signal  mcb4_dram_dq : std_logic_vector(C4_NUM_DQ_PINS-1 downto 0);   
   signal  mcb4_dram_dqs   : std_logic;    
   signal  mcb4_dram_dqs_n : std_logic;  
   signal  mcb4_dram_dm    : std_logic;   
   signal  mcb4_dram_ras_n : std_logic;   
   signal  mcb4_dram_cas_n : std_logic;   
   signal  mcb4_dram_we_n  : std_logic;    
   signal  mcb4_dram_cke   : std_logic;   
   signal  mcb4_dram_odt   : std_logic;  
   signal  mcb4_dram_reset_n : std_logic;  
       signal  calib_done                        : std_logic;  
   signal  error                             : std_logic;  


      signal  mcb4_dram_udqs   : std_logic;
   signal  mcb4_dram_udqs_n : std_logic;
   signal mcb4_dram_dqs_vector : std_logic_vector(1 downto 0);
   signal mcb4_dram_dqs_n_vector : std_logic_vector(1 downto 0);
      signal   mcb4_dram_udm :std_logic;     -- for X16 parts
   signal mcb4_dram_dm_vector : std_logic_vector(1 downto 0);
   
   

   signal mcb4_command               : std_logic_vector(2 downto 0);
   signal mcb4_enable1                : std_logic;
   signal mcb4_enable2              : std_logic;

   signal  rzq4     : std_logic;
      



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
    c4_sys_clk <= not c4_sys_clk;
    wait for (C4_TCYC_SYS_DIV2);
  end process;

  c4_sys_clk_p <= c4_sys_clk;
  c4_sys_clk_n <= not c4_sys_clk;

-- ========================================================================== --
-- Reset Generation                                                           --
-- ========================================================================== --
 
 process
  begin
    c4_sys_rst <= '0';
    wait for 200 ns;
    c4_sys_rst <= '1';
    wait;
  end process;

    c4_sys_rst_i <= c4_sys_rst when (C4_RST_ACT_LOW = 1) else (not c4_sys_rst);





   


   rzq_pulldown4 : PULLDOWN port map(O => rzq4);
      

-- ========================================================================== --
-- DESIGN TOP INSTANTIATION                                                    --
-- ========================================================================== --

design_top : example_top generic map
(
  
C4_P0_MASK_SIZE  =>     C4_P0_MASK_SIZE,
C4_P0_DATA_PORT_SIZE  => C4_P0_DATA_PORT_SIZE,
C4_P1_MASK_SIZE       => C4_P1_MASK_SIZE,
C4_P1_DATA_PORT_SIZE  => C4_P1_DATA_PORT_SIZE, 
	C4_MEMCLK_PERIOD  =>       C4_MEMCLK_PERIOD,
C4_RST_ACT_LOW    =>     C4_RST_ACT_LOW,
C4_INPUT_CLK_TYPE =>     C4_INPUT_CLK_TYPE, 
DEBUG_EN              =>     DEBUG_EN,

C4_MEM_ADDR_ORDER     => C4_MEM_ADDR_ORDER,
C4_NUM_DQ_PINS        => C4_NUM_DQ_PINS,
C4_MEM_ADDR_WIDTH     => C4_MEM_ADDR_WIDTH,
C4_MEM_BANKADDR_WIDTH => C4_MEM_BANKADDR_WIDTH,

C4_HW_TESTING   =>      C4_HW_TESTING,
C4_SIMULATION   =>      C4_SIMULATION,

C4_CALIB_SOFT_IP      => C4_CALIB_SOFT_IP
) 
port map ( 

  calib_done         =>                      calib_done,
  error                          =>           error,
  c4_sys_clk  =>         c4_sys_clk,
  c4_sys_rst_i    =>       c4_sys_rst_i,                        

  mcb4_dram_dq     =>      mcb4_dram_dq,  
  mcb4_dram_a      =>      mcb4_dram_a,  
  mcb4_dram_ba     =>      mcb4_dram_ba,
  mcb4_dram_ras_n  =>      mcb4_dram_ras_n,                        
  mcb4_dram_cas_n  =>      mcb4_dram_cas_n,                        
  mcb4_dram_we_n   =>      mcb4_dram_we_n,                          
  mcb4_dram_odt    =>      mcb4_dram_odt,
  mcb4_dram_cke    =>      mcb4_dram_cke,                          
  mcb4_dram_ck     =>      mcb4_dram_ck,                          
  mcb4_dram_ck_n   =>      mcb4_dram_ck_n,       
  mcb4_dram_dqs_n  =>      mcb4_dram_dqs_n,
  mcb4_dram_reset_n =>     mcb4_dram_reset_n,

  mcb4_dram_udqs  =>       mcb4_dram_udqs,    -- for X16 parts           
    mcb4_dram_udqs_n  =>       mcb4_dram_udqs_n,    -- for X16 parts
  mcb4_dram_udm  =>        mcb4_dram_udm,     -- for X16 parts
  mcb4_dram_dm  =>       mcb4_dram_dm,
     mcb4_rzq         =>            rzq4,
        
  
  mcb4_dram_dqs    =>      mcb4_dram_dqs
);      



-- ========================================================================== --
-- Memory model instances                                                     -- 
-- ========================================================================== --

    mcb4_command <= (mcb4_dram_ras_n & mcb4_dram_cas_n & mcb4_dram_we_n);

    process(mcb4_dram_ck)
    begin
      if (rising_edge(mcb4_dram_ck)) then
        if (c4_sys_rst = '0') then
          mcb4_enable1   <= '0';
          mcb4_enable2 <= '0';
        elsif (mcb4_command = "100") then
          mcb4_enable2 <= '0';
        elsif (mcb4_command = "101") then
          mcb4_enable2 <= '1';
        else
          mcb4_enable2 <= mcb4_enable2;
        end if;
        mcb4_enable1     <= mcb4_enable2;
      end if;
    end process;

-----------------------------------------------------------------------------
--read
-----------------------------------------------------------------------------
    mcb4_dram_dqs_vector(1 downto 0)               <= (mcb4_dram_udqs & mcb4_dram_dqs)
                                                           when (mcb4_enable2 = '0' and mcb4_enable1 = '0')
							   else "ZZ";
    mcb4_dram_dqs_n_vector(1 downto 0)             <= (mcb4_dram_udqs_n & mcb4_dram_dqs_n)
                                                           when (mcb4_enable2 = '0' and mcb4_enable1 = '0')
							   else "ZZ";
    
-----------------------------------------------------------------------------
--write
-----------------------------------------------------------------------------
    mcb4_dram_dqs          <= mcb4_dram_dqs_vector(0)
                              when ( mcb4_enable1 = '1') else 'Z';

    mcb4_dram_udqs          <= mcb4_dram_dqs_vector(1)
                              when (mcb4_enable1 = '1') else 'Z';


     mcb4_dram_dqs_n        <= mcb4_dram_dqs_n_vector(0)
                              when (mcb4_enable1 = '1') else 'Z';
    mcb4_dram_udqs_n         <= mcb4_dram_dqs_n_vector(1)
                              when (mcb4_enable1 = '1') else 'Z';

   
   

mcb4_dram_dm_vector <= (mcb4_dram_udm & mcb4_dram_dm);

     u_mem_c4 : ddr3_model_c4 port map
	 (
      ck        => mcb4_dram_ck,
      ck_n      => mcb4_dram_ck_n,
      cke       => mcb4_dram_cke,
      cs_n      => '0',
      ras_n     => mcb4_dram_ras_n,
      cas_n     => mcb4_dram_cas_n,
      we_n      => mcb4_dram_we_n,
      dm_tdqs   => mcb4_dram_dm_vector,
      ba        => mcb4_dram_ba,
      addr      => mcb4_dram_a,
      dq        => mcb4_dram_dq,
      dqs       => mcb4_dram_dqs_vector,
      dqs_n     => mcb4_dram_dqs_n_vector,
      tdqs_n    => open,
      odt       => mcb4_dram_odt,
      rst_n     => mcb4_dram_reset_n
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
