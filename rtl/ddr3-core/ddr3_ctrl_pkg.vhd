--==============================================================================
--! @file ddr3_ctrl_pkg.vhd
--==============================================================================

--! Standard library
library IEEE;
--! Standard packages
use IEEE.STD_LOGIC_1164.all;
use IEEE.NUMERIC_STD.all;
--! Specific packages

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
-- DDR3 Controller Package
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--! @brief
--! DDR3 controller package
--------------------------------------------------------------------------------
--! @details
--! Contains DDR3 controller core top level component declaration.
--------------------------------------------------------------------------------
--! @version
--! 0.1 | mc | 12.08.2011 | File creation and Doxygen comments
--!
--! @author
--! mc : Matthieu Cattin, CERN (BE-CO-HT)
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
-- GNU LESSER GENERAL PUBLIC LICENSE
--------------------------------------------------------------------------------
-- This source file is free software; you can redistribute it and/or modify it
-- under the terms of the GNU Lesser General Public License as published by the
-- Free Software Foundation; either version 2.1 of the License, or (at your
-- option) any later version. This source is distributed in the hope that it
-- will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
-- of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
-- See the GNU Lesser General Public License for more details. You should have
-- received a copy of the GNU Lesser General Public License along with this
-- source; if not, download it from http://www.gnu.org/licenses/lgpl-2.1.html
--------------------------------------------------------------------------------

--==============================================================================
--! Entity declaration for ddr3_ctrl_pkg
--==============================================================================
package ddr3_ctrl_pkg is

  --==============================================================================
  --! Functions declaration
  --==============================================================================
  function log2_ceil(N : natural) return positive;

  --==============================================================================
  --! Components declaration
  --==============================================================================
  component ddr3_ctrl
    generic(
      --! Bank and port size selection
      g_BANK_PORT_SELECT   : string  := "BANK3_32B_32B";
      --! Core's clock period in ps
      g_MEMCLK_PERIOD      : integer := 3000;
      --! If TRUE, uses Xilinx calibration core (Input term, DQS centering)
      g_CALIB_SOFT_IP      : string  := "TRUE";
      --! User ports addresses maping (BANK_ROW_COLUMN or ROW_BANK_COLUMN)
      g_MEM_ADDR_ORDER     : string  := "ROW_BANK_COLUMN";
      --! Simulation mode
      g_SIMULATION         : string  := "FALSE";
      --! DDR3 data port width
      g_NUM_DQ_PINS        : integer := 16;
      --! DDR3 address port width
      g_MEM_ADDR_WIDTH     : integer := 14;
      --! DDR3 bank address width
      g_MEM_BANKADDR_WIDTH : integer := 3;
      --! Wishbone port 0 data mask size (8-bit granularity)
      g_P0_MASK_SIZE       : integer := 4;
      --! Wishbone port 0 data width
      g_P0_DATA_PORT_SIZE  : integer := 32;
      --! Port 0 byte address width
      g_P0_BYTE_ADDR_WIDTH : integer := 30;
      --! Wishbone port 1 data mask size (8-bit granularity)
      g_P1_MASK_SIZE       : integer := 4;
      --! Wishbone port 1 data width
      g_P1_DATA_PORT_SIZE  : integer := 32;
      --! Port 1 byte address width
      g_P1_BYTE_ADDR_WIDTH : integer := 30
      );

    port(
      ----------------------------------------------------------------------------
      -- Clock, control and status
      ----------------------------------------------------------------------------
      --! Clock input
      clk_i            : in    std_logic;
      --! Reset input (active low)
      rst_n_i          : in    std_logic;
      --! Status output
      status_o         : out   std_logic_vector(31 downto 0);
      ----------------------------------------------------------------------------
      -- DDR3 interface
      ----------------------------------------------------------------------------
      --! DDR3 data bus
      ddr3_dq_b        : inout std_logic_vector(g_NUM_DQ_PINS-1 downto 0);
      --! DDR3 address bus
      ddr3_a_o         : out   std_logic_vector(g_MEM_ADDR_WIDTH-1 downto 0);
      --! DDR3 bank address
      ddr3_ba_o        : out   std_logic_vector(g_MEM_BANKADDR_WIDTH-1 downto 0);
      --! DDR3 row address strobe
      ddr3_ras_n_o     : out   std_logic;
      --! DDR3 column address strobe
      ddr3_cas_n_o     : out   std_logic;
      --! DDR3 write enable
      ddr3_we_n_o      : out   std_logic;
      --! DDR3 on-die termination
      ddr3_odt_o       : out   std_logic;
      --! DDR3 reset
      ddr3_rst_n_o     : out   std_logic;
      --! DDR3 clock enable
      ddr3_cke_o       : out   std_logic;
      --! DDR3 lower byte data mask
      ddr3_dm_o        : out   std_logic;
      --! DDR3 upper byte data mask
      ddr3_udm_o       : out   std_logic;
      --! DDR3 lower byte data strobe (pos)
      ddr3_dqs_p_b     : inout std_logic;
      --! DDR3 lower byte data strobe (neg)
      ddr3_dqs_n_b     : inout std_logic;
      --! DDR3 upper byte data strobe (pos)
      ddr3_udqs_p_b    : inout std_logic;
      --! DDR3 upper byte data strobe (pos)
      ddr3_udqs_n_b    : inout std_logic;
      --! DDR3 clock (pos)
      ddr3_clk_p_o     : out   std_logic;
      --! DDR3 clock (neg)
      ddr3_clk_n_o     : out   std_logic;
      --! MCB internal termination calibration resistor
      ddr3_rzq_b       : inout std_logic;
      --! MCB internal termination calibration
      ddr3_zio_b       : inout std_logic;
      ----------------------------------------------------------------------------
      -- Wishbone bus - Port 0
      ----------------------------------------------------------------------------
      --! Wishbone bus clock
      wb0_clk_i        : in    std_logic;
      --! Wishbone bus byte select
      wb0_sel_i        : in    std_logic_vector(g_P0_MASK_SIZE - 1 downto 0);
      --! Wishbone bus cycle select
      wb0_cyc_i        : in    std_logic;
      --! Wishbone bus cycle strobe
      wb0_stb_i        : in    std_logic;
      --! Wishbone bus write enable
      wb0_we_i         : in    std_logic;
      --! Wishbone bus address
      wb0_addr_i       : in    std_logic_vector(31 downto 0);
      --! Wishbone bus data input
      wb0_data_i       : in    std_logic_vector(g_P0_DATA_PORT_SIZE - 1 downto 0);
      --! Wishbone bus data output
      wb0_data_o       : out   std_logic_vector(g_P0_DATA_PORT_SIZE - 1 downto 0);
      --! Wishbone bus acknowledge
      wb0_ack_o        : out   std_logic;
      --! Wishbone bus stall (for pipelined mode)
      wb0_stall_o      : out   std_logic;
      ----------------------------------------------------------------------------
      -- Status - Port 0
      ----------------------------------------------------------------------------
      --! Command FIFO empty
      p0_cmd_empty_o   : out   std_logic;
      --! Command FIFO full
      p0_cmd_full_o    : out   std_logic;
      --! Read FIFO full
      p0_rd_full_o     : out   std_logic;
      --! Read FIFO empty
      p0_rd_empty_o    : out   std_logic;
      --! Read FIFO count
      p0_rd_count_o    : out   std_logic_vector(6 downto 0);
      --! Read FIFO overflow
      p0_rd_overflow_o : out   std_logic;
      --! Read FIFO error (pointers unsynchronized, reset required)
      p0_rd_error_o    : out   std_logic;
      --! Write FIFO full
      p0_wr_full_o     : out   std_logic;
      --! Write FIFO empty
      p0_wr_empty_o    : out   std_logic;
      --! Write FIFO count
      p0_wr_count_o    : out   std_logic_vector(6 downto 0);
      --! Write FIFO underrun
      p0_wr_underrun_o : out   std_logic;
      --! Write FIFO error (pointers unsynchronized, reset required)
      p0_wr_error_o    : out   std_logic;
      ----------------------------------------------------------------------------
      -- Wishbone bus - Port 1
      ----------------------------------------------------------------------------
      --! Wishbone bus clock
      wb1_clk_i        : in    std_logic;
      --! Wishbone bus byte select
      wb1_sel_i        : in    std_logic_vector(g_P1_MASK_SIZE - 1 downto 0);
      --! Wishbone bus cycle select
      wb1_cyc_i        : in    std_logic;
      --! Wishbone bus cycle strobe
      wb1_stb_i        : in    std_logic;
      --! Wishbone bus write enable
      wb1_we_i         : in    std_logic;
      --! Wishbone bus address
      wb1_addr_i       : in    std_logic_vector(31 downto 0);
      --! Wishbone bus data input
      wb1_data_i       : in    std_logic_vector(g_P1_DATA_PORT_SIZE - 1 downto 0);
      --! Wishbone bus data output
      wb1_data_o       : out   std_logic_vector(g_P1_DATA_PORT_SIZE - 1 downto 0);
      --! Wishbone bus acknowledge
      wb1_ack_o        : out   std_logic;
      --! Wishbone bus stall (for pipelined mode)
      wb1_stall_o      : out   std_logic;
      ----------------------------------------------------------------------------
      -- Status - Port 1
      ----------------------------------------------------------------------------
      --! Command FIFO empty
      p1_cmd_empty_o   : out   std_logic;
      --! Command FIFO full
      p1_cmd_full_o    : out   std_logic;
      --! Read FIFO full
      p1_rd_full_o     : out   std_logic;
      --! Read FIFO empty
      p1_rd_empty_o    : out   std_logic;
      --! Read FIFO count
      p1_rd_count_o    : out   std_logic_vector(6 downto 0);
      --! Read FIFO overflow
      p1_rd_overflow_o : out   std_logic;
      --! Read FIFO error (pointers unsynchronized, reset required)
      p1_rd_error_o    : out   std_logic;
      --! Write FIFO full
      p1_wr_full_o     : out   std_logic;
      --! Write FIFO empty
      p1_wr_empty_o    : out   std_logic;
      --! Write FIFO count
      p1_wr_count_o    : out   std_logic_vector(6 downto 0);
      --! Write FIFO underrun
      p1_wr_underrun_o : out   std_logic;
      --! Write FIFO error (pointers unsynchronized, reset required)
      p1_wr_error_o    : out   std_logic
      );
  end component ddr3_ctrl;


end ddr3_ctrl_pkg;

package body ddr3_ctrl_pkg is

  -----------------------------------------------------------------------------
  -- Returns log of 2 of a natural number
  -----------------------------------------------------------------------------
  function log2_ceil(N : natural) return positive is
  begin
    if N <= 2 then
      return 1;
    elsif N mod 2 = 0 then
      return 1 + log2_ceil(N/2);
    else
      return 1 + log2_ceil((N+1)/2);
    end if;
  end;

end ddr3_ctrl_pkg;
