--------------------------------------------------------------------------------
--                                                                            --
-- CERN BE-CO-HT         GN4124 core for PCIe FMC carrier                     --
--                       http://www.ohwr.org/projects/gn4124-core             --
--------------------------------------------------------------------------------
--
-- unit name: spec_gn4124_test (spec_gn4124_test.vhd)
--
-- author: Matthieu Cattin (matthieu.cattin@cern.ch)
--
-- date: 07-07-2011
--
-- version: 0.1
--
-- description: Wrapper for the GN4124 core to drop into the FPGA on the
--              SPEC (Simple PCIe FMC Carrier) board
--
-- dependencies:
--
--------------------------------------------------------------------------------
-- last changes: see svn log.
--------------------------------------------------------------------------------
-- TODO: - 
--------------------------------------------------------------------------------

library IEEE;
use IEEE.STD_LOGIC_1164.all;
use IEEE.NUMERIC_STD.all;
use work.gn4124_core_pkg.all;

library UNISIM;
use UNISIM.vcomponents.all;


entity yarr is
  port
    (
      -- On board 20MHz oscillator
      clk20_vcxo_i : in std_logic;

      -- From GN4124 Local bus
      L_CLKp : in std_logic;            -- Local bus clock (frequency set in GN4124 config registers)
      L_CLKn : in std_logic;            -- Local bus clock (frequency set in GN4124 config registers)
		
		--clk_125m_pllref_n_i : in std_logic;
		--clk_125m_pllref_p_i : in std_logic;

      L_RST_N : in std_logic;           -- Reset from GN4124 (RSTOUT18_N)

      -- General Purpose Interface
      GPIO : out std_logic_vector(1 downto 0);  -- GPIO[0] -> GN4124 GPIO8
                                                  -- GPIO[1] -> GN4124 GPIO9

      -- PCIe to Local [Inbound Data] - RX
      P2L_RDY    : out std_logic;                      -- Rx Buffer Full Flag
      P2L_CLKn   : in  std_logic;                      -- Receiver Source Synchronous Clock-
      P2L_CLKp   : in  std_logic;                      -- Receiver Source Synchronous Clock+
      P2L_DATA   : in  std_logic_vector(15 downto 0);  -- Parallel receive data
      P2L_DFRAME : in  std_logic;                      -- Receive Frame
      P2L_VALID  : in  std_logic;                      -- Receive Data Valid

      -- Inbound Buffer Request/Status
      P_WR_REQ : in  std_logic_vector(1 downto 0);  -- PCIe Write Request
      P_WR_RDY : out std_logic_vector(1 downto 0);  -- PCIe Write Ready
      RX_ERROR : out std_logic;                     -- Receive Error

      -- Local to Parallel [Outbound Data] - TX
      L2P_DATA   : out std_logic_vector(15 downto 0);  -- Parallel transmit data
      L2P_DFRAME : out std_logic;                      -- Transmit Data Frame
      L2P_VALID  : out std_logic;                      -- Transmit Data Valid
      L2P_CLKn   : out std_logic;                      -- Transmitter Source Synchronous Clock-
      L2P_CLKp   : out std_logic;                      -- Transmitter Source Synchronous Clock+
      L2P_EDB    : out std_logic;                      -- Packet termination and discard

      -- Outbound Buffer Status
      L2P_RDY    : in std_logic;                     -- Tx Buffer Full Flag
      L_WR_RDY   : in std_logic_vector(1 downto 0);  -- Local-to-PCIe Write
      P_RD_D_RDY : in std_logic_vector(1 downto 0);  -- PCIe-to-Local Read Response Data Ready
      TX_ERROR   : in std_logic;                     -- Transmit Error
      VC_RDY     : in std_logic_vector(1 downto 0);  -- Channel ready

      -- Font panel LEDs
      led_red_o   : out std_logic;
      led_green_o : out std_logic;

      -- Auxiliary pins
      AUX_LEDS_O    : out std_logic_vector(3 downto 0);
      AUX_BUTTONS_I : in  std_logic_vector(1 downto 0);

      -- PCB version
      pcb_ver_i : in std_logic_vector(3 downto 0);
		
		-- DDR3
		DDR3_CAS_N : out std_logic;
		DDR3_CK_P : out std_logic;
		DDR3_CK_N : out std_logic;
		DDR3_CKE : out std_logic;
		DDR3_LDM : out std_logic;
		DDR3_LDQS_N : inout std_logic;
		DDR3_LDQS_P : inout std_logic;
		DDR3_ODT : out std_logic;
		DDR3_RAS_N : out std_logic;
		DDR3_RESET_N : out std_logic;
		DDR3_UDM : out std_logic;
		DDR3_UDQS_N : inout std_logic;
		DDR3_UDQS_P : inout std_logic;
		DDR3_WE_N : out std_logic;
		DDR3_RZQ : inout std_logic;
		DDR3_ZIO : inout std_logic;
		DDR3_A : out std_logic_vector(13 downto 0);
		DDR3_BA : out std_logic_vector(2 downto 0);
		DDR3_DQ : inout std_logic_vector(15 downto 0)
      );
end yarr;



architecture rtl of yarr is

  ------------------------------------------------------------------------------
  -- Components declaration
  ------------------------------------------------------------------------------

  component gn4124_core
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

        ---------------------------------------------------------
        -- L2P Direction
        --
        -- Source Sync DDR related signals
        l2p_clk_p_o  : out std_logic;                      -- Transmitter Source Synchronous Clock+
        l2p_clk_n_o  : out std_logic;                      -- Transmitter Source Synchronous Clock-
        l2p_data_o   : out std_logic_vector(15 downto 0);  -- Parallel transmit data
        l2p_dframe_o : out std_logic;                      -- Transmit Data Frame
        l2p_valid_o  : out std_logic;                      -- Transmit Data Valid
        l2p_edb_o    : out std_logic;                      -- Packet termination and discard
        -- L2P Control
        l2p_rdy_i    : in  std_logic;                      -- Tx Buffer Full Flag
        l_wr_rdy_i   : in  std_logic_vector(1 downto 0);   -- Local-to-PCIe Write
        p_rd_d_rdy_i : in  std_logic_vector(1 downto 0);   -- PCIe-to-Local Read Response Data Ready
        tx_error_i   : in  std_logic;                      -- Transmit Error
        vc_rdy_i     : in  std_logic_vector(1 downto 0);   -- Channel ready

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
        -- DMA interface (Pipelined wishbone master)
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
  end component;  --  gn4124_core

  component wb_addr_decoder
    generic
      (
        g_WINDOW_SIZE  : integer := 18;  -- Number of bits to address periph on the board (32-bit word address)
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
  end component wb_addr_decoder;

  component dummy_stat_regs_wb_slave
    port (
      rst_n_i                 : in  std_logic;
      wb_clk_i                : in  std_logic;
      wb_addr_i               : in  std_logic_vector(1 downto 0);
      wb_data_i               : in  std_logic_vector(31 downto 0);
      wb_data_o               : out std_logic_vector(31 downto 0);
      wb_cyc_i                : in  std_logic;
      wb_sel_i                : in  std_logic_vector(3 downto 0);
      wb_stb_i                : in  std_logic;
      wb_we_i                 : in  std_logic;
      wb_ack_o                : out std_logic;
      dummy_stat_reg_1_i      : in  std_logic_vector(31 downto 0);
      dummy_stat_reg_2_i      : in  std_logic_vector(31 downto 0);
      dummy_stat_reg_3_i      : in  std_logic_vector(31 downto 0);
      dummy_stat_reg_switch_i : in  std_logic_vector(31 downto 0)
      );
  end component;

  component dummy_ctrl_regs_wb_slave
    port (
      rst_n_i         : in  std_logic;
      wb_clk_i        : in  std_logic;
      wb_addr_i       : in  std_logic_vector(1 downto 0);
      wb_data_i       : in  std_logic_vector(31 downto 0);
      wb_data_o       : out std_logic_vector(31 downto 0);
      wb_cyc_i        : in  std_logic;
      wb_sel_i        : in  std_logic_vector(3 downto 0);
      wb_stb_i        : in  std_logic;
      wb_we_i         : in  std_logic;
      wb_ack_o        : out std_logic;
      dummy_reg_1_o   : out std_logic_vector(31 downto 0);
      dummy_reg_2_o   : out std_logic_vector(31 downto 0);
      dummy_reg_3_o   : out std_logic_vector(31 downto 0);
      dummy_reg_led_o : out std_logic_vector(31 downto 0)
      );
  end component;
  
	component ddr3_ctrl
	  generic(
		 --! Bank and port size selection
		 g_BANK_PORT_SELECT   : string  := "SPEC_BANK3_32B_32B";
		 --! Core's clock period in ps
		 g_MEMCLK_PERIOD      : integer := 3125;
		 --! If TRUE, uses Xilinx calibration core (Input term, DQS centering)
		 g_CALIB_SOFT_IP      : string  := "TRUE";
		 --! User ports addresses maping (BANK_ROW_COLUMN or ROW_BANK_COLUMN)
		 g_MEM_ADDR_ORDER     : string  := "BANK_ROW_COLUMN";
		 --! Simulation mode
		 g_SIMULATION         : string  := "FALSE";
		 --! DDR3 data port width
		 g_NUM_DQ_PINS        : integer := 16;
		 --! DDR3 address port width
		 g_MEM_ADDR_WIDTH     : integer := 14;
		 --! DDR3 bank address width
		 g_MEM_BANKADDR_WIDTH : integer := 3;
		 --! Wishbone port 0 data mask size (8-bit granularity)
		 g_P0_MASK_SIZE       : integer := 4;
		 --! Wishbone port 0 data width
		 g_P0_DATA_PORT_SIZE  : integer := 32;
		 --! Port 0 byte address width
		 g_P0_BYTE_ADDR_WIDTH : integer := 30;
		 --! Wishbone port 1 data mask size (8-bit granularity)
		 g_P1_MASK_SIZE       : integer := 4;
		 --! Wishbone port 1 data width
		 g_P1_DATA_PORT_SIZE  : integer := 32;
		 --! Port 1 byte address width
		 g_P1_BYTE_ADDR_WIDTH : integer := 30
		 );
	  port(
		 ----------------------------------------------------------------------------
		 -- Clock, control and status
		 ----------------------------------------------------------------------------
		 --! Clock input
		 clk_i    : in  std_logic;
		 --! Reset input (active low)
		 rst_n_i  : in  std_logic;
		 --! Status output
		 status_o : out std_logic_vector(31 downto 0);

		 ----------------------------------------------------------------------------
		 -- DDR3 interface
		 ----------------------------------------------------------------------------
		 --! DDR3 data bus
		 ddr3_dq_b     : inout std_logic_vector(g_NUM_DQ_PINS-1 downto 0);
		 --! DDR3 address bus
		 ddr3_a_o      : out   std_logic_vector(g_MEM_ADDR_WIDTH-1 downto 0);
		 --! DDR3 bank address
		 ddr3_ba_o     : out   std_logic_vector(g_MEM_BANKADDR_WIDTH-1 downto 0);
		 --! DDR3 row address strobe
		 ddr3_ras_n_o  : out   std_logic;
		 --! DDR3 column address strobe
		 ddr3_cas_n_o  : out   std_logic;
		 --! DDR3 write enable
		 ddr3_we_n_o   : out   std_logic;
		 --! DDR3 on-die termination
		 ddr3_odt_o    : out   std_logic;
		 --! DDR3 reset
		 ddr3_rst_n_o  : out   std_logic;
		 --! DDR3 clock enable
		 ddr3_cke_o    : out   std_logic;
		 --! DDR3 lower byte data mask
		 ddr3_dm_o     : out   std_logic;
		 --! DDR3 upper byte data mask
		 ddr3_udm_o    : out   std_logic;
		 --! DDR3 lower byte data strobe (pos)
		 ddr3_dqs_p_b  : inout std_logic;
		 --! DDR3 lower byte data strobe (neg)
		 ddr3_dqs_n_b  : inout std_logic;
		 --! DDR3 upper byte data strobe (pos)
		 ddr3_udqs_p_b : inout std_logic;
		 --! DDR3 upper byte data strobe (pos)
		 ddr3_udqs_n_b : inout std_logic;
		 --! DDR3 clock (pos)
		 ddr3_clk_p_o  : out   std_logic;
		 --! DDR3 clock (neg)
		 ddr3_clk_n_o  : out   std_logic;
		 --! MCB internal termination calibration resistor
		 ddr3_rzq_b    : inout std_logic;
		 --! MCB internal termination calibration
		 ddr3_zio_b    : inout std_logic;

		 ----------------------------------------------------------------------------
		 -- Wishbone bus - Port 0
		 ----------------------------------------------------------------------------
		 --! Wishbone bus clock
		 wb0_clk_i   : in  std_logic;
		 --! Wishbone bus byte select
		 wb0_sel_i   : in  std_logic_vector(g_P0_MASK_SIZE - 1 downto 0);
		 --! Wishbone bus cycle select
		 wb0_cyc_i   : in  std_logic;
		 --! Wishbone bus cycle strobe
		 wb0_stb_i   : in  std_logic;
		 --! Wishbone bus write enable
		 wb0_we_i    : in  std_logic;
		 --! Wishbone bus address
		 wb0_addr_i  : in  std_logic_vector(31 downto 0);
		 --! Wishbone bus data input
		 wb0_data_i  : in  std_logic_vector(g_P0_DATA_PORT_SIZE - 1 downto 0);
		 --! Wishbone bus data output
		 wb0_data_o  : out std_logic_vector(g_P0_DATA_PORT_SIZE - 1 downto 0);
		 --! Wishbone bus acknowledge
		 wb0_ack_o   : out std_logic;
		 --! Wishbone bus stall (for pipelined mode)
		 wb0_stall_o : out std_logic;

		 ----------------------------------------------------------------------------
		 -- Status - Port 0
		 ----------------------------------------------------------------------------
		 --! Command FIFO empty
		 p0_cmd_empty_o   : out std_logic;
		 --! Command FIFO full
		 p0_cmd_full_o    : out std_logic;
		 --! Read FIFO full
		 p0_rd_full_o     : out std_logic;
		 --! Read FIFO empty
		 p0_rd_empty_o    : out std_logic;
		 --! Read FIFO count
		 p0_rd_count_o    : out std_logic_vector(6 downto 0);
		 --! Read FIFO overflow
		 p0_rd_overflow_o : out std_logic;
		 --! Read FIFO error (pointers unsynchronized, reset required)
		 p0_rd_error_o    : out std_logic;
		 --! Write FIFO full
		 p0_wr_full_o     : out std_logic;
		 --! Write FIFO empty
		 p0_wr_empty_o    : out std_logic;
		 --! Write FIFO count
		 p0_wr_count_o    : out std_logic_vector(6 downto 0);
		 --! Write FIFO underrun
		 p0_wr_underrun_o : out std_logic;
		 --! Write FIFO error (pointers unsynchronized, reset required)
		 p0_wr_error_o    : out std_logic;

		 ----------------------------------------------------------------------------
		 -- Wishbone bus - Port 1
		 ----------------------------------------------------------------------------
		 --! Wishbone bus clock
		 wb1_clk_i   : in  std_logic;
		 --! Wishbone bus byte select
		 wb1_sel_i   : in  std_logic_vector(g_P1_MASK_SIZE - 1 downto 0);
		 --! Wishbone bus cycle select
		 wb1_cyc_i   : in  std_logic;
		 --! Wishbone bus cycle strobe
		 wb1_stb_i   : in  std_logic;
		 --! Wishbone bus write enable
		 wb1_we_i    : in  std_logic;
		 --! Wishbone bus address
		 wb1_addr_i  : in  std_logic_vector(31 downto 0);
		 --! Wishbone bus data input
		 wb1_data_i  : in  std_logic_vector(g_P1_DATA_PORT_SIZE - 1 downto 0);
		 --! Wishbone bus data output
		 wb1_data_o  : out std_logic_vector(g_P1_DATA_PORT_SIZE - 1 downto 0);
		 --! Wishbone bus acknowledge
		 wb1_ack_o   : out std_logic;
		 --! Wishbone bus stall (for pipelined mode)
		 wb1_stall_o : out std_logic;

		 ----------------------------------------------------------------------------
		 -- Status - Port 1
		 ----------------------------------------------------------------------------
		 --! Command FIFO empty
		 p1_cmd_empty_o   : out std_logic;
		 --! Command FIFO full
		 p1_cmd_full_o    : out std_logic;
		 --! Read FIFO full
		 p1_rd_full_o     : out std_logic;
		 --! Read FIFO empty
		 p1_rd_empty_o    : out std_logic;
		 --! Read FIFO count
		 p1_rd_count_o    : out std_logic_vector(6 downto 0);
		 --! Read FIFO overflow
		 p1_rd_overflow_o : out std_logic;
		 --! Read FIFO error (pointers unsynchronized, reset required)
		 p1_rd_error_o    : out std_logic;
		 --! Write FIFO full
		 p1_wr_full_o     : out std_logic;
		 --! Write FIFO empty
		 p1_wr_empty_o    : out std_logic;
		 --! Write FIFO count
		 p1_wr_count_o    : out std_logic_vector(6 downto 0);
		 --! Write FIFO underrun
		 p1_wr_underrun_o : out std_logic;
		 --! Write FIFO error (pointers unsynchronized, reset required)
		 p1_wr_error_o    : out std_logic
		 );
	end component ddr3_ctrl;

	component clk_gen
	port
	 (-- Clock in ports
	  CLK_200			: in std_logic;
	  -- Clock out ports
	  CLK_160          : out    std_logic;
	  CLK_333          : out    std_logic;
	  -- Status and control signals
	  RESET             : in     std_logic;
	  LOCKED            : out    std_logic
	 );
	end component;

	component ila
	  PORT (
		 CONTROL : INOUT STD_LOGIC_VECTOR(35 DOWNTO 0);
		 CLK : IN STD_LOGIC;
		 TRIG0 : IN STD_LOGIC_VECTOR(31 DOWNTO 0);
		 TRIG1 : IN STD_LOGIC_VECTOR(31 DOWNTO 0);
		 TRIG2 : IN STD_LOGIC_VECTOR(31 DOWNTO 0));

	end component;
	
	component ila_icon
	PORT (
    CONTROL0 : INOUT STD_LOGIC_VECTOR(35 DOWNTO 0));

	end component;

  ------------------------------------------------------------------------------
  -- Constants declaration
  ------------------------------------------------------------------------------
  constant c_BAR0_APERTURE    : integer := 18;  -- nb of bits for 32-bit word address
  constant c_CSR_WB_SLAVES_NB : integer := 3;

  ------------------------------------------------------------------------------
  -- Signals declaration
  ------------------------------------------------------------------------------

  -- System clock
  signal sys_clk : std_logic;
  signal CLK_160 : std_logic;
  signal CLK_333 : std_logic;
  
  signal locked : std_logic;
  signal rst : std_logic;

  -- LCLK from GN4124 used as system clock
  signal l_clk : std_logic;

  -- P2L colck PLL status
  signal p2l_pll_locked : std_logic;

  -- CSR wishbone bus (master)
  signal wbm_adr   : std_logic_vector(31 downto 0);
  signal wbm_dat_i : std_logic_vector(31 downto 0);
  signal wbm_dat_o : std_logic_vector(31 downto 0);
  signal wbm_sel   : std_logic_vector(3 downto 0);
  signal wbm_cyc   : std_logic;
  signal wbm_stb   : std_logic;
  signal wbm_we    : std_logic;
  signal wbm_ack   : std_logic;
  signal wbm_stall : std_logic;

  -- CSR wishbone bus (slaves)
  signal wb_adr   : std_logic_vector(31 downto 0);
  signal wb_dat_i : std_logic_vector((32*c_CSR_WB_SLAVES_NB)-1 downto 0);
  signal wb_dat_o : std_logic_vector(31 downto 0);
  signal wb_sel   : std_logic_vector(3 downto 0);
  signal wb_cyc   : std_logic_vector(c_CSR_WB_SLAVES_NB-1 downto 0);
  signal wb_stb   : std_logic;
  signal wb_we    : std_logic;
  signal wb_ack   : std_logic_vector(c_CSR_WB_SLAVES_NB-1 downto 0);
  signal wb_stall : std_logic_vector(c_CSR_WB_SLAVES_NB-1 downto 0);

  -- DMA wishbone bus
  signal dma_adr   : std_logic_vector(31 downto 0);
  signal dma_dat_i : std_logic_vector(31 downto 0);
  signal dma_dat_o : std_logic_vector(31 downto 0);
  signal dma_sel   : std_logic_vector(3 downto 0);
  signal dma_cyc   : std_logic;
  signal dma_stb   : std_logic;
  signal dma_we    : std_logic;
  signal dma_ack   : std_logic;
  signal dma_stall : std_logic;
  signal ram_we    : std_logic;

  -- Interrupts stuff
  signal irq_sources   : std_logic_vector(1 downto 0);
  signal irq_to_gn4124 : std_logic;
  signal irq_out : std_logic;

  -- CSR whisbone slaves for test
  signal dummy_stat_reg_1      : std_logic_vector(31 downto 0);
  signal dummy_stat_reg_2      : std_logic_vector(31 downto 0);
  signal dummy_stat_reg_3      : std_logic_vector(31 downto 0);
  signal dummy_stat_reg_switch : std_logic_vector(31 downto 0);

  signal dummy_ctrl_reg_1   : std_logic_vector(31 downto 0);
  signal dummy_ctrl_reg_2   : std_logic_vector(31 downto 0);
  signal dummy_ctrl_reg_3   : std_logic_vector(31 downto 0);
  signal dummy_ctrl_reg_led : std_logic_vector(31 downto 0);

  -- FOR TESTS
  signal debug       : std_logic_vector(7 downto 0);
  signal clk_div_cnt : unsigned(3 downto 0);
  signal clk_div     : std_logic;

  -- LED
  signal led_cnt   : unsigned(24 downto 0);
  signal led_en    : std_logic;
  signal led_k2000 : unsigned(2 downto 0);
  signal led_pps   : std_logic;
  signal leds      : std_logic_vector(3 downto 0);
  
  -- ILA
  signal CONTROL : STD_LOGIC_VECTOR(35 DOWNTO 0);
  signal TRIG0 : STD_LOGIC_VECTOR(31 DOWNTO 0);
  signal TRIG1 : STD_LOGIC_VECTOR(31 DOWNTO 0);
  signal TRIG2 : STD_LOGIC_VECTOR(31 DOWNTO 0);
  signal debug_dma : std_logic_vector(31 downto 0);
  
  signal ddr_status : std_logic_vector(31 downto 0);
  signal gn4124_core_Status : std_logic_vector(31 downto 0);
  
begin


  ------------------------------------------------------------------------------
  -- Local clock from gennum LCLK
  ------------------------------------------------------------------------------
  cmp_l_clk_buf : IBUFDS
    generic map (
      DIFF_TERM    => false,            -- Differential Termination
      IBUF_LOW_PWR => true,             -- Low power (TRUE) vs. performance (FALSE) setting for referenced I/O standards
      IOSTANDARD   => "DEFAULT")
    port map (
      O  => l_clk,                      -- Buffer output
      I  => L_CLKp,                     -- Diff_p buffer input (connect directly to top-level port)
      IB => L_CLKn                      -- Diff_n buffer input (connect directly to top-level port)
      );

  ------------------------------------------------------------------------------
  -- GN4124 interface
  ------------------------------------------------------------------------------
  cmp_gn4124_core : gn4124_core
    port map
    (
      ---------------------------------------------------------
      -- Control and status
      rst_n_a_i             => L_RST_N,
      status_o => gn4124_core_status,


      ---------------------------------------------------------
      -- P2L Direction
      --
      -- Source Sync DDR related signals
      p2l_clk_p_i  => P2L_CLKp,
      p2l_clk_n_i  => P2L_CLKn,
      p2l_data_i   => P2L_DATA,
      p2l_dframe_i => P2L_DFRAME,
      p2l_valid_i  => P2L_VALID,

      -- P2L Control
      p2l_rdy_o  => P2L_RDY,
      p_wr_req_i => P_WR_REQ,
      p_wr_rdy_o => P_WR_RDY,
      rx_error_o => RX_ERROR,

      ---------------------------------------------------------
      -- L2P Direction
      --
      -- Source Sync DDR related signals
      l2p_clk_p_o  => L2P_CLKp,
      l2p_clk_n_o  => L2P_CLKn,
      l2p_data_o   => L2P_DATA,
      l2p_dframe_o => L2P_DFRAME,
      l2p_valid_o  => L2P_VALID,
      l2p_edb_o    => L2P_EDB,

      -- L2P Control
      l2p_rdy_i    => L2P_RDY,
      l_wr_rdy_i   => L_WR_RDY,
      p_rd_d_rdy_i => P_RD_D_RDY,
      tx_error_i   => TX_ERROR,
      vc_rdy_i     => VC_RDY,

      ---------------------------------------------------------
      -- Interrupt interface
      dma_irq_o => irq_sources,
      irq_p_i   => irq_to_gn4124,
      irq_p_o   => irq_out,

      ---------------------------------------------------------
      -- DMA registers wishbone interface (slave classic)
      dma_reg_clk_i   => l_clk,
      dma_reg_adr_i   => wb_adr,
      dma_reg_dat_i   => wb_dat_o,
      dma_reg_sel_i   => wb_sel,
      dma_reg_stb_i   => wb_stb,
      dma_reg_we_i    => wb_we,
      dma_reg_cyc_i   => wb_cyc(0),
      dma_reg_dat_o   => wb_dat_i(31 downto 0),
      dma_reg_ack_o   => wb_ack(0),
      dma_reg_stall_o => wb_stall(0),

      ---------------------------------------------------------
      -- CSR wishbone interface (master pipelined)
      csr_clk_i   => l_clk,
      csr_adr_o   => wbm_adr,
      csr_dat_o   => wbm_dat_o,
      csr_sel_o   => wbm_sel,
      csr_stb_o   => wbm_stb,
      csr_we_o    => wbm_we,
      csr_cyc_o   => wbm_cyc,
      csr_dat_i   => wbm_dat_i,
      csr_ack_i   => wbm_ack,
      csr_stall_i => wbm_stall,
      csr_err_i   => '0',
      csr_rty_i   => '0',
      csr_int_i   => '0',

      ---------------------------------------------------------
      -- DMA wishbone interface (master pipelined)
      dma_clk_i   => l_clk,
      dma_adr_o   => dma_adr,
      dma_dat_o   => dma_dat_o,
      dma_sel_o   => dma_sel,
      dma_stb_o   => dma_stb,
      dma_we_o    => dma_we,
      dma_cyc_o   => dma_cyc,
      dma_dat_i   => dma_dat_i,
      dma_ack_i   => dma_ack,
      dma_stall_i => dma_stall,
      dma_err_i   => '0',
      dma_rty_i   => '0',
      dma_int_i   => '0'
      );
	GPIO(0) <= irq_out;
	GPIO(1) <= '0';
  ------------------------------------------------------------------------------
  -- CSR wishbone address decoder
  ------------------------------------------------------------------------------
  cmp_csr_wb_addr_decoder : wb_addr_decoder
    generic map (
      g_WINDOW_SIZE  => c_BAR0_APERTURE,
      g_WB_SLAVES_NB => c_CSR_WB_SLAVES_NB
      )
    port map (
      ---------------------------------------------------------
      -- GN4124 core clock and reset
      clk_i   => l_clk,
      rst_n_i => L_RST_N,

      ---------------------------------------------------------
      -- wishbone master interface
      wbm_adr_i   => wbm_adr,
      wbm_dat_i   => wbm_dat_o,
      wbm_sel_i   => wbm_sel,
      wbm_stb_i   => wbm_stb,
      wbm_we_i    => wbm_we,
      wbm_cyc_i   => wbm_cyc,
      wbm_dat_o   => wbm_dat_i,
      wbm_ack_o   => wbm_ack,
      wbm_stall_o => wbm_stall,

      ---------------------------------------------------------
      -- wishbone slaves interface
      wb_adr_o   => wb_adr,
      wb_dat_o   => wb_dat_o,
      wb_sel_o   => wb_sel,
      wb_stb_o   => wb_stb,
      wb_we_o    => wb_we,
      wb_cyc_o   => wb_cyc,
      wb_dat_i   => wb_dat_i,
      wb_ack_i   => wb_ack,
      wb_stall_i => wb_stall
      );

  ------------------------------------------------------------------------------
  -- CSR wishbone bus slaves
  ------------------------------------------------------------------------------
  cmp_dummy_stat_regs : dummy_stat_regs_wb_slave
    port map(
      rst_n_i                 => L_RST_N,
      wb_clk_i                => l_clk,
      wb_addr_i               => wb_adr(1 downto 0),
      wb_data_i               => wb_dat_o,
      wb_data_o               => wb_dat_i(63 downto 32),
      wb_cyc_i                => wb_cyc(1),
      wb_sel_i                => wb_sel,
      wb_stb_i                => wb_stb,
      wb_we_i                 => wb_we,
      wb_ack_o                => wb_ack(1),
      dummy_stat_reg_1_i      => dummy_stat_reg_1,
      dummy_stat_reg_2_i      => dummy_stat_reg_2,
      dummy_stat_reg_3_i      => dummy_stat_reg_3,
      dummy_stat_reg_switch_i => dummy_stat_reg_switch
      );

  wb_stall(1) <= '0' when wb_cyc(1) = '0' else not(wb_ack(1));
--  wb_stall(2) <= '0' when wb_cyc(2) = '0' else not(wb_ack(2));

  dummy_stat_reg_1      <= X"DEADBABE";
  dummy_stat_reg_2      <= X"BEEFFACE";
  dummy_stat_reg_3      <= X"12345678";
  dummy_stat_reg_switch <= X"0000000" & "000" & p2l_pll_locked;

  led_red_o   <= dummy_ctrl_reg_led(0);
  led_green_o <= dummy_ctrl_reg_led(1);

--	TRIG0(31 downto 0) <= ddr_status;
--	TRIG1(31 downto 0) <= dma_adr;
--	TRIG2(31 downto 0) <= dma_dat_i;
--	ila_i : ila
--	  port map (
--		 CONTROL => CONTROL,
--		 CLK => l_clk,
--		 TRIG0 => TRIG0,
--		 TRIG1 => TRIG1,
--		 TRIG2 => TRIG2);
--		 
--	ila_icon_i : ila_icon
--		port map (
--    CONTROL0 => CONTROL);
	 
  ------------------------------------------------------------------------------
  -- Interrupt stuff
  ------------------------------------------------------------------------------
  -- just forward irq pulses for test
  irq_to_gn4124 <= irq_sources(1) or irq_sources(0);


  ------------------------------------------------------------------------------
  -- FOR TEST
  ------------------------------------------------------------------------------
  p_div_clk : process (l_clk, L_RST_N)
  begin
    if L_RST_N = '0' then
      clk_div     <= '0';
      clk_div_cnt <= (others => '0');
    elsif rising_edge(l_clk) then
      if clk_div_cnt = 4 then
        clk_div     <= not (clk_div);
        clk_div_cnt <= (others => '0');
      else
        clk_div_cnt <= clk_div_cnt + 1;
      end if;
    end if;
  end process p_div_clk;


  p_led_cnt : process (L_RST_N, CLK_333)
  begin
    if L_RST_N = '0' then
      led_cnt <= (others => '1');
      led_en  <= '1';
    elsif rising_edge(CLK_333) then
      led_cnt <= led_cnt - 1;
      led_en  <= led_cnt(23);
    end if;
  end process p_led_cnt;

  led_pps <= led_cnt(23) and not(led_en);


  p_led_k2000 : process (CLK_333, L_RST_N)
  begin
    if L_RST_N = '0' then
      led_k2000 <= (others => '0');
      leds      <= "0001";
    elsif rising_edge(CLK_333) then
      if led_pps = '1' then
        if led_k2000(2) = '0' then
          if leds /= "1000" then
            leds <= leds(2 downto 0) & '0';
          end if;
        else
          if leds /= "0001" then
            leds <= '0' & leds(3 downto 1);
          end if;
        end if;
        led_k2000 <= led_k2000 + 1;
      end if;
    end if;
  end process p_led_k2000;

  AUX_LEDS_O <= not(leds);

--AUX_LEDS_O(0) <= led_en;
--AUX_LEDS_O(1) <= not(led_en);
--AUX_LEDS_O(2) <= '1';
--AUX_LEDS_O(3) <= '0';

rst <= (L_RST_N and locked);

    cmp_clk_gen : clk_gen
    port map (
        -- Clock in ports
        CLK_200 => l_clk,
        -- Clock out ports
        CLK_160 => CLK_160,
        CLK_333 => CLK_333,
        -- Status and control signals
        RESET  => not L_RST_N,
        LOCKED => locked
    );

	cmp_ddr3_ctrl: ddr3_ctrl PORT MAP(
		clk_i => CLK_333,
		rst_n_i => rst,
		status_o => ddr_status,
		ddr3_dq_b => DDR3_DQ,
		ddr3_a_o => DDR3_A,
		ddr3_ba_o => DDR3_BA,
		ddr3_ras_n_o => DDR3_RAS_N,
		ddr3_cas_n_o => DDR3_CAS_N,
		ddr3_we_n_o => DDR3_WE_N,
		ddr3_odt_o => DDR3_ODT,
		ddr3_rst_n_o => DDR3_RESET_N,
		ddr3_cke_o => DDR3_CKE,
		ddr3_dm_o => DDR3_LDM,
		ddr3_udm_o => DDR3_UDM,
		ddr3_dqs_p_b => DDR3_LDQS_P,
		ddr3_dqs_n_b => DDR3_LDQS_N,
		ddr3_udqs_p_b => DDR3_UDQS_P,
		ddr3_udqs_n_b => DDR3_UDQS_N,
		ddr3_clk_p_o => DDR3_CK_P,
		ddr3_clk_n_o => DDR3_CK_N,
		ddr3_rzq_b => DDR3_RZQ,
		ddr3_zio_b => DDR3_ZIO,
		wb0_clk_i => l_clk,
		wb0_sel_i => dma_sel,
		wb0_cyc_i => dma_cyc,
		wb0_stb_i => dma_stb,
		wb0_we_i => dma_we,
		wb0_addr_i => dma_adr,
		wb0_data_i => dma_dat_o,
		wb0_data_o => dma_dat_i,
		wb0_ack_o => dma_ack,
		wb0_stall_o => dma_stall,
		p0_cmd_empty_o => open,
		p0_cmd_full_o => open,
		p0_rd_full_o => open,
		p0_rd_empty_o => open,
		p0_rd_count_o => open,
		p0_rd_overflow_o => open,
		p0_rd_error_o => open,
		p0_wr_full_o => open,
		p0_wr_empty_o => open,
		p0_wr_count_o => open,
		p0_wr_underrun_o => open,
		p0_wr_error_o => open,
		wb1_clk_i => l_clk,
		wb1_sel_i => wb_sel,
		wb1_cyc_i => wb_cyc(2),
		wb1_stb_i => wb_stb,
		wb1_we_i => wb_we,
		wb1_addr_i => wb_adr,
		wb1_data_i => wb_dat_o,
		wb1_data_o => wb_dat_i(95 downto 64),
		wb1_ack_o => wb_ack(2),
		wb1_stall_o => wb_stall(2),
		p1_cmd_empty_o => open,
		p1_cmd_full_o => open,
		p1_rd_full_o => open,
		p1_rd_empty_o => open,
		p1_rd_count_o => open,
		p1_rd_overflow_o => open,
		p1_rd_error_o => open,
		p1_wr_full_o => open,
		p1_wr_empty_o => open,
		p1_wr_count_o => open,
		p1_wr_underrun_o => open,
		p1_wr_error_o => open 
	);

end rtl;


