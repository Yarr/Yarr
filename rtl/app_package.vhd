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

package app_pkg is

    component simple_counter is
        Port ( 
               rst_i : in STD_LOGIC;
               clk_i : in STD_LOGIC;
               count_o : out STD_LOGIC_VECTOR (28 downto 0)
                );
    end component;
    

        Component p2l_decoder is
        Port (
            clk_i : in STD_LOGIC;
            rst_i : in STD_LOGIC;
            -- From Slave AXI-Stream
            s_axis_rx_tdata_i : in STD_LOGIC_VECTOR (64 - 1 downto 0);
            s_axis_rx_tkeep_i : in STD_LOGIC_VECTOR (64/8 - 1 downto 0);
            s_axis_rx_tuser_i : in STD_LOGIC_VECTOR (21 downto 0);
            s_axis_rx_tlast_i : in STD_LOGIC;
            s_axis_rx_tvalid_i : in STD_LOGIC;
            s_axis_rx_tready_o : out STD_LOGIC;
            -- To the wishbone master
            pd_wbm_address_o : out STD_LOGIC_VECTOR(63 downto 0);
            pd_wbm_data_o : out STD_LOGIC_VECTOR(31 downto 0);
            pd_wbm_valid_o : out std_logic;
            pd_wbm_hdr_rid_o    : out std_logic_vector(15 downto 0);  -- Requester ID
            pd_wbm_hdr_tag_o    : out std_logic_vector(7 downto 0);
            pd_wbm_target_mrd_o : out std_logic;                      -- Target memory read
            pd_wbm_target_mwr_o : out std_logic;                      -- Target memory write
            wbm_pd_ready_i : in std_logic;
            -- to L2P DMA
            pd_pdm_data_valid_o  : out std_logic;                      -- Indicates Data is valid
            pd_pdm_data_valid_w_o  : out std_logic_vector(1 downto 0);
            pd_pdm_data_last_o   : out std_logic;                      -- Indicates end of the packet
            pd_pdm_keep_o         : out std_logic_vector(7 downto 0);
            pd_pdm_data_o        : out std_logic_vector(63 downto 0);  -- Data
            --debug outputs
            states_do : out STD_LOGIC_VECTOR(3 downto 0);
            pd_op_o : out STD_LOGIC_VECTOR(2 downto 0);
            pd_header_type_o : out STD_LOGIC;
            pd_payload_length_o : out STD_LOGIC_VECTOR(9 downto 0)
        );
        end component;
        
        component wbmaster32 is
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
              --pd_wbm_hdr_length_i : in std_logic_vector(9 downto 0);   -- Packet length in 32-bit words multiples
              pd_wbm_hdr_rid_i    : in std_logic_vector(15 downto 0);  -- Requester ID
              pd_wbm_hdr_cid_i    : in std_logic_vector(15 downto 0);  -- Completer ID
              pd_wbm_hdr_tag_i    : in std_logic_vector(7 downto 0);   -- Completion ID
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
        end component;
       
 
	component dma_controller is
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
		  dma_ctrl_byte_swap_o    : out std_logic_vector(1 downto 0);
		  dma_ctrl_abort_o        : out std_logic;
		  dma_ctrl_done_i         : in  std_logic;
		  dma_ctrl_error_i        : in  std_logic;

		  ---------------------------------------------------------
		  -- From P2L DMA master
		  next_item_carrier_addr_i : in std_logic_vector(31 downto 0);
		  next_item_host_addr_h_i  : in std_logic_vector(31 downto 0);
		  next_item_host_addr_l_i  : in std_logic_vector(31 downto 0);
		  next_item_len_i          : in std_logic_vector(31 downto 0);
		  next_item_next_l_i       : in std_logic_vector(31 downto 0);
		  next_item_next_h_i       : in std_logic_vector(31 downto 0);
		  next_item_attrib_i       : in std_logic_vector(31 downto 0);
		  next_item_valid_i        : in std_logic;

		  ---------------------------------------------------------
		  -- Wishbone slave interface
		  wb_clk_i : in  std_logic;                      -- Bus clock
		  wb_adr_i : in  std_logic_vector(3 downto 0);   -- Adress
		  wb_dat_o : out std_logic_vector(31 downto 0);  -- Data in
		  wb_dat_i : in  std_logic_vector(31 downto 0);  -- Data out
		  wb_sel_i : in  std_logic_vector(3 downto 0);   -- Byte select
		  wb_cyc_i : in  std_logic;                      -- Read or write cycle
		  wb_stb_i : in  std_logic;                      -- Read or write strobe
		  wb_we_i  : in  std_logic;                      -- Write
		  wb_ack_o : out std_logic;                       -- Acknowledge
		  
		  ---------------------------------------------------------
          -- debug outputs
          dma_ctrl_current_state_do : out std_logic_vector (2 downto 0);
          dma_ctrl_do    : out std_logic_vector(31 downto 0);
          dma_stat_do    : out std_logic_vector(31 downto 0);
          dma_attrib_do  : out std_logic_vector(31 downto 0)
		  );
	end component;
 
    
    component bram_wbs32 is
        generic (
            constant ADDR_WIDTH : integer := 16;
            constant DATA_WIDTH : integer := 32 
        );
        port (
            -- SYS CON
            clk            : in std_logic;
            rst            : in std_logic;
            
            -- Wishbone Slave in
            wb_adr_i            : in std_logic_vector(5-1 downto 0);
            wb_dat_i            : in std_logic_vector(32-1 downto 0);
            wb_we_i            : in std_logic;
            wb_stb_i            : in std_logic;
            wb_cyc_i            : in std_logic; 
            wb_lock_i        : in std_logic; -- nyi
            
            -- Wishbone Slave out
            wb_dat_o            : out std_logic_vector(32-1 downto 0);
            wb_ack_o            : out std_logic        
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
    
	
	component l2p_arbiter is
	  generic(
		axis_data_width_c : integer := 64
	  );
	  port
		(
		  ---------------------------------------------------------
		  -- GN4124 core clock and reset
		  clk_i   : in std_logic;
		  rst_n_i : in std_logic;

		  ---------------------------------------------------------
		  -- From Wishbone master (wbm) to arbiter (arb)      
		  wbm_arb_tdata_i : in std_logic_vector (axis_data_width_c - 1 downto 0);
		  wbm_arb_tkeep_i : in std_logic_vector (axis_data_width_c/8 - 1 downto 0);
		  wbm_arb_tlast_i : in std_logic;
		  wbm_arb_tvalid_i : in std_logic;
		  wbm_arb_tready_o : out std_logic;
		  wbm_arb_req_i    : in  std_logic;
		  arb_wbm_gnt_o : out std_logic;

		  ---------------------------------------------------------
		  -- From P2L DMA master (pdm) to arbiter (arb)
		  pdm_arb_tdata_i : in std_logic_vector (axis_data_width_c - 1 downto 0);
		  pdm_arb_tkeep_i : in std_logic_vector (axis_data_width_c/8 - 1 downto 0);
		  pdm_arb_tlast_i : in std_logic;
		  pdm_arb_tvalid_i : in std_logic;
		  pdm_arb_tready_o : out std_logic;
		  pdm_arb_req_i    : in  std_logic;
		  arb_pdm_gnt_o : out std_logic;

		  ---------------------------------------------------------
		  -- From L2P DMA master (ldm) to arbiter (arb)
		  ldm_arb_tdata_i : in std_logic_vector (axis_data_width_c - 1 downto 0);
		  ldm_arb_tkeep_i : in std_logic_vector (axis_data_width_c/8 - 1 downto 0);
		  ldm_arb_tlast_i : in std_logic;
		  ldm_arb_tvalid_i : in std_logic;
		  ldm_arb_tready_o : out std_logic;
		  ldm_arb_req_i    : in  std_logic;
		  arb_ldm_gnt_o : out std_logic;

		  ---------------------------------------------------------
		  -- From arbiter (arb) to pcie_tx (tx)
		  axis_tx_tdata_o : out STD_LOGIC_VECTOR (axis_data_width_c - 1 downto 0);
		  axis_tx_tkeep_o : out STD_LOGIC_VECTOR (axis_data_width_c/8 - 1 downto 0);
		  axis_tx_tuser_o : out STD_LOGIC_VECTOR (3 downto 0);
		  axis_tx_tlast_o : out STD_LOGIC;
		  axis_tx_tvalid_o : out STD_LOGIC;
		  axis_tx_tready_i : in STD_LOGIC;
		  
		  ---------------------------------------------------------
		  -- Debug
		  eop_do : out std_logic
		  );
	end component;
    
	
	
	component p2l_dma_master is
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
		  pd_pdm_master_cpld_i : in std_logic;                      -- Master read completion with data
		  pd_pdm_master_cpln_i : in std_logic;                      -- Master read completion without data
		  --
		  -- Data
		  pd_pdm_data_valid_i  : in std_logic;                      -- Indicates Data is valid
		  pd_pdm_data_valid_w_i: in std_logic_vector(1 downto 0);
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
	end component;
	
	component l2p_dma_master is
		generic (
			g_BYTE_SWAP : boolean := false;
			axis_data_width_c : integer := 64;
			wb_address_width_c : integer := 64;
			wb_data_width_c : integer := 64
		);
		port (
			-- GN4124 core clk and reset
			clk_i   : in std_logic;
			rst_n_i : in std_logic;
			
			-- From PCIe IP core
            l2p_rid_i : in std_logic_vector(16-1 downto 0);

			-- From the DMA controller
			dma_ctrl_target_addr_i : in  std_logic_vector(32-1 downto 0);
			dma_ctrl_host_addr_h_i : in  std_logic_vector(32-1 downto 0);
			dma_ctrl_host_addr_l_i : in  std_logic_vector(32-1 downto 0);
			dma_ctrl_len_i         : in  std_logic_vector(32-1 downto 0);
			dma_ctrl_start_l2p_i   : in  std_logic;
			dma_ctrl_done_o        : out std_logic;
			dma_ctrl_error_o       : out std_logic;
			dma_ctrl_byte_swap_i   : in  std_logic_vector(2 downto 0);
			dma_ctrl_abort_i       : in  std_logic;

			-- To the arbiter (L2P data)
			ldm_arb_tvalid_o  : out std_logic;
			--ldm_arb_dframe_o : out std_logic;
			ldm_arb_tlast_o   : out std_logic;
			ldm_arb_tdata_o   : out std_logic_vector(axis_data_width_c-1 downto 0);
			ldm_arb_tkeep_o   : out std_logic_vector(axis_data_width_c/8-1 downto 0);
			ldm_arb_tready_i : in  std_logic;
			ldm_arb_req_o    : out std_logic;
			arb_ldm_gnt_i    : in  std_logic;


			-- L2P channel control
			l2p_edb_o  : out std_logic;                    -- Asserted when transfer is aborted
			l2p_rdy_i  : in  std_logic;                    -- De-asserted to pause transdert already in progress
			tx_error_i : in  std_logic;                    -- Asserted when unexpected or malformed paket received

			-- DMA Interface (Pipelined Wishbone)
			l2p_dma_clk_i   : in  std_logic;
			l2p_dma_adr_o   : out std_logic_vector(wb_address_width_c-1 downto 0);
			l2p_dma_dat_i   : in  std_logic_vector(wb_data_width_c-1 downto 0);
			l2p_dma_dat_o   : out std_logic_vector(wb_data_width_c-1 downto 0);
			l2p_dma_sel_o   : out std_logic_vector(3 downto 0);
			l2p_dma_cyc_o   : out std_logic;
			l2p_dma_stb_o   : out std_logic;
			l2p_dma_we_o    : out std_logic;
			l2p_dma_ack_i   : in  std_logic;
			l2p_dma_stall_i : in  std_logic;
			p2l_dma_cyc_i   : in  std_logic; -- P2L dma WB cycle for bus arbitration
			
			--DMA Debug
            l2p_current_state_do : out std_logic_vector (2 downto 0);
            l2p_data_cnt_do : out unsigned(12 downto 0);
            l2p_len_cnt_do  : out unsigned(12 downto 0);
            l2p_timeout_cnt_do : out unsigned(12 downto 0);
            wb_timeout_cnt_do  : out unsigned(12 downto 0);
            
            -- Data FIFO
            data_fifo_rd_do    : out std_logic;
            data_fifo_wr_do    : out std_logic;
            data_fifo_empty_do : out std_logic;
            data_fifo_full_do  : out std_logic;
            data_fifo_dout_do  : out std_logic_vector(axis_data_width_c-1 downto 0);
            data_fifo_din_do   : out std_logic_vector(axis_data_width_c-1 downto 0);
            
            -- Addr FIFO
            addr_fifo_rd_do    : out std_logic;
            addr_fifo_wr_do    : out std_logic;
            addr_fifo_empty_do : out std_logic;
            addr_fifo_full_do  : out std_logic;
            addr_fifo_dout_do  : out std_logic_vector(axis_data_width_c-1 downto 0);
            addr_fifo_din_do   : out std_logic_vector(axis_data_width_c-1 downto 0)
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
          -- Debug ports
          ----------------------------------------------------------------------------
          ddr_rd_mask_rd_data_count_do : out std_logic_vector(3 downto 0);
          ddr_rd_data_rd_data_count_do : out std_logic_vector(3 downto 0);
          ddr_rd_fifo_full_do : out std_logic_vector(1 downto 0);
          ddr_rd_fifo_empty_do : out std_logic_vector(1 downto 0);
          ddr_rd_fifo_rd_do : out std_logic_vector(1 downto 0)
        );
    end component ddr3_ctrl_wb;
    
    component debugregisters is
        generic (
            constant ADDR_WIDTH : integer := 4;
            constant DATA_WIDTH : integer := 32
        );
        Port ( 
        -- SYS CON
        clk            : in std_logic;
        rst            : in std_logic;
        
        -- Wishbone Slave in
        wb_adr_i            : in std_logic_vector(ADDR_WIDTH-1 downto 0);
        wb_dat_i            : in std_logic_vector(DATA_WIDTH-1 downto 0);
        wb_we_i            : in std_logic;
        wb_stb_i            : in std_logic;
        wb_cyc_i            : in std_logic; 
        
        -- Wishbone Slave out
        wb_dat_o            : out std_logic_vector(DATA_WIDTH-1 downto 0);
        wb_ack_o            : out std_logic  ;
        
        -- input/ouput  
        dummyram_sel_o      : out std_logic;
        ddr3ram_sel_o       : out std_logic;
        dummyaddress_sel_o      : out std_logic;
        dummydeadbeef_sel_o     : out std_logic;
        
        usr_led_o : out STD_LOGIC_VECTOR (3 downto 0);
        usr_sw_i : in STD_LOGIC_VECTOR (2 downto 0)--;
        
        --ddr_init_calib_complete_i : in std_logic
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
            probe22 : IN STD_LOGIC_VECTOR(0 DOWNTO 0)
            
        );
    END COMPONENT  ;
    
    COMPONENT ila_dma_ctrl_reg
    
    PORT (
        clk : IN STD_LOGIC;
    
    
    
	    probe0 : IN STD_LOGIC_VECTOR(31 DOWNTO 0); 
        probe1 : IN STD_LOGIC_VECTOR(31 DOWNTO 0); 
        probe2 : IN STD_LOGIC_VECTOR(31 DOWNTO 0); 
        probe3 : IN STD_LOGIC_VECTOR(31 DOWNTO 0); 
        probe4 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
        probe5 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
        probe6 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
        probe7 : IN STD_LOGIC_VECTOR(1 DOWNTO 0); 
        probe8 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
        probe9 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
        probe10 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
        probe11 : IN STD_LOGIC_VECTOR(2 DOWNTO 0); 
        probe12 : IN STD_LOGIC_VECTOR(31 DOWNTO 0); 
        probe13 : IN STD_LOGIC_VECTOR(31 DOWNTO 0); 
        probe14 : IN STD_LOGIC_VECTOR(31 DOWNTO 0); 
        probe15 : IN STD_LOGIC_VECTOR(31 DOWNTO 0); 
        probe16 : IN STD_LOGIC_VECTOR(31 DOWNTO 0); 
        probe17 : IN STD_LOGIC_VECTOR(31 DOWNTO 0); 
        probe18 : IN STD_LOGIC_VECTOR(31 DOWNTO 0);
        probe19 : IN STD_LOGIC_VECTOR(0 DOWNTO 0);
        probe20 : IN STD_LOGIC_VECTOR(1 DOWNTO 0)
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
        probe8 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
        probe9 : IN STD_LOGIC_VECTOR(0 DOWNTO 0);
        probe10 : IN STD_LOGIC_VECTOR(0 DOWNTO 0);
        probe11 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
        probe12 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
        probe13 : IN STD_LOGIC_VECTOR(0 DOWNTO 0);
        probe14 : IN STD_LOGIC_VECTOR(3 DOWNTO 0);
        probe15 : IN STD_LOGIC_VECTOR(3 DOWNTO 0)
    );
    END COMPONENT  ;
    
    COMPONENT ila_pd_pdm
    
    PORT (
        clk : IN STD_LOGIC;
    
    
    
        probe0 : IN STD_LOGIC_VECTOR(63 DOWNTO 0); 
        probe1 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
        probe2 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
        probe3 : IN STD_LOGIC_VECTOR(63 DOWNTO 0); 
        probe4 : IN STD_LOGIC_VECTOR(7 DOWNTO 0); 
        probe5 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
        probe6 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
        probe7 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
        probe8 : IN STD_LOGIC_VECTOR(2 DOWNTO 0);
        probe9 : IN STD_LOGIC_VECTOR(2 DOWNTO 0);
        probe10 : IN STD_LOGIC_VECTOR(1 DOWNTO 0);
        probe11 : IN STD_LOGIC_VECTOR(1 DOWNTO 1)
    );
    END COMPONENT  ;
    
COMPONENT ila_l2p_dma
    
    PORT (
        clk : IN STD_LOGIC;
    
    
    
        probe0 : IN STD_LOGIC_VECTOR(31 DOWNTO 0); 
        probe1 : IN STD_LOGIC_VECTOR(31 DOWNTO 0); 
        probe2 : IN STD_LOGIC_VECTOR(31 DOWNTO 0); 
        probe3 : IN STD_LOGIC_VECTOR(31 DOWNTO 0); 
        probe4 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
        probe5 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
        probe6 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
        probe7 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
        probe8 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
        probe9 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
        probe10 : IN STD_LOGIC_VECTOR(63 DOWNTO 0); 
        probe11 : IN STD_LOGIC_VECTOR(7 DOWNTO 0); 
        probe12 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
        probe13 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
        probe14 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
        probe15 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
        probe16 : IN STD_LOGIC_VECTOR(31 DOWNTO 0); 
        probe17 : IN STD_LOGIC_VECTOR(63 DOWNTO 0); 
        probe18 : IN STD_LOGIC_VECTOR(63 DOWNTO 0); 
        probe19 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
        probe20 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
        probe21 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
        probe22 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
        probe23 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
        probe24 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
        probe25 : IN STD_LOGIC_VECTOR(2 DOWNTO 0); 
        probe26 : IN STD_LOGIC_VECTOR(12 DOWNTO 0); 
        probe27 : IN STD_LOGIC_VECTOR(12 DOWNTO 0); 
        probe28 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
        probe29 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
        probe30 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
        probe31 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
        probe32 : IN STD_LOGIC_VECTOR(63 DOWNTO 0); 
        probe33 : IN STD_LOGIC_VECTOR(63 DOWNTO 0); 
        probe34 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
        probe35 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
        probe36 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
        probe37 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
        probe38 : IN STD_LOGIC_VECTOR(63 DOWNTO 0); 
        probe39 : IN STD_LOGIC_VECTOR(63 DOWNTO 0); 
        probe40 : IN STD_LOGIC_VECTOR(12 DOWNTO 0);
        probe41 : IN STD_LOGIC_VECTOR(12 DOWNTO 0)
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


end app_pkg;


package body app_pkg is

end app_pkg;