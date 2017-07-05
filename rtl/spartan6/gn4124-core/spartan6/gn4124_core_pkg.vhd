--==============================================================================
--! @file gn4124_core_pkg_s6.vhd
--==============================================================================

--! Standard library
library IEEE;
--! Standard packages
use IEEE.STD_LOGIC_1164.all;
use IEEE.NUMERIC_STD.all;

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
-- Package for gn4124 core
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--! @brief
--! Package for components declaration and core wide constants.
--! Spartan6 FPGAs version.
--------------------------------------------------------------------------------
--! @version
--! 0.1 | mc | 01.09.2010 | File creation and Doxygen comments
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
--! Package declaration
--==============================================================================
package gn4124_core_pkg is


--==============================================================================
--! Constants declaration
--==============================================================================
  constant c_RST_ACTIVE : std_logic := '0';  -- Active low reset


--==============================================================================
--! Functions declaration
--==============================================================================
  function f_byte_swap (
    constant enable    : boolean;
    signal   din       : std_logic_vector(31 downto 0);
    signal   byte_swap : std_logic_vector(1 downto 0))
    return std_logic_vector;

  function log2_ceil(N : natural) return positive;

--==============================================================================
--! Components declaration
--==============================================================================

-----------------------------------------------------------------------------
  component gn4124_core
    generic (
      g_ACK_TIMEOUT : positive := 100                    -- Wishbone ACK timeout (in wishbone clock cycles)
      );
    port
      (
        ---------------------------------------------------------
        -- Control and status
        rst_n_a_i : in  std_logic;                      -- Asynchronous reset from GN4124
        status_o  : out std_logic_vector(31 downto 0);  -- Core status output

        ---------------------------------------------------------
        -- P2L Direction
        --
        -- Source Sync DDR related signals
        p2l_clk_p_i  : in  std_logic;                      -- Receiver Source Synchronous Clock+
        p2l_clk_n_i  : in  std_logic;                      -- Receiver Source Synchronous Clock-
        p2l_data_i   : in  std_logic_vector(15 downto 0);  -- Parallel receive data
        p2l_dframe_i : in  std_logic;                      -- Receive Frame
        p2l_valid_i  : in  std_logic;                      -- Receive Data Valid
        -- P2L Control
        p2l_rdy_o    : out std_logic;                      -- Rx Buffer Full Flag
        p_wr_req_i   : in  std_logic_vector(1 downto 0);   -- PCIe Write Request
        p_wr_rdy_o   : out std_logic_vector(1 downto 0);   -- PCIe Write Ready
        rx_error_o   : out std_logic;                      -- Receive Error
        vc_rdy_i     : in  std_logic_vector(1 downto 0);   -- Virtual channel ready

        ---------------------------------------------------------
        -- L2P Direction
        --
        -- Source Sync DDR related signals
        l2p_clk_p_o  : out std_logic;                      -- Transmitter Source Synchronous Clock+
        l2p_clk_n_o  : out std_logic;                      -- Transmitter Source Synchronous Clock-
        l2p_data_o   : out std_logic_vector(15 downto 0);  -- Parallel transmit data
        l2p_dframe_o : out std_logic;                      -- Transmit Data Frame
        l2p_valid_o  : out std_logic;                      -- Transmit Data Valid
        -- L2P Control
        l2p_edb_o    : out std_logic;                      -- Packet termination and discard
        l2p_rdy_i    : in  std_logic;                      -- Tx Buffer Full Flag
        l_wr_rdy_i   : in  std_logic_vector(1 downto 0);   -- Local-to-PCIe Write
        p_rd_d_rdy_i : in  std_logic_vector(1 downto 0);   -- PCIe-to-Local Read Response Data Ready
        tx_error_i   : in  std_logic;                      -- Transmit Error

        ---------------------------------------------------------
        -- Interrupt interface
        dma_irq_o : out std_logic_vector(1 downto 0);  -- Interrupts sources to IRQ manager
        irq_p_i   : in  std_logic;                     -- Interrupt request pulse from IRQ manager
        irq_p_o   : out std_logic;                     -- Interrupt request pulse to GN4124 GPIO

        ---------------------------------------------------------
        -- DMA registers wishbone interface (slave classic)
        dma_reg_clk_i   : in  std_logic;
        dma_reg_adr_i   : in  std_logic_vector(31 downto 0);
        dma_reg_dat_i   : in  std_logic_vector(31 downto 0);
        dma_reg_sel_i   : in  std_logic_vector(3 downto 0);
        dma_reg_stb_i   : in  std_logic;
        dma_reg_we_i    : in  std_logic;
        dma_reg_cyc_i   : in  std_logic;
        dma_reg_dat_o   : out std_logic_vector(31 downto 0);
        dma_reg_ack_o   : out std_logic;
        dma_reg_stall_o : out std_logic;

        ---------------------------------------------------------
        -- CSR wishbone interface (master pipelined)
        csr_clk_i   : in  std_logic;
        csr_adr_o   : out std_logic_vector(31 downto 0);
        csr_dat_o   : out std_logic_vector(31 downto 0);
        csr_sel_o   : out std_logic_vector(3 downto 0);
        csr_stb_o   : out std_logic;
        csr_we_o    : out std_logic;
        csr_cyc_o   : out std_logic;
        csr_dat_i   : in  std_logic_vector(31 downto 0);
        csr_ack_i   : in  std_logic;
        csr_stall_i : in  std_logic;
        csr_err_i   : in  std_logic;
        csr_rty_i   : in  std_logic;
        csr_int_i   : in  std_logic;

        ---------------------------------------------------------
        -- DMA wishbone interface (master pipelined)
        dma_clk_i   : in  std_logic;
        dma_adr_o   : out std_logic_vector(31 downto 0);
        dma_dat_o   : out std_logic_vector(31 downto 0);
        dma_sel_o   : out std_logic_vector(3 downto 0);
        dma_stb_o   : out std_logic;
        dma_we_o    : out std_logic;
        dma_cyc_o   : out std_logic;
        dma_dat_i   : in  std_logic_vector(31 downto 0);
        dma_ack_i   : in  std_logic;
        dma_stall_i : in  std_logic;
        dma_err_i   : in  std_logic;
        dma_rty_i   : in  std_logic;
        dma_int_i   : in  std_logic
        );
  end component gn4124_core;

-----------------------------------------------------------------------------
  component p2l_des
    port
      (
        ---------------------------------------------------------
        -- Reset and clock
        rst_n_i         : in std_logic;
        sys_clk_i       : in std_logic;
        io_clk_i        : in std_logic;
        serdes_strobe_i : in std_logic;

        ---------------------------------------------------------
        -- P2L DDR inputs
        p2l_valid_i  : in std_logic;
        p2l_dframe_i : in std_logic;
        p2l_data_i   : in std_logic_vector(15 downto 0);

        ---------------------------------------------------------
        -- P2L SDR outputs
        p2l_valid_o  : out std_logic;
        p2l_dframe_o : out std_logic;
        p2l_data_o   : out std_logic_vector(31 downto 0)
        );
  end component;  -- p2l_des

-----------------------------------------------------------------------------
  component p2l_decode32
    port
      (
        ---------------------------------------------------------
        -- Clock/Reset
        clk_i   : in std_logic;
        rst_n_i : in std_logic;

        ---------------------------------------------------------
        -- Input from the Deserializer
        des_p2l_valid_i  : in std_logic;
        des_p2l_dframe_i : in std_logic;
        des_p2l_data_i   : in std_logic_vector(31 downto 0);

        ---------------------------------------------------------
        -- Decoder Outputs
        --
        -- Header
        p2l_hdr_start_o   : out std_logic;                      -- Indicates Header start cycle
        p2l_hdr_length_o  : out std_logic_vector(9 downto 0);   -- Latched LENGTH value from header
        p2l_hdr_cid_o     : out std_logic_vector(1 downto 0);   -- Completion ID
        p2l_hdr_last_o    : out std_logic;                      -- Indicates Last packet in a completion
        p2l_hdr_stat_o    : out std_logic_vector(1 downto 0);   -- Completion Status
        p2l_target_mrd_o  : out std_logic;                      -- Target memory read
        p2l_target_mwr_o  : out std_logic;                      -- Target memory write
        p2l_master_cpld_o : out std_logic;                      -- Master completion with data
        p2l_master_cpln_o : out std_logic;                      -- Master completion without data
        --
        -- Address
        p2l_addr_start_o  : out std_logic;                      -- Indicates Address Start
        p2l_addr_o        : out std_logic_vector(31 downto 0);  -- Latched Address that will increment with data
        --
        -- Data
        p2l_d_valid_o     : out std_logic;                      -- Indicates Data is valid
        p2l_d_last_o      : out std_logic;                      -- Indicates end of the packet
        p2l_d_o           : out std_logic_vector(31 downto 0);  -- Data
        p2l_be_o          : out std_logic_vector(3 downto 0)    -- Byte Enable for data
        );
  end component;  -- p2l_decode32


-----------------------------------------------------------------------------
  component l2p_ser
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
  end component;  -- l2p_ser


-----------------------------------------------------------------------------
  component wbmaster32
    generic (
      g_ACK_TIMEOUT : positive := 100    -- Wishbone ACK timeout (in wb_clk cycles)
      );
    port
      (
        ---------------------------------------------------------
        -- Clock/Reset
        clk_i   : in std_logic;
        rst_n_i : in std_logic;

        ---------------------------------------------------------
        -- From P2L Decoder
        --
        -- Header
        pd_wbm_hdr_start_i  : in std_logic;                      -- Indicates Header start cycle
        pd_wbm_hdr_length_i : in std_logic_vector(9 downto 0);   -- Latched LENGTH value from header
        pd_wbm_hdr_cid_i    : in std_logic_vector(1 downto 0);   -- Completion ID
        pd_wbm_target_mrd_i : in std_logic;                      -- Target memory read
        pd_wbm_target_mwr_i : in std_logic;                      -- Target memory write
        --
        -- Address
        pd_wbm_addr_start_i : in std_logic;                      -- Indicates Address Start
        pd_wbm_addr_i       : in std_logic_vector(31 downto 0);  -- Latched Address that will increment with data
        --
        -- Data
        pd_wbm_data_valid_i : in std_logic;                      -- Indicates Data is valid
        pd_wbm_data_last_i  : in std_logic;                      -- Indicates end of the packet
        pd_wbm_data_i       : in std_logic_vector(31 downto 0);  -- Data
        pd_wbm_be_i         : in std_logic_vector(3 downto 0);   -- Byte Enable for data

        ---------------------------------------------------------
        -- P2L Control
        p_wr_rdy_o   : out std_logic_vector(1 downto 0);  -- Ready to accept target write
        p2l_rdy_o    : out std_logic;                     -- De-asserted to pause transfer already in progress
        p_rd_d_rdy_i : in  std_logic_vector(1 downto 0);  -- Asserted when GN4124 ready to accept read completion with data

        ---------------------------------------------------------
        -- To the L2P Interface
        wbm_arb_valid_o  : out std_logic;  -- Read completion signals
        wbm_arb_dframe_o : out std_logic;  -- Toward the arbiter
        wbm_arb_data_o   : out std_logic_vector(31 downto 0);
        wbm_arb_req_o    : out std_logic;
        arb_wbm_gnt_i    : in  std_logic;

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
  end component;  -- wbmaster32

-----------------------------------------------------------------------------
  component dma_controller
    port
      (
        ---------------------------------------------------------
        -- GN4124 core clock and reset
        clk_i   : in std_logic;
        rst_n_i : in std_logic;

        ---------------------------------------------------------
        -- Interrupt request
        dma_ctrl_irq_o : out std_logic_vector(1 downto 0);

        ---------------------------------------------------------
        -- To the L2P DMA master and P2L DMA master
        dma_ctrl_carrier_addr_o : out std_logic_vector(31 downto 0);
        dma_ctrl_host_addr_h_o  : out std_logic_vector(31 downto 0);
        dma_ctrl_host_addr_l_o  : out std_logic_vector(31 downto 0);
        dma_ctrl_len_o          : out std_logic_vector(31 downto 0);
        dma_ctrl_start_l2p_o    : out std_logic;  -- To the L2P DMA master
        dma_ctrl_start_p2l_o    : out std_logic;  -- To the P2L DMA master
        dma_ctrl_start_next_o   : out std_logic;  -- To the P2L DMA master
        dma_ctrl_done_i         : in  std_logic;
        dma_ctrl_error_i        : in  std_logic;
        dma_ctrl_byte_swap_o    : out std_logic_vector(1 downto 0);
        dma_ctrl_abort_o        : out std_logic;

        ---------------------------------------------------------
        -- From P2L DMA MASTER
        next_item_carrier_addr_i : in std_logic_vector(31 downto 0);
        next_item_host_addr_h_i  : in std_logic_vector(31 downto 0);
        next_item_host_addr_l_i  : in std_logic_vector(31 downto 0);
        next_item_len_i          : in std_logic_vector(31 downto 0);
        next_item_next_l_i       : in std_logic_vector(31 downto 0);
        next_item_next_h_i       : in std_logic_vector(31 downto 0);
        next_item_attrib_i       : in std_logic_vector(31 downto 0);
        next_item_valid_i        : in std_logic;

        ---------------------------------------------------------
        -- Wishbone Slave Interface
        wb_clk_i : in  std_logic;                      -- Bus clock
        wb_adr_i : in  std_logic_vector(3 downto 0);   -- Adress
        wb_dat_o : out std_logic_vector(31 downto 0);  -- Data in
        wb_dat_i : in  std_logic_vector(31 downto 0);  -- Data out
        wb_sel_i : in  std_logic_vector(3 downto 0);   -- Byte select
        wb_cyc_i : in  std_logic;                      -- Read or write cycle
        wb_stb_i : in  std_logic;                      -- Read or write strobe
        wb_we_i  : in  std_logic;                      -- Write
        wb_ack_o : out std_logic                       -- Acknowledge
        );
  end component;  -- dma_controller

-----------------------------------------------------------------------------
  component l2p_dma_master
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
        -- To the L2P Interface (send the DMA data)
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
        tx_error_i : in  std_logic;                     -- Transmit Error

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
  end component;  -- l2p_dma_master

-----------------------------------------------------------------------------
  component p2l_dma_master
    generic (
      -- Enable byte swap module (if false, no swap)
      g_BYTE_SWAP : boolean := false
      );
    port
      (
        ---------------------------------------------------------
        -- Clock/Reset
        clk_i   : in std_logic;
        rst_n_i : in std_logic;

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
        dma_ctrl_byte_swap_i    : in  std_logic_vector(1 downto 0);
        dma_ctrl_abort_i        : in  std_logic;

        ---------------------------------------------------------
        -- From P2L Decoder (receive the read completion)
        --
        -- Header
        pd_pdm_hdr_start_i   : in std_logic;                      -- Indicates Header start cycle
        pd_pdm_hdr_length_i  : in std_logic_vector(9 downto 0);   -- Latched LENGTH value from header
        pd_pdm_hdr_cid_i     : in std_logic_vector(1 downto 0);   -- Completion ID
        pd_pdm_master_cpld_i : in std_logic;                      -- Master read completion with data
        pd_pdm_master_cpln_i : in std_logic;                      -- Master read completion without data
        --
        -- Data
        pd_pdm_data_valid_i  : in std_logic;                      -- Indicates Data is valid
        pd_pdm_data_last_i   : in std_logic;                      -- Indicates end of the packet
        pd_pdm_data_i        : in std_logic_vector(31 downto 0);  -- Data
        pd_pdm_be_i          : in std_logic_vector(3 downto 0);   -- Byte Enable for data

        ---------------------------------------------------------
        -- P2L control
        p2l_rdy_o  : out std_logic;     -- De-asserted to pause transfer already in progress
        rx_error_o : out std_logic;     -- Asserted when transfer is aborted

        ---------------------------------------------------------
        -- To the L2P Interface (send the DMA Master Read request)
        pdm_arb_valid_o  : out std_logic;  -- Read completion signals
        pdm_arb_dframe_o : out std_logic;  -- Toward the arbiter
        pdm_arb_data_o   : out std_logic_vector(31 downto 0);
        pdm_arb_req_o    : out std_logic;
        arb_pdm_gnt_i    : in  std_logic;

        ---------------------------------------------------------
        -- DMA Interface (Pipelined Wishbone)
        p2l_dma_clk_i   : in  std_logic;                      -- Bus clock
        p2l_dma_adr_o   : out std_logic_vector(31 downto 0);  -- Adress
        p2l_dma_dat_i   : in  std_logic_vector(31 downto 0);  -- Data in
        p2l_dma_dat_o   : out std_logic_vector(31 downto 0);  -- Data out
        p2l_dma_sel_o   : out std_logic_vector(3 downto 0);   -- Byte select
        p2l_dma_cyc_o   : out std_logic;                      -- Read or write cycle
        p2l_dma_stb_o   : out std_logic;                      -- Read or write strobe
        p2l_dma_we_o    : out std_logic;                      -- Write
        p2l_dma_ack_i   : in  std_logic;                      -- Acknowledge
        p2l_dma_stall_i : in  std_logic;                      -- for pipelined Wishbone
        l2p_dma_cyc_i   : in  std_logic;                      -- L2P dma wb cycle (for bus arbitration)

        ---------------------------------------------------------
        -- From P2L DMA MASTER
        next_item_carrier_addr_o : out std_logic_vector(31 downto 0);
        next_item_host_addr_h_o  : out std_logic_vector(31 downto 0);
        next_item_host_addr_l_o  : out std_logic_vector(31 downto 0);
        next_item_len_o          : out std_logic_vector(31 downto 0);
        next_item_next_l_o       : out std_logic_vector(31 downto 0);
        next_item_next_h_o       : out std_logic_vector(31 downto 0);
        next_item_attrib_o       : out std_logic_vector(31 downto 0);
        next_item_valid_o        : out std_logic
        );
  end component;  -- p2l_dma_master

-----------------------------------------------------------------------------
  component l2p_arbiter
    port (
      ---------------------------------------------------------
      -- Clock/Reset
      clk_i   : in std_logic;
      rst_n_i : in std_logic;

      ---------------------------------------------------------
      -- From Wishbone master (wbm) to arbiter (arb)
      wbm_arb_valid_i  : in  std_logic;
      wbm_arb_dframe_i : in  std_logic;
      wbm_arb_data_i   : in  std_logic_vector(31 downto 0);
      wbm_arb_req_i    : in  std_logic;
      arb_wbm_gnt_o    : out std_logic;

      ---------------------------------------------------------
      -- From DMA controller (pdm) to arbiter (arb)
      pdm_arb_valid_i  : in  std_logic;
      pdm_arb_dframe_i : in  std_logic;
      pdm_arb_data_i   : in  std_logic_vector(31 downto 0);
      pdm_arb_req_i    : in  std_logic;
      arb_pdm_gnt_o    : out std_logic;

      ---------------------------------------------------------
      -- From P2L DMA master (ldm) to arbiter (arb)
      ldm_arb_valid_i  : in  std_logic;
      ldm_arb_dframe_i : in  std_logic;
      ldm_arb_data_i   : in  std_logic_vector(31 downto 0);
      ldm_arb_req_i    : in  std_logic;
      arb_ldm_gnt_o    : out std_logic;

      ---------------------------------------------------------
      -- From arbiter (arb) to serializer (ser)
      arb_ser_valid_o  : out std_logic;
      arb_ser_dframe_o : out std_logic;
      arb_ser_data_o   : out std_logic_vector(31 downto 0)
      );
  end component;  -- l2p_arbiter

-----------------------------------------------------------------------------
  component fifo_32x512
    port (
      rst                     : in  std_logic;
      wr_clk                  : in  std_logic;
      rd_clk                  : in  std_logic;
      din                     : in  std_logic_vector(31 downto 0);
      wr_en                   : in  std_logic;
      rd_en                   : in  std_logic;
      prog_full_thresh_assert : in  std_logic_vector(8 downto 0);
      prog_full_thresh_negate : in  std_logic_vector(8 downto 0);
      dout                    : out std_logic_vector(31 downto 0);
      full                    : out std_logic;
      empty                   : out std_logic;
      valid                   : out std_logic;
      prog_full               : out std_logic);
  end component;

-----------------------------------------------------------------------------
  component fifo_64x512
    port (
      rst                     : in  std_logic;
      wr_clk                  : in  std_logic;
      rd_clk                  : in  std_logic;
      din                     : in  std_logic_vector(63 downto 0);
      wr_en                   : in  std_logic;
      rd_en                   : in  std_logic;
      prog_full_thresh_assert : in  std_logic_vector(8 downto 0);
      prog_full_thresh_negate : in  std_logic_vector(8 downto 0);
      dout                    : out std_logic_vector(63 downto 0);
      full                    : out std_logic;
      empty                   : out std_logic;
      valid                   : out std_logic;
      prog_full               : out std_logic);
  end component;


end gn4124_core_pkg;

package body gn4124_core_pkg is

  -----------------------------------------------------------------------------
  -- Byte swap function
  --
  -- enable | byte_swap | din  | dout
  -- false  | XX        | ABCD | ABCD
  -- true   | 00        | ABCD | ABCD
  -- true   | 01        | ABCD | BADC
  -- true   | 10        | ABCD | CDAB
  -- true   | 11        | ABCD | DCBA
  -----------------------------------------------------------------------------
  function f_byte_swap (
    constant enable    : boolean;
    signal   din       : std_logic_vector(31 downto 0);
    signal   byte_swap : std_logic_vector(1 downto 0))
    return std_logic_vector is
    variable dout : std_logic_vector(31 downto 0) := din;
  begin
    if (enable = true) then
      case byte_swap is
        when "00" =>
          dout := din;
        when "01" =>
          dout := din(23 downto 16)
                  & din(31 downto 24)
                  & din(7 downto 0)
                  & din(15 downto 8);
        when "10" =>
          dout := din(15 downto 0)
                  & din(31 downto 16);
        when "11" =>
          dout := din(7 downto 0)
                  & din(15 downto 8)
                  & din(23 downto 16)
                  & din(31 downto 24);
        when others =>
          dout := din;
      end case;
    else
      dout := din;
    end if;
    return dout;
  end function f_byte_swap;

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

end gn4124_core_pkg;
