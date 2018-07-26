----------------------------------------------------------------------------------
-- Company: 
-- Engineer: 
-- 
-- Create Date: 05/11/2017 10:26:23 AM
-- Design Name: 
-- Module Name: app_package - Behavioral
-- Project Name: 
-- Target Devices: 
-- Tool Versions: 
-- Description: 
-- 
-- Dependencies: 
-- 
-- Revision:
-- Revision 0.01 - File Created
-- Additional Comments:
-- 
----------------------------------------------------------------------------------


library IEEE;
use IEEE.STD_LOGIC_1164.ALL;

-- Uncomment the following library declaration if using
-- arithmetic functions with Signed or Unsigned values
use IEEE.NUMERIC_STD.ALL;

-- Uncomment the following library declaration if instantiating
-- any Xilinx leaf cells in this code.
--library UNISIM;
--use UNISIM.VComponents.all;

library work;
use work.board_pkg.all;

package app_pkg is

    --constant c_TX_CHANNELS : integer := 8;
	--constant c_RX_CHANNELS : integer := 8;
    
    component simple_counter is
        Port ( 
               enable_i : in STD_LOGIC;
               rst_i : in STD_LOGIC;
               clk_i : in STD_LOGIC;
               count_o : out STD_LOGIC_VECTOR (28 downto 0);
               gray_count_o : out STD_LOGIC_VECTOR (28 downto 0)
                );
    end component;
    

------------------------------------------------------------------------------
--  Output     Output      Phase    Duty Cycle   Pk-to-Pk     Phase
--   Clock     Freq (MHz)  (degrees)    (%)     Jitter (ps)  Error (ps)
------------------------------------------------------------------------------
-- _clk_640___640.000______0.000______50.0______468.793____919.522
-- _clk_160___160.000______0.000______50.0______568.382____919.522
-- __clk_80____80.000______0.000______50.0______625.965____919.522
-- __clk_40____40.000______0.000______50.0______689.448____919.522
-- clk_40_90____40.000_____90.000______50.0______689.448____919.522
--
------------------------------------------------------------------------------
-- Input Clock   Freq (MHz)    Input Jitter (UI)
------------------------------------------------------------------------------
-- __primary_____________250____________0.010
    
    component clk_gen
    port
     (-- Clock in ports
      clk_250_in           : in     std_logic;
      -- Clock out ports
      clk_300           : out std_logic;
      clk_640          : out    std_logic;
      clk_160          : out    std_logic;
      clk_80          : out    std_logic;
      clk_40          : out    std_logic;
      clk_40_90          : out    std_logic;
      clk_250          : out    std_logic;
      -- Status and control signals
      reset             : in     std_logic;
      locked            : out    std_logic
     );
    end component;

    COMPONENT axis_data_fifo_0
      PORT (
        s_axis_aresetn : IN STD_LOGIC;
        m_axis_aresetn : IN STD_LOGIC;
        s_axis_aclk : IN STD_LOGIC;
        s_axis_tvalid : IN STD_LOGIC;
        s_axis_tready : OUT STD_LOGIC;
        s_axis_tdata : IN STD_LOGIC_VECTOR(63 DOWNTO 0);
        s_axis_tkeep : IN STD_LOGIC_VECTOR(7 DOWNTO 0);
        s_axis_tlast : IN STD_LOGIC;
        s_axis_tuser : IN STD_LOGIC_VECTOR(21 DOWNTO 0);
        m_axis_aclk : IN STD_LOGIC;
        m_axis_tvalid : OUT STD_LOGIC;
        m_axis_tready : IN STD_LOGIC;
        m_axis_tdata : OUT STD_LOGIC_VECTOR(63 DOWNTO 0);
        m_axis_tkeep : OUT STD_LOGIC_VECTOR(7 DOWNTO 0);
        m_axis_tlast : OUT STD_LOGIC;
        m_axis_tuser : OUT STD_LOGIC_VECTOR(21 DOWNTO 0);
        axis_data_count : OUT STD_LOGIC_VECTOR(31 DOWNTO 0);
        axis_wr_data_count : OUT STD_LOGIC_VECTOR(31 DOWNTO 0);
        axis_rd_data_count : OUT STD_LOGIC_VECTOR(31 DOWNTO 0)
      );
    END COMPONENT;

    COMPONENT axis_data_fifo_1
      PORT (
        s_axis_aresetn : IN STD_LOGIC;
        m_axis_aresetn : IN STD_LOGIC;
        s_axis_aclk : IN STD_LOGIC;
        s_axis_tvalid : IN STD_LOGIC;
        s_axis_tready : OUT STD_LOGIC;
        s_axis_tdata : IN STD_LOGIC_VECTOR(63 DOWNTO 0);
        s_axis_tkeep : IN STD_LOGIC_VECTOR(7 DOWNTO 0);
        s_axis_tlast : IN STD_LOGIC;
        s_axis_tuser : IN STD_LOGIC_VECTOR(3 DOWNTO 0);
        m_axis_aclk : IN STD_LOGIC;
        m_axis_tvalid : OUT STD_LOGIC;
        m_axis_tready : IN STD_LOGIC;
        m_axis_tdata : OUT STD_LOGIC_VECTOR(63 DOWNTO 0);
        m_axis_tkeep : OUT STD_LOGIC_VECTOR(7 DOWNTO 0);
        m_axis_tlast : OUT STD_LOGIC;
        m_axis_tuser : OUT STD_LOGIC_VECTOR(3 DOWNTO 0);
        axis_data_count : OUT STD_LOGIC_VECTOR(31 DOWNTO 0);
        axis_wr_data_count : OUT STD_LOGIC_VECTOR(31 DOWNTO 0);
        axis_rd_data_count : OUT STD_LOGIC_VECTOR(31 DOWNTO 0)
      );
    END COMPONENT;

    component synchronizer is
        port (
            -- Sys connect
            clk_i : in std_logic;
            rst_n_i : in std_logic;
    
            -- Async input
            async_in : in std_logic;
            sync_out : out std_logic
        );
    end component;

    component wshexp_core is
        Generic(
            AXI_BUS_WIDTH : integer := 64
        );
        Port ( 
            clk_i : in STD_LOGIC;
            wb_clk_i : in STD_LOGIC;
            --sys_clk_n_i : IN STD_LOGIC;
            --sys_clk_p_i : IN STD_LOGIC;
            rst_i : in STD_LOGIC;
            --user_lnk_up_i : in STD_LOGIC;
            --user_app_rdy_i : in STD_LOGIC;
            
            ---------------------------------------------------------
            -- AXI-Stream bus
            m_axis_tx_tready_i : in STD_LOGIC;
            m_axis_tx_tdata_o : out STD_LOGIC_VECTOR(AXI_BUS_WIDTH-1 DOWNTO 0);
            m_axis_tx_tkeep_o : out STD_LOGIC_VECTOR(AXI_BUS_WIDTH/8-1 DOWNTO 0);
            m_axis_tx_tlast_o : out STD_LOGIC;
            m_axis_tx_tvalid_o : out STD_LOGIC;
            m_axis_tx_tuser_o : out STD_LOGIC_VECTOR(3 DOWNTO 0);
            s_axis_rx_tdata_i : in STD_LOGIC_VECTOR(AXI_BUS_WIDTH-1 DOWNTO 0);
            s_axis_rx_tkeep_i : in STD_LOGIC_VECTOR(AXI_BUS_WIDTH/8-1 DOWNTO 0);
            s_axis_rx_tlast_i : in STD_LOGIC;
            s_axis_rx_tvalid_i : in STD_LOGIC;
            s_axis_rx_tready_o : out STD_LOGIC;
            s_axis_rx_tuser_i : in STD_LOGIC_VECTOR(21 DOWNTO 0);
    
            ---------------------------------------------------------
            -- DMA wishbone interface (master pipelined)        
            dma_adr_o   : out std_logic_vector(31 downto 0);  -- Adress
            dma_dat_o   : out  std_logic_vector(63 downto 0);  -- Data out
            dma_dat_i   : in  std_logic_vector(63 downto 0);  -- Data in
            dma_sel_o   : out std_logic_vector(7 downto 0);   -- Byte select
            dma_cyc_o   : out std_logic;                      -- Read or write cycle
            dma_stb_o   : out std_logic;                      -- Read or write strobe
            dma_we_o    : out std_logic;                      -- Write
            dma_ack_i   : in std_logic;                      -- Acknowledge
            dma_stall_i : in std_logic;                      -- for pipelined Wishbone
            
            ---------------------------------------------------------
            -- CSR wishbone interface (master classic)
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
            csr_rty_i   : in  std_logic;      -- not used internally
            csr_int_i   : in  std_logic;      -- not used internally
            
            ---------------------------------------------------------
            -- DMA registers wishbone interface (slave classic)
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
            -- PCIe interrupt config
            cfg_interrupt_o : out STD_LOGIC;
            cfg_interrupt_rdy_i : in STD_LOGIC;
            cfg_interrupt_assert_o : out STD_LOGIC;
            cfg_interrupt_di_o : out STD_LOGIC_VECTOR(7 DOWNTO 0);
            cfg_interrupt_do_i : in STD_LOGIC_VECTOR(7 DOWNTO 0);
            cfg_interrupt_mmenable_i : in STD_LOGIC_VECTOR(2 DOWNTO 0);
            cfg_interrupt_msienable_i : in STD_LOGIC;
            cfg_interrupt_msixenable_i : in STD_LOGIC;
            cfg_interrupt_msixfm_i : in STD_LOGIC;
            cfg_interrupt_stat_o : out STD_LOGIC;
            cfg_pciecap_interrupt_msgnum_o : out STD_LOGIC_VECTOR(4 DOWNTO 0);
            
            ---------------------------------------------------------
            -- PCIe ID
            cfg_bus_number_i : in STD_LOGIC_VECTOR(7 DOWNTO 0);
            cfg_device_number_i : in STD_LOGIC_VECTOR(4 DOWNTO 0);
            cfg_function_number_i : in STD_LOGIC_VECTOR(2 DOWNTO 0)
        );
    end component;

        
 
 
    

    
    component k_bram is
      generic (
        constant ADDR_WIDTH : integer := 9+4;
        constant DATA_WIDTH : integer := 64 
      );
      Port ( 
          -- SYS CON
          clk            : in std_logic;
          rst            : in std_logic;
          
          -- Wishbone Slave in
          wb_adr_i            : in std_logic_vector(9+4-1 downto 0);
          wb_dat_i            : in std_logic_vector(64-1 downto 0);
          wb_we_i            : in std_logic;
          wb_stb_i            : in std_logic;
          wb_cyc_i            : in std_logic; 
          wb_lock_i        : in std_logic; -- nyi
          
          -- Wishbone Slave out
          wb_dat_o            : out std_logic_vector(64-1 downto 0);
          wb_ack_o            : out std_logic
      
      );
    end component;
    
    component k_dual_bram is
        Port ( 
        -- SYS CON
        clk_i            : in std_logic;
        rst_i            : in std_logic;
        
        -- Wishbone Slave in
        wba_adr_i            : in std_logic_vector(32-1 downto 0);
        wba_dat_i            : in std_logic_vector(64-1 downto 0);
        wba_we_i            : in std_logic;
        wba_stb_i            : in std_logic;
        wba_cyc_i            : in std_logic; 
        
        -- Wishbone Slave out
        wba_dat_o            : out std_logic_vector(64-1 downto 0);
        wba_ack_o            : out std_logic;
               
        -- Wishbone Slave in
        wbb_adr_i            : in std_logic_vector(32-1 downto 0);
        wbb_dat_i            : in std_logic_vector(64-1 downto 0);
        wbb_we_i            : in std_logic;
        wbb_stb_i            : in std_logic;
        wbb_cyc_i            : in std_logic; 
        
        -- Wishbone Slave out
        wbb_dat_o            : out std_logic_vector(64-1 downto 0);
        wbb_ack_o            : out std_logic 
               
               );
    end component;    

    component wb_addr_decoder is
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
    end component;	



    component mig_7series_0
      port (
          ddr3_dq       : inout std_logic_vector(63 downto 0);
          ddr3_dqs_p    : inout std_logic_vector(7 downto 0);
          ddr3_dqs_n    : inout std_logic_vector(7 downto 0);
    
          ddr3_addr     : out   std_logic_vector(14 downto 0);
          ddr3_ba       : out   std_logic_vector(2 downto 0);
          ddr3_ras_n    : out   std_logic;
          ddr3_cas_n    : out   std_logic;
          ddr3_we_n     : out   std_logic;
          ddr3_reset_n  : out   std_logic;
          ddr3_ck_p     : out   std_logic_vector(0 downto 0);
          ddr3_ck_n     : out   std_logic_vector(0 downto 0);
          ddr3_cke      : out   std_logic_vector(0 downto 0);
          ddr3_cs_n     : out   std_logic_vector(0 downto 0);
          ddr3_dm       : out   std_logic_vector(7 downto 0);
          ddr3_odt      : out   std_logic_vector(0 downto 0);
          app_addr                  : in    std_logic_vector(28 downto 0);
          app_cmd                   : in    std_logic_vector(2 downto 0);
          app_en                    : in    std_logic;
          app_wdf_data              : in    std_logic_vector(511 downto 0);
          app_wdf_end               : in    std_logic;
          app_wdf_mask         : in    std_logic_vector(63 downto 0);
          app_wdf_wren              : in    std_logic;
          app_rd_data               : out   std_logic_vector(511 downto 0);
          app_rd_data_end           : out   std_logic;
          app_rd_data_valid         : out   std_logic;
          app_rdy                   : out   std_logic;
          app_wdf_rdy               : out   std_logic;
          app_sr_req                : in    std_logic;
          app_ref_req               : in    std_logic;
          app_zq_req                : in    std_logic;
          app_sr_active             : out   std_logic;
          app_ref_ack               : out   std_logic;
          app_zq_ack                : out   std_logic;
          ui_clk                    : out   std_logic;
          ui_clk_sync_rst           : out   std_logic;
          init_calib_complete       : out   std_logic;
          -- System Clock Ports
          sys_clk_p                      : in    std_logic;
          sys_clk_n                      : in    std_logic;
          sys_rst                     : in    std_logic
      );
    end component mig_7series_0;
	
    component ddr3_ctrl_wb
      generic(
        g_BYTE_ADDR_WIDTH : integer := 29;
        g_MASK_SIZE       : integer := 8;
        g_DATA_PORT_SIZE  : integer := 64
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
          
          ddr_addr_o                  : out  std_logic_vector(28 downto 0);
          ddr_cmd_o                   : out  std_logic_vector(2 downto 0);
          ddr_cmd_en_o                : out  std_logic;
          ddr_wdf_data_o              : out  std_logic_vector(511 downto 0);
          ddr_wdf_end_o               : out  std_logic;
          ddr_wdf_mask_o              : out  std_logic_vector(63 downto 0);
          ddr_wdf_wren_o              : out  std_logic;
          ddr_rd_data_i               : in   std_logic_vector(511 downto 0);
          ddr_rd_data_end_i           : in   std_logic;
          ddr_rd_data_valid_i         : in   std_logic;
          ddr_rdy_i                   : in   std_logic;
          ddr_wdf_rdy_i               : in   std_logic;
          ddr_sr_req_o                : out  std_logic;
          ddr_ref_req_o               : out  std_logic;
          ddr_zq_req_o                : out  std_logic;
          ddr_sr_active_i             : in   std_logic;
          ddr_ref_ack_i               : in   std_logic;
          ddr_zq_ack_i                : in   std_logic;
          ddr_ui_clk_i                : in   std_logic;
          ddr_ui_clk_sync_rst_i       : in   std_logic;
          ddr_init_calib_complete_i   : in   std_logic;
      
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
          wb_stall_o : out std_logic;
          
          ----------------------------------------------------------------------------
          -- Wishbone bus port
          ----------------------------------------------------------------------------
          wb1_sel_i   : in  std_logic_vector(g_MASK_SIZE - 1 downto 0);
          wb1_cyc_i   : in  std_logic;
          wb1_stb_i   : in  std_logic;
          wb1_we_i    : in  std_logic;
          wb1_addr_i  : in  std_logic_vector(32 - 1 downto 0);
          wb1_data_i  : in  std_logic_vector(g_DATA_PORT_SIZE - 1 downto 0);
          wb1_data_o  : out std_logic_vector(g_DATA_PORT_SIZE - 1 downto 0);
          wb1_ack_o   : out std_logic;
          wb1_stall_o : out std_logic;       
          
          ----------------------------------------------------------------------------
          -- Debug ports
          ----------------------------------------------------------------------------
          ddr_wb_rd_mask_dout_do : out std_logic_vector(7 downto 0);
          ddr_wb_rd_mask_addr_dout_do : out std_logic_vector(g_BYTE_ADDR_WIDTH-1 downto 0);
          ddr_rd_mask_rd_data_count_do : out std_logic_vector(3 downto 0);
          ddr_rd_data_rd_data_count_do : out std_logic_vector(3 downto 0);
          ddr_rd_fifo_full_do : out std_logic_vector(1 downto 0);
          ddr_rd_fifo_empty_do : out std_logic_vector(1 downto 0);
          ddr_rd_fifo_rd_do : out std_logic_vector(1 downto 0)
        );
    end component ddr3_ctrl_wb;
    
	component wb_tx_core
    generic (
        g_NUM_TX : integer range 1 to 32 := c_TX_CHANNELS
    );
    port (
        -- Sys connect
        wb_clk_i    : in  std_logic;
        rst_n_i        : in  std_logic;

        -- Wishbone slave interface
        wb_adr_i    : in  std_logic_vector(31 downto 0);
        wb_dat_i    : in  std_logic_vector(31 downto 0);
        wb_dat_o    : out std_logic_vector(31 downto 0);
        wb_cyc_i    : in  std_logic;
        wb_stb_i    : in  std_logic;
        wb_we_i        : in  std_logic;
        wb_ack_o    : out std_logic;
        wb_stall_o    : out std_logic;

        -- TX
        tx_clk_i    : in  std_logic;
        tx_data_o    : out std_logic_vector(g_NUM_TX-1 downto 0);
        trig_pulse_o : out std_logic;

        -- TRIGGER
        ext_trig_i : in std_logic
    );
    end component;
    
	component wb_rx_core
        generic (
            g_NUM_RX : integer range 1 to 32 := c_RX_CHANNELS;
            g_TYPE : string := c_FE_TYPE;
            g_NUM_LANES : integer range 1 to 4 := c_RX_NUM_LANES
        );
        port (
            -- Sys connect
            wb_clk_i    : in  std_logic;
            rst_n_i        : in  std_logic;
            -- Wishbone slave interface
            wb_adr_i    : in  std_logic_vector(31 downto 0);
            wb_dat_i    : in  std_logic_vector(31 downto 0);
            wb_dat_o    : out std_logic_vector(31 downto 0);
            wb_cyc_i    : in  std_logic;
            wb_stb_i    : in  std_logic;
            wb_we_i        : in  std_logic;
            wb_ack_o    : out std_logic;
            wb_stall_o    : out std_logic;
            -- RX IN
            rx_clk_i    : in  std_logic;
            rx_serdes_clk_i : in std_logic;
            rx_data_i_p    : in std_logic_vector((g_NUM_RX*g_NUM_LANES)-1 downto 0);
            rx_data_i_n    : in std_logic_vector((g_NUM_RX*g_NUM_LANES)-1 downto 0);
            trig_tag_i : in std_logic_vector(31 downto 0);
            -- RX OUT (sync to sys_clk)
            rx_valid_o : out std_logic;
            rx_data_o : out std_logic_vector(63 downto 0);
            busy_o : out std_logic;
            debug_o : out std_logic_vector(31 downto 0)
        );
    end component;
 

        component wb_rx_bridge is
        port (
            -- Sys Connect
            sys_clk_i        : in  std_logic;
            rst_n_i            : in  std_logic;
            -- Wishbone slave interface
            wb_adr_i    : in  std_logic_vector(31 downto 0);
            wb_dat_i    : in  std_logic_vector(31 downto 0);
            wb_dat_o    : out std_logic_vector(31 downto 0);
            wb_cyc_i    : in  std_logic;
            wb_stb_i    : in  std_logic;
            wb_we_i        : in  std_logic;
            wb_ack_o    : out std_logic;
            wb_stall_o    : out std_logic;
            -- Wishbone DMA Master Interface
            dma_clk_i    : in  std_logic;
            dma_adr_o    : out std_logic_vector(31 downto 0);
            dma_dat_o    : out std_logic_vector(63 downto 0);
            dma_dat_i    : in  std_logic_vector(63 downto 0);
            dma_cyc_o    : out std_logic;
            dma_stb_o    : out std_logic;
            dma_we_o    : out std_logic;
            dma_ack_i    : in  std_logic;
            dma_stall_i    : in  std_logic;
            -- Rx Interface
            rx_data_i     : in  std_logic_vector(63 downto 0);
            rx_valid_i    : in  std_logic;
            -- Status in
            trig_pulse_i : in std_logic;
            -- Status out
            irq_o        : out std_logic;
            busy_o        : out std_logic
        );
        end component;
        
        component i2c_master_wb_top
            port (
                  wb_clk_i : in  std_logic;
                  wb_rst_i : in  std_logic;
                  arst_i   : in  std_logic;
                  wb_adr_i : in  std_logic_vector(2 downto 0);
                  wb_dat_i : in  std_logic_vector(7 downto 0);
                  wb_dat_o : out std_logic_vector(7 downto 0);
                  wb_we_i  : in  std_logic;
                  wb_stb_i : in  std_logic;
                  wb_cyc_i : in  std_logic;
                  wb_ack_o : out std_logic;
                  wb_inta_o: out std_logic;
                  scl      : inout std_logic;
                  sda      : inout std_logic
                  );
        end component;
        
        component wb_trigger_logic
            port (
                -- Sys connect
                wb_clk_i    : in  std_logic;
                rst_n_i        : in  std_logic;
                
                -- Wishbone slave interface
                wb_adr_i    : in  std_logic_vector(31 downto 0);
                wb_dat_i    : in  std_logic_vector(31 downto 0);
                wb_dat_o    : out std_logic_vector(31 downto 0);
                wb_cyc_i    : in  std_logic;
                wb_stb_i    : in  std_logic;
                wb_we_i        : in  std_logic;
                wb_ack_o    : out std_logic;
                
                -- To/From outside world
                ext_trig_i : in std_logic_vector(3 downto 0);
                ext_trig_o : out std_logic;
                ext_busy_i : in std_logic;
                ext_busy_o : out std_logic;
                
                -- Eudet TLU
                eudet_clk_o : out std_logic;
                eudet_busy_o : out std_logic;
                eudet_trig_i : in std_logic;
                eudet_rst_i : in std_logic;
    
                -- To/From inside world
                clk_i : in std_logic;
                --int_trig_i : in std_logic_vector(3 downto 0);
                --int_trig_o : out std_logic;
                --int_busy_i : in std_logic;
                trig_tag : out std_logic_vector(31 downto 0);
                debug_o : out std_logic_vector(31 downto 0)
            );
        end component;

   
    COMPONENT ila_axis
    
        PORT (
            clk : IN STD_LOGIC;
        
        
        
            probe0 : IN STD_LOGIC_VECTOR(63 DOWNTO 0); 
            probe1 : IN STD_LOGIC_VECTOR(7 DOWNTO 0); 
            probe2 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
            probe3 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
            probe4 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
            probe5 : IN STD_LOGIC_VECTOR(63 DOWNTO 0); 
            probe6 : IN STD_LOGIC_VECTOR(7 DOWNTO 0); 
            probe7 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
            probe8 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
            probe9 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
            probe10 : IN STD_LOGIC_VECTOR(21 DOWNTO 0); 
            probe11 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
            probe12 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
            probe13 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
            probe14 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
            probe15 : IN STD_LOGIC_VECTOR(0 DOWNTO 0);
            probe16 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
            probe17 : IN STD_LOGIC_VECTOR(0 DOWNTO 0);
            probe18 : IN STD_LOGIC_VECTOR(0 DOWNTO 0);
            probe19 : IN STD_LOGIC_VECTOR(0 DOWNTO 0);
            probe20 : IN STD_LOGIC_VECTOR(0 DOWNTO 0);
            probe21 : IN STD_LOGIC_VECTOR(2 DOWNTO 0);
            probe22 : IN STD_LOGIC_VECTOR(0 DOWNTO 0);
            probe23 : IN STD_LOGIC_VECTOR(28 DOWNTO 0)
        );
    END COMPONENT  ;
    
COMPONENT ila_wsh_pipe
    
    PORT (
        clk : IN STD_LOGIC;
    
    
    
        probe0 : IN STD_LOGIC_VECTOR(31 DOWNTO 0); 
        probe1 : IN STD_LOGIC_VECTOR(63 DOWNTO 0); 
        probe2 : IN STD_LOGIC_VECTOR(63 DOWNTO 0); 
        probe3 : IN STD_LOGIC_VECTOR(7 DOWNTO 0); 
        probe4 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
        probe5 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
        probe6 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
        probe7 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
        probe8 : IN STD_LOGIC_VECTOR(0 DOWNTO 0)--; 
--        probe9 : IN STD_LOGIC_VECTOR(31 DOWNTO 0); 
--        probe10 : IN STD_LOGIC_VECTOR(63 DOWNTO 0); 
--        probe11 : IN STD_LOGIC_VECTOR(63 DOWNTO 0); 
--        probe12 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
--        probe13 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
--        probe14 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
--        probe15 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
--        probe16 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
--        probe17 : IN STD_LOGIC_VECTOR(31 DOWNTO 0); 
--        probe18 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
--        probe19 : IN STD_LOGIC_VECTOR(0 DOWNTO 0);
--        probe20 : IN STD_LOGIC_VECTOR(0 DOWNTO 0)
    );
    END COMPONENT  ;
    
    
    COMPONENT ila_ddr
    
    PORT (
        clk : IN STD_LOGIC;
    
    
    
        probe0 : IN STD_LOGIC_VECTOR(28 DOWNTO 0); 
        probe1 : IN STD_LOGIC_VECTOR(2 DOWNTO 0); 
        probe2 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
        probe3 : IN STD_LOGIC_VECTOR(511 DOWNTO 0); 
        probe4 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
        probe5 : IN STD_LOGIC_VECTOR(63 DOWNTO 0); 
        probe6 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
        probe7 : IN STD_LOGIC_VECTOR(511 DOWNTO 0); 
        probe8 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
        probe9 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
        probe10 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
        probe11 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
        probe12 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
        probe13 : IN STD_LOGIC_VECTOR(0 DOWNTO 0)
    );
    END COMPONENT  ;

    COMPONENT ila_rx_dma_wb
    
    PORT (
        clk : IN STD_LOGIC;
    
    
    
        probe0 : IN STD_LOGIC_VECTOR(31 DOWNTO 0); 
        probe1 : IN STD_LOGIC_VECTOR(63 DOWNTO 0); 
        probe2 : IN STD_LOGIC_VECTOR(63 DOWNTO 0); 
        probe3 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
        probe4 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
        probe5 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
        probe6 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
        probe7 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
        probe8 : IN STD_LOGIC_VECTOR(31 DOWNTO 0); 
        probe9 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
        probe10 : IN STD_LOGIC_VECTOR(0 DOWNTO 0);
        probe11 : IN STD_LOGIC_VECTOR(0 DOWNTO 0)
    );
    END COMPONENT  ;

end app_pkg;


package body app_pkg is

end app_pkg;
