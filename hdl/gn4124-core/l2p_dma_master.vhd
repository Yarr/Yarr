--------------------------------------------------------------------------------
--                                                                            --
-- CERN BE-CO-HT         GN4124 core for PCIe FMC carrier                     --
--                       http://www.ohwr.org/projects/gn4124-core             --
--------------------------------------------------------------------------------
--
-- unit name: 32-bit DMA master (l2p_dma_master.vhd)
--
-- authors: Simon Deprez (simon.deprez@cern.ch)
--          Matthieu Cattin (matthieu.cattin@cern.ch)
--
-- date: 31-08-2010
--
-- version: 1.0
--
-- description: Provides a pipelined Wishbone interface to performs DMA
--              transfers from local application to PCI express host.
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
-- last changes: 11-07-2011 (mcattin) Replaced Xilinx Coregen FIFOs with genrams
--               library cores from ohwr.org
-- 26.02.14 (theim) Fixed a racecondition where the statemachine could end up
--                  in a deadlock. Added a Setup state after Idle and fixed the
--                  counting of the l2p_data_cnt.
--------------------------------------------------------------------------------
-- TODO: - byte enable support
--------------------------------------------------------------------------------

library IEEE;
use IEEE.STD_LOGIC_1164.all;
use IEEE.NUMERIC_STD.all;
use work.gn4124_core_pkg.all;
use work.common_pkg.all;


entity l2p_dma_master is
  generic (
    -- Enable byte swap module (if false, no swap)
    g_BYTE_SWAP : boolean := false
    );
  port
    (
      ---------------------------------------------------------
      -- GN4124 core clock and reset
      clk_i   : in std_logic;
      rst_n_i : in std_logic;

      ---------------------------------------------------------
      -- From the DMA controller
      dma_ctrl_target_addr_i : in  std_logic_vector(31 downto 0);
      dma_ctrl_host_addr_h_i : in  std_logic_vector(31 downto 0);
      dma_ctrl_host_addr_l_i : in  std_logic_vector(31 downto 0);
      dma_ctrl_len_i         : in  std_logic_vector(31 downto 0);
      dma_ctrl_start_l2p_i   : in  std_logic;
      dma_ctrl_done_o        : out std_logic;
      dma_ctrl_error_o       : out std_logic;
      dma_ctrl_byte_swap_i   : in  std_logic_vector(1 downto 0);
      dma_ctrl_abort_i       : in  std_logic;

      ---------------------------------------------------------
      -- To the arbiter (L2P data)
      ldm_arb_valid_o  : out std_logic;  -- Read completion signals
      ldm_arb_dframe_o : out std_logic;  -- Toward the arbiter
      ldm_arb_data_o   : out std_logic_vector(31 downto 0);
      ldm_arb_req_o    : out std_logic;
      arb_ldm_gnt_i    : in  std_logic;

      ---------------------------------------------------------
      -- L2P channel control
      l2p_edb_o  : out std_logic;                     -- Asserted when transfer is aborted
      l_wr_rdy_i : in  std_logic_vector(1 downto 0);  -- Asserted when GN4124 is ready to receive master write
      l2p_rdy_i  : in  std_logic;                     -- De-asserted to pause transfer already in progress
      tx_error_i : in  std_logic;                     -- Asserted when unexpected or malformed packet recevied

      ---------------------------------------------------------
      -- DMA Interface (Pipelined Wishbone)
      l2p_dma_clk_i   : in  std_logic;                      -- Bus clock
      l2p_dma_adr_o   : out std_logic_vector(31 downto 0);  -- Adress
      l2p_dma_dat_i   : in  std_logic_vector(31 downto 0);  -- Data in
      l2p_dma_dat_o   : out std_logic_vector(31 downto 0);  -- Data out
      l2p_dma_sel_o   : out std_logic_vector(3 downto 0);   -- Byte select
      l2p_dma_cyc_o   : out std_logic;                      -- Read or write cycle
      l2p_dma_stb_o   : out std_logic;                      -- Read or write strobe
      l2p_dma_we_o    : out std_logic;                      -- Write
      l2p_dma_ack_i   : in  std_logic;                      -- Acknowledge
      l2p_dma_stall_i : in  std_logic;                      -- for pipelined Wishbone
      p2l_dma_cyc_i   : in  std_logic                       -- P2L dma wb cycle (for bus arbitration)
      );
end l2p_dma_master;


architecture behaviour of l2p_dma_master is
--    component generic_async_fifo is
--    generic (
--        g_data_width : natural;
--        g_size       : natural;
--        g_show_ahead : boolean := false;
--
--        -- Read-side flag selection
--        g_with_rd_empty        : boolean := true;   -- with empty flag
--        g_with_rd_full         : boolean := false;  -- with full flag
--        g_with_rd_almost_empty : boolean := false;
--        g_with_rd_almost_full  : boolean := false;
--        g_with_rd_count        : boolean := false;  -- with words counter
--
--        g_with_wr_empty        : boolean := false;
--        g_with_wr_full         : boolean := true;
--        g_with_wr_almost_empty : boolean := false;
--        g_with_wr_almost_full  : boolean := false;
--        g_with_wr_count        : boolean := false;
--
--        g_almost_empty_threshold : integer;  -- threshold for almost empty flag
--        g_almost_full_threshold  : integer   -- threshold for almost full flag
--    );
--
--    port (
--        rst_n_i : in std_logic := '1';
--
--        -- write port
--        clk_wr_i : in std_logic;
--        d_i      : in std_logic_vector(g_data_width-1 downto 0);
--        we_i     : in std_logic;
--
--        wr_empty_o        : out std_logic;
--        wr_full_o         : out std_logic;
--        wr_almost_empty_o : out std_logic;
--        wr_almost_full_o  : out std_logic;
--        wr_count_o        : out std_logic_vector(log2_ceil(g_size)-1 downto 0);
--
--        -- read port
--        clk_rd_i : in  std_logic;
--        q_o      : out std_logic_vector(g_data_width-1 downto 0);
--        rd_i     : in  std_logic;
--
--        rd_empty_o        : out std_logic;
--        rd_full_o         : out std_logic;
--        rd_almost_empty_o : out std_logic;
--        rd_almost_full_o  : out std_logic;
--        rd_count_o        : out std_logic_vector(log2_ceil(g_size)-1 downto 0)
--    );
--    end component generic_async_fifo;
  -----------------------------------------------------------------------------
  -- Constants declaration
  -----------------------------------------------------------------------------

  -- c_L2P_MAX_PAYLOAD is the maximum size (in 32-bit words) of the payload of a packet.
  -- Allowed c_L2P_MAX_PAYLOAD values are: 32, 64, 128, 256, 512, 1024.
  -- This constant must be set according to the GN4124 and motherboard chipset capabilities.
  constant c_L2P_MAX_PAYLOAD      : unsigned(10 downto 0) := to_unsigned(32, 11);  -- in 32-bit words
  constant c_ADDR_FIFO_FULL_THRES : integer               := 800;
  constant c_DATA_FIFO_FULL_THRES : integer               := 800;

  -----------------------------------------------------------------------------
  -- Signals declaration
  -----------------------------------------------------------------------------

  -- Target address counter
  signal target_addr_cnt : unsigned(29 downto 0);
  signal dma_length_cnt  : unsigned(29 downto 0);

  -- Sync FIFOs
  signal fifo_rst_n      : std_logic;
  signal addr_fifo_rd    : std_logic;
  signal addr_fifo_valid : std_logic;
  signal addr_fifo_empty : std_logic;
  signal addr_fifo_dout  : std_logic_vector(31 downto 0);
  signal addr_fifo_din   : std_logic_vector(31 downto 0);
  signal addr_fifo_wr    : std_logic;
  signal addr_fifo_full  : std_logic;

  signal data_fifo_rd    : std_logic;
  signal data_fifo_valid : std_logic;
  signal data_fifo_empty : std_logic;
  signal data_fifo_dout  : std_logic_vector(31 downto 0);
  signal data_fifo_din   : std_logic_vector(31 downto 0);
  signal data_fifo_wr    : std_logic;
  signal data_fifo_full  : std_logic;
  
  signal data_fifo_rd_cnt : unsigned(10 downto 0);

  signal data_fifo_valid_d : std_logic;

  -- Wishbone
  signal wb_read_cnt   : unsigned(31 downto 0);
  signal wb_left_cnt   : unsigned(31 downto 0);
  signal wb_ack_cnt    : unsigned(31 downto 0);
  signal wb_timeout_cnt : unsigned(31 downto 0);
  signal l2p_dma_cyc_t : std_logic;
  signal l2p_dma_stb_t : std_logic;

  -- L2P DMA Master FSM
  type l2p_dma_state_type is (L2P_IDLE, L2P_SETUP, L2P_WAIT_DATA, L2P_HEADER, L2P_ADDR_H,
                              L2P_ADDR_L, L2P_DATA, L2P_LAST_DATA, L2P_WAIT_RDY, L2P_ERROR);
  signal l2p_dma_current_state : l2p_dma_state_type;

  -- L2P packet generator
  signal s_l2p_header : std_logic_vector(31 downto 0);

  signal l2p_len_cnt     : unsigned(29 downto 0);
  signal l2p_address_h   : unsigned(31 downto 0);
  signal l2p_address_l   : unsigned(31 downto 0);
  signal l2p_data_cnt    : unsigned(10 downto 0);
  signal l2p_timeout_cnt : unsigned(31 downto 0);
  signal l2p_64b_address : std_logic;
  signal l2p_len_header  : unsigned(9 downto 0);
  signal l2p_byte_swap   : std_logic_vector(1 downto 0);
  signal l2p_last_packet : std_logic;
  signal l2p_lbe_header  : std_logic_vector(3 downto 0);


begin


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
  -- Target address counter
  ------------------------------------------------------------------------------
  p_target_cnt : process (clk_i, rst_n_i)
  begin
    if(rst_n_i = c_RST_ACTIVE) then
      target_addr_cnt  <= (others => '0');
      dma_length_cnt   <= (others => '0');
      dma_ctrl_error_o <= '0';
      addr_fifo_wr     <= '0';
    elsif rising_edge(clk_i) then
      if (dma_ctrl_start_l2p_i = '1') then
        if (l2p_dma_current_state = L2P_IDLE) then
          -- dma_ctrl_target_addr_i is a byte address and target_addr_cnt is a
          -- 32-bit word address
          target_addr_cnt  <= unsigned(dma_ctrl_target_addr_i(31 downto 2));
          -- dma_ctrl_len_i is in byte and dma_length_cnt is in 32-bit word
          dma_length_cnt   <= unsigned(dma_ctrl_len_i(31 downto 2));
          dma_ctrl_error_o <= '0';
        else
          -- trying to start a DMA transfert when another is still in progress
          -- will gives an error
          target_addr_cnt  <= (others => '0');
          dma_length_cnt   <= (others => '0');
          dma_ctrl_error_o <= '1';
        end if;
      elsif (dma_length_cnt /= 0 and addr_fifo_full = '0') then
        -- increment the target address and write it to address fifo
        addr_fifo_wr    <= '1';
        target_addr_cnt <= target_addr_cnt + 1;
        dma_length_cnt  <= dma_length_cnt - 1;
        -- Adust data width, fifo width is 32 bits
        addr_fifo_din   <= "00" & std_logic_vector(target_addr_cnt);
      else
        addr_fifo_wr <= '0';
      end if;
    end if;
  end process p_target_cnt;

  ------------------------------------------------------------------------------
  -- Packet generator
  ------------------------------------------------------------------------------
  -- Sends data to the host.
  -- Split in several packets if amount of data exceeds max payload size.

  p_pkt_gen : process (clk_i, rst_n_i)
  begin
    if (rst_n_i = c_RST_ACTIVE) then
      l2p_len_cnt     <= (others => '0');
      l2p_data_cnt    <= (others => '0');
      l2p_address_h   <= (others => '0');
      l2p_address_l   <= (others => '0');
      l2p_64b_address <= '0';
      l2p_len_header  <= (others => '0');
      l2p_byte_swap   <= (others => '0');
      l2p_last_packet <= '0';
    elsif rising_edge(clk_i) then
      -- First packet
      if (l2p_dma_current_state = L2P_IDLE) then
        if (dma_ctrl_start_l2p_i = '1') then
          -- store DMA info locally
          l2p_len_cnt   <= unsigned(dma_ctrl_len_i(31 downto 2));
          l2p_address_h <= unsigned(dma_ctrl_host_addr_h_i);
          l2p_address_l <= unsigned(dma_ctrl_host_addr_l_i);
          l2p_byte_swap <= dma_ctrl_byte_swap_i;
        end if;
      elsif (l2p_dma_current_state = L2P_HEADER) then
        -- if DMA length is bigger than the max PCIe payload size,
        -- the data is split in several packets
        if (l2p_len_cnt > c_L2P_MAX_PAYLOAD) then
          l2p_data_cnt    <= c_L2P_MAX_PAYLOAD;
          -- when max payload length is 1024, the header length field = 0
          l2p_len_header  <= c_L2P_MAX_PAYLOAD(9 downto 0);
          l2p_last_packet <= '0';
        elsif (l2p_len_cnt = c_L2P_MAX_PAYLOAD) then
          l2p_data_cnt    <= c_L2P_MAX_PAYLOAD;
          -- when max payload length is 1024, the header length field = 0
          l2p_len_header  <= c_L2P_MAX_PAYLOAD(9 downto 0);
          l2p_last_packet <= '1';
        else
          l2p_data_cnt    <= l2p_len_cnt(10 downto 0);
          l2p_len_header  <= l2p_len_cnt(9 downto 0);
          l2p_last_packet <= '1';
        end if;
        -- if host address is 64-bit, generates a 64-bit address memory write
        if (l2p_address_h = 0) then
          l2p_64b_address <= '0';
        else
          l2p_64b_address <= '1';
        end if;
        -- Next packet (if any)
      elsif (l2p_dma_current_state = L2P_ADDR_L) then
        if (l2p_last_packet = '0' and data_fifo_empty = '0') then
          l2p_len_cnt <= l2p_len_cnt - c_L2P_MAX_PAYLOAD;
        elsif (l2p_last_packet = '1') then 
          l2p_len_cnt <= (others => '0');
        end if;
      elsif ((l2p_dma_current_state = L2P_DATA) and (data_fifo_valid = '1')) then
        l2p_data_cnt <= l2p_data_cnt - 1;
      elsif (l2p_last_packet = '0' and l2p_dma_current_state = L2P_LAST_DATA) then
        -- load the host address of the next packet
        l2p_address_l <= l2p_address_l + (c_L2P_MAX_PAYLOAD * 4);
        -- load the size of the next packet
        if (l2p_len_cnt > c_L2P_MAX_PAYLOAD) then
          l2p_data_cnt    <= c_L2P_MAX_PAYLOAD;
          -- when max payload length is 1024, the header length field = 0
          l2p_len_header  <= c_L2P_MAX_PAYLOAD(9 downto 0);
          l2p_last_packet <= '0';
        elsif (l2p_len_cnt = c_L2P_MAX_PAYLOAD) then
          l2p_data_cnt    <= c_L2P_MAX_PAYLOAD;
          -- when max payload length is 1024, the header length field = 0
          l2p_len_header  <= c_L2P_MAX_PAYLOAD(9 downto 0);
          l2p_last_packet <= '1';
        else
          l2p_data_cnt    <= l2p_len_cnt(10 downto 0);
          l2p_len_header  <= l2p_len_cnt(9 downto 0);
          l2p_last_packet <= '1';
        end if;
      end if;
    end if;
  end process p_pkt_gen;

  -- Last Byte Enable must be "0000" when length = 1
  l2p_lbe_header <= "0000" when l2p_len_header = 1 else "1111";

  -- Packet header
  s_l2p_header <= "000"                                -->  Traffic Class
                  & '0'                                -->  Snoop
                  & "001" & l2p_64b_address            -->  Header type,
                                                       --   memory write 32-bit or
                                                       --   memory write 64-bit
                  & l2p_lbe_header                     -->  LBE (Last Byte Enable)
                  & "1111"                             -->  FBE (First Byte Enable)
                  & "000"                              -->  Reserved
                  & '0'                                -->  VC (Virtual Channel)
                  & "00"                               -->  Reserved
                  & std_logic_vector(l2p_len_header);  -->  Length (in 32-bit words)
                                                       --   0x000 => 1024 words (4096 bytes)

  -----------------------------------------------------------------------------
  -- L2P packet write FSM
  -----------------------------------------------------------------------------
  process(clk_i, rst_n_i)
  begin
    if (rst_n_i = c_RST_ACTIVE) then
      l2p_dma_current_state <= L2P_IDLE;
      ldm_arb_req_o         <= '0';
      ldm_arb_data_o        <= (others => '0');
      ldm_arb_valid_o       <= '0';
      ldm_arb_dframe_o      <= '0';
      data_fifo_rd          <= '0';
      dma_ctrl_done_o       <= '0';
      l2p_edb_o             <= '0';
      data_fifo_rd_cnt <= (others => '0');
      l2p_timeout_cnt <= (others => '0');
      --data_fifo_valid_ex    <= '0';
    elsif rising_edge(clk_i) then
      case l2p_dma_current_state is

        when L2P_IDLE =>
          -- do nothing !
          data_fifo_rd       <= '0';
          dma_ctrl_done_o    <= '0';
          ldm_arb_data_o     <= (others => '0');
          ldm_arb_valid_o    <= '0';
          ldm_arb_dframe_o   <= '0';
          l2p_edb_o          <= '0';
          l2p_timeout_cnt <= (others => '0');
          --data_fifo_valid_ex <= '0';

          if (dma_ctrl_start_l2p_i = '1') then
            l2p_dma_current_state <= L2P_SETUP;
          end if;
          
        when L2P_SETUP =>
          if (l2p_rdy_i = '1') then
            -- We have data to send -> prepare a packet, first the header
            l2p_dma_current_state <= L2P_HEADER;
            -- request access to PCIe bus
            ldm_arb_req_o         <= '1';
          end if;

        when L2P_HEADER =>
          if(arb_ldm_gnt_i = '1' and l_wr_rdy_i = "11") then
            -- clear access request to the arbiter
            -- access is granted until dframe is cleared
            ldm_arb_req_o    <= '0';
            -- send header
            ldm_arb_data_o   <= s_l2p_header;
            ldm_arb_valid_o  <= '1';
            ldm_arb_dframe_o <= '1';
            data_fifo_rd_cnt <= l2p_data_cnt(10 downto 0);
            if(l2p_64b_address = '1') then
              -- if host address is 64-bit, we have to send an additionnal
              -- 32-word containing highest bits of the host address
              l2p_dma_current_state <= L2P_ADDR_H;
            else
              -- for 32-bit host address, we only have to send lowest bits
              l2p_dma_current_state <= L2P_ADDR_L;
              -- Starts reading data in the fifo now, because there is
              -- 1 cycle delay until data are available
              --data_fifo_rd          <= '1';
            end if;
          else
            -- arbiter or GN4124 not ready to receive a new packet
            ldm_arb_valid_o <= '0';
          end if;

        when L2P_ADDR_H =>
          ldm_arb_dframe_o <= '1';
          ldm_arb_valid_o  <= '1';
          -- send host address 32 highest bits
          ldm_arb_data_o        <= std_logic_vector(l2p_address_h);
          -- Now we still have to send lowest bits of the host address
          l2p_dma_current_state <= L2P_ADDR_L;
          -- Starts reading data in the fifo now, because there is
          -- 1 cycle delay until data are available
          --data_fifo_rd          <= '1';

        when L2P_ADDR_L =>
          ldm_arb_dframe_o <= '1';
          -- send host address 32 lowest bits
          --data_fifo_rd          <= '1';
          ldm_arb_data_o  <= std_logic_vector(l2p_address_l);
          if (data_fifo_empty = '0') then
            l2p_dma_current_state <= L2P_DATA;
            data_fifo_rd <= '1';
            ldm_arb_valid_o <= '1';
          else
            ldm_arb_valid_o <= '0';
            data_fifo_rd <= '0';
          end if;
          
          
          when L2P_DATA =>
             ldm_arb_dframe_o <= '1';
             if (data_fifo_rd = '1' and data_fifo_empty ='0') then
               data_fifo_rd_cnt <= data_fifo_rd_cnt -1;
             end if;
             
             l2p_edb_o <= '1';
             
             -- Pause until gennum ready
             if (l2p_rdy_i = '0' or (data_fifo_rd_cnt <= 1 and data_fifo_rd = '1' and data_fifo_empty ='0') or data_fifo_rd_cnt = 0) then
               data_fifo_rd <= '0';
             else
               data_fifo_rd <= '1';
             end if;
                          
             if (data_fifo_valid = '1') then
               l2p_timeout_cnt <= (others => '0');
               -- send data with byte swap if requested
               ldm_arb_data_o  <= f_byte_swap(g_BYTE_SWAP, data_fifo_dout, l2p_byte_swap);
               ldm_arb_valid_o <= '1';
               -- last data signaled w/o dframe
--               if (data_fifo_rd_cnt = 0) then
--                  ldm_arb_dframe_o <= '0';
--                  l2p_dma_current_state <= L2P_LAST_DATA;
--                  data_fifo_rd <= '0';
--               end if;
             else
               ldm_arb_valid_o <= '0';
               l2p_timeout_cnt <= l2p_timeout_cnt + 1;
             end if;
             
             if (data_fifo_rd_cnt = 0 or l2p_timeout_cnt > 2000) then
               l2p_dma_current_state <= L2P_LAST_DATA;
               ldm_arb_dframe_o <= '0';
               ldm_arb_valid_o <= '1';
               data_fifo_rd <= '0';
             end if;            
             
          when L2P_LAST_DATA =>
            data_fifo_rd <= '0';
            l2p_edb_o <= '0';
            ldm_arb_valid_o <= '0';
            ldm_arb_dframe_o <= '0';
            if (dma_ctrl_abort_i = '1' or tx_error_i = '1') then
               -- Abort transmission
               l2p_dma_current_state <= L2P_IDLE;
               dma_ctrl_done_o       <= '1';
            elsif(l2p_len_cnt > 0) then
              -- There is still data to be send -> start a new packet
              l2p_dma_current_state <= L2P_SETUP;
            else
              -- Nomore data to send, go back to sleep
              l2p_dma_current_state <= L2P_IDLE;
              -- Indicate that the DMA transfer is finished
              dma_ctrl_done_o       <= '1';
            end if;
            

--        when L2P_DATA =>
--          data_fifo_valid_ex <= '0';
--          if (data_fifo_valid = '1' or data_fifo_valid_ex = '1') then
--            -- send data with byte swap if requested
--            --ldm_arb_data_o  <= f_byte_swap(g_BYTE_SWAP, data_fifo_dout, l2p_byte_swap);
--            ldm_arb_data_o(10 downto 0)  <= std_logic_vector(l2p_data_cnt);
--            ldm_arb_data_o(31 downto 11) <= (others => '0');
--            ldm_arb_valid_o <= '1';
--          else
--            ldm_arb_valid_o <= '0';
--          end if;
--
--          if (dma_ctrl_abort_i = '1' or tx_error_i = '1') then
--            l2p_edb_o             <= '1';
--            l2p_dma_current_state <= L2P_IDLE;
--          elsif (l2p_rdy_i = '0') then
--            -- GN4124 not able to receive more data, have to wait
--            l2p_dma_current_state <= L2P_WAIT_RDY;
--            -- Stop reading from fifo
--            data_fifo_rd          <= '0';
--            -- Invalidate data
--            ldm_arb_valid_o       <= '0';
--          elsif(data_fifo_empty = '1') then
--            -- data not ready yet, wait for it
--            l2p_dma_current_state <= L2P_WAIT_DATA;
--          elsif(l2p_data_cnt <= 2) then
--            -- Only one 32-bit data word to send
--            l2p_dma_current_state <= L2P_LAST_DATA;
--            -- Stop reading from fifo
--            data_fifo_rd          <= '0';
--          end if;
--
--        when L2P_WAIT_RDY =>
--          ldm_arb_valid_o <= '0';
--          if (l2p_rdy_i = '1') then
--            -- GN4124 is ready to receive more data
--            -- Validate last data read before de-assertion of l2p_rdy
--            ldm_arb_valid_o  <= '1';
--            if (l2p_data_cnt <= 1) then
--              -- Last data word of the packet
--              l2p_dma_current_state <= L2P_LAST_DATA;
--            else
--              -- More data word to be sent
--              l2p_dma_current_state <= L2P_DATA;
--              -- Re-start fifo reading
--              data_fifo_rd          <= '1';
--              data_fifo_valid_ex    <= '1';
--            end if;
--          end if;
--
--        when L2P_WAIT_DATA =>
--          ldm_arb_valid_o <= '0';
--          if(data_fifo_empty = '0') then
--            if(l2p_data_cnt <= 1) then
--              -- Only one 32-bit data word to send
--              l2p_dma_current_state <= L2P_LAST_DATA;
--              -- Stop reading from fifo
--              data_fifo_rd          <= '0';
--            else
--              -- data ready to be send again
--              l2p_dma_current_state <= L2P_DATA;
--            end if;
--          end if;
--
--        when L2P_LAST_DATA =>
--          if (l2p_rdy_i = '0') then
--            -- GN4124 not able to receive more data, have to wait
--            -- Invalidate data
--            ldm_arb_valid_o <= '0';
--          else
--            -- send last data word with byte swap if requested
--            --ldm_arb_data_o   <= f_byte_swap(g_BYTE_SWAP, data_fifo_dout, l2p_byte_swap);
--            ldm_arb_data_o(10 downto 0)  <= std_logic_vector(l2p_data_cnt);
--            ldm_arb_data_o(31 downto 11) <= (others => '0');
--            ldm_arb_valid_o  <= '1';
--            -- clear dframe signal to indicate the end of packet
--            ldm_arb_dframe_o <= '0';
--            if(l2p_len_cnt > 0) then
--              -- There is still data to be send -> start a new packet
--              l2p_dma_current_state <= L2P_SETUP;
--            else
--              -- Nomore data to send, go back to sleep
--              l2p_dma_current_state <= L2P_IDLE;
--              -- Indicate that the DMA transfer is finished
--              dma_ctrl_done_o       <= '1';
--            end if;
--          end if;

        when others =>
          -- should no arrive here, but just in case...
          l2p_dma_current_state <= L2P_IDLE;
          ldm_arb_req_o         <= '0';
          ldm_arb_data_o        <= (others => '0');
          ldm_arb_valid_o       <= '0';
          ldm_arb_dframe_o      <= '0';
          data_fifo_rd          <= '0';
          dma_ctrl_done_o       <= '0';
          --data_fifo_valid_ex    <= '0';

      end case;
    end if;
  end process;

  ------------------------------------------------------------------------------
  -- FIFOs for transition between GN4124 core and wishbone clock domain
  ------------------------------------------------------------------------------
  cmp_addr_fifo : generic_async_fifo
    generic map (
      g_data_width             => 32,
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
      g_almost_full_threshold  => c_ADDR_FIFO_FULL_THRES)
    port map (
      rst_n_i           => fifo_rst_n,
      clk_wr_i          => clk_i,
      d_i               => addr_fifo_din,
      we_i              => addr_fifo_wr,
      wr_empty_o        => open,
      wr_full_o         => open,
      wr_almost_empty_o => open,
      wr_almost_full_o  => addr_fifo_full,
      wr_count_o        => open,
      clk_rd_i          => l2p_dma_clk_i,
      q_o               => addr_fifo_dout,
      rd_i              => addr_fifo_rd,
      rd_empty_o        => addr_fifo_empty,
      rd_full_o         => open,
      rd_almost_empty_o => open,
      rd_almost_full_o  => open,
      rd_count_o        => open);

  p_gen_addr_fifo_valid : process(l2p_dma_clk_i)
  begin
    if rising_edge(l2p_dma_clk_i) then
      addr_fifo_valid <= addr_fifo_rd and (not addr_fifo_empty);
    end if;
  end process;

  cmp_data_fifo : generic_async_fifo
    generic map (
      g_data_width             => 32,
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
      g_almost_full_threshold  => c_DATA_FIFO_FULL_THRES)
    port map (
      rst_n_i           => fifo_rst_n,
      clk_wr_i          => l2p_dma_clk_i,
      d_i               => data_fifo_din,
      we_i              => data_fifo_wr,
      wr_empty_o        => open,
      wr_full_o         => open,
      wr_almost_empty_o => open,
      wr_almost_full_o  => data_fifo_full,
      wr_count_o        => open,
      clk_rd_i          => clk_i,
      q_o               => data_fifo_dout,
      rd_i              => data_fifo_rd,
      rd_empty_o        => data_fifo_empty,
      rd_full_o         => open,
      rd_almost_empty_o => open,
      rd_almost_full_o  => open,
      rd_count_o        => open);

  p_gen_data_fifo_valid : process(clk_i)
  begin
    if rising_edge(clk_i) then
      data_fifo_valid <= data_fifo_rd and (not data_fifo_empty);
      data_fifo_valid_d <= data_fifo_empty;
    end if;
  end process;

  data_fifo_din <= l2p_dma_dat_i;
  -- latch data when receiving ack and the cycle has been initiated by this master
  data_fifo_wr  <= l2p_dma_ack_i and l2p_dma_cyc_t;

  ------------------------------------------------------------------------------
  -- Pipelined wishbone master
  ------------------------------------------------------------------------------
  -- Initatiates read transactions as long there is an address present
  -- in the address fifo. Then fills the data fifo with the read data.

  -- Wishbone master only make reads
  l2p_dma_we_o  <= '0';
  l2p_dma_dat_o <= (others => '0');

  -- Read address FIFO
  addr_fifo_rd <= not(addr_fifo_empty)
                  and not(l2p_dma_stall_i)
                  and not(data_fifo_full)
                  and not(p2l_dma_cyc_i);

  -- Wishbone master process
  p_wb_master : process (l2p_dma_clk_i, rst_n_i)
  begin
    if (rst_n_i = c_RST_ACTIVE) then
      l2p_dma_adr_o <= (others => '0');
      l2p_dma_stb_t <= '0';
      l2p_dma_cyc_t <= '0';
      l2p_dma_sel_o <= (others => '0');
    elsif rising_edge(l2p_dma_clk_i) then
      -- adr signal management
      if (addr_fifo_valid = '1') then
        l2p_dma_adr_o <= addr_fifo_dout;
      end if;
      -- stb and sel signals management
      if (addr_fifo_valid = '1') then   --or (l2p_dma_stall_i = '1' and l2p_dma_stb_t = '1') then
        l2p_dma_stb_t <= '1';
        l2p_dma_sel_o <= (others => '1');
      else
        l2p_dma_stb_t <= '0';
        l2p_dma_sel_o <= (others => '0');
      end if;
      -- cyc signal management
      if (addr_fifo_valid = '1') then
        l2p_dma_cyc_t <= '1';
      elsif (wb_left_cnt = 0 and dma_length_cnt = 0 and addr_fifo_empty = '1') then
        -- last ack received -> end of the transaction
        l2p_dma_cyc_t <= '0';
      elsif (wb_timeout_cnt >= 6000) then
        -- timeout, data not coming
        --l2p_dma_cyc_t <= '0';
      end if;
    end if;
  end process p_wb_master;

  -- for read back
  l2p_dma_cyc_o <= l2p_dma_cyc_t;
  l2p_dma_stb_o <= l2p_dma_stb_t;

  -- Wishbone read cycle counter
  p_wb_read_cnt : process (l2p_dma_clk_i, rst_n_i)
  begin
    if (rst_n_i = c_RST_ACTIVE) then
      wb_read_cnt <= (others => '0');
    elsif rising_edge(l2p_dma_clk_i) then
      if (addr_fifo_valid = '1') then
        wb_read_cnt <= wb_read_cnt + 1;
      end if;
    end if;
  end process p_wb_read_cnt;

  -- Wishbone ack counter
  p_wb_ack_cnt : process (l2p_dma_clk_i, rst_n_i)
  begin
    if (rst_n_i = c_RST_ACTIVE) then
      wb_ack_cnt <= (others => '0');
    elsif rising_edge(l2p_dma_clk_i) then
      if (l2p_dma_ack_i = '1' and l2p_dma_cyc_t = '1') then
        wb_ack_cnt <= wb_ack_cnt + 1;
      end if;
    end if;
  end process p_wb_ack_cnt;
  
  -- Wishbone timeout counter
  p_wb_timeout_cnt : process (l2p_dma_clk_i, rst_n_i)
  begin
    if (rst_n_i = c_RST_ACTIVE) then
      wb_timeout_cnt <= (others => '0');
    elsif rising_edge(l2p_dma_clk_i) then
      if (not(wb_ack_cnt = wb_read_cnt)) then
        wb_timeout_cnt <= wb_timeout_cnt + 1;
      else
        wb_timeout_cnt <= (others =>'0');
      end if;
    end if;
  end process p_wb_timeout_cnt;
  
  -- Wishbone left counter
  p_wb_left_cnt : process (l2p_dma_clk_i, rst_n_i)
  begin
    if (rst_n_i = c_RST_ACTIVE) then
      wb_left_cnt <= (others => '0');
    elsif rising_edge(l2p_dma_clk_i) then
      if (l2p_dma_ack_i = '0' and l2p_dma_stb_t = '1') then
        wb_left_cnt <= wb_left_cnt + 1;
      elsif (l2p_dma_ack_i = '1' and l2p_dma_stb_t = '0') then
        wb_left_cnt <= wb_left_cnt - 1;
      elsif (l2p_dma_cyc_t = '0') then
        wb_left_cnt <= (others => '0');
      end if;
    end if;
  end process p_wb_left_cnt;


end behaviour;

