--------------------------------------------
-- Project: YARR
-- Author: Timon Heim (timon.heim@cern.ch)
-- Description: Top module for YARR on SPEC
-- Dependencies: -
--------------------------------------------

library IEEE;
use IEEE.STD_LOGIC_1164.all;
use IEEE.NUMERIC_STD.all;

library UNISIM;
use UNISIM.vcomponents.all;

library work;
use work.gn4124_core_pkg.all;

entity yarr is
    port (
        ----------------------------------
        -- Clocks
        ----------------------------------
        L_CLKP              : IN STD_LOGIC;
        L_CLKN              : IN STD_LOGIC;
        
        CLK20_VCXO_I        : IN STD_LOGIC;

        CLK_125M_PLLREF_P_I : IN STD_LOGIC;
        CLK_125M_PLLREF_N_I : IN STD_LOGIC;

        ----------------------------------
        -- Reset
        ----------------------------------
        L_RST_N             : IN STD_LOGIC;

        ----------------------------------
        -- GPIOs used for IRQ
        ----------------------------------
        GPIO                : OUT STD_LOGIC_VECTOR(1 downto 0);

        ----------------------------------
        -- PCIe to Local [Inbound Data] - RX
        ----------------------------------
        P2L_RDY             : OUT std_logic;                      -- Rx Buffer Full Flag
        P2L_CLKn            : IN  std_logic;                      -- Receiver Source Synchronous Clock-
        P2L_CLKp            : IN  std_logic;                      -- Receiver Source Synchronous Clock+
        P2L_DATA            : IN  std_logic_vector(15 downto 0);  -- Parallel receive data
        P2L_DFRAME          : IN  std_logic;                      -- Receive Frame
        P2L_VALID           : IN  std_logic;                      -- Receive Data Valid

        ----------------------------------
        -- Inbound buffer status
        ----------------------------------
        P_WR_REQ            : IN  std_logic_vector(1 downto 0); -- PCIe Write Request
        P_WR_RDY            : OUT std_logic_vector(1 downto 0); -- PCIe Write Ready
        RC_ERROR            : OUT std_logic;                    -- Receive Error

        ----------------------------------
        -- Local to PCIe [Outbound Data] - Tx
        ----------------------------------
        L2P_DATA            : OUT std_logic_vector(15 downto 0);
        L2P_DFRAME          : OUT std_logic;
        L2P_VALID           : OUT std_logic;
        L2P_CLKn            : OUT std_logic;
        L2P_CLKp            : OUT std_logic;
        L2P_EDB             : OUT std_logic;

        ----------------------------------
        -- Outbound buffer status
        ----------------------------------
        L2P_RDY             : IN std_logic;
        L_WR_RDY            : IN std_logic_vector(1 downto 0);
        P_RD_D_RDY          : IN std_logic_vector(1 downto 0);
        TX_ERROR            : IN std_logic;
        VC_RDY              : IN std_logic;

        ----------------------------------
        -- Front Panel LEDs
        ----------------------------------
        LED_GREEN_O         : OUT std_logic;
        LED_RED_O           : OUT std_logic;
        
        ----------------------------------
        -- Auxiliary ports 
        ----------------------------------
        AUX_LEDS_O          : OUT std_logic_vector(3 downto 0);
        AUX_BUTTONS_I       : IN  std_logic_vector(1 downto 0);

        ----------------------------------
        -- DDR3
        ----------------------------------
		DDR3_CAS_N 			: OUT std_logic;
		DDR3_CK_P 			: OUT std_logic;
		DDR3_CK_N 			: OUT std_logic;
		DDR3_CKE 			: OUT std_logic;
		DDR3_LDM 			: OUT std_logic;
		DDR3_LDQS_N 		: INOUT std_logic;
		DDR3_LDQS_P 		: INOUT std_logic;
		DDR3_ODT 			: OUT std_logic;
		DDR3_RAS_N 			: OUT std_logic;
		DDR3_RESET_N 		: OUT std_logic;
		DDR3_UDM 			: OUT std_logic;
		DDR3_UDQS_N 		: INOUT std_logic;
		DDR3_UDQS_P 		: INOUT std_logic;
		DDR3_WE_N 			: OUT std_logic;
		DDR3_RZQ 			: INOUT std_logic;
		DDR3_ZIO 			: INOUT std_logic;
		DDR3_A 			    : OUT std_logic_vector(13 downto 0);
		DDR3_BA 			: OUT std_logic_vector(2 downto 0);
		DDR3_DQ 			: INOUT std_logic_vector(15 downto 0)
    );
end yarr;

architecture rtl of yarr is
    ----------------------------------
    -- Components
    ----------------------------------
    component gn4124_core
    port (
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
    end component wb_addr_decoder; -- wb_addr_decoder

	component ddr3_ctrl
	  generic(
		 --! Bank and port size selection
		 g_BANK_PORT_SELECT   : string  := "SPEC_BANK3_32B_32B";
		 --! Core's clock period in ps
		 g_MEMCLK_PERIOD      : integer := 3000;
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


begin
end rtl;

