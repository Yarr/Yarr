--------------------------------------------------------------------------------
--                                                                            --
-- CERN BE-CO-HT         GN4124 core for PCIe FMC carrier                     --
--                       http://www.ohwr.org/projects/gn4124-core             --
--------------------------------------------------------------------------------
--
-- unit name: P2L deserializer (p2l_des_s6.vhd)
--
-- authors: Simon Deprez (simon.deprez@cern.ch)
--          Matthieu Cattin (matthieu.cattin@cern.ch)
--
-- date: 31-08-2010
--
-- version: 1.0
--
-- description: Takes the DDR P2L bus and converts to SDR that is synchronous
--              to the core clock. Spartan6 FPGAs version.
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


entity p2l_des is
  port
    (
      ---------------------------------------------------------
      -- Reset and clock
      rst_n_i         : in std_logic;
      sys_clk_i       : in std_logic;
      io_clk_i        : in std_logic;
      serdes_strobe_i : in std_logic;

      ---------------------------------------------------------
      -- P2L clock domain (DDR)
      --
      -- P2L inputs
      p2l_valid_i  : in std_logic;
      p2l_dframe_i : in std_logic;
      p2l_data_i   : in std_logic_vector(15 downto 0);

      ---------------------------------------------------------
      -- Core clock domain (SDR)
      --
      -- Deserialized output
      p2l_valid_o  : out std_logic;
      p2l_dframe_o : out std_logic;
      p2l_data_o   : out std_logic_vector(31 downto 0)
      );
end p2l_des;


architecture rtl of p2l_des is


  -----------------------------------------------------------------------------
  -- Components declaration
  -----------------------------------------------------------------------------
  component serdes_1_to_n_data_s2_se
    generic (
      USE_PD : boolean := false;                                    -- Parameter to set generation of phase detector logic
      S      : integer := 2;                                        -- Parameter to set the serdes factor 1..8
      D      : integer := 16) ;                                     -- Set the number of inputs and outputs
    port (
      use_phase_detector : in  std_logic;                           -- Set generation of phase detector logic
      datain             : in  std_logic_vector(D-1 downto 0);      -- Input from se receiver pin
      rxioclk            : in  std_logic;                           -- IO Clock network
      rxserdesstrobe     : in  std_logic;                           -- Parallel data capture strobe
      reset              : in  std_logic;                           -- Reset line
      gclk               : in  std_logic;                           -- Global clock
      bitslip            : in  std_logic;                           -- Bitslip control line
      debug_in           : in  std_logic_vector(1 downto 0);        -- input debug data
      data_out           : out std_logic_vector((D*S)-1 downto 0);  -- Output data
      -- Debug bus, 2D+6 = 2 lines per input (from mux and ce) + 7, leave nc if debug not required
      debug              : out std_logic_vector((2*D)+6 downto 0)) ;
  end component serdes_1_to_n_data_s2_se;

  -----------------------------------------------------------------------------
  -- Comnstants declaration
  -----------------------------------------------------------------------------
  constant S : integer := 2;            -- Set the serdes factor to 2
  constant D : integer := 16;           -- Set the number of inputs and outputs

  -----------------------------------------------------------------------------
  -- Signals declaration
  -----------------------------------------------------------------------------

  -- Serdes reset
  signal rst : std_logic;

  -- SDR signals
  signal p2l_valid_v        : std_logic_vector(0 downto 0);
  signal p2l_dframe_v       : std_logic_vector(0 downto 0);
  signal p2l_valid_t        : std_logic_vector(1 downto 0);
  signal p2l_dframe_t       : std_logic_vector(1 downto 0);
  signal p2l_data_t         : std_logic_vector(p2l_data_o'range);
  signal p2l_valid_t2       : std_logic;
  signal p2l_dframe_t2      : std_logic;
  signal p2l_data_t2        : std_logic_vector(p2l_data_o'range);
  signal p2l_data_bitslip   : std_logic_vector(1 downto 0);
  signal p2l_data_bitslip_p : std_logic;
  --signal p2l_ctrl_v   : std_logic_vector(1 downto 0);
  --signal p2l_ctrl_t   : std_logic_vector(3 downto 0);


begin


  ------------------------------------------------------------------------------
  -- Active high reset
  ------------------------------------------------------------------------------
  gen_rst_n : if c_RST_ACTIVE = '0' generate
    rst <= not(rst_n_i);
  end generate;

  gen_rst : if c_RST_ACTIVE = '1' generate
    rst <= rst_n_i;
  end generate;


  ------------------------------------------------------------------------------
  -- data input bit slip
  ------------------------------------------------------------------------------
  p_din_bitslip : process (sys_clk_i, rst_n_i)
  begin
    if rst_n_i = c_RST_ACTIVE then
      p2l_data_bitslip <= (others => '0');
    elsif rising_edge(sys_clk_i) then
      p2l_data_bitslip <= p2l_data_bitslip(0) & '1';
    end if;
  end process p_din_bitslip;

  p2l_data_bitslip_p <= p2l_data_bitslip(0) and not(p2l_data_bitslip(1));

  ------------------------------------------------------------------------------
  -- data inputs
  ------------------------------------------------------------------------------
  cmp_data_in : serdes_1_to_n_data_s2_se
    generic map(
      USE_PD => false,
      S      => S,
      D      => D)
    port map (
      use_phase_detector => '0',        -- '1' enables the phase detector logic
      datain             => p2l_data_i,
      rxioclk            => io_clk_i,
      rxserdesstrobe     => serdes_strobe_i,
      gclk               => sys_clk_i,
      bitslip            => '0',        --p2l_data_bitslip_p,
      reset              => rst,
      data_out           => p2l_data_t,
      debug_in           => "00",
      debug              => open);

  ------------------------------------------------------------------------------
  -- dframe input
  ------------------------------------------------------------------------------
  cmp_dframe_in : serdes_1_to_n_data_s2_se
    generic map(
      USE_PD => false,
      S      => S,
      D      => 1)
    port map (
      use_phase_detector => '0',        -- '1' enables the phase detector logic
      datain             => p2l_dframe_v,
      rxioclk            => io_clk_i,
      rxserdesstrobe     => serdes_strobe_i,
      gclk               => sys_clk_i,
      bitslip            => '0',
      reset              => rst,
      data_out           => p2l_dframe_t,
      debug_in           => "00",
      debug              => open);

  -- Type conversion, std_logic to std_logic_vector
  p2l_dframe_v(0) <= p2l_dframe_i;

  ------------------------------------------------------------------------------
  -- valid input
  ------------------------------------------------------------------------------
  cmp_valid_in : serdes_1_to_n_data_s2_se
    generic map(
      USE_PD => false,
      S      => S,
      D      => 1)
    port map (
      use_phase_detector => '0',        -- '1' enables the phase detector logic
      datain             => p2l_valid_v,
      rxioclk            => io_clk_i,
      rxserdesstrobe     => serdes_strobe_i,
      gclk               => sys_clk_i,
      bitslip            => '0',
      reset              => rst,
      data_out           => p2l_valid_t,
      debug_in           => "00",
      debug              => open);

  -- Type conversion, std_logic to std_logic_vector
  p2l_valid_v(0) <= p2l_valid_i;


  p_in_sys_sync : process (sys_clk_i, rst_n_i)
  begin
    if rst_n_i = c_RST_ACTIVE then
      p2l_data_o    <= (others => '0');
      p2l_dframe_o  <= '0';
      p2l_valid_o   <= '0';
      p2l_data_t2   <= (others => '0');
      p2l_dframe_t2 <= '0';
      p2l_valid_t2  <= '0';
    elsif rising_edge(sys_clk_i) then
      p2l_data_t2   <= p2l_data_t;
      p2l_dframe_t2 <= p2l_dframe_t(0);
      p2l_valid_t2  <= p2l_valid_t(0);
      p2l_data_o    <= p2l_data_t2;
      p2l_dframe_o  <= p2l_dframe_t2;
      p2l_valid_o   <= p2l_valid_t2;
    end if;
  end process p_in_sys_sync;

end rtl;


