--==============================================================================
--! @file ddr3_ctrl_wb_single.vhd
--==============================================================================

--! Standard library
library IEEE;
--! Standard packages
use IEEE.STD_LOGIC_1164.all;
use IEEE.NUMERIC_STD.all;
--! Specific packages

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
-- DDR3 Controller Wishbone Interface (single access only)
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--! @brief
--! DDR3 Controller Wishbone Interface
--------------------------------------------------------------------------------
--! @details
--! Wishbone interface for DDR3 controller.
--------------------------------------------------------------------------------
--! @version
--! 0.1 | mc | 14.07.2011 | File creation and Doxygen comments
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
--! Entity declaration for ddr3_ctrl_wb
--==============================================================================
entity ddr3_ctrl_wb is

  generic(
    --! DDR3 byte address width
    g_BYTE_ADDR_WIDTH : integer := 30;
    --! Data mask size (8-bit granularity)
    g_MASK_SIZE       : integer := 4;
    --! Data width
    g_DATA_PORT_SIZE  : integer := 32
    );

  port(
    ----------------------------------------------------------------------------
    -- Reset input (active low)
    ----------------------------------------------------------------------------
    rst_n_i : in std_logic;

    ----------------------------------------------------------------------------
    -- DDR controller port
    ----------------------------------------------------------------------------
    ddr_cmd_clk_o       : out std_logic;
    ddr_cmd_en_o        : out std_logic;
    ddr_cmd_instr_o     : out std_logic_vector(2 downto 0);
    ddr_cmd_bl_o        : out std_logic_vector(5 downto 0);
    ddr_cmd_byte_addr_o : out std_logic_vector(g_BYTE_ADDR_WIDTH - 1 downto 0);
    ddr_cmd_empty_i     : in  std_logic;
    ddr_cmd_full_i      : in  std_logic;
    ddr_wr_clk_o        : out std_logic;
    ddr_wr_en_o         : out std_logic;
    ddr_wr_mask_o       : out std_logic_vector(g_MASK_SIZE - 1 downto 0);
    ddr_wr_data_o       : out std_logic_vector(g_DATA_PORT_SIZE - 1 downto 0);
    ddr_wr_full_i       : in  std_logic;
    ddr_wr_empty_i      : in  std_logic;
    ddr_wr_count_i      : in  std_logic_vector(6 downto 0);
    ddr_wr_underrun_i   : in  std_logic;
    ddr_wr_error_i      : in  std_logic;
    ddr_rd_clk_o        : out std_logic;
    ddr_rd_en_o         : out std_logic;
    ddr_rd_data_i       : in  std_logic_vector(g_DATA_PORT_SIZE - 1 downto 0);
    ddr_rd_full_i       : in  std_logic;
    ddr_rd_empty_i      : in  std_logic;
    ddr_rd_count_i      : in  std_logic_vector(6 downto 0);
    ddr_rd_overflow_i   : in  std_logic;
    ddr_rd_error_i      : in  std_logic;

    ----------------------------------------------------------------------------
    -- Wishbone bus port
    ----------------------------------------------------------------------------
    wb_clk_i   : in  std_logic;
    wb_sel_i   : in  std_logic_vector(g_MASK_SIZE - 1 downto 0);
    wb_cyc_i   : in  std_logic;
    wb_stb_i   : in  std_logic;
    wb_we_i    : in  std_logic;
    wb_addr_i  : in  std_logic_vector(g_BYTE_ADDR_WIDTH - 3 downto 0);
    wb_data_i  : in  std_logic_vector(g_DATA_PORT_SIZE - 1 downto 0);
    wb_data_o  : out std_logic_vector(g_DATA_PORT_SIZE - 1 downto 0);
    wb_ack_o   : out std_logic;
    wb_stall_o : out std_logic
    );

end entity ddr3_ctrl_wb;



--==============================================================================
--! Architecure declaration for ddr3_ctrl_wb
--==============================================================================
architecture rtl of ddr3_ctrl_wb is


  ------------------------------------------------------------------------------
  -- Constants declaration
  ------------------------------------------------------------------------------
  constant c_DDR_BURST_LENGTH : integer := 32;  -- must not exceed 63

  constant c_FIFO_ALMOST_FULL : std_logic_vector(6 downto 0) := std_logic_vector(to_unsigned(57, 7));

  ------------------------------------------------------------------------------
  -- Types declaration
  ------------------------------------------------------------------------------
  type t_wb_fsm_states is (WB_IDLE, WB_WRITE, WB_READ, WB_READ_WAIT);

  ------------------------------------------------------------------------------
  -- Signals declaration
  ------------------------------------------------------------------------------
  signal rst_n           : std_logic;

  signal wb_fsm_state    : t_wb_fsm_states := WB_IDLE;

  signal ddr_burst_cnt     : unsigned(5 downto 0);
  signal ddr_cmd_en        : std_logic;
  signal ddr_cmd_en_d      : std_logic;
  signal ddr_cmd_en_r_edge : std_logic;
  signal ddr_cmd_instr     : std_logic_vector(2 downto 0);
  signal ddr_cmd_bl        : std_logic_vector(5 downto 0);
  signal ddr_cmd_byte_addr : std_logic_vector(g_BYTE_ADDR_WIDTH - 1 downto 0);
  signal ddr_wr_en         : std_logic;
  signal ddr_wr_mask       : std_logic_vector(g_MASK_SIZE - 1 downto 0);
  signal ddr_wr_data       : std_logic_vector(g_DATA_PORT_SIZE - 1 downto 0);
  signal ddr_rd_en         : std_logic;


--==============================================================================
--! Architecure begin
--==============================================================================
begin


  ------------------------------------------------------------------------------
  -- Wishbone interface
  ------------------------------------------------------------------------------

  -- Reset sync to wishbone clock
  p_rst_sync : process (rst_n_i, wb_clk_i)
  begin
    if (rst_n_i = '0') then
      rst_n <= '0';
    elsif rising_edge(wb_clk_i) then
      rst_n <= '1';
    end if;
  end process p_rst_sync;

  -- Clocking
  ddr_cmd_clk_o <= wb_clk_i;
  ddr_wr_clk_o <= wb_clk_i;
  ddr_rd_clk_o <= wb_clk_i;

  p_wb_interface : process (wb_clk_i)
  begin
    if (rising_edge(wb_clk_i)) then
      if (rst_n = '0') then
        wb_fsm_state    <= WB_IDLE;
        wb_ack_o        <= '0';
        wb_data_o       <= (others => '0');
        --wb_stall_o      <= '0';
        ddr_cmd_en        <= '0';
        ddr_cmd_byte_addr <= (others => '0');
        ddr_cmd_bl        <= (others => '0');
        ddr_cmd_instr     <= (others => '0');
        ddr_wr_data       <= (others => '0');
        ddr_wr_mask       <= (others => '0');
        ddr_wr_en         <= '0';
        ddr_rd_en         <= '0';
      else
        case wb_fsm_state is

          when WB_IDLE =>
            if (wb_cyc_i = '1' and wb_stb_i = '1' and wb_we_i = '1') then
              -- Write from wishbone
              ddr_rd_en         <= '0';
              wb_ack_o        <= '0';
              ddr_cmd_en        <= '0';
              ddr_cmd_instr     <= "000";
              ddr_cmd_bl        <= "000000";
              ddr_cmd_byte_addr <= wb_addr_i & "00";
              ddr_wr_mask       <= "0000";
              ddr_wr_data       <= wb_data_i;
              ddr_wr_en         <= '1';
              wb_fsm_state    <= WB_WRITE;
            elsif (wb_cyc_i = '1' and wb_stb_i = '1' and wb_we_i = '0') then
              -- Read from wishbone
              ddr_wr_en         <= '0';
              wb_ack_o        <= '0';
              ddr_cmd_en        <= '0';
              ddr_cmd_instr     <= "001";
              ddr_cmd_bl        <= "000000";
              ddr_cmd_byte_addr <= wb_addr_i & "00";
              wb_fsm_state    <= WB_READ;
            else
              wb_ack_o <= '0';
              ddr_cmd_en <= '0';
              ddr_wr_en  <= '0';
              ddr_rd_en  <= '0';
            end if;

          when WB_WRITE =>
            wb_ack_o     <= '1';
            ddr_wr_en      <= '0';
            ddr_cmd_en     <= '1';
            wb_fsm_state <= WB_IDLE;

          when WB_READ =>
            ddr_cmd_en     <= '1';
            wb_fsm_state <= WB_READ_WAIT;

          when WB_READ_WAIT =>
            ddr_cmd_en  <= '0';
            ddr_rd_en   <= not(ddr_rd_empty_i);
            wb_ack_o  <= ddr_rd_en;
            wb_data_o <= ddr_rd_data_i;
            if (ddr_rd_en = '1') then
              wb_fsm_state <= WB_IDLE;
            end if;

          when others => null;

        end case;
      end if;
    end if;
  end process p_wb_interface;

  -- Port 1 pipelined mode compatibility
  wb_stall_o <= ddr_cmd_full_i or ddr_wr_full_i or ddr_rd_full_i;

  -- Assign outputs
  ddr_cmd_en_o <= ddr_cmd_en;
  ddr_cmd_instr_o <= ddr_cmd_instr;
  ddr_cmd_bl_o <= ddr_cmd_bl;
  ddr_cmd_byte_addr_o <= ddr_cmd_byte_addr;

  ddr_wr_en_o <= ddr_wr_en;
  ddr_wr_mask_o <= ddr_wr_mask;
  ddr_wr_data_o <= ddr_wr_data;

  ddr_rd_en_o <= ddr_rd_en;


end architecture rtl;
--==============================================================================
--! Architecure end
--==============================================================================
