--==============================================================================
--! @file ddr3_ctrl_wb.vhd
--==============================================================================

--! Standard library
library IEEE;
--! Standard packages
use IEEE.STD_LOGIC_1164.all;
use IEEE.NUMERIC_STD.all;
--! Specific packages
library work;
use work.ddr3_ctrl_pkg.all;

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
-- DDR3 Controller Wishbone Interface
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--! @brief
--! DDR3 Controller Wishbone Interface
--------------------------------------------------------------------------------
--! @details
--! Wishbone interface for DDR3 controller.
--------------------------------------------------------------------------------
--! @version
--! 0.1 | mc | 12.07.2011 | File creation and Doxygen comments
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
    -- Status
    ----------------------------------------------------------------------------

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
    wb_addr_i  : in  std_logic_vector(31 downto 0);
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

  constant c_FIFO_ALMOST_FULL : std_logic_vector(6 downto 0) := std_logic_vector(to_unsigned(53, 7));

  constant c_ADDR_SHIFT : integer := log2_ceil(g_DATA_PORT_SIZE/8);

  ------------------------------------------------------------------------------
  -- Types declaration
  ------------------------------------------------------------------------------
  --type t_wb_fsm_states is (WB_IDLE, WB_WRITE, WB_READ_REQ, WB_READ_WAIT,
  --                         WB_READ_ACK, WB_READ_REQ_ACK);

  ------------------------------------------------------------------------------
  -- Signals declaration
  ------------------------------------------------------------------------------
  signal rst_n : std_logic;

  signal wb_cyc_d      : std_logic;
  signal wb_cyc_f_edge : std_logic;
  signal wb_cyc_r_edge : std_logic;
  signal wb_stb_valid  : std_logic;
  signal wb_stb_d      : std_logic;
  signal wb_stb_f_edge : std_logic;
  signal wb_we_d       : std_logic;
  signal wb_we_f_edge  : std_logic;
  signal wb_addr_d     : std_logic_vector(31 downto 0);
  signal wb_stall      : std_logic;
  signal wb_data_d     : std_logic_vector(31 downto 0);

  signal ddr_burst_cnt     : unsigned(5 downto 0);
  signal ddr_cmd_en        : std_logic;
  signal ddr_cmd_en_d      : std_logic;
  signal ddr_cmd_en_r_edge : std_logic;
  signal ddr_cmd_instr     : std_logic_vector(2 downto 0);
  signal ddr_cmd_bl        : std_logic_vector(5 downto 0);
  signal ddr_cmd_byte_addr : std_logic_vector(g_BYTE_ADDR_WIDTH - 1 downto 0);
  signal addr_shift        : std_logic_vector(c_ADDR_SHIFT-1 downto 0);
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
  ddr_wr_clk_o  <= wb_clk_i;
  ddr_rd_clk_o  <= wb_clk_i;

  -- stb is valid only if cyc is '1'
  wb_stb_valid <= wb_stb_i and wb_cyc_i;

  -- Cycle, we and strobe rising and falling edge detection
  p_wb_cyc_f_edge : process (wb_clk_i)
  begin
    if rising_edge(wb_clk_i) then
      if (rst_n = '0') then
        wb_cyc_d <= '0';
        wb_stb_d <= '0';
        wb_we_d  <= '0';
      else
        wb_cyc_d <= wb_cyc_i;
        wb_stb_d <= wb_stb_valid;
        wb_we_d  <= wb_we_i;
      end if;
    end if;
  end process p_wb_cyc_f_edge;

  wb_cyc_f_edge <= not(wb_cyc_i) and wb_cyc_d;
  wb_cyc_r_edge <= wb_cyc_i and not(wb_cyc_d);
  wb_stb_f_edge <= not(wb_stb_valid) and wb_stb_d;
  wb_we_f_edge  <= not(wb_we_i) and wb_we_d;

  -- Data inputs
  p_ddr_inputs : process (wb_clk_i)
  begin
    if rising_edge(wb_clk_i) then
      if (rst_n = '0') then
        ddr_wr_data <= (others => '0');
        wb_data_d <= (others => '0');
        ddr_wr_en   <= '0';
      else
        if (wb_stb_valid = '1') and (wb_cyc_i = '1') and (wb_we_i = '1') then
          ddr_wr_en <= '1';
        else
          ddr_wr_en <= '0';
        end if;
        --wb_data_d <=  wb_data_i;
        ddr_wr_data <= wb_data_i;
        ddr_wr_mask <= not(wb_sel_i);
      end if;
    end if;
  end process p_ddr_inputs;

  -- Command parameters (burst length and address) registration
--  p_ddr_cmd : process (wb_clk_i)
--  begin
--    if rising_edge(wb_clk_i) then
--      if (rst_n = '0') then
--        ddr_cmd_byte_addr <= (others => '0');
--        ddr_cmd_instr     <= "000";
--        ddr_cmd_bl        <= (others => '0');
--        wb_addr_d         <= (others => '0');
--      else
--        wb_addr_d <= wb_addr_i;
--        if ((ddr_burst_cnt = 0 and wb_cyc_r_edge = '1' and wb_stb_valid = '1') or
--            (ddr_burst_cnt = to_unsigned(1, ddr_burst_cnt'length))) then
--          ddr_cmd_byte_addr <= wb_addr_d(g_BYTE_ADDR_WIDTH-c_ADDR_SHIFT-1 downto 0) & addr_shift;
--          ddr_cmd_instr     <= "00" & not(wb_we_d);
--        end if;
--        ddr_cmd_bl <= std_logic_vector(ddr_burst_cnt - 1);
--      end if;
--    end if;
--  end process p_ddr_cmd;

  addr_shift <= (others => '0');

  -- Command enable signal generation
--  p_ddr_cmd_en : process (wb_clk_i)
--  begin
--    if rising_edge(wb_clk_i) then
--      if (rst_n = '0') then
--        ddr_cmd_en   <= '0';
--        ddr_cmd_en_d <= '0';
--      else
--        ddr_cmd_en_d <= ddr_cmd_en;
--        if (((ddr_burst_cnt = c_DDR_BURST_LENGTH) or
--             (wb_cyc_f_edge = '1' and wb_we_d = '1') or
--             (wb_stb_f_edge = '1' and wb_we_d = '0')) and ddr_cmd_full_i = '0' and ddr_burst_cnt > 0) then
--          ddr_cmd_en <= '1';            -- might have problem if burst_cnt = BURST_LENGTH for more than 2 clk cycles
--        else
--          ddr_cmd_en <= '0';
--        end if;
--      end if;
--    end if;
--  end process p_ddr_cmd_en;

  -- Command enable rising edge detection
  ddr_cmd_en_r_edge <= ddr_cmd_en and not(ddr_cmd_en_d);

  -- Burst counter
--  p_ddr_burst_cnt : process (wb_clk_i)
--  begin
--    if rising_edge(wb_clk_i) then
--      if (rst_n = '0') then
--        ddr_burst_cnt <= (others => '0');
--      else
--        if ((wb_cyc_f_edge = '1' and wb_we_d = '1') or (wb_stb_f_edge = '1' and wb_we_d = '0')) then
--          ddr_burst_cnt <= to_unsigned(0, ddr_burst_cnt'length);
--        elsif (wb_stb_valid = '1' and wb_cyc_i = '1') then
--          if (ddr_burst_cnt = c_DDR_BURST_LENGTH) then
--            ddr_burst_cnt <= to_unsigned(1, ddr_burst_cnt'length);
--          else
--            ddr_burst_cnt <= ddr_burst_cnt + 1;
--          end if;
--        elsif (ddr_burst_cnt = c_DDR_BURST_LENGTH) then
--          ddr_burst_cnt <= to_unsigned(0, ddr_burst_cnt'length);
--        end if;
--      end if;
--    end if;
--  end process p_ddr_burst_cnt;
  
  p_ddr_control : process(wb_clk_i)
  begin
   if rising_edge(wb_clk_i) then
      if (rst_n = '0') then
         ddr_burst_cnt     <= (others => '0');
         ddr_cmd_en        <= '0';
         ddr_cmd_en_d      <= '0';
         ddr_cmd_byte_addr <= (others => '0');
         ddr_cmd_instr     <= "000";
         ddr_cmd_bl        <= (others => '0');
         wb_addr_d         <= (others => '0');
      else
         if (wb_stb_i = '1' and wb_cyc_i = '1') then
            if (ddr_burst_cnt = c_DDR_BURST_LENGTH) then
               ddr_burst_cnt <= to_unsigned(1, ddr_burst_cnt'length);
               ddr_cmd_en <= '1';
            else
               ddr_burst_cnt <= ddr_burst_cnt + 1;
               ddr_cmd_en <= '0';
            end if;
         elsif (wb_stb_i = '0' and wb_cyc_i = '0' and ddr_burst_cnt > 0) then
            ddr_burst_cnt <= to_unsigned(0, ddr_burst_cnt'length);
            ddr_cmd_en <= '1';
         elsif (wb_stb_i = '0' and wb_cyc_i = '1' and ddr_burst_cnt > 0 and wb_we_d = '0' and wb_stall = '0') then
            ddr_burst_cnt <= to_unsigned(0, ddr_burst_cnt'length);
            ddr_cmd_en <= '1';
         else
            ddr_cmd_en <= '0';
         end if;
         
         if (ddr_burst_cnt = to_unsigned(1, ddr_burst_cnt'length)) then
            ddr_cmd_byte_addr <= wb_addr_d(g_BYTE_ADDR_WIDTH-c_ADDR_SHIFT-1 downto 0) & addr_shift;
            ddr_cmd_instr     <= "00" & not(wb_we_d);
         end if;
         
         ddr_cmd_en_d <= ddr_cmd_en;
         wb_addr_d <= wb_addr_i;
         ddr_cmd_bl <= std_logic_vector(ddr_burst_cnt - 1);
         end if;
      end if;
   end process p_ddr_control;

  -- Read enable signal generation
  ddr_rd_en <= not(ddr_rd_empty_i);

  -- Data output and ack
  p_ddr_outputs : process (wb_clk_i)
  begin
    if rising_edge(wb_clk_i) then
      if (rst_n = '0') then
        wb_ack_o  <= '0';
        wb_data_o <= (others => '0');
      else
        -- Generates ack signal
        if (ddr_rd_en = '1') or (ddr_wr_en = '1') then
          wb_ack_o <= '1';
        else
          wb_ack_o <= '0';
        end if;
        -- Registered data output
        wb_data_o <= ddr_rd_data_i;
      end if;
    end if;
  end process p_ddr_outputs;

  -- Stall signal output
  p_ddr_stall : process (wb_clk_i)
  begin
    if rising_edge(wb_clk_i) then
      if (rst_n = '0') then
        wb_stall <= '0';
      else
        if ((ddr_wr_count_i > c_FIFO_ALMOST_FULL) or
            (ddr_wr_full_i = '1') or
            (ddr_rd_count_i > c_FIFO_ALMOST_FULL) or
            (ddr_rd_full_i = '1')) then
          wb_stall <= '1';
        else
          wb_stall <= '0';
        end if;
      end if;
    end if;
  end process p_ddr_stall;
  wb_stall_o <= wb_stall;
  --wb_stall_o <= ddr_cmd_full_i or ddr_wr_full_i or ddr_rd_full_i;


  -- Assign outputs
  ddr_cmd_en_o        <= ddr_cmd_en;
  ddr_cmd_instr_o     <= ddr_cmd_instr;
  ddr_cmd_bl_o        <= ddr_cmd_bl;
  ddr_cmd_byte_addr_o <= ddr_cmd_byte_addr;

  ddr_wr_en_o   <= ddr_wr_en;
  ddr_wr_mask_o <= ddr_wr_mask;
  ddr_wr_data_o <= ddr_wr_data;

  ddr_rd_en_o <= ddr_rd_en;


end architecture rtl;
--==============================================================================
--! Architecure end
--==============================================================================
