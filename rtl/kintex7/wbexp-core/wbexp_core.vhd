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
-- Wishbone express top level
-- Dependencies:
-- wbexp_core_pkg
-- Revision:
-- Revision 0.01 - File Created
-- Additional Comments:
-- 
----------------------------------------------------------------------------------

library IEEE;
use IEEE.STD_LOGIC_1164.ALL;

use work.wshexp_core_pkg.ALL;

entity wshexp_core is
    Generic(
        AXI_BUS_WIDTH : integer := 64
    );
    Port ( 
        clk_i    : in STD_LOGIC; --! PCIe user clock 250 MHz
        wb_clk_i : in STD_LOGIC; --! Wishbone bus clock
        rst_i    : in STD_LOGIC; --! Reset input active high
        
        ---------------------------------------------------------
        -- AXI-Stream bus
        m_axis_tx_tready_i : in  STD_LOGIC;					--! AXI-Stream bus: Transmit destination ready to accept data
        m_axis_tx_tdata_o  : out STD_LOGIC_VECTOR(AXI_BUS_WIDTH-1 DOWNTO 0);	--! AXI-Stream bus: Transmit data
        m_axis_tx_tkeep_o  : out STD_LOGIC_VECTOR(AXI_BUS_WIDTH/8-1 DOWNTO 0);  --! AXI-Stream bus: Transmit data strobe
        m_axis_tx_tlast_o  : out STD_LOGIC;					--! AXI-Stream bus: Indicates the last data beaf of a packet
        m_axis_tx_tvalid_o : out STD_LOGIC;					--! AXI-Stream bus: Indicates valid transmit data
        m_axis_tx_tuser_o  : out STD_LOGIC_VECTOR(3 DOWNTO 0);			--! AXI-Stream bus: Indicates custom informations about the transmit destination [PG054]
        s_axis_rx_tdata_i  : in  STD_LOGIC_VECTOR(AXI_BUS_WIDTH-1 DOWNTO 0);	--! AXI-Stream bus: Receive data
        s_axis_rx_tkeep_i  : in  STD_LOGIC_VECTOR(AXI_BUS_WIDTH/8-1 DOWNTO 0);  --! AXI-Stream bus: Receive data strobe
        s_axis_rx_tlast_i  : in  STD_LOGIC;					--! AXI-Stream bus: Indicates the last data beaf of a packet
        s_axis_rx_tvalid_i : in  STD_LOGIC;					--! AXI-Stream bus: Indicates valid receive data
        s_axis_rx_tready_o : out STD_LOGIC;					--! AXI-Stream bus: Receive source ready to accept data
        s_axis_rx_tuser_i  : in  STD_LOGIC_VECTOR(21 DOWNTO 0);			--! AXI-Stream bus: Indicates custom informations about the receive source [PG054]

        ---------------------------------------------------------
        -- DMA wishbone interface (master pipelined)        
        dma_adr_o   : out std_logic_vector(31 downto 0);  --! DMA Wishbone Bus: Adress
        dma_dat_o   : out  std_logic_vector(63 downto 0); --! DMA Wishbone Bus: Data out
        dma_dat_i   : in  std_logic_vector(63 downto 0);  --! DMA Wishbone Bus: Data in
        dma_sel_o   : out std_logic_vector(7 downto 0);   --! DMA Wishbone Bus: Byte select
        dma_cyc_o   : out std_logic;                      --! DMA Wishbone Bus: Read or write cycle
        dma_stb_o   : out std_logic;                      --! DMA Wishbone Bus: Read or write strobe
        dma_we_o    : out std_logic;                      --! DMA Wishbone Bus: Write enable
        dma_ack_i   : in std_logic;                       --! DMA Wishbone Bus: Acknowledge
        dma_stall_i : in std_logic;                       --! DMA Wishbone Bus: for pipelined Wishbone
        
        ---------------------------------------------------------
        -- CSR wishbone interface (master classic)
        csr_adr_o   : out std_logic_vector(31 downto 0);  --! CSR Wishbone Bus: Address
        csr_dat_o   : out std_logic_vector(31 downto 0);  --! CSR Wishbone Bus: Data out
        csr_sel_o   : out std_logic_vector(3 downto 0);   --! CSR Wishbone Bus: Byte select
        csr_stb_o   : out std_logic;                      --! CSR Wishbone Bus: Read or write cycle
        csr_we_o    : out std_logic;                      --! CSR Wishbone Bus: Write enable
        csr_cyc_o   : out std_logic;                      --! CSR Wishbone Bus: Read or write strobe
        csr_dat_i   : in  std_logic_vector(31 downto 0);  --! CSR Wishbone Bus: Data in
        csr_ack_i   : in  std_logic;                      --! CSR Wishbone Bus: Acknoledge
        csr_stall_i : in  std_logic;                      --! CSR Wishbone Bus: for pipelined Wishbone
        csr_err_i   : in  std_logic;                      --! CSR Wishbone Bus: Error
        csr_rty_i   : in  std_logic;                      --! not used internally
        csr_int_i   : in  std_logic;                      --! not used internally
        
        ---------------------------------------------------------
        -- DMA registers wishbone interface (slave classic)
        dma_reg_adr_i   : in  std_logic_vector(31 downto 0); --! DMA Registers Bus: Address
        dma_reg_dat_i   : in  std_logic_vector(31 downto 0); --! DMA Registers Bus: Data in
        dma_reg_sel_i   : in  std_logic_vector(3 downto 0);  --! DMA Registers Bus: Byte select
        dma_reg_stb_i   : in  std_logic;                     --! DMA Registers Bus: Read or write strobe
        dma_reg_we_i    : in  std_logic;                     --! DMA Registers Bus: Write enable
        dma_reg_cyc_i   : in  std_logic;                     --! DMA Registers Bus: Read or write cycle
        dma_reg_dat_o   : out std_logic_vector(31 downto 0); --! DMA Registers Bus: Data out
        dma_reg_ack_o   : out std_logic;                     --! DMA Registers Bus: Acknoledge
        dma_reg_stall_o : out std_logic;                     --! DMA Registers Bus: for pipelined wishbone
        
        ---------------------------------------------------------
        -- PCIe interrupt config
        cfg_interrupt_o                : out STD_LOGIC;                     --! Interrupt request signal
        cfg_interrupt_rdy_i            : in  STD_LOGIC;                     --! Interrupt grant signal
        cfg_interrupt_assert_o         : out STD_LOGIC;                     --! Not used
        cfg_interrupt_di_o             : out STD_LOGIC_VECTOR(7 DOWNTO 0);  --! Interrupt message data out
        cfg_interrupt_do_i             : in  STD_LOGIC_VECTOR(7 DOWNTO 0);  --! Interrupt message data in
        cfg_interrupt_mmenable_i       : in  STD_LOGIC_VECTOR(2 DOWNTO 0);  --! Intteurpt multiple message enable
        cfg_interrupt_msienable_i      : in  STD_LOGIC;                     --! Not used
        cfg_interrupt_msixenable_i     : in  STD_LOGIC;                     --! Not used
        cfg_interrupt_msixfm_i         : in  STD_LOGIC;                     --! Not used
        cfg_interrupt_stat_o           : out STD_LOGIC;                     --! Not used
        cfg_pciecap_interrupt_msgnum_o : out STD_LOGIC_VECTOR(4 DOWNTO 0);  --! Not used
        
        ---------------------------------------------------------
        -- PCIe ID
        cfg_bus_number_i : in STD_LOGIC_VECTOR(7 DOWNTO 0);                 --! PCIe bus number (usually 0)
        cfg_device_number_i : in STD_LOGIC_VECTOR(4 DOWNTO 0);              --! PCIe device number (lspci)
        cfg_function_number_i : in STD_LOGIC_VECTOR(2 DOWNTO 0)             --! PCIe function number (lspci)
    );
end wshexp_core;

architecture Behavioral of wshexp_core is
    constant axis_data_width_c : integer := 64;
    ---------------------------------------------------------
    -- Reset and Clocks
    signal rst_n_s : std_logic; 
    
    ---------------------------------------------------------
    -- PCIe
    signal cfg_interrupt_s : std_logic;
    signal pcie_id_s : std_logic_vector (15 downto 0); -- Completer/Requester ID

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
    -- From Packet decoder to Wishbone master (wbm)    
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
    signal pd_pdm_data_s : STD_LOGIC_VECTOR(AXI_BUS_WIDTH - 1 downto 0);
    signal pd_pdm_keep_s : std_logic_vector(7 downto 0);
    signal p2l_dma_rdy_s : std_logic;
    
    ---------------------------------------------------------
    -- From the DMA ctrl registers to the L2P DMA master and P2L DMA master
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
    -- From P2L Master to the Arbiter
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
    
    ---------------------------------------------------------
    -- From DMA ctrl registers to PCIe
    signal dma_ctrl_irq_s : std_logic_vector(1 downto 0);
    
	---------------------------------------------------------
    -- From P2L DMA master to DMA ctrl registers
    signal next_item_carrier_addr_s : std_logic_vector(31 downto 0);
    signal next_item_host_addr_h_s  : std_logic_vector(31 downto 0);
    signal next_item_host_addr_l_s  : std_logic_vector(31 downto 0);
    signal next_item_len_s          : std_logic_vector(31 downto 0);
    signal next_item_next_l_s       : std_logic_vector(31 downto 0);
    signal next_item_next_h_s       : std_logic_vector(31 downto 0);
    signal next_item_attrib_s       : std_logic_vector(31 downto 0);
    signal next_item_valid_s        : std_logic;
    
    ---------------------------------------------------------
    -- From L2P DMA master (ldm) to arbiter (arb)
    signal ldm_arb_tdata_s : std_logic_vector (AXI_BUS_WIDTH - 1 downto 0);
    signal ldm_arb_tkeep_s : std_logic_vector (AXI_BUS_WIDTH/8 - 1 downto 0);
    signal ldm_arb_tlast_s : std_logic;
    signal ldm_arb_tvalid_s : std_logic;
    signal ldm_arb_tready_s : std_logic;
    signal ldm_arb_req_s    : std_logic;
    
    ---------------------------------------------------------
    -- From Wishbone master (wbm) to arbiter (arb)      
    signal wbm_arb_tdata_s : std_logic_vector (AXI_BUS_WIDTH - 1 downto 0);
    signal wbm_arb_tkeep_s : std_logic_vector (AXI_BUS_WIDTH/8 - 1 downto 0);
    signal wbm_arb_tlast_s : std_logic;
    signal wbm_arb_tvalid_s : std_logic;
    signal wbm_arb_req_s    : std_logic;
    signal wbm_arb_tready_s : std_logic;

begin

    rst_n_s <= not rst_i;
    
    
    wbm_pd_ready_s <= p2l_wbm_rdy_s and p2l_dma_rdy_s;
    -- Slave AXI-Stream
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
    
    ---------------------------------------------------------
    -- PCIe interrupt and ID
    cfg_interrupt_assert_o <= '0';
    cfg_interrupt_di_o <= (others => '0');
    cfg_interrupt_stat_o <= '0';
    cfg_pciecap_interrupt_msgnum_o <= (others => '0');
    
    cfg_interrupt_o <= cfg_interrupt_s;

    interrupt_p : process(rst_i,clk_i,cfg_interrupt_rdy_i)
--    interrupt_p : process(rst_i,clk_i)
    begin


        
       
        
        if (rst_i = '1') then
            cfg_interrupt_s <= '0';
        elsif (cfg_interrupt_rdy_i = '1') then
                cfg_interrupt_s <= '0';

        elsif(clk_i'event and clk_i = '1') then
            cfg_interrupt_s <= cfg_interrupt_s;
            --if (cfg_interrupt_rdy_i = '1') then
                --cfg_interrupt_s <= '0';
            if (dma_ctrl_irq_s /= "00") then
                cfg_interrupt_s <= '1';
            end if;
            
    
        end if;
    end process interrupt_p;
    
    id_p : process(rst_i,clk_i)
    begin
        if (rst_i = '1') then
            pcie_id_s <= (others=> '0');
        elsif(clk_i'event and clk_i = '1') then
            pcie_id_s <= cfg_bus_number_i & cfg_device_number_i & cfg_function_number_i;
            
    
        end if;
    end process id_p;
    
    
    
    
    -- DMA registers is a classic wishbone slave supporting single pipelined cycles
    dma_reg_stall_o <= '0';
    

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
    
    csr_adr_o(31) <= '0';
    
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
        pd_wbm_data_i       => pd_wbm_data_s, -- Data
        
        ---------------------------------------------------------
        -- P2L channel control
        p_wr_rdy_o   =>  open,-- Ready to accept target write
        p2l_rdy_o    =>  p2l_wbm_rdy_s,--wbm_pd_ready_s,                   -- De-asserted to pause transfer already in progress
        p_rd_d_rdy_i =>  "11",-- Asserted when GN4124 ready to accept read completion with data
        
        ---------------------------------------------------------
        -- To the arbiter (L2P data)
        wbm_arb_tdata_o => wbm_arb_tdata_s,
        wbm_arb_tkeep_o => wbm_arb_tkeep_s,
        wbm_arb_tlast_o => wbm_arb_tlast_s,
        wbm_arb_tvalid_o => wbm_arb_tvalid_s,
        wbm_arb_tready_i => wbm_arb_tready_s,
        wbm_arb_req_o    => wbm_arb_req_s,
        
        ---------------------------------------------------------
        -- CSR wishbone interface
        wb_clk_i   =>  wb_clk_i,                     -- Wishbone bus clock
        wb_adr_o   =>  csr_adr_o(30 downto 0),-- Address
        wb_dat_o   =>  csr_dat_o,-- Data out
        wb_sel_o   =>  csr_sel_o, -- Byte select
        wb_stb_o   =>  csr_stb_o,                    -- Strobe
        wb_we_o    =>  csr_we_o,                    -- Write
        wb_cyc_o   =>  csr_cyc_o,                    -- Cycle
        wb_dat_i   =>  csr_dat_i,-- Data in
        wb_ack_i   =>  csr_ack_i,                    -- Acknowledge
        wb_stall_i =>  csr_stall_i,                    -- Stall
        wb_err_i   =>  csr_err_i,                    -- Error
        wb_rty_i   =>  csr_rty_i,                    -- Retry
        wb_int_i   =>  csr_int_i                     -- Interrupt
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
		  p2l_dma_clk_i   => wb_clk_i,                      -- Bus clock
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
		dma_ctrl_byte_swap_i   => "000",
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

		l2p_dma_clk_i   => wb_clk_i,
		l2p_dma_adr_o   => l2p_dma_adr_s,
		l2p_dma_dat_i   => l2p_dma_dat_s2m_s,
		l2p_dma_dat_o   => l2p_dma_dat_m2s_s,
		l2p_dma_sel_o   => l2p_dma_sel_s,
		l2p_dma_cyc_o   => l2p_dma_cyc_s,
		l2p_dma_stb_o   => l2p_dma_stb_s,
		l2p_dma_we_o    => l2p_dma_we_s,
		l2p_dma_ack_i   => l2p_dma_ack_s,
		l2p_dma_stall_i => l2p_dma_stall_s,
		p2l_dma_cyc_i   => p2l_dma_cyc_s--,
		
		--DMA Debug
        --l2p_current_state_do => l2p_current_state_ds,
        --l2p_data_cnt_do => l2p_data_cnt_ds,
        --l2p_len_cnt_do  => l2p_len_cnt_ds,
        --l2p_timeout_cnt_do => l2p_timeout_cnt_ds,
        --wb_timeout_cnt_do => wb_timeout_cnt_ds,
        
        -- Data FIFO
        --data_fifo_rd_do    => data_fifo_rd_ds,
        --data_fifo_wr_do    => data_fifo_wr_ds,
        --data_fifo_empty_do => data_fifo_empty_ds,
        --data_fifo_full_do  => data_fifo_full_ds,
        --data_fifo_dout_do  => data_fifo_dout_ds,
        --data_fifo_din_do   => data_fifo_din_ds,
        
        -- Addr FIFO
        --addr_fifo_rd_do    => addr_fifo_rd_ds,
        --addr_fifo_wr_do    => addr_fifo_wr_ds,
        --addr_fifo_empty_do => addr_fifo_empty_ds,
        --addr_fifo_full_do  => addr_fifo_full_ds,
        --addr_fifo_dout_do  => addr_fifo_dout_ds,
        --addr_fifo_din_do   => addr_fifo_din_ds
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
          wb_clk_i => wb_clk_i,                     -- Bus clock
          wb_adr_i => dma_reg_adr_i(3 downto 0),   -- Adress
          wb_dat_o => dma_reg_dat_o,  -- Data in
          wb_dat_i => dma_reg_dat_i,  -- Data out
          wb_sel_i => dma_reg_sel_i,   -- Byte select
          wb_cyc_i => dma_reg_cyc_i,                      -- Read or write cycle
          wb_stb_i => dma_reg_stb_i,                      -- Read or write strobe
          wb_we_i  => dma_reg_we_i,                      -- Write
          wb_ack_o => dma_reg_ack_o--,                       -- Acknowledge

          ---------------------------------------------------------
          -- Debug interface          
          --dma_ctrl_current_state_do => dma_ctrl_current_state_ds,
          --dma_ctrl_do => dma_ctrl_ds,
          --dma_stat_do => dma_stat_ds,
          --dma_attrib_do => dma_attrib_ds
          );

      -- Status signals from DMA masters
    dma_ctrl_done_s  <= dma_ctrl_l2p_done_s or dma_ctrl_p2l_done_s;
    dma_ctrl_error_s <= dma_ctrl_l2p_error_s or dma_ctrl_p2l_error_s;
    
	arbiter:l2p_arbiter
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
        axis_tx_tready_i => m_axis_tx_tready_s--,
        
        ---------------------------------------------------------
        -- Debug
        --eop_do => eop_s
    );
    
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
    --dma_stall_s <= '0';
    
    ---------------------------------------------------------
    -- DMA wishbone interface (master pipelined)        
    dma_adr_o   <= dma_adr_s;
    dma_dat_o   <= dma_dat_m2s_s;
    dma_dat_s2m_s <= dma_dat_i;
    dma_sel_o   <= dma_sel_s;
    dma_cyc_o   <= dma_cyc_s;
    dma_stb_o   <= dma_stb_s;
    dma_we_o    <= dma_we_s;
    dma_ack_s   <= dma_ack_i;
    dma_stall_s <= dma_stall_i;
    
--    axis_debug : ila_axis
--    PORT MAP (
--        clk => clk_i,
    
    
    
--        probe0 => s_axis_rx_tdata_s, 
--        probe1 => s_axis_rx_tkeep_s, 
--        probe2(0) => s_axis_rx_tlast_s, 
--        probe3(0) => s_axis_rx_tvalid_s, 
--        probe4(0) => s_axis_rx_tready_s, 
--        probe5 => m_axis_tx_tdata_s, 
--        probe6 => m_axis_tx_tkeep_s, 
--        probe7(0) => m_axis_tx_tlast_s, 
--        probe8(0) => m_axis_tx_tvalid_s,
--        probe9(0) => m_axis_tx_tready_s,
--        probe10 => s_axis_rx_tuser_i, 
--        probe11(0) => dma_ctrl_start_l2p_s, 
--        probe12(0) => dma_ctrl_start_p2l_s, 
--        probe13(0) => dma_ctrl_start_next_s,
--        probe14(0) => dma_ctrl_abort_s, 
--        probe15(0) => dma_ctrl_done_s,
--        probe16(0) => dma_ctrl_error_s,
--        probe17(0) => '0',--user_lnk_up_i,
--        probe18(0) => '0',--cfg_interrupt_s,
--        probe19(0) => '0',--cfg_interrupt_rdy_i,
--        probe20(0) => '0',--dma_ctrl_done_s,
--        probe21 => (others => '0'),--wbm_arb_tready_s & wbm_arb_tready_s & ldm_arb_tready_s,--dma_ctrl_current_state_ds,
--        probe22(0) => next_item_valid_s, --tx_err_drop_i,
--        probe23 => (others => '0')--iteration_count_s
--    );

--      pipelined_wishbone_debug : ila_wsh_pipe
--      PORT MAP (
--          clk => wb_clk_i,
      
      
      
--          probe0 => dma_adr_s, 
--          probe1 => dma_dat_s2m_s, 
--          probe2 => dma_dat_m2s_s, 
--          probe3 => dma_sel_s, 
--          probe4(0) => dma_cyc_s, 
--          probe5(0) => dma_stb_s, 
--          probe6(0) => dma_we_s, 
--          probe7(0) => dma_ack_s,
--          probe8(0) => dma_stall_s, 
--          probe9(0) => l2p_dma_cyc_s,
--          probe10(0) => p2l_dma_cyc_s,
--          probe11(0) => dma_ctrl_start_l2p_s, 
--          probe12(0) => dma_ctrl_start_p2l_s, 
--          probe13(0) => dma_ctrl_start_next_s,
--          probe14 => (others => '0'),--ddr_rd_mask_rd_data_count_ds,
--          probe15 => (others => '0'),--ddr_rd_data_rd_data_count_ds,
--          probe16 => (others => '0'),--ddr_wb_rd_mask_addr_dout_ds & ddr_wb_rd_mask_dout_ds,
--          probe17 => (others => '0')--iteration_count_s
--      );

end Behavioral;
