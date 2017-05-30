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
    
    constant DEBUG_C : std_logic_vector(5 downto 0) := "000000";
    

    
    signal rst_n_s : std_logic;
    signal count_s : STD_LOGIC_VECTOR (28 downto 0);
    signal eop_s : std_logic; -- Arbiter end of operation
    signal cfg_interrupt_s : std_logic;
    signal pcie_id_s : std_logic_vector (15 downto 0); -- Completer/Requester ID
    
    ---------------------------------------------------------
    -- debug signals
    signal wbm_states_ds : STD_LOGIC_VECTOR(3 downto 0);
    signal wbm_op_ds : STD_LOGIC_VECTOR(2 downto 0);
    signal wbm_header_type_ds : STD_LOGIC;
    signal wbm_payload_length_ds : STD_LOGIC_VECTOR(9 downto 0);
    signal wbm_address_ds : STD_LOGIC_VECTOR(31 downto 0);
    signal dma_ctrl_current_state_ds : std_logic_vector (2 downto 0);
    signal dma_ctrl_ds    : std_logic_vector(31 downto 0);
    signal dma_stat_ds    : std_logic_vector(31 downto 0);
    signal dma_attrib_ds  : std_logic_vector(31 downto 0);
    
	---------------------------------------------------------
    -- CSR Wishbone bus
    signal wb_adr_s : STD_LOGIC_VECTOR (32 - 1 downto 0);
    signal wb_dat_m2s_s : STD_LOGIC_VECTOR (wb_data_width_c - 1 downto 0);
    signal wb_dat_s2m_s : STD_LOGIC_VECTOR (wb_data_width_c - 1 downto 0);
    signal wb_cyc_s : STD_LOGIC;
    signal wb_sel_s : STD_LOGIC_VECTOR (4 - 1 downto 0);
    signal wb_stb_s : STD_LOGIC;
    signal wb_we_s : STD_LOGIC;
    signal wb_ack_s : STD_LOGIC;
    signal wb_stall_s : std_logic;                      -- Stall
    signal wb_err_s   : std_logic;                      -- Error
    signal wb_rty_s   : std_logic;                      -- Retry
    signal wb_int_s   : std_logic;                       -- Interrupt
    
    signal wb_dma_ctrl_adr_s : STD_LOGIC_VECTOR (32 - 1 downto 0);
    signal wb_dma_ctrl_dat_m2s_s : STD_LOGIC_VECTOR (wb_data_width_c - 1 downto 0);
    signal wb_dma_ctrl_dat_s2m_s : STD_LOGIC_VECTOR (wb_data_width_c - 1 downto 0);
    signal wb_dma_ctrl_cyc_s : STD_LOGIC;
    --signal wb_dma_ctrl_sel_s : STD_LOGIC_VECTOR (wb_data_width_c/8 - 1 downto 0);
    signal wb_dma_ctrl_stb_s : STD_LOGIC;
    signal wb_dma_ctrl_we_s : STD_LOGIC;
    signal wb_dma_ctrl_ack_s : STD_LOGIC;    
    
    signal wb_mem_adr_s : STD_LOGIC_VECTOR (32 - 1 downto 0);
    signal wb_mem_dat_m2s_s : STD_LOGIC_VECTOR (wb_data_width_c - 1 downto 0);
    signal wb_mem_dat_s2m_s : STD_LOGIC_VECTOR (wb_data_width_c - 1 downto 0);
    signal wb_mem_cyc_s : STD_LOGIC;
    --signal wb_mem_sel_s : STD_LOGIC_VECTOR (wb_data_width_c/8 - 1 downto 0);
    signal wb_mem_stb_s : STD_LOGIC;
    signal wb_mem_we_s : STD_LOGIC;
    signal wb_mem_ack_s : STD_LOGIC;
    
    
    signal wb_dbg_adr_s : STD_LOGIC_VECTOR (32 - 1 downto 0);
    signal wb_dbg_dat_m2s_s : STD_LOGIC_VECTOR (wb_data_width_c - 1 downto 0);
    signal wb_dbg_dat_s2m_s : STD_LOGIC_VECTOR (wb_data_width_c - 1 downto 0);
    signal wb_dbg_cyc_s : std_logic;
    signal wb_dbg_sel_s : STD_LOGIC_VECTOR (wb_data_width_c/8 - 1 downto 0);
    signal wb_dbg_stb_s : STD_LOGIC;
    signal wb_dbg_we_s : STD_LOGIC;
    signal wb_dbg_ack_s : STD_LOGIC;
    

    
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
    -- From Wishbone master (wbm) to L2P DMA    
    signal pd_wbm_address_s : STD_LOGIC_VECTOR(63 downto 0);
    signal pd_wbm_data_s : STD_LOGIC_VECTOR(31 downto 0);
    signal p2l_wbm_rdy_s : std_logic;
    signal pd_pdm_data_valid_w_s : std_logic_vector(1 downto 0);
    signal pd_wbm_valid_s : std_logic;
    signal pd_wbm_hdr_rid_s      : std_logic_vector(15 downto 0);  -- Requester ID
    signal pd_wbm_hdr_tag_s      : std_logic_vector(7 downto 0);
    signal pd_wbm_target_mrd_s : std_logic;                      -- Target memory read
    signal pd_wbm_target_mwr_s : std_logic; 
    signal wbm_pd_ready_s : std_logic;
    signal pd_op_s : STD_LOGIC_VECTOR(2 downto 0);
    signal pd_header_type_s : STD_LOGIC;
    signal pd_payload_length_s : STD_LOGIC_VECTOR(9 downto 0);
    
	---------------------------------------------------------
	-- From Wishbone master (wbm) to L2P DMA
	signal pd_pdm_data_valid_s : STD_LOGIC;
	signal pd_pdm_data_last_s : STD_LOGIC;
	signal pd_pdm_data_s : STD_LOGIC_VECTOR(axis_data_width_c - 1 downto 0);
	signal pd_pdm_keep_s : std_logic_vector(7 downto 0);
	signal p2l_dma_rdy_s : std_logic;
	
    ---------------------------------------------------------
    -- From Wishbone master (wbm) to arbiter (arb)      
    signal wbm_arb_tdata_s : std_logic_vector (axis_data_width_c - 1 downto 0);
    signal wbm_arb_tkeep_s : std_logic_vector (axis_data_width_c/8 - 1 downto 0);
    signal wbm_arb_tlast_s : std_logic;
    signal wbm_arb_tvalid_s : std_logic;
    signal wbm_arb_req_s    : std_logic;
    signal wbm_arb_tready_s : std_logic;
	
	signal dma_ctrl_irq_s : std_logic_vector(1 downto 0);
	
	---------------------------------------------------------
	-- To the L2P DMA master and P2L DMA master
	signal dma_ctrl_carrier_addr_s : std_logic_vector(31 downto 0);
	signal dma_ctrl_host_addr_h_s  : std_logic_vector(31 downto 0);
	signal dma_ctrl_host_addr_l_s  : std_logic_vector(31 downto 0);
	signal dma_ctrl_len_s          : std_logic_vector(31 downto 0);
	signal dma_ctrl_start_l2p_s    : std_logic;  -- To the L2P DMA master
	signal dma_ctrl_start_p2l_s    : std_logic;  -- To the P2L DMA master
	signal dma_ctrl_start_next_s   : std_logic;  -- To the P2L DMA master
	signal dma_ctrl_byte_swap_s    : std_logic_vector(1 downto 0);
	signal dma_ctrl_abort_s        : std_logic;
	signal dma_ctrl_done_s         : std_logic;
	signal dma_ctrl_error_s        : std_logic;
	signal dma_ctrl_l2p_done_s         : std_logic;
	signal dma_ctrl_l2p_error_s        : std_logic;
	signal dma_ctrl_p2l_done_s         : std_logic;
	signal dma_ctrl_p2l_error_s        : std_logic;
	
	---------------------------------------------------------
	-- From P2L DMA master
	signal next_item_carrier_addr_s : std_logic_vector(31 downto 0);
	signal next_item_host_addr_h_s  : std_logic_vector(31 downto 0);
	signal next_item_host_addr_l_s  : std_logic_vector(31 downto 0);
	signal next_item_len_s          : std_logic_vector(31 downto 0);
	signal next_item_next_l_s       : std_logic_vector(31 downto 0);
	signal next_item_next_h_s       : std_logic_vector(31 downto 0);
	signal next_item_attrib_s       : std_logic_vector(31 downto 0);
	signal next_item_valid_s        : std_logic;
	
	
	---------------------------------------------------------
	-- To the P2L Interface (send the DMA Master Read request)
	signal pdm_arb_tvalid_s  : std_logic;  -- Read completion signals
	signal pdm_arb_tlast_s : std_logic;  -- Toward the arbiter
	signal pdm_arb_tdata_s   : std_logic_vector(63 downto 0);
	signal pdm_arb_tkeep_s : std_logic_vector(7 downto 0);
	signal pdm_arb_req_s    : std_logic;
	signal pdm_arb_tready_s    : std_logic;
	
	---------------------------------------------------------
	-- DMA Interface (Pipelined Wishbone)
	signal p2l_dma_adr_s   :  std_logic_vector(31 downto 0);  -- Adress
	signal p2l_dma_dat_s2m_s   :  std_logic_vector(63 downto 0);  -- Data in
	signal p2l_dma_dat_m2s_s   :  std_logic_vector(63 downto 0);  -- Data out
	signal p2l_dma_sel_s   :  std_logic_vector(7 downto 0);   -- Byte select
	signal p2l_dma_cyc_s   :  std_logic;                      -- Read or write cycle
	signal p2l_dma_stb_s   :  std_logic;                      -- Read or write strobe
	signal p2l_dma_we_s    :  std_logic;                      -- Write
	signal p2l_dma_ack_s   :  std_logic;                      -- Acknowledge
	signal p2l_dma_stall_s :  std_logic;                      -- for pipelined Wishbone
	signal l2p_dma_adr_s   :  std_logic_vector(64-1 downto 0);
	signal l2p_dma_dat_s2m_s   :  std_logic_vector(64-1 downto 0);
	signal l2p_dma_dat_m2s_s   :  std_logic_vector(64-1 downto 0);
	signal l2p_dma_sel_s   :  std_logic_vector(3 downto 0);
	signal l2p_dma_cyc_s   :  std_logic;
	signal l2p_dma_stb_s   :  std_logic;
	signal l2p_dma_we_s    :  std_logic;
	signal l2p_dma_ack_s   :  std_logic;
	signal l2p_dma_stall_s :  std_logic;
	signal dma_adr_s   :  std_logic_vector(31 downto 0);  -- Adress
	signal dma_dat_s2m_s   :  std_logic_vector(63 downto 0);  -- Data in
	signal dma_dat_m2s_s   :  std_logic_vector(63 downto 0);  -- Data out
	signal dma_sel_s   :  std_logic_vector(7 downto 0);   -- Byte select
	signal dma_cyc_s   :  std_logic;                      -- Read or write cycle
	signal dma_stb_s   :  std_logic;                      -- Read or write strobe
	signal dma_we_s    :  std_logic;                      -- Write
	signal dma_ack_s   :  std_logic;                      -- Acknowledge
	signal dma_stall_s :  std_logic;                      -- for pipelined Wishbone	
	
	signal l2p_current_state_ds : std_logic_vector (2 downto 0);
    signal l2p_data_cnt_ds      : unsigned(12 downto 0);
    signal l2p_len_cnt_ds       : unsigned(12 downto 0);
    signal l2p_timeout_cnt_ds   : unsigned(12 downto 0);
    signal wb_timeout_cnt_ds    : unsigned(12 downto 0);
        -- Data FIFO
    signal data_fifo_rd_ds    : std_logic;
    signal data_fifo_wr_ds    : std_logic;
    signal data_fifo_empty_ds : std_logic;
    signal data_fifo_full_ds  : std_logic;
    signal data_fifo_dout_ds  : std_logic_vector(axis_data_width_c-1 downto 0);
    signal data_fifo_din_ds   : std_logic_vector(axis_data_width_c-1 downto 0);
    -- Addr FIFO
    signal addr_fifo_rd_ds    : std_logic;
    signal addr_fifo_wr_ds    : std_logic;
    signal addr_fifo_empty_ds : std_logic;
    signal addr_fifo_full_ds  : std_logic;
    signal addr_fifo_dout_ds  : std_logic_vector(64-1 downto 0);
    signal addr_fifo_din_ds   : std_logic_vector(axis_data_width_c-1 downto 0);
	
	--constant cyc_nb_exp_c : integer := 2;
	--constant cyc_nb_c : integer := 2**cyc_nb_exp_c;
	--type ram_dma_data_bus is array (cyc_nb_c-1 downto 0) of std_logic_vector(64-1 downto 0);
	
	
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
    
    
    cfg_interrupt_assert_o <= '0';
    cfg_interrupt_di_o <= (others => '0');
    cfg_interrupt_stat_o <= '0';
    cfg_pciecap_interrupt_msgnum_o <= (others => '0');
    
    cfg_interrupt_o <= cfg_interrupt_s;
    
    pcie_id_s <= cfg_bus_number_i & cfg_device_number_i & cfg_function_number_i;
    
    wbm_pd_ready_s <= p2l_wbm_rdy_s and p2l_dma_rdy_s;
    
    interrupt_p : process(rst_i,clk_i)
    begin
        if (rst_i = '1') then
            cfg_interrupt_s <= '0';
        elsif(clk_i'event and clk_i = '1') then
            cfg_interrupt_s <= cfg_interrupt_s;
            if (cfg_interrupt_rdy_i = '1') then
                cfg_interrupt_s <= '0';
            end if;
            if (dma_ctrl_irq_s /= "00") then
                cfg_interrupt_s <= '1';
            end if;
            
    
        end if;
    end process interrupt_p;

    cnt:simple_counter
    port map(
        rst_i => rst_i,
        clk_i => clk_i,
        count_o =>  count_s
    );
    

    p2l_dec_comp:p2l_decoder
    port map(
        clk_i => clk_i,
        rst_i => rst_i,
        -- Slave AXI-Stream
        s_axis_rx_tdata_i => s_axis_rx_tdata_s,
        s_axis_rx_tkeep_i => s_axis_rx_tkeep_s,
        s_axis_rx_tlast_i => s_axis_rx_tlast_s,
        s_axis_rx_tready_o => s_axis_rx_tready_s,
        s_axis_rx_tuser_i => s_axis_rx_tuser_s,
        s_axis_rx_tvalid_i => s_axis_rx_tvalid_s,
        -- To the wishbone master
        pd_wbm_address_o => pd_wbm_address_s,
        pd_wbm_data_o => pd_wbm_data_s,
        pd_wbm_valid_o => pd_wbm_valid_s,
        pd_wbm_hdr_rid_o    => pd_wbm_hdr_rid_s,
        pd_wbm_hdr_tag_o    => pd_wbm_hdr_tag_s,
        pd_wbm_target_mrd_o => pd_wbm_target_mrd_s,
        pd_wbm_target_mwr_o => pd_wbm_target_mwr_s,
        wbm_pd_ready_i => wbm_pd_ready_s,
        pd_op_o => pd_op_s,
        pd_header_type_o => pd_header_type_s,
        pd_payload_length_o => pd_payload_length_s,
 
        -- L2P DMA
        pd_pdm_data_valid_o => pd_pdm_data_valid_s,
        pd_pdm_data_valid_w_o => pd_pdm_data_valid_w_s,
        pd_pdm_data_last_o => pd_pdm_data_last_s,
        pd_pdm_keep_o => pd_pdm_keep_s,
        pd_pdm_data_o => pd_pdm_data_s
    );
   
    wb32:wbmaster32
    generic map (
        g_ACK_TIMEOUT => 100     -- Wishbone ACK timeout (in wb_clk cycles)
    )
    port map
    (
        ---------------------------------------------------------
        -- GN4124 core clock and reset
        clk_i   => clk_i,
        rst_n_i => rst_n_s,
        
        ---------------------------------------------------------
        -- From P2L packet decoder
        --
        -- Header
        pd_wbm_hdr_start_i  => pd_wbm_valid_s,                     -- Header strobe
        --pd_wbm_hdr_length_i : in std_logic_vector(9 downto 0);   -- Packet length in 32-bit words multiples
        pd_wbm_hdr_rid_i => pd_wbm_hdr_rid_s,  -- Requester ID
        pd_wbm_hdr_cid_i => pcie_id_s, --X"0100",  -- Completer ID
        
        pd_wbm_hdr_tag_i => pd_wbm_hdr_tag_s,
        pd_wbm_target_mrd_i => pd_wbm_target_mrd_s,                     -- Target memory read
        pd_wbm_target_mwr_i => pd_wbm_target_mwr_s,                     -- Target memory write
        --
        -- Address
        pd_wbm_addr_start_i =>  pd_wbm_valid_s,                    -- Address strobe
        pd_wbm_addr_i       =>  pd_wbm_address_s(31 downto 0),-- Target address (in byte) that will increment with data
                                                               -- increment = 4 bytes
        --
        -- Data
        pd_wbm_data_valid_i => pd_wbm_valid_s,                     -- Indicates Data is valid
        --pd_wbm_data_last_i  : in std_logic;                      -- Indicates end of the packet
        pd_wbm_data_i       => pd_wbm_data_s, -- Data
        --pd_wbm_be_i         : in std_logic_vector(3 downto 0);   -- Byte Enable for data
        
        ---------------------------------------------------------
        -- P2L channel control
        p_wr_rdy_o   =>  open,-- Ready to accept target write
        p2l_rdy_o    =>  p2l_wbm_rdy_s,--wbm_pd_ready_s,                   -- De-asserted to pause transfer already in progress
        p_rd_d_rdy_i =>  "11",-- Asserted when GN4124 ready to accept read completion with data
        
        ---------------------------------------------------------
        -- To the arbiter (L2P data)
        wbm_arb_tdata_o => wbm_arb_tdata_s,
        wbm_arb_tkeep_o => wbm_arb_tkeep_s,
        --wbm_arb_tuser_o => wbm_arb_tuser_s,
        wbm_arb_tlast_o => wbm_arb_tlast_s,
        wbm_arb_tvalid_o => wbm_arb_tvalid_s,
        wbm_arb_tready_i => wbm_arb_tready_s,
        wbm_arb_req_o    => wbm_arb_req_s,
        
        ---------------------------------------------------------
        -- CSR wishbone interface
        wb_clk_i   =>  clk_i,                     -- Wishbone bus clock
        wb_adr_o   =>  wb_adr_s(30 downto 0),-- Address
        wb_dat_o   =>  wb_dat_m2s_s,-- Data out
        wb_sel_o   =>  wb_sel_s, -- Byte select
        wb_stb_o   =>  wb_stb_s,                    -- Strobe
        wb_we_o    =>  wb_we_s,                    -- Write
        wb_cyc_o   =>  wb_cyc_s,                    -- Cycle
        wb_dat_i   =>  wb_dat_s2m_s,-- Data in
        wb_ack_i   =>  wb_ack_s,                    -- Acknowledge
        wb_stall_i =>  wb_stall_s,                    -- Stall
        wb_err_i   =>  wb_err_s,                    -- Error
        wb_rty_i   =>  wb_rty_s,                    -- Retry
        wb_int_i   =>  wb_int_s                     -- Interrupt
    );
    
    wb_stall_s <= '0';
    wb_err_s <= '0';
    wb_rty_s <= '0';
    wb_int_s <= '0';
    
   
    

    
    wb_dma_ctrl_adr_s <= wb_adr_s(31 downto 0);
    wb_dma_ctrl_dat_m2s_s <= wb_dat_m2s_s;
    wb_dma_ctrl_stb_s <= wb_stb_s;
    wb_dma_ctrl_we_s <= wb_we_s;   
    
    wb_dbg_adr_s <= wb_adr_s(31 downto 0);
    wb_dbg_dat_m2s_s <= wb_dat_m2s_s;
    wb_dbg_stb_s <= wb_stb_s;
    wb_dbg_we_s <= wb_we_s;
    wb_dbg_sel_s <= (others => '1');
    
    wb_mem_adr_s <= wb_adr_s(31 downto 0);
    wb_mem_dat_m2s_s <= wb_dat_m2s_s;
    wb_mem_stb_s <= wb_stb_s;
    wb_mem_we_s <= wb_we_s;    
    
    -- CSR Wishbone adress demux
	process(wb_adr_s,wb_cyc_s,wb_mem_cyc_s,wb_cyc_s,wb_dma_ctrl_dat_s2m_s,wb_dma_ctrl_ack_s,wb_mem_dat_s2m_s,wb_mem_ack_s,wb_dbg_dat_s2m_s,wb_dbg_ack_s)
    begin
        if wb_adr_s(31 downto 4) = X"0000000" then
            wb_dma_ctrl_cyc_s <= wb_cyc_s;
            wb_dbg_cyc_s <= '0';
            wb_mem_cyc_s <= '0';
            wb_dat_s2m_s <= wb_dma_ctrl_dat_s2m_s;
            wb_ack_s <= wb_dma_ctrl_ack_s;
        elsif wb_adr_s(31 downto 4) = X"0000001" then
            wb_dma_ctrl_cyc_s <= '0';
            wb_dbg_cyc_s <= wb_cyc_s;
            wb_mem_cyc_s <= '0';
            wb_dat_s2m_s <= wb_dbg_dat_s2m_s;
            wb_ack_s <= wb_dbg_ack_s;
        else
            wb_dma_ctrl_cyc_s <= '0';
            wb_dbg_cyc_s <= '0';
            wb_mem_cyc_s <= wb_cyc_s;
            wb_dat_s2m_s <= wb_mem_dat_s2m_s;
            wb_ack_s <= wb_mem_ack_s;
        end if;
    end process;
    
    csr_ram:bram_wbs32
    generic map (
      ADDR_WIDTH => 5,
      DATA_WIDTH => 32 
    )
    port map (
      -- SYS CON
      clk            => clk_i,
      rst            => rst_i,
      
      -- Wishbone Slave in
      wb_adr_i    => wb_mem_adr_s(5 - 1 downto 0),
      --wb_dat_i(63 downto 32)    => X"00000000",
      wb_dat_i    => wb_mem_dat_m2s_s,
      wb_we_i        => wb_mem_we_s,
      wb_stb_i    => wb_mem_stb_s,
      wb_cyc_i    => wb_mem_cyc_s,
      wb_lock_i    => wb_mem_stb_s,
      
      -- Wishbone Slave out
      --wb_dat_o(63 downto 32) => wb_null,--open,
      wb_dat_o    => wb_mem_dat_s2m_s,
      wb_ack_o    => wb_mem_ack_s
    );
	
	dma_ctrl:dma_controller
	  port map
		(
		  ---------------------------------------------------------
		  -- GN4124 core clock and reset
		  clk_i   => clk_i,
		  rst_n_i => rst_n_s,

		  ---------------------------------------------------------
		  -- Interrupt request
		  dma_ctrl_irq_o => dma_ctrl_irq_s,

		  ---------------------------------------------------------
		  -- To the L2P DMA master and P2L DMA master
		  dma_ctrl_carrier_addr_o => dma_ctrl_carrier_addr_s,
		  dma_ctrl_host_addr_h_o  => dma_ctrl_host_addr_h_s,
		  dma_ctrl_host_addr_l_o  => dma_ctrl_host_addr_l_s,
		  dma_ctrl_len_o          => dma_ctrl_len_s,
		  dma_ctrl_start_l2p_o    => dma_ctrl_start_l2p_s, -- To the L2P DMA master
		  dma_ctrl_start_p2l_o    => dma_ctrl_start_p2l_s, -- To the P2L DMA master
		  dma_ctrl_start_next_o   => dma_ctrl_start_next_s, -- To the P2L DMA master
		  dma_ctrl_byte_swap_o    => dma_ctrl_byte_swap_s,
		  dma_ctrl_abort_o        => dma_ctrl_abort_s,
		  dma_ctrl_done_i         => dma_ctrl_done_s,
		  dma_ctrl_error_i        => dma_ctrl_error_s,

		  ---------------------------------------------------------
		  -- From P2L DMA master
		  next_item_carrier_addr_i => next_item_carrier_addr_s,
		  next_item_host_addr_h_i  => next_item_host_addr_h_s,
		  next_item_host_addr_l_i  => next_item_host_addr_l_s,
		  next_item_len_i          => next_item_len_s,
		  next_item_next_l_i       => next_item_next_l_s,
		  next_item_next_h_i       => next_item_next_h_s,
		  next_item_attrib_i       => next_item_attrib_s,
		  next_item_valid_i        => next_item_valid_s,

		  ---------------------------------------------------------
		  -- Wishbone slave interface
		  wb_clk_i => clk_i,                     -- Bus clock
		  wb_adr_i => wb_dma_ctrl_adr_s(3 downto 0),   -- Adress
		  wb_dat_o => wb_dma_ctrl_dat_s2m_s,  -- Data in
		  wb_dat_i => wb_dma_ctrl_dat_m2s_s,  -- Data out
		  wb_sel_i => "1111",   -- Byte select
		  wb_cyc_i => wb_dma_ctrl_cyc_s,                      -- Read or write cycle
		  wb_stb_i => wb_dma_ctrl_stb_s,                      -- Read or write strobe
		  wb_we_i  => wb_dma_ctrl_we_s,                      -- Write
		  wb_ack_o => wb_dma_ctrl_ack_s,                       -- Acknowledge
		  
		  dma_ctrl_current_state_do => dma_ctrl_current_state_ds,
          dma_ctrl_do => dma_ctrl_ds,
          dma_stat_do => dma_stat_ds,
          dma_attrib_do => dma_attrib_ds
		  );

	  -- Status signals from DMA masters
	dma_ctrl_done_s  <= dma_ctrl_l2p_done_s or dma_ctrl_p2l_done_s;
	dma_ctrl_error_s <= dma_ctrl_l2p_error_s or dma_ctrl_p2l_error_s;
    
    dbg_reg_comp:debugregisters
        Port map( 
          -- SYS CON
            clk            => clk_i,
            rst            => rst_i,
            
            -- Wishbone Slave in
            wb_adr_i       => wb_dbg_adr_s(3 downto 0),
            wb_dat_i       => wb_dbg_dat_m2s_s,
            wb_we_i        => wb_dbg_we_s,
            wb_stb_i       => wb_dbg_stb_s,
            wb_cyc_i       => wb_dbg_cyc_s, 
            
            -- Wishbone Slave out
            wb_dat_o       => wb_dbg_dat_s2m_s,
            wb_ack_o       => wb_dbg_ack_s,
            
            -- input/ouput  
            dummyram_sel_o      => dummyram_sel_s,
            ddr3ram_sel_o       => ddr3ram_sel_s,
            dummyaddress_sel_o  => dummyaddress_sel_s,
            dummydeadbeef_sel_o => dummydeadbeef_sel_s,
            
            usr_led_o => usr_led_o,
            usr_sw_i => usr_sw_i--,
            
            --ddr_init_calib_complete_i => init_calib_complete_s
        );
	
	p2l_dma:p2l_dma_master
	  generic map (
		-- Enable byte swap module (if false, no swap)
		g_BYTE_SWAP => false
		)
	  port map
		(
		  ---------------------------------------------------------
		  -- GN4124 core clock and reset
		  clk_i   => clk_i,
		  rst_n_i => rst_n_s,
		  
		  l2p_rid_i => pcie_id_s,

		  ---------------------------------------------------------
		  -- From the DMA controller
		  dma_ctrl_carrier_addr_i => dma_ctrl_carrier_addr_s,
		  dma_ctrl_host_addr_h_i  => dma_ctrl_host_addr_h_s,
		  dma_ctrl_host_addr_l_i  => dma_ctrl_host_addr_l_s,
		  dma_ctrl_len_i          => dma_ctrl_len_s,
		  dma_ctrl_start_p2l_i    => dma_ctrl_start_p2l_s,
		  dma_ctrl_start_next_i   => dma_ctrl_start_next_s,
		  dma_ctrl_done_o         => dma_ctrl_p2l_done_s,
		  dma_ctrl_error_o        => dma_ctrl_p2l_error_s,
		  dma_ctrl_byte_swap_i    => "111",
		  dma_ctrl_abort_i        => dma_ctrl_abort_s,

		  ---------------------------------------------------------
		  -- From P2L Decoder (receive the read completion)
		  --
		  -- Header
		  pd_pdm_master_cpld_i => '1',                      -- Master read completion with data
		  pd_pdm_master_cpln_i => '0',                      -- Master read completion without data
		  --
		  -- Data
		  pd_pdm_data_valid_i  => pd_pdm_data_valid_s,                      -- Indicates Data is valid
		  pd_pdm_data_valid_w_i => pd_pdm_data_valid_w_s,
		  pd_pdm_data_last_i   => pd_pdm_data_last_s,                      -- Indicates end of the packet
		  pd_pdm_data_i        => pd_pdm_data_s,  -- Data
		  pd_pdm_be_i          => pd_pdm_keep_s,   -- Byte Enable for data

		  ---------------------------------------------------------
		  -- P2L control
		  p2l_rdy_o  => p2l_dma_rdy_s,      -- De-asserted to pause transfer already in progress
		  rx_error_o => open,       -- Asserted when transfer is aborted

		  ---------------------------------------------------------
		  -- To the P2L Interface (send the DMA Master Read request)
		  pdm_arb_tvalid_o  => pdm_arb_tvalid_s,  -- Read completion signals
		  pdm_arb_tlast_o => pdm_arb_tlast_s,  -- Toward the arbiter
		  pdm_arb_tdata_o   => pdm_arb_tdata_s,
		  pdm_arb_tkeep_o   => pdm_arb_tkeep_s,
		  pdm_arb_req_o    => pdm_arb_req_s,
		  arb_pdm_gnt_i    => pdm_arb_tready_s,

		  ---------------------------------------------------------
		  -- DMA Interface (Pipelined Wishbone)
		  p2l_dma_clk_i   => clk_i,                      -- Bus clock
		  p2l_dma_adr_o   => p2l_dma_adr_s,  -- Adress
		  p2l_dma_dat_i   => p2l_dma_dat_s2m_s,  -- Data in
		  p2l_dma_dat_o   => p2l_dma_dat_m2s_s,  -- Data out
		  p2l_dma_sel_o   => p2l_dma_sel_s,   -- Byte select
		  p2l_dma_cyc_o   => p2l_dma_cyc_s,                      -- Read or write cycle
		  p2l_dma_stb_o   => p2l_dma_stb_s,                      -- Read or write strobe
		  p2l_dma_we_o    => p2l_dma_we_s,                      -- Write
		  p2l_dma_ack_i   => p2l_dma_ack_s,                      -- Acknowledge
		  p2l_dma_stall_i => p2l_dma_stall_s,                      -- for pipelined Wishbone
		  l2p_dma_cyc_i   => l2p_dma_cyc_s,                      -- L2P dma wb cycle (for bus arbitration)

		  ---------------------------------------------------------
		  -- To the DMA controller
		  next_item_carrier_addr_o => next_item_carrier_addr_s,
		  next_item_host_addr_h_o  => next_item_host_addr_h_s,
		  next_item_host_addr_l_o  => next_item_host_addr_l_s,
		  next_item_len_o          => next_item_len_s,
		  next_item_next_l_o       => next_item_next_l_s,
		  next_item_next_h_o       => next_item_next_h_s,
		  next_item_attrib_o       => next_item_attrib_s,
		  next_item_valid_o        => next_item_valid_s
		  );
		  
		  
	-----------------------------------------------------------------------------
	-- L2P DMA master
	-----------------------------------------------------------------------------
	--l2p_dma_stall_s <= '0';
	
	l2p_dma : l2p_dma_master
	port map
	(
		clk_i   => clk_i,
		rst_n_i => rst_n_s,
		
        l2p_rid_i => pcie_id_s,

		dma_ctrl_target_addr_i => dma_ctrl_carrier_addr_s,
		dma_ctrl_host_addr_h_i => dma_ctrl_host_addr_h_s,
		dma_ctrl_host_addr_l_i => dma_ctrl_host_addr_l_s,
		dma_ctrl_len_i         => dma_ctrl_len_s,
		dma_ctrl_start_l2p_i   => dma_ctrl_start_l2p_s,
		dma_ctrl_done_o        => dma_ctrl_l2p_done_s,
		dma_ctrl_error_o       => dma_ctrl_l2p_error_s,
		dma_ctrl_byte_swap_i   => "000", --TODO
		dma_ctrl_abort_i       => dma_ctrl_abort_s,

		ldm_arb_tvalid_o  => ldm_arb_tvalid_s,
		ldm_arb_tlast_o => ldm_arb_tlast_s,
		ldm_arb_tdata_o   => ldm_arb_tdata_s,
		ldm_arb_tkeep_o   => ldm_arb_tkeep_s,
		ldm_arb_req_o    => ldm_arb_req_s,
		arb_ldm_gnt_i    => ldm_arb_tready_s,

		l2p_edb_o  => open,
		ldm_arb_tready_i => ldm_arb_tready_s,
		l2p_rdy_i  => '1',
		tx_error_i => '0',

		l2p_dma_clk_i   => clk_i,
		l2p_dma_adr_o   => l2p_dma_adr_s,
		l2p_dma_dat_i   => l2p_dma_dat_s2m_s,
		l2p_dma_dat_o   => l2p_dma_dat_m2s_s,
		l2p_dma_sel_o   => l2p_dma_sel_s,
		l2p_dma_cyc_o   => l2p_dma_cyc_s,
		l2p_dma_stb_o   => l2p_dma_stb_s,
		l2p_dma_we_o    => l2p_dma_we_s,
		l2p_dma_ack_i   => l2p_dma_ack_s,
		l2p_dma_stall_i => l2p_dma_stall_s,
		p2l_dma_cyc_i   => p2l_dma_cyc_s,
		
		--DMA Debug
        l2p_current_state_do => l2p_current_state_ds,
        l2p_data_cnt_do => l2p_data_cnt_ds,
        l2p_len_cnt_do  => l2p_len_cnt_ds,
        l2p_timeout_cnt_do => l2p_timeout_cnt_ds,
        wb_timeout_cnt_do => wb_timeout_cnt_ds,
        
        -- Data FIFO
        data_fifo_rd_do    => data_fifo_rd_ds,
        data_fifo_wr_do    => data_fifo_wr_ds,
        data_fifo_empty_do => data_fifo_empty_ds,
        data_fifo_full_do  => data_fifo_full_ds,
        data_fifo_dout_do  => data_fifo_dout_ds,
        data_fifo_din_do   => data_fifo_din_ds,
        
        -- Addr FIFO
        addr_fifo_rd_do    => addr_fifo_rd_ds,
        addr_fifo_wr_do    => addr_fifo_wr_ds,
        addr_fifo_empty_do => addr_fifo_empty_ds,
        addr_fifo_full_do  => addr_fifo_full_ds,
        addr_fifo_dout_do  => addr_fifo_dout_ds,
        addr_fifo_din_do   => addr_fifo_din_ds
	);
	
	
	arbiter:l2p_arbiter
	generic map(
		axis_data_width_c => axis_data_width_c
	)
	port map(
		---------------------------------------------------------
		-- GN4124 core clock and reset
		clk_i   => clk_i,
		rst_n_i => rst_n_s,
		
		---------------------------------------------------------
		-- From Wishbone master (wbm) to arbiter (arb)      
		wbm_arb_tdata_i => wbm_arb_tdata_s,
		wbm_arb_tkeep_i => wbm_arb_tkeep_s,
		wbm_arb_tlast_i => wbm_arb_tlast_s,
		wbm_arb_tvalid_i => wbm_arb_tvalid_s,
		wbm_arb_req_i => wbm_arb_req_s,
		wbm_arb_tready_o => wbm_arb_tready_s,
		
		---------------------------------------------------------
		-- From P2L DMA master (pdm) to arbiter (arb)
		pdm_arb_tdata_i => pdm_arb_tdata_s,
		pdm_arb_tkeep_i => pdm_arb_tkeep_s,
		pdm_arb_tlast_i => pdm_arb_tlast_s,
		pdm_arb_tvalid_i => pdm_arb_tvalid_s,
		pdm_arb_req_i => pdm_arb_req_s,
		pdm_arb_tready_o => pdm_arb_tready_s,
		arb_pdm_gnt_o => open,
		
		---------------------------------------------------------
		-- From L2P DMA master (ldm) to arbiter (arb)
		ldm_arb_tdata_i => ldm_arb_tdata_s,
		ldm_arb_tkeep_i => ldm_arb_tkeep_s,
		ldm_arb_tlast_i => ldm_arb_tlast_s,
		ldm_arb_tvalid_i => ldm_arb_tvalid_s,
		ldm_arb_req_i    => ldm_arb_req_s,
		ldm_arb_tready_o => ldm_arb_tready_s,
		arb_ldm_gnt_o => open,
		
		---------------------------------------------------------
		-- From arbiter (arb) to pcie_tx (tx)
		axis_tx_tdata_o => m_axis_tx_tdata_s,
		axis_tx_tkeep_o => m_axis_tx_tkeep_s,
		axis_tx_tuser_o => m_axis_tx_tuser_s,
		axis_tx_tlast_o => m_axis_tx_tlast_s,
		axis_tx_tvalid_o => m_axis_tx_tvalid_s,
		axis_tx_tready_i => m_axis_tx_tready_s,
		
		eop_do => eop_s
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
  
  dma_mux: process(
  l2p_dma_adr_s,l2p_dma_dat_m2s_s,l2p_dma_sel_s,l2p_dma_cyc_s,l2p_dma_stb_s,l2p_dma_we_s,
  p2l_dma_adr_s,p2l_dma_dat_m2s_s,p2l_dma_sel_s,p2l_dma_cyc_s,p2l_dma_stb_s,p2l_dma_we_s)
  begin
	if l2p_dma_cyc_s = '1' then
		dma_adr_s      <= l2p_dma_adr_s(31 downto 0);
		dma_dat_m2s_s  <= l2p_dma_dat_m2s_s;
		dma_sel_s      <= l2p_dma_sel_s & l2p_dma_sel_s;
		dma_cyc_s      <= l2p_dma_cyc_s;
		dma_stb_s      <= l2p_dma_stb_s;
		dma_we_s       <= l2p_dma_we_s;
	elsif p2l_dma_cyc_s = '1' then
		dma_adr_s      <= p2l_dma_adr_s;
		dma_dat_m2s_s  <= p2l_dma_dat_m2s_s;
		dma_sel_s      <= p2l_dma_sel_s;
		dma_cyc_s      <= p2l_dma_cyc_s;
		dma_stb_s      <= p2l_dma_stb_s;
		dma_we_s       <= p2l_dma_we_s;
	else
		dma_adr_s      <= (others => '0');
		dma_dat_m2s_s  <= (others => '0');
		dma_sel_s      <= (others => '0');
		dma_cyc_s      <= '0';
		dma_stb_s      <= '0';
		dma_we_s       <= '0';
	end if;
  end process dma_mux;
  
  l2p_dma_dat_s2m_s <= dma_dat_s2m_s;
  p2l_dma_dat_s2m_s <= dma_dat_s2m_s;
  l2p_dma_ack_s     <= dma_ack_s;
  p2l_dma_ack_s     <= dma_ack_s;
  l2p_dma_stall_s   <= dma_stall_s;
  p2l_dma_stall_s   <= dma_stall_s;
  dma_stall_s <= '0';
  
  front_led_o <= count_s(28 downto 25);
  --usr_led_o <= '1' & usr_sw_i;
  
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
          probe11(0) => dma_ctrl_start_l2p_s, 
          probe12(0) => dma_ctrl_start_p2l_s, 
          probe13(0) => dma_ctrl_start_next_s,
          probe14(0) => dma_ctrl_abort_s, 
          probe15(0) => dma_ctrl_done_s,
          probe16(0) => dma_ctrl_error_s,
          probe17(0) => user_lnk_up_i,
          probe18(0) => cfg_interrupt_s,
          probe19(0) => cfg_interrupt_rdy_i,
          probe20(0) => dma_ctrl_done_s,
          probe21 => dma_ctrl_current_state_ds,
          probe22(0) => tx_err_drop_i--next_item_valid_s
      );
  end generate dbg_0;
  
  dbg_1 : if DEBUG_C(1) = '1' generate
      dma_ctrl_debug : ila_dma_ctrl_reg
      PORT MAP (
          clk => clk_i,
      
      
      
          probe0 => dma_ctrl_carrier_addr_s, 
          probe1 => dma_ctrl_host_addr_h_s, 
          probe2 => dma_ctrl_host_addr_l_s, 
          probe3 => dma_ctrl_len_s, 
          probe4(0) => dma_ctrl_start_l2p_s, 
          probe5(0) => dma_ctrl_start_p2l_s, 
          probe6(0) => dma_ctrl_start_next_s, 
          probe7 => dma_ctrl_byte_swap_s, 
          probe8(0) => dma_ctrl_abort_s, 
          probe9(0) => dma_ctrl_done_s, 
          probe10(0) => dma_ctrl_error_s,
          probe11 => dma_ctrl_current_state_ds,
          probe12 => next_item_carrier_addr_s,
          probe13  => next_item_host_addr_h_s,
          probe14  => next_item_host_addr_l_s,
          probe15          => next_item_len_s,
          probe16       => next_item_next_l_s,
          probe17       => next_item_next_h_s,
          probe18       => next_item_attrib_s,
          probe19(0)        => next_item_valid_s,
          probe20 => dma_ctrl_irq_s
      );
  end generate dbg_1;
  
  
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
          probe9(0) => l2p_dma_cyc_s,
          probe10(0) => p2l_dma_cyc_s,
          probe11(0) => dma_ctrl_start_l2p_s, 
          probe12(0) => dma_ctrl_start_p2l_s, 
          probe13(0) => dma_ctrl_start_next_s,
          probe14 => ddr_rd_mask_rd_data_count_ds,
          probe15 => ddr_rd_data_rd_data_count_ds
      );
  end generate dbg_2;
  
  dbg_3 : if DEBUG_C(3) = '1' generate
      wbm_to_p2l_debug : ila_pd_pdm
      PORT MAP (
          clk => clk_i,
      
      
      
          probe0 => pd_pdm_data_s, 
          probe1(0) => pd_pdm_data_last_s,
          probe2(0) => pd_pdm_data_valid_s,
          probe3 => s_axis_rx_tdata_s,  
          probe4 => s_axis_rx_tkeep_s,
          probe5(0) => s_axis_rx_tlast_s, 
          probe6(0) => s_axis_rx_tvalid_s,
          probe7(0) => ldm_arb_tready_s, 
          probe8 => l2p_current_state_ds, 
          probe9 => dma_ctrl_current_state_ds,
          probe10 => pd_pdm_data_valid_w_s,
          probe11(1) => next_item_valid_s
 
      );
  end generate dbg_3;
  
  dbg_4 : if DEBUG_C(4) = '1' generate
      l2p_debug : ila_l2p_dma
      PORT MAP (
          clk => clk_i,
      
      
      
          probe0 => dma_ctrl_carrier_addr_s, 
          probe1 => dma_ctrl_host_addr_h_s, 
          probe2 => dma_ctrl_host_addr_l_s, 
          probe3 => dma_ctrl_len_s, 
          probe4(0) => dma_ctrl_start_l2p_s, 
          probe5(0) => dma_ctrl_done_s , 
          probe6(0) => dma_ctrl_l2p_error_s , 
          probe7(0) => dma_ctrl_abort_s, 
          probe8(0) => ldm_arb_tvalid_s, 
          probe9(0) => ldm_arb_tlast_s, 
          probe10 => ldm_arb_tdata_s, 
          probe11 => ldm_arb_tkeep_s, 
          probe12(0) => ldm_arb_tready_s, 
          probe13(0) => ldm_arb_req_s, 
          probe14(0) => pdm_arb_tready_s, 
          probe15(0) => '0', 
          probe16 => l2p_dma_adr_s(31 downto 0), 
          probe17 => l2p_dma_dat_m2s_s, 
          probe18 => l2p_dma_dat_s2m_s, 
          probe19(0) => l2p_dma_cyc_s, 
          probe20(0) => l2p_dma_stb_s, 
          probe21(0) => l2p_dma_we_s, 
          probe22(0) => l2p_dma_ack_s, 
          probe23(0) => l2p_dma_stall_s, 
          probe24(0) => p2l_dma_cyc_s, 
          probe25 => l2p_current_state_ds, 
          probe26 => std_logic_vector(l2p_data_cnt_ds), 
          probe27 => std_logic_vector(l2p_len_cnt_ds), 
          probe28(0) => data_fifo_rd_ds, 
          probe29(0) => data_fifo_wr_ds, 
          probe30(0) => data_fifo_empty_ds, 
          probe31(0) => data_fifo_full_ds, 
          probe32 => data_fifo_dout_ds, 
          probe33 => data_fifo_din_ds, 
          probe34(0) => addr_fifo_rd_ds, 
          probe35(0) => addr_fifo_wr_ds, 
          probe36(0) => addr_fifo_empty_ds, 
          probe37(0) => addr_fifo_full_ds, 
          probe38 => addr_fifo_dout_ds, 
          probe39 => addr_fifo_din_ds,
          probe40 => std_logic_vector(wb_timeout_cnt_ds),
          probe41 => std_logic_vector(l2p_timeout_cnt_ds)
      );
  end generate dbg_4;
  
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
          probe13(0) => init_calib_complete_s


      );
   end generate dbg_5;
  
end Behavioral;
