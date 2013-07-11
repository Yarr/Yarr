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
      constant C1_P0_MASK_SIZE : integer      := 8;
   constant C1_P0_DATA_PORT_SIZE : integer := 64;  
   constant C1_P1_MASK_SIZE   : integer    := 4;
   constant C1_P1_DATA_PORT_SIZE  : integer := 32;
   constant C1_CALIB_SOFT_IP      : string := "TRUE";
   constant C1_SIMULATION      : string := "TRUE";
   

-- ========================================================================== --
-- Component Declarations
-- ========================================================================== --

component example_top is
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
    C1_HW_TESTING           : string;
    C1_MEM_ADDR_ORDER       : string;
    C1_NUM_DQ_PINS          : integer; 
    C1_MEM_ADDR_WIDTH       : integer; 
    C1_MEM_BANKADDR_WIDTH   : integer
);  
  port
  (
        calib_done                    : out std_logic;
   error                                 : out std_logic;
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
	
   mcb1_dram_dqs                           : inout  std_logic;
   mcb1_dram_dqs_n                         : inout  std_logic;
   mcb1_dram_ck                            : out std_logic;
   mcb1_dram_ck_n                          : out std_logic;
      mcb1_dram_udqs                           : inout  std_logic;
   mcb1_dram_udqs_n                         : inout  std_logic;
   mcb1_dram_udm                            : out std_logic;
   mcb1_dram_reset_n                       : out std_logic
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

-- ========================================================================== --
-- Signal Declarations                                                        --
-- ========================================================================== --

 			-- Clocks
   signal  c1_sys_clk     : std_logic := '0';
   signal  c1_sys_clk_p   : std_logic;
   signal  c1_sys_clk_n   : std_logic;
-- System Reset
   signal  c1_sys_rst   : std_logic := '0';
   signal  c1_sys_rst_i     : std_logic;



-- Design-Top Port Map   
   signal mcb1_dram_a : std_logic_vector(C1_MEM_ADDR_WIDTH-1 downto 0);
   signal mcb1_dram_ba : std_logic_vector(C1_MEM_BANKADDR_WIDTH-1 downto 0);  
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
       signal  calib_done                        : std_logic;  
   signal  error                             : std_logic;  


      signal  mcb1_dram_udqs   : std_logic;
   signal  mcb1_dram_udqs_n : std_logic;
   signal mcb1_dram_dqs_vector : std_logic_vector(1 downto 0);
   signal mcb1_dram_dqs_n_vector : std_logic_vector(1 downto 0);
      signal   mcb1_dram_udm :std_logic;     -- for X16 parts
   signal mcb1_dram_dm_vector : std_logic_vector(1 downto 0);
   
   

   signal mcb1_command               : std_logic_vector(2 downto 0);
   signal mcb1_enable1                : std_logic;
   signal mcb1_enable2              : std_logic;

   signal  rzq1     : std_logic;
      



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





   


   rzq_pulldown1 : PULLDOWN port map(O => rzq1);
      

-- ========================================================================== --
-- DESIGN TOP INSTANTIATION                                                    --
-- ========================================================================== --

design_top : example_top generic map
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

C1_HW_TESTING   =>      C1_HW_TESTING,
C1_SIMULATION   =>      C1_SIMULATION,

C1_CALIB_SOFT_IP      => C1_CALIB_SOFT_IP
) 
port map ( 

  calib_done         =>                      calib_done,
  error                          =>           error,
  c1_sys_clk  =>         c1_sys_clk,
  c1_sys_rst_i    =>       c1_sys_rst_i,                        

  mcb1_dram_dq     =>      mcb1_dram_dq,  
  mcb1_dram_a      =>      mcb1_dram_a,  
  mcb1_dram_ba     =>      mcb1_dram_ba,
  mcb1_dram_ras_n  =>      mcb1_dram_ras_n,                        
  mcb1_dram_cas_n  =>      mcb1_dram_cas_n,                        
  mcb1_dram_we_n   =>      mcb1_dram_we_n,                          
  mcb1_dram_odt    =>      mcb1_dram_odt,
  mcb1_dram_cke    =>      mcb1_dram_cke,                          
  mcb1_dram_ck     =>      mcb1_dram_ck,                          
  mcb1_dram_ck_n   =>      mcb1_dram_ck_n,       
  mcb1_dram_dqs_n  =>      mcb1_dram_dqs_n,
  mcb1_dram_reset_n =>     mcb1_dram_reset_n,

  mcb1_dram_udqs  =>       mcb1_dram_udqs,    -- for X16 parts           
    mcb1_dram_udqs_n  =>       mcb1_dram_udqs_n,    -- for X16 parts
  mcb1_dram_udm  =>        mcb1_dram_udm,     -- for X16 parts
  mcb1_dram_dm  =>       mcb1_dram_dm,
     mcb1_rzq         =>            rzq1,
        
  
  mcb1_dram_dqs    =>      mcb1_dram_dqs
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
