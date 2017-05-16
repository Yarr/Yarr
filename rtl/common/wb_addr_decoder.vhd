--------------------------------------------------------------------------------
--                                                                            --
-- CERN BE-CO-HT         GN4124 core for PCIe FMC carrier                     --
--                       http://www.ohwr.org/projects/gn4124-core             --
--------------------------------------------------------------------------------
--
-- unit name: wishbone address decoder
--
-- author: Matthieu Cattin (matthieu.cattin@cern.ch)
--
-- date: 02-08-2011
--
-- version: 0.1
--
-- description: Provides a simple wishbone address decoder.
--              Splits the memory windows into equal parts.
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
-- last changes: 
--------------------------------------------------------------------------------
-- TODO: 
--------------------------------------------------------------------------------

library IEEE;
use IEEE.STD_LOGIC_1164.all;
use IEEE.NUMERIC_STD.all;
use work.gn4124_core_pkg.all;


entity wb_addr_decoder is
  generic
    (
      g_WINDOW_SIZE  : integer := 18;   -- Number of bits to address periph on the board (32-bit word address)
      g_WB_SLAVES_NB : integer := 2
      );
  port
    (
      ---------------------------------------------------------
      -- GN4124 core clock and reset
      clk_i   : in std_logic;
      rst_n_i : in std_logic;

      ---------------------------------------------------------
      -- wishbone master interface
      wbm_adr_i   : in  std_logic_vector(31 downto 0);  -- Address
      wbm_dat_i   : in  std_logic_vector(31 downto 0);  -- Data out
      wbm_sel_i   : in  std_logic_vector(3 downto 0);   -- Byte select
      wbm_stb_i   : in  std_logic;                      -- Strobe
      wbm_we_i    : in  std_logic;                      -- Write
      wbm_cyc_i   : in  std_logic;                      -- Cycle
      wbm_dat_o   : out std_logic_vector(31 downto 0);  -- Data in
      wbm_ack_o   : out std_logic;                      -- Acknowledge
      wbm_stall_o : out std_logic;                      -- Stall

      ---------------------------------------------------------
      -- wishbone slaves interface
      wb_adr_o   : out std_logic_vector(31 downto 0);                     -- Address
      wb_dat_o   : out std_logic_vector(31 downto 0);                     -- Data out
      wb_sel_o   : out std_logic_vector(3 downto 0);                      -- Byte select
      wb_stb_o   : out std_logic;                                         -- Strobe
      wb_we_o    : out std_logic;                                         -- Write
      wb_cyc_o   : out std_logic_vector(g_WB_SLAVES_NB-1 downto 0);       -- Cycle
      wb_dat_i   : in  std_logic_vector((32*g_WB_SLAVES_NB)-1 downto 0);  -- Data in
      wb_ack_i   : in  std_logic_vector(g_WB_SLAVES_NB-1 downto 0);       -- Acknowledge
      wb_stall_i : in  std_logic_vector(g_WB_SLAVES_NB-1 downto 0)        -- Stall
      );
end wb_addr_decoder;


architecture behaviour of wb_addr_decoder is


  -----------------------------------------------------------------------------
  -- Constants declaration
  -----------------------------------------------------------------------------

  -----------------------------------------------------------------------------
  -- Signals declaration
  -----------------------------------------------------------------------------

  -- Wishbone
  signal s_wb_periph_addr   : std_logic_vector(log2_ceil(g_WB_SLAVES_NB)-1 downto 0);
  signal wb_periph_addr     : std_logic_vector(log2_ceil(g_WB_SLAVES_NB)-1 downto 0);
  signal s_wb_periph_select : std_logic_vector((2**s_wb_periph_addr'length)-1 downto 0);
  signal s_wb_ack_muxed     : std_logic;
  signal wb_ack_t           : std_logic;
  signal s_wb_dat_i_muxed   : std_logic_vector(31 downto 0);
  signal s_wb_cyc_demuxed   : std_logic_vector(g_WB_SLAVES_NB-1 downto 0);
  signal wb_adr_t           : std_logic_vector(g_WINDOW_SIZE-log2_ceil(g_WB_SLAVES_NB)-1 downto 0);


begin

  ------------------------------------------------------------------------------
  -- Wishbone master address decoding
  ------------------------------------------------------------------------------

  -- Take the first N bits of the address to select the active wb peripheral
  -- g_WINDOW_SIZE represents 32-bit word address window
  s_wb_periph_addr <= wbm_adr_i(g_WINDOW_SIZE-1 downto g_WINDOW_SIZE-log2_ceil(g_WB_SLAVES_NB));

  -----------------------------------------------------------------------------
  -- One-hot decode function,  s_wb_periph_select <= onehot_decode(s_wb_periph_addr);
  -----------------------------------------------------------------------------
  onehot_decode : process(s_wb_periph_addr)
    variable v_onehot : std_logic_vector((2**s_wb_periph_addr'length)-1 downto 0);
    variable v_index  : integer range 0 to (2**s_wb_periph_addr'length)-1;
  begin
    v_onehot := (others => '0');
    v_index  := 0;
    for i in s_wb_periph_addr'range loop
      if (s_wb_periph_addr(i) = '1') then
        v_index := 2*v_index+1;
      else
        v_index := 2*v_index;
      end if;
    end loop;
    v_onehot(v_index)  := '1';
    s_wb_periph_select <= v_onehot;
  end process onehot_decode;

  -- Register multiplexed ack and data + periph address
  p_wb_in_regs : process (clk_i, rst_n_i)
  begin
    if (rst_n_i = c_RST_ACTIVE) then
      wb_periph_addr <= (others => '0');
      wbm_dat_o      <= (others => '0');
      wb_ack_t       <= '0';
    elsif rising_edge(clk_i) then
      wb_periph_addr <= s_wb_periph_addr;
      wbm_dat_o      <= s_wb_dat_i_muxed;
      wb_ack_t       <= s_wb_ack_muxed;
    end if;
  end process p_wb_in_regs;

  wbm_ack_o <= wb_ack_t;

  -- Select ack line of the active peripheral
  p_ack_mux : process (wb_ack_i, wb_periph_addr)
  begin
    if (to_integer(unsigned(wb_periph_addr)) < g_WB_SLAVES_NB) then
      s_wb_ack_muxed <= wb_ack_i(to_integer(unsigned(wb_periph_addr)));
    else
      s_wb_ack_muxed <= '0';
    end if;
  end process p_ack_mux;

  -- Select stall line of the active peripheral
  p_stall_mux : process (wb_stall_i, s_wb_periph_addr)
  begin
    if (to_integer(unsigned(s_wb_periph_addr)) < g_WB_SLAVES_NB) then
      wbm_stall_o <= wb_stall_i(to_integer(unsigned(s_wb_periph_addr)));
    else
      wbm_stall_o <= '0';
    end if;
  end process p_stall_mux;

  -- Select input data of the active peripheral
  p_din_mux : process (wb_dat_i, wb_periph_addr)
  begin
    if (to_integer(unsigned(wb_periph_addr)) < g_WB_SLAVES_NB) then
      s_wb_dat_i_muxed <=
        wb_dat_i(31+(32*to_integer(unsigned(wb_periph_addr))) downto 32*to_integer(unsigned(wb_periph_addr)));
    else
      s_wb_dat_i_muxed <= (others => 'X');
    end if;
  end process p_din_mux;

  -- Assert the cyc line of the selected peripheral
  gen_cyc_demux : for i in 0 to g_WB_SLAVES_NB-1 generate
    s_wb_cyc_demuxed(i) <= wbm_cyc_i and s_wb_periph_select(i) and not(wb_ack_t);
  end generate gen_cyc_demux;

  -- Slaves wishbone bus outputs
  wb_dat_o   <= wbm_dat_i;
  wb_stb_o   <= wbm_stb_i;
  wb_we_o    <= wbm_we_i;
  wb_sel_o   <= wbm_sel_i;
  wb_cyc_o   <= s_wb_cyc_demuxed;
  -- extend address bus to 32-bit
  wb_adr_t   <= wbm_adr_i(g_WINDOW_SIZE-log2_ceil(g_WB_SLAVES_NB)-1 downto 0);
  wb_adr_o(wb_adr_t'left downto 0) <= wb_adr_t;
  wb_adr_o(31 downto wb_adr_t'left+1) <= (others => '0');


end behaviour;

