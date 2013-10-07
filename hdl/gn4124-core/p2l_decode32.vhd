--------------------------------------------------------------------------------
--                                                                            --
-- CERN BE-CO-HT         GN4124 core for PCIe FMC carrier                     --
--                       http://www.ohwr.org/projects/gn4124-core             --
--------------------------------------------------------------------------------
--
-- unit name: P2L 32-bit datapath decoder (p2l_decode32.vhd)
--
-- authors: Simon Deprez (simon.deprez@cern.ch)
--          Matthieu Cattin (matthieu.cattin@cern.ch)
--
-- date: 31-08-2010
--
-- version: 1.0
--
-- description: PCIe to local bus packet decoder - For 32-bit data path design.
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
-- last changes: 27-09-2010 (mcattin) Rewrite a part of the decoder logic
--------------------------------------------------------------------------------

library IEEE;
use IEEE.STD_LOGIC_1164.all;
use IEEE.NUMERIC_STD.all;
use work.gn4124_core_pkg.all;


entity p2l_decode32 is
  port
    (
      ---------------------------------------------------------
      -- GN4124 core clock and reset
      clk_i   : in std_logic;
      rst_n_i : in std_logic;

      ---------------------------------------------------------
      -- Input from the deserializer
      des_p2l_valid_i  : in std_logic;
      des_p2l_dframe_i : in std_logic;
      des_p2l_data_i   : in std_logic_vector(31 downto 0);

      ---------------------------------------------------------
      -- Decoder outputs
      --
      -- Header
      p2l_hdr_start_o   : out std_logic;                      -- Header strobe
      p2l_hdr_length_o  : out std_logic_vector(9 downto 0);   -- Packet length in 32-bit words multiples
      p2l_hdr_cid_o     : out std_logic_vector(1 downto 0);   -- Completion ID
      p2l_hdr_last_o    : out std_logic;                      -- Indicates Last packet in a completion
      p2l_hdr_stat_o    : out std_logic_vector(1 downto 0);   -- Completion Status
                                                              -- "00" = Successful completion
                                                              -- "01" = Unsupported request
                                                              -- "10" = Completer abort
                                                              -- "11" = Completion time-out
      -- Packet type (for routing)
      p2l_target_mrd_o  : out std_logic;                      -- Target memory read (to wbmaster32)
      p2l_target_mwr_o  : out std_logic;                      -- Target memory write (to wbmaster32)
      p2l_master_cpld_o : out std_logic;                      -- Master completion with data (to p2l_dma_master)
      p2l_master_cpln_o : out std_logic;                      -- Master completion without data (to p2l_dma_master)
      -- Address
      p2l_addr_start_o  : out std_logic;                      -- Address strobe
      p2l_addr_o        : out std_logic_vector(31 downto 0);  -- Target address (in byte) that will increment with data
                                                              -- increment = 4 bytes
      -- Data
      p2l_d_valid_o     : out std_logic;                      -- Indicates Data is valid
      p2l_d_last_o      : out std_logic;                      -- Indicates end of the packet
      p2l_d_o           : out std_logic_vector(31 downto 0);  -- Data
      p2l_be_o          : out std_logic_vector(3 downto 0)    -- Byte Enable for data
      );
end p2l_decode32;


architecture rtl of p2l_decode32 is


  -----------------------------------------------------------------------------
  -- to_mvl Function
  -----------------------------------------------------------------------------
  function f_to_mvl (b : in boolean) return std_logic is
  begin
    if (b = true) then
      return('1');
    else
      return('0');
    end if;
  end f_to_mvl;

  -----------------------------------------------------------------------------
  -- Signals declaration
  -----------------------------------------------------------------------------
  signal des_p2l_valid_d  : std_logic;
  signal des_p2l_dframe_d : std_logic;

  signal p2l_packet_start   : std_logic;
  signal p2l_packet_start_d : std_logic;
  signal p2l_packet_end     : std_logic;

  signal p2l_addr_cycle : std_logic;
  signal p2l_data_cycle : std_logic;

  signal p2l_hdr_strobe : std_logic;                     -- Indicates Header start cycle
  signal p2l_hdr_length : std_logic_vector(9 downto 0);  -- Latched LENGTH value from header
  signal p2l_hdr_cid    : std_logic_vector(1 downto 0);  -- Completion ID
  signal p2l_hdr_last   : std_logic;                     -- Indicates Last packet in a completion
  signal p2l_hdr_stat   : std_logic_vector(1 downto 0);  -- Completion Status

  signal p2l_addr_start : std_logic;
  signal p2l_addr       : unsigned(31 downto 0);  -- Registered and counting Address

  signal p2l_d_valid : std_logic;                      -- Indicates Address/Data is valid
  signal p2l_d_first : std_logic;
  signal p2l_d_last  : std_logic;                      -- Indicates end of the packet
  signal p2l_d       : std_logic_vector(31 downto 0);  -- Address/Data
  signal p2l_be      : std_logic_vector(3 downto 0);   -- Byte Enable for data

  signal p2l_hdr_fbe : std_logic_vector(3 downto 0);  -- First Byte Enable
  signal p2l_hdr_lbe : std_logic_vector(3 downto 0);  -- Last Byte Enable

  signal target_mrd  : std_logic;
  signal target_mwr  : std_logic;
  signal master_cpld : std_logic;
  signal master_cpln : std_logic;


begin


  -----------------------------------------------------------------------------
  -- 1 tick delay version of des_p2l_valid_i and des_p2l_dframe_i,
  -- for start and end frame detection
  -----------------------------------------------------------------------------
  process (clk_i, rst_n_i)
  begin
    if rst_n_i = c_RST_ACTIVE then
      des_p2l_dframe_d <= '0';
      des_p2l_valid_d  <= '0';
    elsif rising_edge(clk_i) then
      des_p2l_dframe_d <= des_p2l_dframe_i;
      des_p2l_valid_d  <= des_p2l_valid_i;
    end if;
  end process;

  ------------------------------------------------------------------------------
  -- Start and end packet detection
  ------------------------------------------------------------------------------
  p2l_packet_start <= des_p2l_dframe_i and not(des_p2l_dframe_d) and des_p2l_valid_i;
  p2l_packet_end   <= des_p2l_valid_d and not(des_p2l_dframe_d);

  -----------------------------------------------------------------------------
  -- Decode packet type
  -----------------------------------------------------------------------------
  p_type_decode : process (clk_i, rst_n_i)
  begin
    if rst_n_i = c_RST_ACTIVE then
      target_mrd  <= '0';
      target_mwr  <= '0';
      master_cpld <= '0';
      master_cpln <= '0';
    elsif rising_edge(clk_i) then
      -- New packet starts, check type for routing
      if (p2l_packet_start = '1') then
        -- Target read request
        target_mrd  <= f_to_mvl(des_p2l_data_i(27 downto 24) = "0000");
        -- Target write
        target_mwr  <= f_to_mvl(des_p2l_data_i(27 downto 24) = "0010");
        -- Master read completion with data
        master_cpld <= f_to_mvl(des_p2l_data_i(27 downto 24) = "0101");
        -- Master read completion without data
        master_cpln <= f_to_mvl(des_p2l_data_i(27 downto 24) = "0100");
      elsif (p2l_packet_end = '1') then
        target_mrd  <= '0';
        target_mwr  <= '0';
        master_cpld <= '0';
        master_cpln <= '0';
      end if;
    end if;
  end process p_type_decode;

  -----------------------------------------------------------------------------
  -- Packet header decoding
  -----------------------------------------------------------------------------
  p_header_decode : process (clk_i, rst_n_i)
  begin
    if rst_n_i = c_RST_ACTIVE then
      p2l_hdr_strobe <= '0';
      p2l_hdr_length <= (others => '0');
      p2l_hdr_cid    <= (others => '0');
      p2l_hdr_last   <= '0';
      p2l_hdr_stat   <= (others => '0');
      p2l_hdr_fbe    <= (others => '0');
      p2l_hdr_lbe    <= (others => '0');
    elsif rising_edge(clk_i) then
      if (p2l_packet_start = '1') then
        p2l_hdr_strobe <= '1';
        p2l_hdr_length <= des_p2l_data_i(9 downto 0);
        p2l_hdr_cid    <= des_p2l_data_i(11 downto 10);
        p2l_hdr_last   <= des_p2l_data_i(15);
        if (des_p2l_data_i(26) = '1') then
          -- packet type = read completion
          p2l_hdr_stat <= des_p2l_data_i(17 downto 16);  -- Completion status
        else
          -- Target read or write
          p2l_hdr_fbe <= des_p2l_data_i(19 downto 16);   -- First Byte Enable
          p2l_hdr_lbe <= des_p2l_data_i(23 downto 20);   -- Last Byte Enable
        end if;
      else
        p2l_hdr_strobe <= '0';
      end if;
    end if;
  end process p_header_decode;

  -----------------------------------------------------------------------------
  -- Packet address decoding
  -----------------------------------------------------------------------------
  p_addr_decode : process (clk_i, rst_n_i)
  begin
    if rst_n_i = c_RST_ACTIVE then
      p2l_addr_cycle <= '0';
      p2l_addr       <= (others => '0');
      p2l_addr_start <= '0';
    elsif rising_edge(clk_i) then

      -- Indicate address cycle(s)
      -- Address cycle comes just after the header.
      -- Read completion packet doesn't have an address field, then addr_cycle is not asserted.
      if (p2l_packet_start = '1' and des_p2l_data_i(26) = '0') then
        p2l_addr_cycle <= '1';
      elsif (p2l_addr_cycle = '1' and des_p2l_valid_i = '1') then
        p2l_addr_cycle <= '0';
      end if;

      -- Generates address strobe
      -- No address strobe for read completion packets
      if ((target_mwr or target_mrd) = '1') then
        p2l_addr_start <= p2l_addr_cycle and des_p2l_valid_i;
      else
        p2l_addr_start <= '0';
      end if;

      -- Put address on a dedicated bus
      -- Bits 1-0 are coding the BAR for target read/write
      -- "00" = BAR 0
      -- "01" = BAR 2
      -- "10" = Expansion ROM
      -- "11" = Reserved
      if (p2l_addr_cycle = '1' and des_p2l_valid_i = '1' and (target_mwr or target_mrd) = '1') then
        -- Latch target address
        p2l_addr <= unsigned(des_p2l_data_i);
      elsif (p2l_d_valid = '1' and (target_mwr or target_mrd) = '1') then
        -- Increment address with data (32-bit data word => increment = +4 bytes)
        p2l_addr(31 downto 2) <= p2l_addr(31 downto 2) + 1;
      end if;

    end if;
  end process p_addr_decode;

  -----------------------------------------------------------------------------
  -- Packet data decoding (data strobe)
  -----------------------------------------------------------------------------
  p_data_decode : process (clk_i, rst_n_i)
  begin
    if rst_n_i = c_RST_ACTIVE then
      p2l_data_cycle <= '0';
      p2l_d_valid    <= '0';
      p2l_d_last     <= '0';
      p2l_d          <= (others => '0');
    elsif rising_edge(clk_i) then

      -- Indicates data cycle(s)
      -- Data cycle comes after an address cycle, exept for read completion packet
      -- in this case it comes just after the header.
      if ((p2l_addr_cycle = '1' or (p2l_packet_start = '1' and des_p2l_data_i(26) = '1'))
           and des_p2l_valid_i = '1' and des_p2l_dframe_i = '1') then
        p2l_data_cycle <= '1';
      elsif (des_p2l_dframe_i = '0') then
        p2l_data_cycle <= '0';
      end if;

      -- Generates data strobe
      -- For read completion, data are valid just after the header (no address)
      --if (master_cpld = '1') then
      --  p2l_d_valid <= des_p2l_valid_i;
      --else
        p2l_d_valid <= p2l_data_cycle and des_p2l_valid_i;
      --end if;

      -- Generates last data signal
      p2l_d_last <= p2l_data_cycle and not(des_p2l_dframe_i);

      -- Latch data on the bus
      if(des_p2l_valid_i = '1') then
        p2l_d <= des_p2l_data_i;
      end if;

    end if;
  end process p_data_decode;

  -----------------------------------------------------------------------------
  -- Byte enable
  -----------------------------------------------------------------------------
  p_be_decode : process (clk_i, rst_n_i)
  begin
    if rst_n_i = c_RST_ACTIVE then
      p2l_be <= (others => '0');
    elsif rising_edge(clk_i) then
      if (p2l_addr_start = '1') then
        p2l_be <= p2l_hdr_fbe;          -- First Byte Enable
      elsif ((p2l_data_cycle and not(des_p2l_dframe_i)) = '1') then
        p2l_be <= p2l_hdr_lbe;          -- Last Byte Enable
      elsif(p2l_data_cycle = '1') then
        p2l_be <= (others => '1');      -- Intermediate Byte Enables
      end if;
    end if;
  end process p_be_decode;

  -----------------------------------------------------------------------------
  -- Assigns signals to output ports
  -----------------------------------------------------------------------------
  p2l_hdr_start_o  <= p2l_hdr_strobe;
  p2l_hdr_length_o <= p2l_hdr_length;
  p2l_hdr_cid_o    <= p2l_hdr_cid;
  p2l_hdr_last_o   <= p2l_hdr_last;
  p2l_hdr_stat_o   <= p2l_hdr_stat;

  p2l_addr_start_o <= p2l_addr_start;
  p2l_addr_o       <= std_logic_vector(p2l_addr);
  p2l_d_valid_o    <= p2l_d_valid;
  p2l_d_last_o     <= p2l_d_last;
  p2l_d_o          <= p2l_d;
  p2l_be_o         <= p2l_be;

  p2l_target_mrd_o  <= target_mrd;
  p2l_target_mwr_o  <= target_mwr;
  p2l_master_cpld_o <= master_cpld;
  p2l_master_cpln_o <= master_cpln;


end rtl;
