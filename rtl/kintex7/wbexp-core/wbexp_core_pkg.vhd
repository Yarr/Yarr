---------------------------------------------------------------------------------- 
-- Company: LBNL 
-- Engineer: Arnaud Sautaux 
--  
-- Create Date: 07/27/2017 10:50:41 AM 
-- Design Name: Wishbone express core 
-- Module Name: wshexp_core - Behavioral 
-- Project Name: YARR 
-- Target Devices:  
-- Tool Versions: Vivado v2016.2 (64 bit) 
-- Description:  
-- Wishbone express package
-- Dependencies: 
-- Revision: 
-- Revision 0.01 - File Created 
-- Additional Comments: 
--  
---------------------------------------------------------------------------------- 



library IEEE;
use IEEE.STD_LOGIC_1164.all;
use IEEE.NUMERIC_STD.all;



--==============================================================================
--! Package declaration
--==============================================================================
package wshexp_core_pkg is


--==============================================================================
--! Constants declaration
--==============================================================================
  constant c_RST_ACTIVE : std_logic := '0';  -- Active low reset


--==============================================================================
--! Functions declaration
--==============================================================================
  function f_byte_swap_64 (
    constant enable    : boolean;
    signal   din       : std_logic_vector(63 downto 0);
    signal   byte_swap : std_logic_vector(2 downto 0))
    return std_logic_vector;
    
  function f_byte_swap (
    constant enable    : boolean;
    signal   din       : std_logic_vector(31 downto 0);
    signal   byte_swap : std_logic_vector(1 downto 0))
    return std_logic_vector;

  function log2_ceil(N : natural) return positive;

--==============================================================================
--! Components declaration
--==============================================================================

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
        probe8 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
        probe9 : IN STD_LOGIC_VECTOR(0 DOWNTO 0);
        probe10 : IN STD_LOGIC_VECTOR(0 DOWNTO 0);
        probe11 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
        probe12 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
        probe13 : IN STD_LOGIC_VECTOR(0 DOWNTO 0);
        probe14 : IN STD_LOGIC_VECTOR(3 DOWNTO 0);
        probe15 : IN STD_LOGIC_VECTOR(3 DOWNTO 0);
        probe16 : IN STD_LOGIC_VECTOR(36 DOWNTO 0);
        probe17 : IN STD_LOGIC_VECTOR(28 DOWNTO 0)
    );
    END COMPONENT  ;
-----------------------------------------------------------------------------



end wshexp_core_pkg;

package body wshexp_core_pkg is

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
  -- Byte swap function
  --
  -- enable | byte_swap  | din      | dout
  -- false  | XXX        | ABCDEFGH | ABCDEFGH
  -- true   | 000        | ABCDEFGH | ABCDEFGH
  -- true   | 001        | ABCDEFGH | BADCFEHG
  -- true   | 010        | ABCDEFGH | CDABGHEF
  -- true   | 011        | ABCDEFGH | DCBAHGFE
  -- true   | 100        | ABCDEFGH | EFGHABCD
  -- true   | 101        | ABCDEFGH | FEHGBADC
  -- true   | 110        | ABCDEFGH | GHEFCDAB
  -- true   | 111        | ABCDEFGH | HGFEDCBA
  -----------------------------------------------------------------------------
  function f_byte_swap_64 (
    constant enable    : boolean;
    signal   din       : std_logic_vector(63 downto 0);
    signal   byte_swap : std_logic_vector(2 downto 0))
    return std_logic_vector is
    variable dout : std_logic_vector(63 downto 0) := din;
  begin
    if (enable = true) then
      if byte_swap(2) = '0' then
        dout := f_byte_swap(true, din(63 downto 32), byte_swap(1 downto 0)) & f_byte_swap(true, din(31 downto 0), byte_swap(1 downto 0));
      else
        dout := f_byte_swap(true, din(31 downto 0), byte_swap(1 downto 0)) & f_byte_swap(true, din(63 downto 32), byte_swap(1 downto 0));
      end if;
      
    else
      dout := din;
    end if;
    return dout;
  end function f_byte_swap_64;



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


end wshexp_core_pkg;
