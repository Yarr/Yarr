----------------------------------------------------------------------------------
-- Company: 
-- Engineer: 
-- 
-- Create Date: 11/18/2016 01:10:56 PM
-- Design Name: 
-- Module Name: app - Behavioral
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
use ieee.std_logic_unsigned.all;

-- Uncomment the following library declaration if using
-- arithmetic functions with Signed or Unsigned values
use IEEE.NUMERIC_STD.ALL;

-- Uncomment the following library declaration if instantiating
-- any Xilinx leaf cells in this code.
library UNISIM;
use UNISIM.VComponents.all;

library work;
use work.app_pkg.all;

entity app is
    Generic(
        g_TX_CHANNELS : integer := 8;
        g_RX_CHANNELS : integer := 8;
        AXI_BUS_WIDTH : integer := 64;
        axis_data_width_c : integer := 64;
        axis_rx_tkeep_width_c : integer := 64/8;
        axis_rx_tuser_width_c : integer := 22;
        wb_address_width_c : integer := 32;
        wb_data_width_c : integer := 32;
        address_mask_c : STD_LOGIC_VECTOR(32-1 downto 0) := X"000FFFFF";
        DMA_MEMORY_SELECTED : string := "DDR3" -- DDR3, BRAM, DEMUX
        );
    Port ( clk_i : in STD_LOGIC;
           sys_clk_n_i : IN STD_LOGIC;
           sys_clk_p_i : IN STD_LOGIC;
           rst_i : in STD_LOGIC;
           user_lnk_up_i : in STD_LOGIC;
           user_app_rdy_i : in STD_LOGIC;
           
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
           
           -- PCIe ID
           cfg_bus_number_i : in STD_LOGIC_VECTOR(7 DOWNTO 0);
           cfg_device_number_i : in STD_LOGIC_VECTOR(4 DOWNTO 0);
           cfg_function_number_i : in STD_LOGIC_VECTOR(2 DOWNTO 0);
           
           -- PCIe debug
           tx_err_drop_i: in STD_LOGIC;
           cfg_dstatus_i : in STD_LOGIC_VECTOR(15 DOWNTO 0);
           
           --DDR3
           ddr3_dq_io       : inout std_logic_vector(63 downto 0);
           ddr3_dqs_p_io    : inout std_logic_vector(7 downto 0);
           ddr3_dqs_n_io    : inout std_logic_vector(7 downto 0);
           --init_calib_complete_o : out std_logic;
     
           ddr3_addr_o     : out   std_logic_vector(14 downto 0);
           ddr3_ba_o       : out   std_logic_vector(2 downto 0);
           ddr3_ras_n_o    : out   std_logic;
           ddr3_cas_n_o    : out   std_logic;
           ddr3_we_n_o     : out   std_logic;
           ddr3_reset_n_o  : out   std_logic;
           ddr3_ck_p_o     : out   std_logic_vector(0 downto 0);
           ddr3_ck_n_o    : out   std_logic_vector(0 downto 0);
           ddr3_cke_o      : out   std_logic_vector(0 downto 0);
           ddr3_cs_n_o     : out   std_logic_vector(0 downto 0);
           ddr3_dm_o       : out   std_logic_vector(7 downto 0);
           ddr3_odt_o      : out   std_logic_vector(0 downto 0);
           
           --I/O
           usr_sw_i : in STD_LOGIC_VECTOR (2 downto 0);
           usr_led_o : out STD_LOGIC_VECTOR (3 downto 0);
           front_led_o : out STD_LOGIC_VECTOR (3 downto 0)
           );
end app;

architecture Behavioral of app is
    
    
    
    ------------------------------------------------------------------------------
    -- Constants declaration
    ------------------------------------------------------------------------------
    constant DEBUG_C : std_logic_vector(5 downto 0) := "000000";
    
    --TODO
    constant c_BAR0_APERTURE    : integer := 18;  -- nb of bits for 32-bit word address
    constant c_CSR_WB_SLAVES_NB : integer := 16; -- upper 4 bits used for addressing slave
    
    constant c_TX_CHANNELS : integer := g_TX_CHANNELS;
    constant c_RX_CHANNELS : integer := g_RX_CHANNELS;
  
    ------------------------------------------------------------------------------
    -- Signals declaration
    ------------------------------------------------------------------------------

    
    signal rst_n_s : std_logic;
    signal led_count_s : STD_LOGIC_VECTOR (28 downto 0);
    signal iteration_count_s : STD_LOGIC_VECTOR (28 downto 0);
    signal gray_iteration_count_s : STD_LOGIC_VECTOR (28 downto 0); 
    signal ddr_iteration_count_s : STD_LOGIC_VECTOR (28 downto 0);
    
    signal rst_640_s : std_logic;
    
    signal clk_640_s : std_logic;
    signal clk_160_s : std_logic;
    signal clk_80_s : std_logic;
    signal clk_40_s : std_logic;
    signal clk_40_90_s : std_logic;
    signal pll_locked_s : std_logic;
    
    
    --signal eop_s : std_logic; -- Arbiter end of operation
    signal cfg_interrupt_s : std_logic;
    signal pcie_id_s : std_logic_vector (15 downto 0); -- Completer/Requester ID
    

    
	---------------------------------------------------------
    -- CSR Wishbone bus
    signal csr_adr_s : STD_LOGIC_VECTOR (32 - 1 downto 0);
    signal csr_dat_m2s_s : STD_LOGIC_VECTOR (wb_data_width_c - 1 downto 0);
    signal csr_dat_s2m_s : STD_LOGIC_VECTOR (wb_data_width_c - 1 downto 0);
    signal csr_cyc_s : STD_LOGIC;
    signal csr_sel_s : STD_LOGIC_VECTOR (4 - 1 downto 0);
    signal csr_stb_s : STD_LOGIC;
    signal csr_we_s : STD_LOGIC;
    signal csr_ack_s : STD_LOGIC;
    signal csr_stall_s : std_logic;                      -- Stall
    signal csr_err_s   : std_logic;                      -- Error
    signal csr_rty_s   : std_logic;                      -- Retry
    signal csr_int_s   : std_logic;                       -- Interrupt

     -- CSR wishbone bus (slaves)
    signal wb_adr_s   : std_logic_vector(31 downto 0);
    signal wb_dat_s2m_s : std_logic_vector((32*c_CSR_WB_SLAVES_NB)-1 downto 0) := (others => '0');
    signal wb_dat_m2s_s : std_logic_vector(31 downto 0);
    signal wb_sel_s   : std_logic_vector(3 downto 0);
    signal wb_cyc_s   : std_logic_vector(c_CSR_WB_SLAVES_NB-1 downto 0) := (others => '0');
    signal wb_stb_s   : std_logic;
    signal wb_we_s    : std_logic;
    signal wb_ack_s   : std_logic_vector(c_CSR_WB_SLAVES_NB-1 downto 0) := (others => '0');
    signal wb_stall_s : std_logic_vector(c_CSR_WB_SLAVES_NB-1 downto 0) := (others => '0');    

	---------------------------------------------------------
    -- CSR Wishbone bus
    signal dma_reg_adr_s : STD_LOGIC_VECTOR (32 - 1 downto 0);
    signal dma_reg_dat_m2s_s : STD_LOGIC_VECTOR (wb_data_width_c - 1 downto 0);
    signal dma_reg_dat_s2m_s : STD_LOGIC_VECTOR (wb_data_width_c - 1 downto 0);
    signal dma_reg_cyc_s : STD_LOGIC;
    signal dma_reg_sel_s : STD_LOGIC_VECTOR (4 - 1 downto 0);
    signal dma_reg_stb_s : STD_LOGIC;
    signal dma_reg_we_s : STD_LOGIC;
    signal dma_reg_ack_s : STD_LOGIC;
    signal dma_reg_stall_s : std_logic;                      -- Stall
    signal dma_reg_err_s   : std_logic;                      -- Error
    signal dma_reg_rty_s   : std_logic;                      -- Retry
    signal dma_reg_int_s   : std_logic;                       -- Interrupt    
    
    
	---------------------------------------------------------
    -- Slave AXI-Stream from arbiter to pcie_tx
    signal s_axis_rx_tdata_s : STD_LOGIC_VECTOR (axis_data_width_c - 1 downto 0);
    signal s_axis_rx_tkeep_s : STD_LOGIC_VECTOR (axis_data_width_c/8 - 1 downto 0);
    signal s_axis_rx_tuser_s : STD_LOGIC_VECTOR (21 downto 0);
    signal s_axis_rx_tlast_s : STD_LOGIC;
    signal s_axis_rx_tvalid_s :STD_LOGIC;
    signal s_axis_rx_tready_s : STD_LOGIC;
    
	---------------------------------------------------------
	-- Master AXI-Stream pcie_rx to wishbone master
    signal m_axis_tx_tdata_s : STD_LOGIC_VECTOR (axis_data_width_c - 1 downto 0);
    signal m_axis_tx_tkeep_s : STD_LOGIC_VECTOR (axis_data_width_c/8 - 1 downto 0);
    signal m_axis_tx_tuser_s : STD_LOGIC_VECTOR (3 downto 0);
    signal m_axis_tx_tlast_s : STD_LOGIC;
    signal m_axis_tx_tvalid_s : STD_LOGIC;
    signal m_axis_tx_tready_s : STD_LOGIC;
    
    

	
	

	

	
	---------------------------------------------------------
	-- DMA Interface (Pipelined Wishbone)
	signal dma_adr_s   :  std_logic_vector(31 downto 0);  -- Adress
	signal dma_dat_s2m_s   :  std_logic_vector(63 downto 0);  -- Data in
	signal dma_dat_m2s_s   :  std_logic_vector(63 downto 0);  -- Data out
	signal dma_sel_s   :  std_logic_vector(7 downto 0);   -- Byte select
	signal dma_cyc_s   :  std_logic;                      -- Read or write cycle
	signal dma_stb_s   :  std_logic;                      -- Read or write strobe
	signal dma_we_s    :  std_logic;                      -- Write
	signal dma_ack_s   :  std_logic;                      -- Acknowledge
	signal dma_stall_s :  std_logic;                      -- for pipelined Wishbone	
	

	
	
	signal dummyram_sel_s      : std_logic;
    signal ddr3ram_sel_s       : std_logic;
    signal dummyaddress_sel_s      : std_logic;
    signal dummydeadbeef_sel_s     : std_logic;
	
	---------------------------------------------------------
    -- From DMA master to Dummy RAM
    signal dma_bram_adr_s   :  std_logic_vector(32-1 downto 0);       -- Adress
    signal dma_bram_dat_s2m_s   :  std_logic_vector(64-1 downto 0);   -- Data in
    signal dma_bram_dat_m2s_s   :  std_logic_vector(64-1 downto 0);   -- Data out
    signal dma_bram_sel_s   :  std_logic_vector(8-1 downto 0);        -- Byte select
    signal dma_bram_cyc_s   :  std_logic;                             -- Read or write cycle
    signal dma_bram_stb_s   :  std_logic;                             -- Read or write strobe
    signal dma_bram_we_s    :  std_logic;                             -- Write
    signal dma_bram_ack_s   :  std_logic;                             -- Acknowledge
    signal dma_bram_stall_s :  std_logic;                             -- for pipelined Wishbone  
    
    
    ---------------------------------------------------------
    -- From DMA master to DDR3 control
    signal dma_ddr_addr_s   :  std_logic_vector(32-1 downto 0);  -- Adress
    signal dma_ddr_dat_s2m_s   :  std_logic_vector(64-1 downto 0);  -- Data in
    signal dma_ddr_dat_m2s_s   :  std_logic_vector(64-1 downto 0);  -- Data out
    signal dma_ddr_sel_s   :  std_logic_vector(8-1 downto 0);   -- Byte select
    signal dma_ddr_cyc_s   :  std_logic;  -- Read or write cycle
    signal dma_ddr_stb_s   :  std_logic;                      -- Read or write strobe
    signal dma_ddr_we_s    :  std_logic;                      -- Write
    signal dma_ddr_ack_s   :  std_logic; -- Acknowledge
    signal dma_ddr_stall_s :  std_logic;                      -- for pipelined Wishbone      
	
	
	---------------------------------------------------------
	-- DDR3 control to output	
	signal ddr3_dq_s       : std_logic_vector(63 downto 0);
    signal ddr3_dqs_p_s    : std_logic_vector(7 downto 0);
    signal ddr3_dqs_n_s    : std_logic_vector(7 downto 0);
    
    signal init_calib_complete_s  : std_logic;
  
    signal ddr3_addr_s     : std_logic_vector(14 downto 0);
    signal ddr3_ba_s       : std_logic_vector(2 downto 0);
    signal ddr3_ras_n_s    : std_logic;
    signal ddr3_cas_n_s    : std_logic;
    signal ddr3_we_n_s     : std_logic;
    signal ddr3_reset_n_s  : std_logic;
    signal ddr3_ck_p_s     : std_logic_vector(0 downto 0);
    signal ddr3_ck_n_s     : std_logic_vector(0 downto 0);
    signal ddr3_cke_s      : std_logic_vector(0 downto 0);
    signal ddr3_cs_n_s     : std_logic_vector(0 downto 0);
    signal ddr3_dm_s       : std_logic_vector(7 downto 0);
    signal ddr3_odt_s      : std_logic_vector(0 downto 0);


	---------------------------------------------------------
	-- DDR3 control to MIG    
    signal ddr_app_addr_s                  :     std_logic_vector(28 downto 0);
    signal ddr_app_cmd_s                   :     std_logic_vector(2 downto 0);
    signal ddr_app_cmd_en_s                :     std_logic;
    signal ddr_app_wdf_data_s              :     std_logic_vector(511 downto 0);
    signal ddr_app_wdf_end_s               :     std_logic;
    signal ddr_app_wdf_mask_s              :     std_logic_vector(63 downto 0);
    signal ddr_app_wdf_wren_s              :     std_logic;
    signal ddr_app_rd_data_s               :     std_logic_vector(511 downto 0);
    signal ddr_app_rd_data_end_s           :     std_logic;
    signal ddr_app_rd_data_valid_s         :     std_logic;
    signal ddr_app_rdy_s                   :     std_logic;
    signal ddr_app_wdf_rdy_s               :     std_logic;
    signal ddr_app_ui_clk_s                :     std_logic;
    signal ddr_app_ui_clk_sync_rst_s       :     std_logic;
    
    ----------------------------------------------------------------------------
    -- DDR3 Debug signalss
    signal ddr_rd_fifo_full_ds : std_logic_vector(1 downto 0);
    signal ddr_rd_fifo_empty_ds : std_logic_vector(1 downto 0);
    signal ddr_rd_fifo_rd_ds : std_logic_vector(1 downto 0);
    signal ddr_rd_mask_rd_data_count_ds : std_logic_vector(3 downto 0);
    signal ddr_rd_data_rd_data_count_ds : std_logic_vector(3 downto 0);
    signal ddr_wb_rd_mask_dout_ds : std_logic_vector(7 downto 0);
    signal ddr_wb_rd_mask_addr_dout_ds : std_logic_vector(29-1 downto 0);
   
	
	---------------------------------------------------------
	-- From L2P DMA master (ldm) to arbiter (arb)
	signal ldm_arb_tdata_s : std_logic_vector (axis_data_width_c - 1 downto 0);
	signal ldm_arb_tkeep_s : std_logic_vector (axis_data_width_c/8 - 1 downto 0);
	signal ldm_arb_tlast_s : std_logic;
	signal ldm_arb_tvalid_s : std_logic;
	signal ldm_arb_tready_s : std_logic;
	signal ldm_arb_req_s    : std_logic;
	--signal arb_ldm_gnt_s : std_logic;

begin
    
    rst_n_s <= not rst_i;
    

    
    s_axis_rx_tdata_s <= s_axis_rx_tdata_i;
    s_axis_rx_tkeep_s <= s_axis_rx_tkeep_i;
    s_axis_rx_tlast_s <= s_axis_rx_tlast_i;
    s_axis_rx_tready_o <= s_axis_rx_tready_s;
    s_axis_rx_tuser_s <= s_axis_rx_tuser_i;
    s_axis_rx_tvalid_s <= s_axis_rx_tvalid_i;
    -- Master AXI-Stream
    m_axis_tx_tdata_o <= m_axis_tx_tdata_s;
    m_axis_tx_tkeep_o <= m_axis_tx_tkeep_s;
    m_axis_tx_tuser_o <= m_axis_tx_tuser_s;
    m_axis_tx_tlast_o <= m_axis_tx_tlast_s;
    m_axis_tx_tvalid_o <= m_axis_tx_tvalid_s;
    m_axis_tx_tready_s <= m_axis_tx_tready_i;
    
    
    --cfg_interrupt_assert_o <= '0';
    --cfg_interrupt_di_o <= (others => '0');
    --cfg_interrupt_stat_o <= '0';
    --cfg_pciecap_interrupt_msgnum_o <= (others => '0');
    
    --cfg_interrupt_o <= cfg_interrupt_s;
    
    pcie_id_s <= cfg_bus_number_i & cfg_device_number_i & cfg_function_number_i;
    

    

    
    clk_gen_cmp : clk_gen
       port map ( 
       -- Clock in ports
       clk_250_in => clk_i,
      -- Clock out ports  
       clk_640 => clk_640_s,
       clk_160 => clk_160_s,
       clk_80 => clk_80_s,
       clk_40 => clk_40_s,
       clk_40_90 => clk_40_90_s,
      -- Status and control signals                
       reset => rst_i,
       locked => pll_locked_s            
     );

    led_cnt:simple_counter
    port map(
	    enable_i => '1',
        rst_i => '0',
        clk_i => clk_640_s,
        count_o =>  led_count_s,
        gray_count_o => open
    );
    


    
--    iterations_cnt:simple_counter
--    port map(
--	    enable_i => dma_ctrl_irq_s(0),
--        rst_i => rst_i,
--        clk_i => clk_i,
--        count_o =>  iteration_count_s,
--        gray_count_o => gray_iteration_count_s
--    );

    

   
    wb_exp_comp:wshexp_core
        Port map( 
            clk_i => clk_i,
            rst_i => rst_i,
            
            ---------------------------------------------------------
            -- AXI-Stream bus
            m_axis_tx_tready_i => m_axis_tx_tready_s,
            m_axis_tx_tdata_o => m_axis_tx_tdata_s,
            m_axis_tx_tkeep_o => m_axis_tx_tkeep_s,
            m_axis_tx_tlast_o => m_axis_tx_tlast_s,
            m_axis_tx_tvalid_o => m_axis_tx_tvalid_s,
            m_axis_tx_tuser_o => m_axis_tx_tuser_s,
            s_axis_rx_tdata_i => s_axis_rx_tdata_s,
            s_axis_rx_tkeep_i => s_axis_rx_tkeep_s,
            s_axis_rx_tlast_i => s_axis_rx_tlast_s,
            s_axis_rx_tvalid_i => s_axis_rx_tvalid_s,
            s_axis_rx_tready_o => s_axis_rx_tready_s,
            s_axis_rx_tuser_i => s_axis_rx_tuser_s,
    
            ---------------------------------------------------------
            -- DMA wishbone interface (master pipelined)        
            dma_adr_o   => dma_adr_s,
            dma_dat_o   => dma_dat_m2s_s,
            dma_dat_i   => dma_dat_s2m_s,
            dma_sel_o   => dma_sel_s,
            dma_cyc_o   => dma_cyc_s,
            dma_stb_o   => dma_stb_s,
            dma_we_o    => dma_we_s,
            dma_ack_i   => dma_ack_s,
            dma_stall_i => dma_stall_s,
            
            ---------------------------------------------------------
            -- CSR wishbone interface (master classic)
            csr_adr_o   => csr_adr_s,
            csr_dat_o   => csr_dat_m2s_s,
            csr_sel_o   => csr_sel_s,
            csr_stb_o   => csr_stb_s,
            csr_we_o    => csr_we_s,
            csr_cyc_o   => csr_cyc_s,
            csr_dat_i   => csr_dat_s2m_s,
            csr_ack_i   => csr_ack_s,
            csr_stall_i => csr_stall_s,
            csr_err_i   => csr_err_s,
            csr_rty_i   => csr_rty_s,
            csr_int_i   => csr_int_s,
            
            ---------------------------------------------------------
            -- DMA registers wishbone interface (slave classic)
            dma_reg_adr_i   => wb_adr_s,
            dma_reg_dat_i   => wb_dat_m2s_s,
            dma_reg_sel_i   => wb_sel_s,
            dma_reg_stb_i   => wb_stb_s,
            dma_reg_we_i    => wb_we_s,
            dma_reg_cyc_i   => wb_cyc_s(0),
            dma_reg_dat_o   => wb_dat_s2m_s(31 downto 0),
            dma_reg_ack_o   => wb_ack_s(0),
            dma_reg_stall_o => wb_stall_s(0),
            
            ---------------------------------------------------------
            -- PCIe interrupt config
            cfg_interrupt_o => cfg_interrupt_o,
            cfg_interrupt_rdy_i => cfg_interrupt_rdy_i,
            cfg_interrupt_assert_o => cfg_interrupt_assert_o,
            cfg_interrupt_di_o => cfg_interrupt_di_o,
            cfg_interrupt_do_i => cfg_interrupt_do_i,
            cfg_interrupt_mmenable_i => cfg_interrupt_mmenable_i,
            cfg_interrupt_msienable_i => cfg_interrupt_msienable_i,
            cfg_interrupt_msixenable_i => cfg_interrupt_msixenable_i,
            cfg_interrupt_msixfm_i => cfg_interrupt_msixfm_i,
            cfg_interrupt_stat_o => cfg_interrupt_stat_o,
            cfg_pciecap_interrupt_msgnum_o => cfg_pciecap_interrupt_msgnum_o,
            
            ---------------------------------------------------------
            -- PCIe ID
            cfg_bus_number_i => cfg_bus_number_i,
            cfg_device_number_i => cfg_device_number_i,
            cfg_function_number_i => cfg_function_number_i
        );

    
   
    

    
    
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
            clk_i   => clk_i,
            rst_n_i => rst_n_s,
      
            ---------------------------------------------------------
            -- wishbone master interface
            wbm_adr_i   => csr_adr_s,
            wbm_dat_i   => csr_dat_m2s_s,
            wbm_sel_i   => csr_sel_s,
            wbm_stb_i   => csr_stb_s,
            wbm_we_i    => csr_we_s,
            wbm_cyc_i   => csr_cyc_s,
            wbm_dat_o   => csr_dat_s2m_s,
            wbm_ack_o   => csr_ack_s,
            wbm_stall_o => csr_stall_s,
      
            ---------------------------------------------------------
            -- wishbone slaves interface
            wb_adr_o   => wb_adr_s,
            wb_dat_o   => wb_dat_m2s_s,
            wb_sel_o   => wb_sel_s,
            wb_stb_o   => wb_stb_s,
            wb_we_o    => wb_we_s,
            wb_cyc_o   => wb_cyc_s,
            wb_dat_i   => wb_dat_s2m_s,
            wb_ack_i   => wb_ack_s,
            wb_stall_i => wb_stall_s
            );   

  
    dma_bram_gen : if DMA_MEMORY_SELECTED = "DEMUX" or DMA_MEMORY_SELECTED = "BRAM" generate
     dma_ram:k_bram
     generic map (
         ADDR_WIDTH => 9+4,
         DATA_WIDTH => 64 
     )
     port map (
         -- SYS CON
         clk            => clk_i,
         rst            => rst_i,
         
         -- Wishbone Slave in
         wb_adr_i    => dma_bram_adr_s(9+4 - 1 downto 0),
         wb_dat_i    => dma_bram_dat_m2s_s,
         wb_we_i        => dma_bram_we_s,
         wb_stb_i    => dma_bram_stb_s,
         wb_cyc_i    => dma_bram_cyc_s,
         wb_lock_i    => dma_bram_stb_s,
         
         -- Wishbone Slave out
         wb_dat_o    => dma_bram_dat_s2m_s,
         wb_ack_o    => dma_bram_ack_s
     );
     
  end generate dma_bram_gen;

  clk200_gen : if DMA_MEMORY_SELECTED = "BRAM" generate
  
   --LVDS input to internal single
    CLK_IBUFDS : IBUFDS
    generic map(
      IOSTANDARD => "DEFAULT"
    )
    port map(
      I  => sys_clk_p_i,
      IB => sys_clk_n_i,
      O  => open
    );
  
  end generate clk200_gen;

  dma_ddr3_gen : if DMA_MEMORY_SELECTED = "DEMUX" or DMA_MEMORY_SELECTED = "DDR3" generate
  cmp_ddr3_ctrl_wb : ddr3_ctrl_wb
    port map(
      rst_n_i             => rst_n_s,
      
      ddr_addr_o          => ddr_app_addr_s,
      ddr_cmd_o           => ddr_app_cmd_s,
      ddr_cmd_en_o        => ddr_app_cmd_en_s,
      ddr_wdf_data_o      => ddr_app_wdf_data_s,
      ddr_wdf_end_o       => ddr_app_wdf_end_s,
      ddr_wdf_mask_o      => ddr_app_wdf_mask_s,
      ddr_wdf_wren_o      => ddr_app_wdf_wren_s,
      ddr_rd_data_i       => ddr_app_rd_data_s,
      ddr_rd_data_end_i   => ddr_app_rd_data_end_s,
      ddr_rd_data_valid_i => ddr_app_rd_data_valid_s,
      ddr_rdy_i           => ddr_app_rdy_s,
      ddr_wdf_rdy_i       => ddr_app_wdf_rdy_s,
      ddr_ui_clk_i        => ddr_app_ui_clk_s,
      ddr_ui_clk_sync_rst_i => ddr_app_ui_clk_sync_rst_s,
      ddr_sr_req_o        => open,
      ddr_ref_req_o       => open,
      ddr_zq_req_o        => open,
      ddr_sr_active_i     => '1',
      ddr_ref_ack_i       => '1',
      ddr_zq_ack_i        => '1',
      ddr_init_calib_complete_i => '1',
      
      wb_clk_i            => clk_i,
      wb_sel_i            => dma_ddr_sel_s,
      wb_cyc_i            => dma_ddr_cyc_s,
      wb_stb_i            => dma_ddr_stb_s,
      wb_we_i             => dma_ddr_we_s,
      wb_addr_i           => dma_ddr_addr_s,
      wb_data_i           => dma_ddr_dat_m2s_s,
      wb_data_o           => dma_ddr_dat_s2m_s,
      wb_ack_o            => dma_ddr_ack_s,
      wb_stall_o          => dma_ddr_stall_s,
      
      ddr_wb_rd_mask_dout_do => ddr_wb_rd_mask_dout_ds,
      ddr_wb_rd_mask_addr_dout_do => ddr_wb_rd_mask_addr_dout_ds,
      ddr_rd_mask_rd_data_count_do => ddr_rd_mask_rd_data_count_ds,
      ddr_rd_data_rd_data_count_do => ddr_rd_data_rd_data_count_ds,
      ddr_rd_fifo_full_do => ddr_rd_fifo_full_ds,
      ddr_rd_fifo_empty_do => ddr_rd_fifo_empty_ds,
      ddr_rd_fifo_rd_do => ddr_rd_fifo_rd_ds
      
      );
      dma_ddr_sel_s <= (others => '1');
      
        u_mig_7series_0 : mig_7series_0
        port map (
            -- Memory interface ports
            ddr3_addr                      => ddr3_addr_s,
            ddr3_ba                        => ddr3_ba_s,
            ddr3_cas_n                     => ddr3_cas_n_s,
            ddr3_ck_n                      => ddr3_ck_n_s,
            ddr3_ck_p                      => ddr3_ck_p_s,
            ddr3_cke                       => ddr3_cke_s,
            ddr3_ras_n                     => ddr3_ras_n_s,
            ddr3_reset_n                   => ddr3_reset_n_s,
            ddr3_we_n                      => ddr3_we_n_s,
            ddr3_dq                        => ddr3_dq_s,
            ddr3_dqs_n                     => ddr3_dqs_n_s,
            ddr3_dqs_p                     => ddr3_dqs_p_s,
            init_calib_complete            => init_calib_complete_s,
            ddr3_cs_n                      => ddr3_cs_n_s,
            ddr3_dm                        => ddr3_dm_s,
            ddr3_odt                       => ddr3_odt_s,
            -- Application interface ports
            app_addr                       => ddr_app_addr_s,
            app_cmd                        => ddr_app_cmd_s,
            app_en                         => ddr_app_cmd_en_s,
            app_wdf_data                   => ddr_app_wdf_data_s,
            app_wdf_end                    => ddr_app_wdf_end_s,
            app_wdf_wren                   => ddr_app_wdf_wren_s,
            app_rd_data                    => ddr_app_rd_data_s,
            app_rd_data_end                => ddr_app_rd_data_end_s,
            app_rd_data_valid              => ddr_app_rd_data_valid_s,
            app_rdy                        => ddr_app_rdy_s,
            app_wdf_rdy                    => ddr_app_wdf_rdy_s,
            app_sr_req                     => '0',
            app_ref_req                    => '0',
            app_zq_req                     => '0',
            app_sr_active                  => open,
            app_ref_ack                    => open,
            app_zq_ack                     => open,
            ui_clk                         => ddr_app_ui_clk_s,
            ui_clk_sync_rst                => ddr_app_ui_clk_sync_rst_s,
            app_wdf_mask                   => ddr_app_wdf_mask_s,
            -- System Clock Ports
            sys_clk_p                       => sys_clk_p_i,
            sys_clk_n                       => sys_clk_n_i,
            sys_rst                        => rst_i
        );
     
    --DDR3
    ddr3_dq_io <= ddr3_dq_s;
    ddr3_dqs_p_io <= ddr3_dqs_p_s;
    ddr3_dqs_n_io <= ddr3_dqs_n_s;
    --init_calib_complete_o <= init_calib_complete_s;
    
    ddr3_addr_o <= ddr3_addr_s;
    ddr3_ba_o <= ddr3_ba_s;
    ddr3_ras_n_o <= ddr3_ras_n_s;
    ddr3_cas_n_o <= ddr3_cas_n_s;
    ddr3_we_n_o <= ddr3_we_n_s;
    ddr3_reset_n_o <= ddr3_reset_n_s;
    ddr3_ck_p_o <= ddr3_ck_p_s;
    ddr3_ck_n_o <= ddr3_ck_n_s;
    ddr3_cke_o <= ddr3_cke_s;
    ddr3_cs_n_o <= ddr3_cs_n_s;
    ddr3_dm_o <= ddr3_dm_s;
    ddr3_odt_o <= ddr3_odt_s;
    
    end generate dma_ddr3_gen;
  
     -- BRAM Wishbone Slave in
     dma_bram_adr_s <= dma_adr_s;
     dma_bram_dat_m2s_s <= dma_dat_m2s_s;
     dma_bram_we_s <= dma_we_s;
     dma_bram_stb_s <= dma_stb_s;
     
     -- DDR CTRL Wishbone Slave in
     dma_ddr_addr_s <= dma_adr_s;
     dma_ddr_dat_m2s_s <= dma_dat_m2s_s;
     dma_ddr_we_s <= dma_we_s;
     dma_ddr_stb_s <= dma_stb_s;
     
     
  dma_demux_gen : if DMA_MEMORY_SELECTED = "DEMUX"  generate
  dma_sel : process(clk_i,dummyram_sel_s, ddr3ram_sel_s, dummyaddress_sel_s, dummydeadbeef_sel_s,
                    dma_bram_dat_s2m_s,dma_bram_ack_s,dma_cyc_s,
                    dma_ddr_dat_s2m_s,dma_ddr_ack_s,
                    dma_ddr_dat_s2m_s,dma_ddr_ack_s)
  begin
  

  
    dma_dat_s2m_s <= (others => '0');
    dma_ack_s  <=  '0';
    dma_bram_cyc_s <= '0';
    dma_ddr_cyc_s <= '0';
    
    
    if(dummyram_sel_s = '1') then
        dma_dat_s2m_s <= dma_bram_dat_s2m_s;
        dma_ack_s  <=  dma_bram_ack_s;
        dma_bram_cyc_s <= dma_cyc_s;
    end if;
    
    if (ddr3ram_sel_s = '1') then
        dma_dat_s2m_s <= dma_ddr_dat_s2m_s;
        dma_ack_s  <=  dma_ddr_ack_s;
        dma_ddr_cyc_s <= dma_cyc_s;
    end if;
    
    if(dummyaddress_sel_s = '1') then
    
    end if;
    
    if(dummyaddress_sel_s = '1') then
    
    end if;
    
  end process dma_sel;
  end generate dma_demux_gen;
  
  dma_bramonly_gen : if DMA_MEMORY_SELECTED = "BRAM"  generate
        dma_dat_s2m_s <= dma_bram_dat_s2m_s;
        dma_ack_s  <=  dma_bram_ack_s;
        dma_bram_cyc_s <= dma_cyc_s;
  end generate dma_bramonly_gen;
  
  dma_ddr3only_gen : if DMA_MEMORY_SELECTED = "DDR3"  generate
        dma_dat_s2m_s <= dma_ddr_dat_s2m_s;
        dma_ack_s  <=  dma_ddr_ack_s;
        dma_ddr_cyc_s <= dma_cyc_s;
  end generate dma_ddr3only_gen;  
  

  

  
  usr_led_o <= led_count_s(28 downto 25);

  
  dbg_0 : if DEBUG_C(0) = '1' generate
      axis_debug : ila_axis
      PORT MAP (
          clk => clk_i,
      
      
      
          probe0 => s_axis_rx_tdata_s, 
          probe1 => s_axis_rx_tkeep_s, 
          probe2(0) => s_axis_rx_tlast_s, 
          probe3(0) => s_axis_rx_tvalid_s, 
          probe4(0) => s_axis_rx_tready_s, 
          probe5 => m_axis_tx_tdata_s, 
          probe6 => m_axis_tx_tkeep_s, 
          probe7(0) => m_axis_tx_tlast_s, 
          probe8(0) => m_axis_tx_tvalid_s,
          probe9(0) => m_axis_tx_tready_s,
          probe10 => s_axis_rx_tuser_i, 
          probe11(0) => '0',--dma_ctrl_start_l2p_s, 
          probe12(0) => '0',--dma_ctrl_start_p2l_s, 
          probe13(0) => '0',--dma_ctrl_start_next_s,
          probe14(0) => '0',--dma_ctrl_abort_s, 
          probe15(0) => '0',--dma_ctrl_done_s,
          probe16(0) => '0',--dma_ctrl_error_s,
          probe17(0) => user_lnk_up_i,
          probe18(0) => cfg_interrupt_s,
          probe19(0) => cfg_interrupt_rdy_i,
          probe20(0) => '0',--dma_ctrl_done_s,
          probe21 => (others => '0'),--wbm_arb_tready_s & wbm_arb_tready_s & ldm_arb_tready_s,--dma_ctrl_current_state_ds,
          probe22(0) => tx_err_drop_i,--next_item_valid_s
          probe23 => iteration_count_s
      );
  end generate dbg_0;
  
  
  
  dbg_2 : if DEBUG_C(2) = '1' generate
      pipelined_wishbone_debug : ila_wsh_pipe
      PORT MAP (
          clk => clk_i,
      
      
      
          probe0 => dma_adr_s, 
          probe1 => dma_dat_s2m_s, 
          probe2 => dma_dat_m2s_s, 
          probe3 => dma_sel_s, 
          probe4(0) => dma_cyc_s, 
          probe5(0) => dma_stb_s, 
          probe6(0) => dma_we_s, 
          probe7(0) => dma_ack_s,
          probe8(0) => dma_stall_s, 
          probe9(0) => '0',--l2p_dma_cyc_s,
          probe10(0) => '0',--p2l_dma_cyc_s,
          probe11(0) => '0',--dma_ctrl_start_l2p_s, 
          probe12(0) => '0',--dma_ctrl_start_p2l_s, 
          probe13(0) => '0',--dma_ctrl_start_next_s,
          probe14 => ddr_rd_mask_rd_data_count_ds,
          probe15 => ddr_rd_data_rd_data_count_ds,
          probe16 => (others => '0'),--ddr_wb_rd_mask_addr_dout_ds & ddr_wb_rd_mask_dout_ds,
          probe17 => iteration_count_s
      );
  end generate dbg_2;
  
  
  dbg_5 : if DEBUG_C(5) = '1' generate
    ddr_debug : ila_ddr
      PORT MAP (
          clk => ddr_app_ui_clk_s,
      
      
      
          probe0 => ddr_app_addr_s, 
          probe1 => ddr_app_cmd_s, 
          probe2(0) => ddr_app_cmd_en_s, 
          probe3 => ddr_app_wdf_data_s, 
          probe4(0) => ddr_app_wdf_end_s, 
          probe5 => ddr_app_wdf_mask_s, 
          probe6(0) => ddr_app_wdf_wren_s, 
          probe7 => ddr_app_rd_data_s, 
          probe8(0) => ddr_app_rd_data_end_s, 
          probe9(0) => ddr_app_rd_data_valid_s, 
          probe10(0) => ddr_app_rdy_s, 
          probe11(0) => ddr_app_wdf_rdy_s,
          probe12(0) => ddr_app_ui_clk_sync_rst_s, 
          probe13(0) => init_calib_complete_s,
          probe14 => ddr_iteration_count_s


      );
   end generate dbg_5;
  
end Behavioral;
