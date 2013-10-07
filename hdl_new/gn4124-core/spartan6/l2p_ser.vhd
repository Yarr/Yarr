--------------------------------------------------------------------------------
--                                                                            --
-- CERN BE-CO-HT         GN4124 core for PCIe FMC carrier                     --
--                       http://www.ohwr.org/projects/gn4124-core             --
--------------------------------------------------------------------------------
--
-- unit name: L2P serializer (l2p_ser_s6.vhd)
--
-- authors: Simon Deprez (simon.deprez@cern.ch)
--          Matthieu Cattin (matthieu.cattin@cern.ch)
--
-- date: 31-08-2010
--
-- version: 1.0
--
-- description: Generates the DDR L2P bus from SDR that is synchronous to the
--              core clock. Spartan6 FPGAs version.
--
--
-- dependencies:
--
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
-- last changes: 23-09-2010 (mcattin) Always active high reset for FFs.
--------------------------------------------------------------------------------

library IEEE;
use IEEE.STD_LOGIC_1164.all;
use IEEE.NUMERIC_STD.all;
use work.gn4124_core_pkg.all;

library UNISIM;
use UNISIM.vcomponents.all;


entity l2p_ser is
  port
    (
      ---------------------------------------------------------
      -- Reset and clock
      rst_n_i         : in std_logic;
      sys_clk_i       : in std_logic;
      io_clk_i        : in std_logic;
      serdes_strobe_i : in std_logic;

      ---------------------------------------------------------
      -- L2P SDR inputs
      l2p_valid_i  : in std_logic;
      l2p_dframe_i : in std_logic;
      l2p_data_i   : in std_logic_vector(31 downto 0);

      ---------------------------------------------------------
      -- L2P DDR outputs
      l2p_clk_p_o  : out std_logic;
      l2p_clk_n_o  : out std_logic;
      l2p_valid_o  : out std_logic;
      l2p_dframe_o : out std_logic;
      l2p_data_o   : out std_logic_vector(15 downto 0)
      );
end l2p_ser;


architecture rtl of l2p_ser is


  -----------------------------------------------------------------------------
  -- Components declaration
  -----------------------------------------------------------------------------
  component serdes_n_to_1_s2_se
    generic (
      S : integer := 2;                                         -- Parameter to set the serdes factor 1..8
      D : integer := 16) ;                                      -- Set the number of inputs and outputs
    port (
      txioclk        : in  std_logic;                           -- IO Clock network
      txserdesstrobe : in  std_logic;                           -- Parallel data capture strobe
      reset          : in  std_logic;                           -- Reset
      gclk           : in  std_logic;                           -- Global clock
      datain         : in  std_logic_vector((D*S)-1 downto 0);  -- Data for output
      dataout        : out std_logic_vector(D-1 downto 0)) ;    -- output
  end component serdes_n_to_1_s2_se;

  component serdes_n_to_1_s2_diff
    generic (
      S : integer := 2;                                         -- Parameter to set the serdes factor 1..8
      D : integer := 1) ;                                       -- Set the number of inputs and outputs
    port (
      txioclk        : in  std_logic;                           -- IO Clock network
      txserdesstrobe : in  std_logic;                           -- Parallel data capture strobe
      reset          : in  std_logic;                           -- Reset
      gclk           : in  std_logic;                           -- Global clock
      datain         : in  std_logic_vector((D*S)-1 downto 0);  -- Data for output
      dataout_p      : out std_logic_vector(D-1 downto 0);      -- output
      dataout_n      : out std_logic_vector(D-1 downto 0)) ;    -- output
  end component serdes_n_to_1_s2_diff;

  -----------------------------------------------------------------------------
  -- Comnstants declaration
  -----------------------------------------------------------------------------
  constant S        : integer                      := 2;   -- Set the serdes factor to 2
  constant D        : integer                      := 16;  -- Set the number of outputs
  constant c_TX_CLK : std_logic_vector(1 downto 0) := "01";

  -----------------------------------------------------------------------------
  -- Signals declaration
  -----------------------------------------------------------------------------

  -- Serdes reset
  signal rst : std_logic;

  -- SDR signals
  signal l2p_dframe_t : std_logic_vector(1 downto 0);
  signal l2p_valid_t  : std_logic_vector(1 downto 0);
  signal l2p_dframe_v : std_logic_vector(0 downto 0);
  signal l2p_valid_v  : std_logic_vector(0 downto 0);
  signal l2p_clk_p_v  : std_logic_vector(0 downto 0);
  signal l2p_clk_n_v  : std_logic_vector(0 downto 0);


begin


  ------------------------------------------------------------------------------
  -- Active high reset for DDR FF
  ------------------------------------------------------------------------------
  gen_fifo_rst_n : if c_RST_ACTIVE = '0' generate
    rst <= not(rst_n_i);
  end generate;

  gen_fifo_rst : if c_RST_ACTIVE = '1' generate
    rst <= rst_n_i;
  end generate;

  ------------------------------------------------------------------------------
  -- Instantiate serialiser to generate forwarded clock
  ------------------------------------------------------------------------------
  cmp_clk_out : serdes_n_to_1_s2_diff
    generic map(
      S => S,
      D => 1)
    port map (
      txioclk       => io_clk_i,
      txserdesstrobe => serdes_strobe_i,
      gclk           => sys_clk_i,
      reset          => rst,
      datain         => c_TX_CLK,       -- Transmit a constant to make the clock
      dataout_p      => l2p_clk_p_v,
      dataout_n      => l2p_clk_n_v);

  -- Type conversion, std_logic_vector to std_logic
  l2p_clk_p_o <= l2p_clk_p_v(0);
  l2p_clk_n_o <= l2p_clk_n_v(0);

  ------------------------------------------------------------------------------
  -- Instantiate serialisers for output data lines
  ------------------------------------------------------------------------------
  cmp_data_out : serdes_n_to_1_s2_se
    generic map(
      S => S,
      D => D)
    port map (
      txioclk       => io_clk_i,
      txserdesstrobe => serdes_strobe_i,
      gclk           => sys_clk_i,
      reset          => rst,
      datain         => l2p_data_i,
      dataout        => l2p_data_o);

  ------------------------------------------------------------------------------
  -- Instantiate serialisers for dframe
  ------------------------------------------------------------------------------
  cmp_dframe_out : serdes_n_to_1_s2_se
    generic map(
      S => S,
      D => 1)
    port map (
      txioclk       => io_clk_i,
      txserdesstrobe => serdes_strobe_i,
      gclk           => sys_clk_i,
      reset          => rst,
      datain         => l2p_dframe_t,
      dataout        => l2p_dframe_v);

  -- Serialize two times the same value
  l2p_dframe_t <= l2p_dframe_i & l2p_dframe_i;

  -- Type conversion, std_logic_vector to std_logic
  l2p_dframe_o <= l2p_dframe_v(0);

  ------------------------------------------------------------------------------
  -- Instantiate serialisers for valid
  ------------------------------------------------------------------------------
  cmp_valid_out : serdes_n_to_1_s2_se
    generic map(
      S => S,
      D => 1)
    port map (
      txioclk       => io_clk_i,
      txserdesstrobe => serdes_strobe_i,
      gclk           => sys_clk_i,
      reset          => rst,
      datain         => l2p_valid_t,
      dataout        => l2p_valid_v);

  -- Serialize two times the same value
  l2p_valid_t <= l2p_valid_i & l2p_valid_i;

  -- Type conversion, std_logic_vector to std_logic
  l2p_valid_o <= l2p_valid_v(0);

end rtl;


