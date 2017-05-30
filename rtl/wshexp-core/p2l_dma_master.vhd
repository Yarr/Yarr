--------------------------------------------------------------------------------
--                                                                            --
-- CERN BE-CO-HT         GN4124 core for PCIe FMC carrier                     --
--                       http://www.ohwr.org/projects/gn4124-core             --
--------------------------------------------------------------------------------
--
-- unit name: 32 bit P2L DMA master (p2l_dma_master.vhd)
--
-- authors: Simon Deprez (simon.deprez@cern.ch)
--          Matthieu Cattin (matthieu.cattin@cern.ch)
--
-- date: 31-08-2010
--
-- version: 0.1
--
-- description: Provides a pipelined Wishbone interface to performs DMA
--              transfers from PCI express host to local application.
--              This entity is also used to catch the next item in chained DMA.
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
--------------------------------------------------------------------------------
-- TODO: - byte enable support.
--------------------------------------------------------------------------------

library IEEE;
use IEEE.STD_LOGIC_1164.all;
use IEEE.NUMERIC_STD.all;
use work.wshexp_core_pkg.all;
use work.common_pkg.all;

entity p2l_dma_master is
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
      
      -- From PCIe IP core
      l2p_rid_i : in std_logic_vector(16-1 downto 0);

      ---------------------------------------------------------
      -- From the DMA controller
      dma_ctrl_carrier_addr_i : in  std_logic_vector(31 downto 0);
      dma_ctrl_host_addr_h_i  : in  std_logic_vector(31 downto 0);
      dma_ctrl_host_addr_l_i  : in  std_logic_vector(31 downto 0);
      dma_ctrl_len_i          : in  std_logic_vector(31 downto 0);
      dma_ctrl_start_p2l_i    : in  std_logic;
      dma_ctrl_start_next_i   : in  std_logic;
      dma_ctrl_done_o         : out std_logic;
      dma_ctrl_error_o        : out std_logic;
      dma_ctrl_byte_swap_i    : in  std_logic_vector(2 downto 0);
      dma_ctrl_abort_i        : in  std_logic;

      ---------------------------------------------------------
      -- From P2L Decoder (receive the read completion)
      --
      -- Header
      --pd_pdm_hdr_start_i   : in std_logic;                      -- Header strobe
      --pd_pdm_hdr_length_i  : in std_logic_vector(9 downto 0);   -- Packet length in 32-bit words multiples
      --pd_pdm_hdr_cid_i     : in std_logic_vector(1 downto 0);   -- Completion ID
      pd_pdm_master_cpld_i : in std_logic;                      -- Master read completion with data
      pd_pdm_master_cpln_i : in std_logic;                      -- Master read completion without data
      --
      -- Data
      pd_pdm_data_valid_i  : in std_logic;  
      pd_pdm_data_valid_w_i: in std_logic_vector(1 downto 0);  -- Indicates Data is valid
      pd_pdm_data_last_i   : in std_logic;                      -- Indicates end of the packet
      pd_pdm_data_i        : in std_logic_vector(63 downto 0);  -- Data
      pd_pdm_be_i          : in std_logic_vector(7 downto 0);   -- Byte Enable for data

      ---------------------------------------------------------
      -- P2L control
      p2l_rdy_o  : out std_logic;       -- De-asserted to pause transfer already in progress
      rx_error_o : out std_logic;       -- Asserted when transfer is aborted

      ---------------------------------------------------------
      -- To the P2L Interface (send the DMA Master Read request)
      pdm_arb_tvalid_o  : out std_logic;  -- Read completion signals
      pdm_arb_tlast_o : out std_logic;  -- Toward the arbiter
      pdm_arb_tdata_o   : out std_logic_vector(63 downto 0);
      pdm_arb_tkeep_o   : out std_logic_vector(7 downto 0);
      pdm_arb_req_o    : out std_logic;
      arb_pdm_gnt_i    : in  std_logic;

      ---------------------------------------------------------
      -- DMA Interface (Pipelined Wishbone)
      p2l_dma_clk_i   : in  std_logic;                      -- Bus clock
      p2l_dma_adr_o   : out std_logic_vector(31 downto 0);  -- Adress
      p2l_dma_dat_i   : in  std_logic_vector(63 downto 0);  -- Data in
      p2l_dma_dat_o   : out std_logic_vector(63 downto 0);  -- Data out
      p2l_dma_sel_o   : out std_logic_vector(7 downto 0);   -- Byte select
      p2l_dma_cyc_o   : out std_logic;                      -- Read or write cycle
      p2l_dma_stb_o   : out std_logic;                      -- Read or write strobe
      p2l_dma_we_o    : out std_logic;                      -- Write
      p2l_dma_ack_i   : in  std_logic;                      -- Acknowledge
      p2l_dma_stall_i : in  std_logic;                      -- for pipelined Wishbone
      l2p_dma_cyc_i   : in  std_logic;                      -- L2P dma wb cycle (for bus arbitration)

      ---------------------------------------------------------
      -- To the DMA controller
      next_item_carrier_addr_o : out std_logic_vector(31 downto 0);
      next_item_host_addr_h_o  : out std_logic_vector(31 downto 0);
      next_item_host_addr_l_o  : out std_logic_vector(31 downto 0);
      next_item_len_o          : out std_logic_vector(31 downto 0);
      next_item_next_l_o       : out std_logic_vector(31 downto 0);
      next_item_next_h_o       : out std_logic_vector(31 downto 0);
      next_item_attrib_o       : out std_logic_vector(31 downto 0);
      next_item_valid_o        : out std_logic
      );
end p2l_dma_master;


architecture behaviour of p2l_dma_master is
    
  -----------------------------------------------------------------------------
  -- Constants declaration
  -----------------------------------------------------------------------------

  -- c_MAX_READ_REQ_SIZE is the maximum size (in 32-bit words) of the payload of a packet.
  -- Allowed c_MAX_READ_REQ_SIZE values are: 32, 64, 128, 256, 512, 1024.
  -- This constant must be set according to the GN4124 and motherboard chipset capabilities.
  constant c_MAX_READ_REQ_SIZE     : unsigned(10 downto 0) := to_unsigned(1024, 11);
  constant c_TO_WB_FIFO_FULL_THRES : integer               := 500;

  -----------------------------------------------------------------------------
  -- Signals declaration
  -----------------------------------------------------------------------------

  -- control signals
  signal is_next_item     : std_logic;
  signal completion_error : std_logic;
  signal dma_busy_error   : std_logic;
  signal dma_length_error : std_logic;
  signal dma_ctrl_done_t  : std_logic;
  signal rx_error_t       : std_logic;

  -- L2P packet generator
  signal l2p_address_h   : std_logic_vector(31 downto 0);
  signal l2p_address_l   : std_logic_vector(31 downto 0);
  signal l2p_len_cnt     : unsigned(28 downto 0);
  signal l2p_len_header  : unsigned(9 downto 0);
  signal l2p_64b_address : std_logic;
  signal s_l2p_header    : std_logic_vector(63 downto 0);
  signal l2p_last_packet : std_logic;
  signal l2p_lbe_header  : std_logic_vector(3 downto 0);

  -- Target address counter
  signal target_addr_cnt : unsigned(28 downto 0);

  -- sync fifo
  signal fifo_rst_n : std_logic;

  signal to_wb_fifo_empty     : std_logic;
  signal to_wb_fifo_full      : std_logic;
  signal to_wb_fifo_rd        : std_logic;
  signal to_wb_fifo_wr        : std_logic;
  signal to_wb_fifo_din       : std_logic_vector(95 downto 0);
  signal to_wb_fifo_dout      : std_logic_vector(95 downto 0);
  signal to_wb_fifo_valid     : std_logic;
  signal to_wb_fifo_byte_swap : std_logic_vector(2 downto 0);

  -- wishbone
  signal wb_write_cnt  : unsigned(31 downto 0);
  signal wb_ack_cnt    : unsigned(31 downto 0);
  signal p2l_dma_cyc_t : std_logic;
  signal p2l_dma_stb_t : std_logic;
  signal p2l_dma_stall_d : std_logic_vector(1 downto 0);

  -- P2L DMA read request FSM
  type   p2l_dma_state_type is (P2L_IDLE, P2L_HEADER, P2L_HEADER_1, P2L_HEADER_2, P2L_WAIT_READ_COMPLETION);
  signal p2l_dma_current_state : p2l_dma_state_type;
  signal p2l_data_cnt          : unsigned(10 downto 0);
  --signal p2l_data_cnt_1          : unsigned(10 downto 0);


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

  -- Errors to DMA controller
  dma_ctrl_error_o <= dma_busy_error or completion_error;

  ------------------------------------------------------------------------------
  -- PCIe read request
  ------------------------------------------------------------------------------
  -- Stores infofmation for read request packet
  -- Can be a P2L DMA transfer or catching the next item of a chained DMA
  p_read_req : process (clk_i, rst_n_i)
  begin
    if (rst_n_i = c_RST_ACTIVE) then
      l2p_address_h   <= (others => '0');
      l2p_address_l   <= (others => '0');
      l2p_len_cnt     <= (others => '0');
      l2p_len_header  <= (others => '0');
      l2p_64b_address <= '0';
      is_next_item    <= '0';
      l2p_last_packet <= '0';
    elsif rising_edge(clk_i) then
      if (p2l_dma_current_state = P2L_IDLE) then
        if (dma_ctrl_start_p2l_i = '1' or dma_ctrl_start_next_i = '1') then
          -- Stores DMA info locally
          l2p_address_h <= dma_ctrl_host_addr_h_i;
          l2p_address_l <= dma_ctrl_host_addr_l_i;
          l2p_len_cnt   <= unsigned(dma_ctrl_len_i(30 downto 2));  -- dma_ctrl_len_i is in byte
          if (dma_ctrl_start_next_i = '1') then
            -- Catching next DMA item
            is_next_item <= '1';                                   -- flag for data retrieve block
          else
            -- P2L DMA transfer
            is_next_item <= '0';
          end if;
          if (dma_ctrl_host_addr_h_i = X"00000000") then
            l2p_64b_address <= '0';
          else
            l2p_64b_address <= '1';
          end if;
        end if;
      elsif (p2l_dma_current_state = P2L_HEADER) then
        -- if DMA length is bigger than the max PCIe payload size,
        -- we have to generate several read request
        if (l2p_len_cnt > c_MAX_READ_REQ_SIZE) then
          -- when max payload length is 1024, the header length field = 0
          l2p_len_header  <= c_MAX_READ_REQ_SIZE(9 downto 0);
          l2p_last_packet <= '0';
        elsif (l2p_len_cnt = c_MAX_READ_REQ_SIZE) then
          -- when max payload length is 1024, the header length field = 0
          l2p_len_header  <= c_MAX_READ_REQ_SIZE(9 downto 0);
          l2p_last_packet <= '1';
        else
          l2p_len_header  <= l2p_len_cnt(9 downto 0);
          l2p_last_packet <= '1';
        end if;
      elsif (p2l_dma_current_state = P2L_HEADER_2) then
        -- Subtract the number of word requested to generate a new read request if needed
        if (l2p_last_packet = '0') then
          l2p_len_cnt <= l2p_len_cnt - c_MAX_READ_REQ_SIZE;
        else
          l2p_len_cnt <= (others => '0');
        end if;
      end if;
    end if;
  end process p_read_req;

  -- Last Byte Enable must be "0000" when length = 1
  l2p_lbe_header <= "0000" when l2p_len_header = 1 else "1111";

  s_l2p_header(63 downto 48) <= l2p_rid_i; --X"0100"; --> H1 Requester ID
  s_l2p_header(47 downto 40) <= X"00"; --> H1 Tag
  s_l2p_header(39 downto 36) <= l2p_lbe_header; -->  LBE (Last Byte Enable)
  s_l2p_header(35 downto 32) <= X"f"; --> FBE (First Byte Enable)
  s_l2p_header(31 downto 29) <= "00" & l2p_64b_address; --> FMT without data (read request)
  s_l2p_header(28 downto 24) <= "00000"; --> type Memory request
  s_l2p_header(23 downto 16) <= X"00";   --> some unused bits
  s_l2p_header(15 downto 10) <= "000000"; --> unused bits 
  s_l2p_header(9 downto 0) <= std_logic_vector(l2p_len_header);  --> length H & length L
  
  -- s_l2p_header <= "000"                                -->  Traffic Class
                  -- & '0'                                -->  Snoop
                  -- & "000" & l2p_64b_address            -->  Packet type = read request (32 or 64 bits)
                  -- & l2p_lbe_header                     -->  LBE (Last Byte Enable)
                  -- & "1111"                             -->  FBE (First Byte Enable)
                  -- & "000"                              -->  Reserved
                  -- & '0'                                -->  VC (Virtual Channel)
                  -- & "01"                               -->  CID
                  -- & std_logic_vector(l2p_len_header);  -->  Length (in 32-bit words)
                                                         --0x000 => 1024 words (4096 bytes)
  -----------------------------------------------------------------------------
  -- PCIe read request FSM
  -----------------------------------------------------------------------------
  p_read_req_fsm : process (clk_i, rst_n_i)
  begin
    if(rst_n_i = c_RST_ACTIVE) then
      p2l_dma_current_state <= P2L_IDLE;
      pdm_arb_req_o         <= '0';
      pdm_arb_tdata_o        <= (others => '0');
      pdm_arb_tvalid_o       <= '0';
      pdm_arb_tlast_o       <= '0';
      pdm_arb_tkeep_o     <= X"FF";
      dma_ctrl_done_t       <= '0';
      next_item_valid_o     <= '0';
      completion_error      <= '0';
      rx_error_t            <= '0';
    elsif rising_edge(clk_i) then
      case p2l_dma_current_state is

        when P2L_IDLE =>
          -- Clear status bits
          dma_ctrl_done_t   <= '0';
          next_item_valid_o <= '0';
          completion_error  <= '0';
          rx_error_t        <= '0';
          -- Start a read request when a P2L DMA is initated or when the DMA
          -- controller asks for the next DMA info (in a chained DMA).
          if (dma_ctrl_start_p2l_i = '1' or dma_ctrl_start_next_i = '1') then
            -- request access to PCIe bus
            pdm_arb_req_o         <= '1';
            -- prepare a packet, first the header
            p2l_dma_current_state <= P2L_HEADER;
          end if;

        when P2L_HEADER =>
          if(arb_pdm_gnt_i = '1') then
            -- clear access request to the arbiter
            -- access is granted until dframe is cleared

            
            --if(l2p_64b_address = '1') then
              -- if host address is 64-bit, we have to send an additionnal
              -- 32-word containing highest bits of the host address
              --p2l_dma_current_state <= P2L_ADDR_H;
            --else
              -- for 32-bit host address, we only have to send lowest bits
              p2l_dma_current_state <= P2L_HEADER_1;
            --end if;
          end if;

        when P2L_HEADER_1 =>
          -- send host address 32 highest bits
          pdm_arb_req_o    <= '0';
          -- send header
          pdm_arb_tdata_o   <= s_l2p_header;
          pdm_arb_tvalid_o  <= '1';
          pdm_arb_tkeep_o <= X"FF";
          --pdm_arb_tdata_o        <= l2p_address_h;
          p2l_dma_current_state <= P2L_HEADER_2;

        when P2L_HEADER_2 =>
          -- send host address 32 lowest bits
          if(l2p_64b_address = '1') then
              pdm_arb_tdata_o        <= l2p_address_l & l2p_address_h;
              pdm_arb_tkeep_o <= X"FF";
          else
              pdm_arb_tdata_o        <= X"00000000" & l2p_address_l;
              pdm_arb_tkeep_o <= X"0F";
          end if;
          -- clear dframe signal to indicate the end of packet
          pdm_arb_tlast_o <= '1';
          p2l_dma_current_state <= P2L_WAIT_READ_COMPLETION;

        when P2L_WAIT_READ_COMPLETION =>
          -- End of the read request packet
          pdm_arb_tlast_o      <= '0';
		  pdm_arb_tvalid_o <= '0';
          if (dma_ctrl_abort_i = '1') then
            rx_error_t            <= '1';
            p2l_dma_current_state <= P2L_IDLE;
          elsif (pd_pdm_master_cpld_i = '1' and pd_pdm_data_last_i = '1'
                 and p2l_data_cnt <= 2) then
            -- last word of read completion has been received
            if (l2p_last_packet = '0') then
              -- A new read request is needed, DMA size > max payload
              p2l_dma_current_state <= P2L_HEADER;
              -- As the end of packet is used to delimit arbitration phases
              -- we have to ask again for permission
              pdm_arb_req_o         <= '1';
            else
              -- indicate end of DMA transfer
              if (is_next_item = '1') then
                next_item_valid_o <= '1';
              else
                dma_ctrl_done_t <= '1';
              end if;
              p2l_dma_current_state <= P2L_IDLE;
            end if;
          elsif (pd_pdm_master_cpln_i = '1') then
            -- should not return a read completion without data
            completion_error      <= '1';
            p2l_dma_current_state <= P2L_IDLE;
          end if;


        when others =>
          p2l_dma_current_state <= P2L_IDLE;
          pdm_arb_req_o         <= '0';
          pdm_arb_tdata_o        <= (others => '0');
          pdm_arb_tvalid_o       <= '0';
          pdm_arb_tlast_o      <= '0';
          dma_ctrl_done_t       <= '0';
          next_item_valid_o     <= '0';
          completion_error      <= '0';
          rx_error_t            <= '0';

      end case;
    end if;
  end process p_read_req_fsm;

  ------------------------------------------------------------------------------
  -- Pipeline control signals
  ------------------------------------------------------------------------------
  p_ctrl_pipe : process (clk_i, rst_n_i)
  begin
    if (rst_n_i = c_RST_ACTIVE) then
      rx_error_o      <= '0';
      dma_ctrl_done_o <= '0';
    elsif rising_edge(clk_i) then
      rx_error_o      <= rx_error_t;
      dma_ctrl_done_o <= dma_ctrl_done_t;
    end if;
  end process p_ctrl_pipe;

  ------------------------------------------------------------------------------
  -- Received data counter
  ------------------------------------------------------------------------------
  p_recv_data_cnt : process (clk_i, rst_n_i)
  begin
    if (rst_n_i = c_RST_ACTIVE) then
      p2l_data_cnt <= (others => '0');
    elsif rising_edge(clk_i) then
      if (p2l_dma_current_state = P2L_HEADER_2) then
        -- Store number of 32-bit data words to be received for the current read request
        if l2p_len_header = 0 then
          p2l_data_cnt <= to_unsigned(1024, p2l_data_cnt'length);
        else
          p2l_data_cnt <= "0" & l2p_len_header(9 downto 0);
        end if;
      elsif (p2l_dma_current_state = P2L_WAIT_READ_COMPLETION
             and pd_pdm_data_valid_w_i = "11"
             and pd_pdm_master_cpld_i = '1') then
        -- decrement number of data to be received
        p2l_data_cnt <= p2l_data_cnt - 2;
      elsif (p2l_dma_current_state = P2L_WAIT_READ_COMPLETION
             and (pd_pdm_data_valid_w_i = "01" or pd_pdm_data_valid_w_i = "10")
             and pd_pdm_master_cpld_i = '1') then
        p2l_data_cnt <= p2l_data_cnt - 1;
      end if;
    end if;
  end process p_recv_data_cnt;

  ------------------------------------------------------------------------------
  -- Next DMA item retrieve
  ------------------------------------------------------------------------------
  p_next_item : process (clk_i, rst_n_i)
  begin
    if (rst_n_i = c_RST_ACTIVE) then
      next_item_carrier_addr_o <= (others => '0');
      next_item_host_addr_h_o  <= (others => '0');
      next_item_host_addr_l_o  <= (others => '0');
      next_item_len_o          <= (others => '0');
      next_item_next_l_o       <= (others => '0');
      next_item_next_h_o       <= (others => '0');
      next_item_attrib_o       <= (others => '0');
    elsif rising_edge(clk_i) then
      --p2l_data_cnt_1 <= p2l_data_cnt;
      if (p2l_dma_current_state = P2L_WAIT_READ_COMPLETION
          and is_next_item = '1' and (pd_pdm_data_valid_w_i(0) = '1' or pd_pdm_data_valid_w_i(1) = '1')) then
        -- next item data are supposed to be received in the rigth order !!
        case p2l_data_cnt(3 downto 0) is
          when "0111" =>
            if pd_pdm_data_valid_w_i(1) = '1' then next_item_host_addr_l_o <= pd_pdm_data_i(63 downto 32); end if; -- 1
            if pd_pdm_data_valid_w_i(0) = '1' then next_item_carrier_addr_o <= pd_pdm_data_i(31 downto 0); end if; -- 0
          when "0110" =>
            if pd_pdm_data_valid_w_i(1) = '1' then next_item_host_addr_h_o <= pd_pdm_data_i(63 downto 32); end if; -- 2
            if pd_pdm_data_valid_w_i(0) = '1' then next_item_host_addr_l_o <= pd_pdm_data_i(31 downto 0); end if; -- 1
          when "0101" =>
            if pd_pdm_data_valid_w_i(1) = '1' then next_item_len_o <= pd_pdm_data_i(63 downto 32); end if; -- 3
            if pd_pdm_data_valid_w_i(0) = '1' then next_item_host_addr_h_o <= pd_pdm_data_i(31 downto 0); end if; -- 2
          when "0100" =>  
            if pd_pdm_data_valid_w_i(1) = '1' then next_item_next_l_o <= pd_pdm_data_i(63 downto 32); end if; -- 4
            if pd_pdm_data_valid_w_i(0) = '1' then next_item_len_o <= pd_pdm_data_i(31 downto 0); end if; -- 3
          when "0011" =>
            if pd_pdm_data_valid_w_i(1) = '1' then next_item_next_h_o <= pd_pdm_data_i(63 downto 32); end if; -- 5
            if pd_pdm_data_valid_w_i(0) = '1' then next_item_next_l_o <= pd_pdm_data_i(31 downto 0); end if; -- 4
          when "0010" =>
            if pd_pdm_data_valid_w_i(1) = '1' then next_item_attrib_o <= pd_pdm_data_i(63 downto 32); end if; -- 6
            if pd_pdm_data_valid_w_i(0) = '1' then next_item_next_h_o <= pd_pdm_data_i(31 downto 0); end if; -- 5
          when "0001" =>
            if pd_pdm_data_valid_w_i(0) = '1' then next_item_attrib_o <= pd_pdm_data_i(31 downto 0); end if; -- 6
          when others =>
            null;
        end case;
      end if;
    end if;
  end process p_next_item;

  ------------------------------------------------------------------------------
  -- Target address counter
  ------------------------------------------------------------------------------
  p_addr_cnt : process (clk_i, rst_n_i)
  begin
    if (rst_n_i = c_RST_ACTIVE) then
      target_addr_cnt      <= (others => '0');
      dma_busy_error       <= '0';
      to_wb_fifo_din       <= (others => '0');
      to_wb_fifo_wr        <= '0';
      to_wb_fifo_byte_swap <= (others => '0');
    elsif rising_edge(clk_i) then
      if (dma_ctrl_start_p2l_i = '1') then
        if (p2l_dma_current_state = P2L_IDLE) then
          -- dma_ctrl_target_addr_i is a byte address and target_addr_cnt is a
          -- 64-bit word address
          target_addr_cnt      <= unsigned(dma_ctrl_carrier_addr_i(31 downto 3));
          -- stores byte swap info for the current DMA transfer
          to_wb_fifo_byte_swap <= dma_ctrl_byte_swap_i;
        else
          dma_busy_error <= '1';
        end if;
      elsif (p2l_dma_current_state = P2L_WAIT_READ_COMPLETION
             and is_next_item = '0' and (pd_pdm_data_valid_w_i(0) = '1' or pd_pdm_data_valid_w_i(1) = '1')) then
        -- increment target address counter
        target_addr_cnt              <= target_addr_cnt + 1;
        -- write target address and data to the sync fifo
        to_wb_fifo_wr                <= '1';
        to_wb_fifo_din(63 downto 0)  <= pd_pdm_data_i; --f_byte_swap(g_BYTE_SWAP, pd_pdm_data_i, to_wb_fifo_byte_swap);
        to_wb_fifo_din(92 downto 64) <= std_logic_vector(target_addr_cnt);
      else
        dma_busy_error <= '0';
        to_wb_fifo_wr  <= '0';
      end if;
    end if;
  end process p_addr_cnt;

  ------------------------------------------------------------------------------
  -- FIFOs for transition between GN4124 core and wishbone clock domain
  ------------------------------------------------------------------------------
  cmp_to_wb_fifo : generic_async_fifo
    generic map (
      g_data_width             => 96,
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
      clk_rd_i          => p2l_dma_clk_i,
      q_o               => to_wb_fifo_dout,
      rd_i              => to_wb_fifo_rd,
      rd_empty_o        => to_wb_fifo_empty,
      rd_full_o         => open,
      rd_almost_empty_o => open,
      rd_almost_full_o  => open,
      rd_count_o        => open);

  --p_gen_fifo_valid : process(p2l_dma_clk_i)
  --begin
    --if rising_edge(p2l_dma_clk_i) then
      to_wb_fifo_valid <= to_wb_fifo_rd and (not to_wb_fifo_empty);
    --end if;
  --end process;

  -- pause transfer from GN4124 if fifo is (almost) full
  p2l_rdy_o <= not(to_wb_fifo_full);

  ------------------------------------------------------------------------------
  -- Wishbone master (write only)
  ------------------------------------------------------------------------------

  -- fifo read
  to_wb_fifo_rd <= not(to_wb_fifo_empty)
                   and not(p2l_dma_stall_i)
                   and not(l2p_dma_cyc_i);

  -- write only
  p2l_dma_we_o <= '1';

  -- Wishbone master process
  p_wb_master : process (rst_n_i, p2l_dma_clk_i)
  begin
    if (rst_n_i = c_RST_ACTIVE) then
      p2l_dma_cyc_t <= '0';
      p2l_dma_stb_t <= '0';
      p2l_dma_sel_o <= (others => '0');
      p2l_dma_adr_o <= (others => '0');
      p2l_dma_dat_o <= (others => '0');
      p2l_dma_stall_d <= (others => '0');
    elsif rising_edge(p2l_dma_clk_i) then
	  p2l_dma_stall_d(0) <= p2l_dma_stall_i;
      p2l_dma_stall_d(1) <= p2l_dma_stall_d(0);
      -- data and address
      if (to_wb_fifo_valid = '1') then
        --p2l_dma_adr_o(31 downto 30) <= "00";
		p2l_dma_adr_o(31 downto 0) <= to_wb_fifo_dout(95 downto 64);
        p2l_dma_dat_o <= to_wb_fifo_dout(63 downto 0);
      end if;
      -- stb and sel signals management
      if (to_wb_fifo_valid = '1') then  --or (p2l_dma_stall_i = '1' and p2l_dma_stb_t = '1') then
        p2l_dma_stb_t <= '1';
        p2l_dma_sel_o <= (others => '1');
      else
        p2l_dma_stb_t <= '0';
        p2l_dma_sel_o <= (others => '0');
      end if;
      -- cyc signal management
      if (to_wb_fifo_valid = '1') then
        p2l_dma_cyc_t <= '1';
      elsif (wb_ack_cnt >= wb_write_cnt and p2l_dma_stall_d(1) = '0') then
        -- last ack received -> end of the transaction
        p2l_dma_cyc_t <= '0';
      end if;
    end if;
  end process p_wb_master;

  -- for read back
  p2l_dma_cyc_o <= p2l_dma_cyc_t;
  p2l_dma_stb_o <= p2l_dma_stb_t;

  -- Wishbone write cycle counter
  p_wb_write_cnt : process (p2l_dma_clk_i, rst_n_i)
  begin
    if (rst_n_i = c_RST_ACTIVE) then
      wb_write_cnt <= (others => '0');
    elsif rising_edge(p2l_dma_clk_i) then
      if (to_wb_fifo_valid = '1') then
        wb_write_cnt <= wb_write_cnt + 1;
      end if;
    end if;
  end process p_wb_write_cnt;

  -- Wishbone ack counter
  p_wb_ack_cnt : process (p2l_dma_clk_i, rst_n_i)
  begin
    if (rst_n_i = c_RST_ACTIVE) then
      wb_ack_cnt <= (others => '0');
    elsif rising_edge(p2l_dma_clk_i) then
      if (p2l_dma_ack_i = '1' and p2l_dma_cyc_t = '1') then
        wb_ack_cnt <= wb_ack_cnt + 1;
      end if;
    end if;
  end process p_wb_ack_cnt;
      

end behaviour;
