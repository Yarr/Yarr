--------------------------------------------------------------------------------
--                                                                            --
-- CERN BE-CO-HT         GN4124 core for PCIe FMC carrier                     --
--                       http://www.ohwr.org/projects/gn4124-core             --
--------------------------------------------------------------------------------
--
-- unit name: 32-bit Wishbone master (wbmaster32.vhd)
--
-- authors: Simon Deprez (simon.deprez@cern.ch)
--          Matthieu Cattin (matthieu.cattin@cern.ch)
--
-- date: 12-08-2010
--
-- version: 0.2
--
-- description: Provides a Wishbone interface for single read and write
--              control and status registers
--
-- dependencies: general-cores library (genrams package)
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
-- last changes: 27-09-2010 (mcattin) Split wishbone and gn4124 clock domains
--               All signals crossing the clock domains are now going through fifos.
--               Dead times optimisation in packet generator.
--               11-07-2011 (mcattin) Replaced Xilinx Coregen FIFOs with genrams
--               library cores from ohwr.org
--------------------------------------------------------------------------------
-- TODO: - byte enable support.
--------------------------------------------------------------------------------

library IEEE;
use IEEE.STD_LOGIC_1164.all;
use IEEE.NUMERIC_STD.all;
use work.wshexp_core_pkg.all;

use work.common_pkg.all;


entity wbmaster32 is
  generic (
    g_ACK_TIMEOUT : positive := 100     -- Wishbone ACK timeout (in wb_clk cycles)
    );
  port
    (
      ---------------------------------------------------------
      -- GN4124 core clock and reset
      clk_i   : in std_logic;
      rst_n_i : in std_logic;

      ---------------------------------------------------------
      -- From P2L packet decoder
      --
      -- Header
      pd_wbm_hdr_start_i  : in std_logic;                      -- Header strobe
      --pd_wbm_hdr_length_i : in std_logic_vector(9 downto 0); -- Packet length in 32-bit words multiples
      pd_wbm_hdr_rid_i    : in std_logic_vector(15 downto 0);  -- Requester ID
      pd_wbm_hdr_cid_i    : in std_logic_vector(15 downto 0);  -- Completer ID
      pd_wbm_hdr_tag_i    : in std_logic_vector(7 downto 0);
      pd_wbm_target_mrd_i : in std_logic;                      -- Target memory read
      pd_wbm_target_mwr_i : in std_logic;                      -- Target memory write
      --
      -- Address
      pd_wbm_addr_start_i : in std_logic;                      -- Address strobe
      pd_wbm_addr_i       : in std_logic_vector(31 downto 0);  -- Target address (in byte) that will increment with data
                                                               -- increment = 4 bytes
      --
      -- Data
      pd_wbm_data_valid_i : in std_logic;                      -- Indicates Data is valid
      --pd_wbm_data_last_i  : in std_logic;                      -- Indicates end of the packet
      pd_wbm_data_i       : in std_logic_vector(31 downto 0);  -- Data
      --pd_wbm_be_i         : in std_logic_vector(3 downto 0);   -- Byte Enable for data

      ---------------------------------------------------------
      -- P2L channel control
      p_wr_rdy_o   : out std_logic_vector(1 downto 0);  -- Ready to accept target write
      p2l_rdy_o    : out std_logic;                     -- De-asserted to pause transfer already in progress
      p_rd_d_rdy_i : in  std_logic_vector(1 downto 0);  -- Asserted when GN4124 ready to accept read completion with data

      ---------------------------------------------------------
      -- To the arbiter (L2P data)
      --wbm_arb_valid_o  : out std_logic;  -- Read completion signals
      --wbm_arb_dframe_o : out std_logic;  -- Toward the arbiter
      --wbm_arb_data_o   : out std_logic_vector(31 downto 0);
      --wbm_arb_req_o    : out std_logic;
      --arb_wbm_gnt_i    : in  std_logic;
      
      
      wbm_arb_tdata_o : out STD_LOGIC_VECTOR (64 - 1 downto 0);
      wbm_arb_tkeep_o : out STD_LOGIC_VECTOR (64/8 - 1 downto 0);
      --wbm_arb_tuser_o : out STD_LOGIC_VECTOR (3 downto 0);
      wbm_arb_tlast_o : out STD_LOGIC;
      wbm_arb_tvalid_o : out STD_LOGIC;
      wbm_arb_tready_i : in STD_LOGIC;
      wbm_arb_req_o    : out  std_logic;

      ---------------------------------------------------------
      -- CSR wishbone interface
      wb_clk_i   : in  std_logic;                      -- Wishbone bus clock
      wb_adr_o   : out std_logic_vector(30 downto 0);  -- Address
      wb_dat_o   : out std_logic_vector(31 downto 0);  -- Data out
      wb_sel_o   : out std_logic_vector(3 downto 0);   -- Byte select
      wb_stb_o   : out std_logic;                      -- Strobe
      wb_we_o    : out std_logic;                      -- Write
      wb_cyc_o   : out std_logic;                      -- Cycle
      wb_dat_i   : in  std_logic_vector(31 downto 0);  -- Data in
      wb_ack_i   : in  std_logic;                      -- Acknowledge
      wb_stall_i : in  std_logic;                      -- Stall
      wb_err_i   : in  std_logic;                      -- Error
      wb_rty_i   : in  std_logic;                      -- Retry
      wb_int_i   : in  std_logic                       -- Interrupt
      );
end wbmaster32;


architecture behaviour of wbmaster32 is


  -----------------------------------------------------------------------------
  -- Constants declaration
  -----------------------------------------------------------------------------
  constant c_TO_WB_FIFO_FULL_THRES   : integer := 500;
  constant c_FROM_WB_FIFO_FULL_THRES : integer := 500;

  -----------------------------------------------------------------------------
  -- Signals declaration
  -----------------------------------------------------------------------------

  -- Sync fifos
  signal fifo_rst_n : std_logic;

  signal to_wb_fifo_empty : std_logic;
  signal to_wb_fifo_full  : std_logic;
  signal to_wb_fifo_rd    : std_logic;
  signal to_wb_fifo_wr    : std_logic;
  signal to_wb_fifo_din   : std_logic_vector(63 downto 0);
  signal to_wb_fifo_dout  : std_logic_vector(63 downto 0);
  signal to_wb_fifo_rw    : std_logic;
  signal to_wb_fifo_data  : std_logic_vector(31 downto 0);
  signal to_wb_fifo_addr  : std_logic_vector(30 downto 0);

  signal from_wb_fifo_empty : std_logic;
  signal from_wb_fifo_full  : std_logic;
  signal from_wb_fifo_rd    : std_logic;
  signal from_wb_fifo_wr    : std_logic;
  signal from_wb_fifo_din   : std_logic_vector(63 downto 0);
  signal from_wb_fifo_dout  : std_logic_vector(63 downto 0);

  -- Wishbone
  type   wishbone_state_type is (WB_IDLE, WB_READ_FIFO, WB_CYCLE, WB_WAIT_ACK);
  signal wishbone_current_state : wishbone_state_type;

  signal wb_ack_t   : std_logic;
  signal wb_err_t   : std_logic;
  signal wb_dat_i_t : std_logic_vector(31 downto 0);
  signal wb_cyc_t   : std_logic;
  signal wb_dat_o_t : std_logic_vector(31 downto 0);
  signal wb_stb_t   : std_logic;
  signal wb_adr_t   : std_logic_vector(30 downto 0);
  signal wb_we_t    : std_logic;
  signal wb_sel_t   : std_logic_vector(3 downto 0);
  signal wb_stall_t : std_logic;

  signal wb_ack_timeout_cnt : unsigned(log2_ceil(g_ACK_TIMEOUT)-1 downto 0);
  signal wb_ack_timeout     : std_logic;

  -- L2P packet generator
  type   l2p_read_cpl_state_type is (L2P_IDLE, L2P_HEADER, L2P_DATA);
  signal l2p_read_cpl_current_state : l2p_read_cpl_state_type;

  signal p2l_cid      : std_logic_vector(15 downto 0);
  signal p2l_rid      : std_logic_vector(15 downto 0);
  signal p2l_tag      : std_logic_vector(7 downto 0);
  signal s_l2p_header0 : std_logic_vector(63 downto 0);
  signal s_l2p_header1 : std_logic_vector(63 downto 0);
  constant payload_length_c : STD_LOGIC_VECTOR(9 downto 0) := "0000000001";

  signal byte_swap_c : STD_LOGIC_VECTOR (1 downto 0);
begin
  byte_swap_c <= "11";

  ------------------------------------------------------------------------------
  -- Active high reset for fifo
  ------------------------------------------------------------------------------
  -- Creates an active high reset for fifos regardless of c_RST_ACTIVE value
  gen_fifo_rst_n : if c_RST_ACTIVE = '0' generate
    fifo_rst_n <= rst_n_i;
  end generate;

  gen_fifo_rst : if c_RST_ACTIVE = '1' generate
    fifo_rst_n <= not(rst_n_i);
  end generate;

  ------------------------------------------------------------------------------
  -- Write frame from P2L decoder to fifo
  ------------------------------------------------------------------------------

  -- ready to receive new target write if fifo not full
  p_wr_rdy_o <= "00" when to_wb_fifo_full = '1' else "11";

  -- pause transfer from GN4124 when fifo is full
  p2l_rdy_o <= not(to_wb_fifo_full);

  p_from_decoder : process (clk_i, rst_n_i)
  begin
    if (rst_n_i = c_RST_ACTIVE) then
      to_wb_fifo_din <= (others => '0');
      to_wb_fifo_wr  <= '0';
    elsif rising_edge(clk_i) then
      if (pd_wbm_target_mwr_i = '1' and pd_wbm_data_valid_i = '1') then
        -- Target write
        -- wishbone address is in 32-bit words and address from PCIe in byte
        -- pd_wbm_addr_i(0) represent the BAR (0 = BAR0, 1 = BAR 2)
        to_wb_fifo_din(62 downto 32) <= pd_wbm_addr_i(0) & pd_wbm_addr_i(31 downto 2);
        to_wb_fifo_din(31 downto 0)  <= pd_wbm_data_i;
        to_wb_fifo_din(63)           <= '1';
        to_wb_fifo_wr                <= '1';
      elsif (pd_wbm_target_mrd_i = '1' and pd_wbm_addr_start_i = '1') then
        -- Target read request
        -- wishbone address is in 32-bit words and address from PCIe in byte
        -- pd_wbm_addr_i(0) represent the BAR (0 = BAR0, 1 = BAR 2)
        to_wb_fifo_din(62 downto 32) <= pd_wbm_addr_i(0) & pd_wbm_addr_i(31 downto 2);
        to_wb_fifo_din(63)           <= '0';
        to_wb_fifo_wr                <= '1';
      else
        to_wb_fifo_wr <= '0';
      end if;
    end if;
  end process p_from_decoder;

  ------------------------------------------------------------------------------
  -- Packet generator
  ------------------------------------------------------------------------------
  -- Generates read completion with requested data
  -- Single 32-bit word read only

  -- Store CID and tag for read completion packet
  p_pkt_gen : process (clk_i, rst_n_i)
  begin
    if (rst_n_i = c_RST_ACTIVE) then
      p2l_cid <= (others => '0');
      p2l_rid <= (others => '0');
      p2l_tag <= (others => '0');
    elsif rising_edge(clk_i) then
      if (pd_wbm_hdr_start_i = '1') then
        p2l_cid <= pd_wbm_hdr_cid_i;
        p2l_rid <= pd_wbm_hdr_rid_i;
        p2l_tag <= pd_wbm_hdr_tag_i;
      end if;
    end if;
  end process p_pkt_gen;

  --read completion header
  s_l2p_header0 <= p2l_cid &     -- H1: Completer ID
                  X"0" &        --     Status and BCM
                  X"004" &      --     Byte count
                  "010" &       -- H0: FMT
                  "01010" &     --     Type
                  X"00" &       --     some bits
                  "000000" &    --     some bits
                  payload_length_c; -- length
  
  s_l2p_header1 <= wb_dat_i_t & p2l_rid & p2l_tag & "0" & wb_adr_t(4 downto 0) & "00";
--  s_l2p_header <= "000"                 -->  Traffic Class
--                  & '0'                 -->  Reserved
--                  & "0101"              -->  Read completion (Master read competition with data)
--                  & "000000"            -->  Reserved
--                  & "00"                -->  Completion Status
--                  & '1'                 -->  Last completion packet
--                  & "00"                -->  Reserved
--                  & '0'                 -->  VC (Vitrual Channel)
--                  & p2l_cid             -->  CID (Completion Identifer)
--                  & "0000000001";       -->  Length (Single 32-bit word read only)

  ------------------------------------------------------------------------------
  -- L2P packet write FSM
  ------------------------------------------------------------------------------
  process (clk_i, rst_n_i)
  begin
    if(rst_n_i = c_RST_ACTIVE) then
      l2p_read_cpl_current_state <= L2P_IDLE;
      wbm_arb_req_o              <= '0';
      wbm_arb_tdata_o            <= (others => '0');
      wbm_arb_tvalid_o           <= '0';
      wbm_arb_tlast_o            <= '0';
      from_wb_fifo_rd            <= '0';
    elsif rising_edge(clk_i) then
      case l2p_read_cpl_current_state is

        when L2P_IDLE =>
          wbm_arb_req_o    <= '0';
          wbm_arb_tdata_o  <= (others => '0');
          wbm_arb_tvalid_o <= '0';
          wbm_arb_tlast_o  <= '0';
          if(from_wb_fifo_empty = '0' and p_rd_d_rdy_i = "11") then
            -- generate a packet when read data in fifo and GN4124 ready to receive the packet
            wbm_arb_req_o              <= '1';
            from_wb_fifo_rd            <= '1';
            l2p_read_cpl_current_state <= L2P_HEADER;
          end if;

        when L2P_HEADER =>
          from_wb_fifo_rd <= '0';
          if(wbm_arb_tready_i = '1') then
            wbm_arb_req_o              <= '0';
            wbm_arb_tdata_o            <= s_l2p_header0;
            wbm_arb_tvalid_o           <= '1';
            wbm_arb_tlast_o            <= '0';
            l2p_read_cpl_current_state <= L2P_DATA;
          end if;

        when L2P_DATA =>
          l2p_read_cpl_current_state <= L2P_IDLE;
          wbm_arb_tdata_o            <= f_byte_swap(true, from_wb_fifo_dout(63 downto 32), byte_swap_c) & from_wb_fifo_dout(31 downto 0); -- DATA and ADDRESS
          wbm_arb_tlast_o            <= '1';

        when others =>
          l2p_read_cpl_current_state <= L2P_IDLE;
          wbm_arb_req_o              <= '0';
          wbm_arb_tdata_o            <= (others => '0');
          wbm_arb_tvalid_o           <= '0';
          wbm_arb_tlast_o           <= '0';
          from_wb_fifo_rd            <= '0';

      end case;
    end if;
  end process;
  
  wbm_arb_tkeep_o <= X"FF";
  
  -----------------------------------------------------------------------------
  -- FIFOs for transition between GN4124 core and wishbone clock domain
  -----------------------------------------------------------------------------

  -- fifo for PCIe to WB transfer
  cmp_fifo_to_wb : generic_async_fifo
    generic map (
      g_data_width             => 64,
      g_size                   => 512,
      g_show_ahead             => false,
      g_with_rd_empty          => true,
      g_with_rd_full           => false,
      g_with_rd_almost_empty   => false,
      g_with_rd_almost_full    => false,
      g_with_rd_count          => false,
      g_with_wr_empty          => false,
      g_with_wr_full           => false,
      g_with_wr_almost_empty   => false,
      g_with_wr_almost_full    => true,
      g_with_wr_count          => false,
      g_almost_empty_threshold => 0,
      g_almost_full_threshold  => c_TO_WB_FIFO_FULL_THRES)
    port map (
      rst_n_i           => fifo_rst_n,
      clk_wr_i          => clk_i,
      d_i               => to_wb_fifo_din,
      we_i              => to_wb_fifo_wr,
      wr_empty_o        => open,
      wr_full_o         => open,
      wr_almost_empty_o => open,
      wr_almost_full_o  => to_wb_fifo_full,
      wr_count_o        => open,
      clk_rd_i          => wb_clk_i,
      q_o               => to_wb_fifo_dout,
      rd_i              => to_wb_fifo_rd,
      rd_empty_o        => to_wb_fifo_empty,
      rd_full_o         => open,
      rd_almost_empty_o => open,
      rd_almost_full_o  => open,
      rd_count_o        => open);

  to_wb_fifo_rw   <= to_wb_fifo_dout(63);
  to_wb_fifo_addr <= to_wb_fifo_dout(62 downto 32);  -- 31-bit
  to_wb_fifo_data <= to_wb_fifo_dout(31 downto 0);   -- 32-bit

  -- fifo for WB to PCIe transfer
  cmp_from_wb_fifo : generic_async_fifo
    generic map (
      g_data_width             => 64,
      g_size                   => 512,
      g_show_ahead             => false,
      g_with_rd_empty          => true,
      g_with_rd_full           => false,
      g_with_rd_almost_empty   => false,
      g_with_rd_almost_full    => false,
      g_with_rd_count          => false,
      g_with_wr_empty          => false,
      g_with_wr_full           => false,
      g_with_wr_almost_empty   => false,
      g_with_wr_almost_full    => true,
      g_with_wr_count          => false,
      g_almost_empty_threshold => 0,
      g_almost_full_threshold  => c_FROM_WB_FIFO_FULL_THRES)
    port map (
      rst_n_i           => fifo_rst_n,
      clk_wr_i          => wb_clk_i,
      d_i               => from_wb_fifo_din,
      we_i              => from_wb_fifo_wr,
      wr_empty_o        => open,
      wr_full_o         => open,
      wr_almost_empty_o => open,
      wr_almost_full_o  => from_wb_fifo_full,
      wr_count_o        => open,
      clk_rd_i          => clk_i,
      q_o               => from_wb_fifo_dout,
      rd_i              => from_wb_fifo_rd,
      rd_empty_o        => from_wb_fifo_empty,
      rd_full_o         => open,
      rd_almost_empty_o => open,
      rd_almost_full_o  => open,
      rd_count_o        => open);

  -----------------------------------------------------------------------------
  -- Wishbone master FSM
  -----------------------------------------------------------------------------
  p_wb_fsm : process (wb_clk_i, rst_n_i)
  begin
    if(rst_n_i = c_RST_ACTIVE) then
      wishbone_current_state <= WB_IDLE;
      to_wb_fifo_rd          <= '0';
      wb_cyc_t               <= '0';
      wb_stb_t               <= '0';
      wb_we_t                <= '0';
      wb_sel_t               <= "0000";
      wb_dat_o_t             <= (others => '0');
      wb_adr_t               <= (others => '0');
      from_wb_fifo_din       <= (others => '0');
      from_wb_fifo_wr        <= '0';
    elsif rising_edge(wb_clk_i) then
      case wishbone_current_state is

        when WB_IDLE =>
          -- stop writing to fifo
          from_wb_fifo_wr <= '0';
          -- clear bus
          wb_cyc_t        <= '0';
          wb_stb_t        <= '0';
          wb_sel_t        <= "0000";
          -- Wait for a Wishbone cycle
          if (to_wb_fifo_empty = '0') then
            -- read requset in fifo (address, data and transfer type)
            to_wb_fifo_rd          <= '1';
            wishbone_current_state <= WB_READ_FIFO;
          end if;

        when WB_READ_FIFO =>
          -- read only one request in fifo (no block transfer)
          to_wb_fifo_rd          <= '0';
          wishbone_current_state <= WB_CYCLE;

        when WB_CYCLE =>
          -- initate a bus cycle
          wb_cyc_t               <= '1';
          wb_stb_t               <= '1';
          wb_we_t                <= to_wb_fifo_rw;
          wb_sel_t               <= "1111";
          wb_adr_t               <= to_wb_fifo_addr;
          --if (to_wb_fifo_rw = '1') then
          wb_dat_o_t             <= to_wb_fifo_data;
          --end if;
          -- wait for slave to ack
          wishbone_current_state <= WB_WAIT_ACK;

        when WB_WAIT_ACK =>
          if wb_stall_t = '0' then
            wb_stb_t <= '0';
          end if;
          if (wb_ack_t = '1') then
            -- for read cycles write read data to fifo
            if (wb_we_t = '0') then
              from_wb_fifo_din <= s_l2p_header1;
              from_wb_fifo_wr  <= '1';
            end if;
            -- end of the bus cycle
            wb_cyc_t               <= '0';
            wishbone_current_state <= WB_IDLE;
          elsif (wb_err_t = '1') or (wb_ack_timeout = '1') then
            -- e.g. When trying to access unmapped wishbone addresses,
            -- the wb crossbar asserts ERR. If ERR is not asserted when
            -- accessing un-mapped addresses, a timeout makes sure the
            -- transaction terminates.
            if (wb_we_t = '0') then
              from_wb_fifo_din <= (others => '1');  -- dummy data as the transaction failed
              from_wb_fifo_wr  <= '1';
            end if;
            -- end of the bus cycle
            wb_cyc_t               <= '0';
            wishbone_current_state <= WB_IDLE;
          end if;

        when others =>
          -- should not get here!
          wishbone_current_state <= WB_IDLE;
          wb_cyc_t               <= '0';
          wb_stb_t               <= '0';
          wb_we_t                <= '0';
          wb_sel_t               <= "0000";
          wb_dat_o_t             <= (others => '0');
          wb_adr_t               <= (others => '0');
          to_wb_fifo_rd          <= '0';
          from_wb_fifo_din       <= (others => '0');
          from_wb_fifo_wr        <= '0';

      end case;
    end if;
  end process p_wb_fsm;

  wb_adr_o   <= wb_adr_t;
  wb_cyc_o   <= wb_cyc_t;
  wb_stb_o   <= wb_stb_t;
  wb_we_o    <= wb_we_t;
  wb_sel_o   <= wb_sel_t;
  wb_dat_i_t <= wb_dat_i;
  wb_dat_o   <= wb_dat_o_t;
  wb_ack_t   <= wb_ack_i;
  wb_stall_t <= wb_stall_i;
  wb_err_t   <= wb_err_i;

  -- ACK timeout
  p_wb_ack_timeout_cnt : process (wb_clk_i, rst_n_i)
  begin
    if rst_n_i = c_RST_ACTIVE then
      wb_ack_timeout_cnt <= (others => '1');
    elsif rising_edge(wb_clk_i) then
      if wishbone_current_state = WB_WAIT_ACK then
        if wb_ack_timeout_cnt /= 0 then
          wb_ack_timeout_cnt <= wb_ack_timeout_cnt - 1;
        end if;
      else
        wb_ack_timeout_cnt <= (others => '1');
      end if;
    end if;
  end process p_wb_ack_timeout_cnt;

  p_ack_timeout : process (wb_clk_i, rst_n_i)
  begin
    if rst_n_i = c_RST_ACTIVE then
      wb_ack_timeout <= '0';
    elsif rising_edge(wb_clk_i) then
      if wb_ack_timeout_cnt = 0 then
        wb_ack_timeout <= '1';
      else
        wb_ack_timeout <= '0';
      end if;
    end if;
  end process p_ack_timeout;

end behaviour;

