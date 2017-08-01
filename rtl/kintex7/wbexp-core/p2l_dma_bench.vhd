library IEEE;
USE IEEE.STD_LOGIC_1164.all;
USE IEEE.NUMERIC_STD.all;
use IEEE.std_logic_unsigned.all; 


entity p2l_dma_bench is
    generic (
		constant period : time := 100 ns;
		constant axis_data_width_c : integer := 64;

		constant wb_address_width_c : integer := 12;
		constant wb_data_width_c : integer := 64
	);
	--port ();
end p2l_dma_bench;

architecture Behavioral of p2l_dma_bench is
		signal clk_tbs : STD_LOGIC;
		signal rst_tbs : STD_LOGIC;
		signal rst_n_tbs : STD_LOGIC;
		-- Test bench specific signals
		signal step : integer;
		
		-- From the DMA controller
		signal dma_ctrl_target_addr_tbs : std_logic_vector(32-1 downto 0);
		signal dma_ctrl_host_addr_h_tbs : std_logic_vector(32-1 downto 0);
		signal dma_ctrl_host_addr_l_tbs : std_logic_vector(32-1 downto 0);
		signal dma_ctrl_len_tbs         : std_logic_vector(32-1 downto 0);
		signal dma_ctrl_start_p2l_tbs   : std_logic;
		signal dma_ctrl_start_next_tbs   : std_logic;
		signal dma_ctrl_done_s        : std_logic;
		signal dma_ctrl_error_s       : std_logic;
		signal dma_ctrl_byte_swap_tbs   : std_logic_vector(2 downto 0);
		signal dma_ctrl_abort_tbs       : std_logic;

		---------------------------------------------------------
		-- From P2L Decoder (receive the read completion)
		--
		-- Header
		--signal pd_pdm_hdr_start_tbs   : std_logic;                      -- Header strobe
		--signal pd_pdm_hdr_length_tbs  : std_logic_vector(9 downto 0);   -- Packet length in 32-bit words multiples
		--signal pd_pdm_hdr_cid_tbs     : std_logic_vector(1 downto 0);   -- Completion ID
		signal pd_pdm_master_cpld_tbs : std_logic;                      -- Master read completion with data
		signal pd_pdm_master_cpln_tbs : std_logic;                      -- Master read completion without data
		--
		-- Data
		signal pd_pdm_data_valid_tbs  : std_logic;                      -- Indicates Data is valid
		signal pd_pdm_data_valid_w_tbs: std_logic_vector(1 downto 0);
		signal pd_pdm_data_last_tbs   : std_logic;                      -- Indicates end of the packet
		signal pd_pdm_data_tbs        : std_logic_vector(wb_data_width_c-1 downto 0);  -- Data
		signal pd_pdm_be_tbs          : std_logic_vector(7 downto 0);   -- Byte Enable for data

		---------------------------------------------------------
		-- P2L control
		signal p2l_rdy_s  : std_logic;       -- De-asserted to pause transfer already in progress
		signal rx_error_s : std_logic;       -- Asserted when transfer is aborted

		---------------------------------------------------------
		-- To the P2L Interface (send the DMA Master Read request)
		signal pdm_arb_tvalid_s  : std_logic;  -- Read completion signals
		signal pdm_arb_tlast_s : std_logic;  -- Toward the arbiter
		signal pdm_arb_tdata_s   : std_logic_vector(wb_data_width_c-1 downto 0);
		signal pdm_arb_tkeep_s   : std_logic_vector(7 downto 0);
		signal pdm_arb_req_s    : std_logic;
		signal arb_pdm_gnt_tbs    : std_logic;

		-- DMA Interface (Pipelined Wishbone)
		signal p2l_dma_adr_s   : std_logic_vector(32-1 downto 0);
		signal p2l_dma_dat_s2m_s   : std_logic_vector(wb_data_width_c-1 downto 0);
		signal p2l_dma_dat_m2s_s   : std_logic_vector(wb_data_width_c-1 downto 0);
		signal p2l_dma_sel_s   : std_logic_vector(7 downto 0);
		signal p2l_dma_cyc_s   : std_logic;
		signal p2l_dma_stb_s   : std_logic;
		signal p2l_dma_we_s    : std_logic;
		signal p2l_dma_ack_s   : std_logic;
		signal p2l_dma_stall_tbs : std_logic;
		signal l2p_dma_cyc_tbs : std_logic;
		
		---------------------------------------------------------
		-- To the DMA controller
		signal next_item_carrier_addr_s : std_logic_vector(31 downto 0);
		signal next_item_host_addr_h_s  : std_logic_vector(31 downto 0);
		signal next_item_host_addr_l_s  : std_logic_vector(31 downto 0);
		signal next_item_len_s          : std_logic_vector(31 downto 0);
		signal next_item_next_l_s       : std_logic_vector(31 downto 0);
		signal next_item_next_h_s       : std_logic_vector(31 downto 0);
		signal next_item_attrib_s       : std_logic_vector(31 downto 0);
		signal next_item_valid_s        : std_logic;

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
		  --pd_pdm_hdr_start_i   : in std_logic;                      -- Header strobe
		  --pd_pdm_hdr_length_i  : in std_logic_vector(9 downto 0);   -- Packet length in 32-bit words multiples
		  --pd_pdm_hdr_cid_i     : in std_logic_vector(1 downto 0);   -- Completion ID
		  pd_pdm_master_cpld_i : in std_logic;                      -- Master read completion with data
		  pd_pdm_master_cpln_i : in std_logic;                      -- Master read completion without data
		  --
		  -- Data
		  pd_pdm_data_valid_i  : in std_logic;                      -- Indicates Data is valid
		  pd_pdm_data_valid_w_i: in std_logic_vector(1 downto 0);  -- Indicates Data is valid
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

		
	component bram_wbs is
	generic (
		constant ADDR_WIDTH : integer := 16;
		constant DATA_WIDTH : integer := 64 
	);
	port (
		-- SYS CON
		clk			: in std_logic;
		rst			: in std_logic;
		
		-- Wishbone Slave in
		wb_adr_i			: in std_logic_vector(wb_address_width_c-1 downto 0);
		wb_dat_i			: in std_logic_vector(wb_data_width_c-1 downto 0);
		wb_we_i			: in std_logic;
		wb_stb_i			: in std_logic;
		wb_cyc_i			: in std_logic; 
		wb_lock_i		: in std_logic; -- nyi
		
		-- Wishbone Slave out
		wb_dat_o			: out std_logic_vector(wb_data_width_c-1 downto 0);
		wb_ack_o			: out std_logic		
	);
	end component;
begin

	clk_p: process
	begin
		clk_tbs <= '1';
		wait for period/2;
		clk_tbs <= '0';
		wait for period/2;
	end process clk_p;
	
	reset_p: process
	begin
	   rst_tbs <= '1';
	   rst_n_tbs <= '0';
	   wait for period;
	   rst_tbs <= '0';
	   rst_n_tbs <= '1';
	   wait;
	end process reset_p;
	
	stimuli_p: process
	begin
    step <= 1;
    dma_ctrl_target_addr_tbs <= (others => '0');
    dma_ctrl_host_addr_h_tbs <= (others => '0');
    dma_ctrl_host_addr_l_tbs <= (others => '0');
    dma_ctrl_len_tbs         <= (others => '0');
    dma_ctrl_start_p2l_tbs   <= '0';
    dma_ctrl_start_next_tbs  <= '0';
    dma_ctrl_byte_swap_tbs   <= (others => '0');
    dma_ctrl_abort_tbs       <= '0';
    
    ---------------------------------------------------------
    -- From P2L Decoder (receive the read completion)
    --
    -- Header
    --pd_pdm_hdr_start_tbs   <= '0';                     -- Header strobe
    --pd_pdm_hdr_length_tbs  <= (others => '0');   -- Packet length in 32-bit words multiples
    --pd_pdm_hdr_cid_tbs     <= (others => '0');   -- Completion ID
    pd_pdm_master_cpld_tbs <= '0';                      -- Master read completion with data
    pd_pdm_master_cpln_tbs <= '0';                      -- Master read completion without data
    --
    -- Data
    pd_pdm_data_valid_tbs  <= '0';                      -- Indicates Data is valid
    pd_pdm_data_last_tbs   <= '0';                      -- Indicates end of the packet
    pd_pdm_data_tbs        <= (others => '0');  -- Data
    pd_pdm_be_tbs          <= (others => '0');   -- Byte Enable for data
    
    arb_pdm_gnt_tbs    <= '0';
    p2l_dma_stall_tbs <= '0';
    l2p_dma_cyc_tbs <= '0';
    
    wait for period;
    
    dma_ctrl_target_addr_tbs <= X"00000010";
    dma_ctrl_host_addr_h_tbs <= X"00000001";
    dma_ctrl_host_addr_l_tbs <= X"0000005A";
    dma_ctrl_len_tbs         <= X"00000040";
    dma_ctrl_start_p2l_tbs   <= '0';
    dma_ctrl_start_next_tbs  <= '0';
    dma_ctrl_byte_swap_tbs   <= (others => '0');
    dma_ctrl_abort_tbs       <= '0';
    
    ---------------------------------------------------------
    -- From P2L Decoder (receive the read completion)
    --
    -- Header
    --pd_pdm_hdr_start_tbs   <= '0';                     -- Header strobe
    --pd_pdm_hdr_length_tbs  <= "00" & X"01";   -- Packet length in 32-bit words multiples
    --pd_pdm_hdr_cid_tbs     <= (others => '0');   -- Completion ID
    pd_pdm_master_cpld_tbs <= '1';                      -- Master read completion with data
    pd_pdm_master_cpln_tbs <= '0';                      -- Master read completion without data
    --
    -- Data
    pd_pdm_data_valid_tbs  <= '0';                      -- Indicates Data is valid
    pd_pdm_data_last_tbs   <= '0';                      -- Indicates end of the packet
    pd_pdm_data_tbs        <= (others => '0');  -- Data
    pd_pdm_be_tbs          <= (others => '0');   -- Byte Enable for data
    
    arb_pdm_gnt_tbs    <= '1';
    p2l_dma_stall_tbs <= '0';
    l2p_dma_cyc_tbs <= '0';		
    wait for period;
    step <= 2;
    
    dma_ctrl_start_p2l_tbs   <= '1';
    
    wait for period;
    dma_ctrl_start_p2l_tbs   <= '0';
    wait for period;
    wait for period;
    wait for period;
    wait for period;
    
    step <= 3;
    
    dma_ctrl_start_p2l_tbs   <= '0';
    
    pd_pdm_data_valid_tbs  <= '1';                      -- Indicates Data is valid
    pd_pdm_data_valid_w_tbs  <= "11";
    pd_pdm_data_last_tbs   <= '0';                      -- Indicates end of the packet
    pd_pdm_data_tbs        <= X"DEADBEEF00000000";  -- Header
    pd_pdm_be_tbs          <= X"FF";   -- Byte Enable for data
    
    wait for period;
    step <= 4;
    
    pd_pdm_data_tbs        <= X"DEADBABE00000001";  -- Adresse
    
    wait for period;
    step <= 5;
    
    pd_pdm_data_tbs        <= X"BABECACA00000002";  -- Data
    
    wait for period;
    step <= 6;
    
    
    pd_pdm_data_tbs        <= X"BABECACA00000003";  -- Data
    
    wait for period;
    step <= 7;
    
    pd_pdm_data_tbs        <= X"BABECACA00000004";  -- Data
    
    wait for period;
    step <= 8;
    
    pd_pdm_data_tbs        <= X"BABECACA00000005";  -- Data
    
    wait for period;
    step <= 9;
    
    pd_pdm_data_tbs        <= X"BABECACA00000006";  -- Data
    
    wait for period;
    step <= 10;
    
    pd_pdm_data_last_tbs   <= '1';
    pd_pdm_data_tbs        <= X"DEADC4C400000007";  -- Data
    
    wait for period;
    step <= 11;
    
    pd_pdm_data_valid_w_tbs  <= "00";
    pd_pdm_data_valid_tbs  <= '0';                      -- Indicates Data is valid
    pd_pdm_data_last_tbs   <= '0';
    
--    wait for period;
--    wait for period;
--    wait for period;
    
--    step <= 20;
--    dma_ctrl_len_tbs         <= X"00000020";
--    dma_ctrl_start_next_tbs  <= '1';
--    wait for period;
    
--    step <= 21;
--    dma_ctrl_start_next_tbs  <= '0';
--    wait for period;
    
--    step <= 22;
--    wait for period;
    
--    step <= 23;
--    wait for period;
    
--    step <= 24;
--    pd_pdm_data_tbs        <= X"0000000300000360";
--    pd_pdm_data_valid_tbs  <= '1';
--    pd_pdm_data_valid_w_tbs  <= "11";
--    wait for period;
    
--    step <= 25;
--    pd_pdm_data_tbs        <= X"000004E0922d1000";
--    wait for period;
    
--    step <= 26;
--    pd_pdm_data_tbs        <= X"0000000403a57038";
--    wait for period;
    
--    step <= 27;
--    pd_pdm_data_tbs        <=  X"CACA000000000001";
--    pd_pdm_data_last_tbs   <= '1';
--    wait for period;       
    
--    step <= 28;
--    pd_pdm_data_tbs        <=  X"CACA1000000000CC";
--    pd_pdm_data_valid_w_tbs  <= "00";
--    pd_pdm_data_valid_tbs  <= '0';                      -- Indicates Data is valid
--    pd_pdm_data_last_tbs   <= '0';
    
    wait for period;
    wait for period;
    wait for period;
    
    step <= 12;
    dma_ctrl_len_tbs         <= X"0000001C";
    dma_ctrl_start_next_tbs  <= '1';
    wait for period;
    
    step <= 13;
    dma_ctrl_start_next_tbs  <= '0';
    wait for period;
    
    step <= 14;
    wait for period;
    
    step <= 15;
    wait for period;
    
    step <= 16;
    pd_pdm_data_tbs        <= X"BEEF0001BEEF0000";
    pd_pdm_data_valid_tbs  <= '1';
    pd_pdm_data_valid_w_tbs  <= "11";
    wait for period;
    
    step <= 17;
    pd_pdm_data_tbs        <= X"BEEF0003BEEF0002";
    wait for period;
    
    step <= 18;
    pd_pdm_data_tbs        <= X"BEEF0005BEEF0004";
    wait for period;
    
    step <= 18;
    pd_pdm_data_tbs        <=  X"BEEF0007BEEF0006";
    pd_pdm_data_valid_w_tbs  <= "01";
    pd_pdm_data_last_tbs   <= '1';
    wait for period;       
    
    step <= 19;
    pd_pdm_data_tbs        <=  X"BEEF0009BEEF0008";
    pd_pdm_data_valid_w_tbs  <= "00";
    pd_pdm_data_valid_tbs  <= '0';                      -- Indicates Data is valid
    pd_pdm_data_last_tbs   <= '0';
    
    wait for period;
    wait for period;
    wait for period;
    
    step <= 22;
    dma_ctrl_len_tbs         <= X"0000001C";
    dma_ctrl_start_next_tbs  <= '1';
    wait for period;
    
    step <= 23;
    dma_ctrl_start_next_tbs  <= '0';
    wait for period;
    
    step <= 24;
    wait for period;
    
    step <= 25;
    wait for period;
    
    step <= 26;
    pd_pdm_data_tbs        <= X"DEAD0001DEAD0000";
    pd_pdm_data_valid_tbs  <= '1';
    pd_pdm_data_valid_w_tbs  <= "11";
    wait for period;
    
    step <= 0;
    pd_pdm_data_tbs        <= X"0000000000000000";
    pd_pdm_data_valid_w_tbs  <= "00";
    pd_pdm_data_valid_tbs  <= '0';                      -- Indicates Data is valid
    pd_pdm_data_last_tbs   <= '0';
    wait for period;
    
    step <= 27;
    pd_pdm_data_tbs        <= X"DEAD0003DEAD0002";
    pd_pdm_data_valid_w_tbs  <= "11";
    pd_pdm_data_valid_tbs  <= '1'; 
    wait for period;
    
    step <= 28;
    pd_pdm_data_tbs        <= X"DEAD0005DEAD0004";
    wait for period;
    
    step <= 28;
    pd_pdm_data_tbs        <=  X"DEAD0007DEAD0006";
    pd_pdm_data_valid_w_tbs  <= "01";
    pd_pdm_data_last_tbs   <= '1';
    wait for period;       
    
    step <= 29;
    pd_pdm_data_tbs        <=  X"DEAD0009DEAD0008";
    pd_pdm_data_valid_w_tbs  <= "00";
    pd_pdm_data_valid_tbs  <= '0';                      -- Indicates Data is valid
    pd_pdm_data_last_tbs   <= '0';
    
    wait for period;
    wait for period;
    wait for period;
    
    step <= 32;
    dma_ctrl_len_tbs         <= X"0000001C";
    dma_ctrl_start_next_tbs  <= '1';
    wait for period;
    
    step <= 33;
    dma_ctrl_start_next_tbs  <= '0';
    wait for period;
    
    step <= 34;
    wait for period;
    
    step <= 35;
    wait for period;
    
    step <= 36;
    pd_pdm_data_tbs        <= X"BEEF000EBABE0000";
    pd_pdm_data_valid_tbs  <= '1';
    pd_pdm_data_last_tbs   <= '1';
    pd_pdm_data_valid_w_tbs  <= "01";
    wait for period;
    
    step <= 0;
    pd_pdm_data_tbs        <= X"0000000000000000";
    pd_pdm_data_valid_w_tbs  <= "00";
    pd_pdm_data_valid_tbs  <= '0';                      -- Indicates Data is valid
    pd_pdm_data_last_tbs   <= '0';
    wait for period;
    
    step <= 37;
    pd_pdm_data_tbs        <= X"BABE0002BABE0001";
    pd_pdm_data_valid_w_tbs  <= "11";
    wait for period;
    
    step <= 38;
    pd_pdm_data_tbs        <= X"BABE0004BABE0003";
    wait for period;
    
    step <= 38;
    pd_pdm_data_tbs        <=  X"BABE0006BABE0005";
    pd_pdm_data_last_tbs   <= '1';
    wait for period;       
        
    
    step <= 39;
    pd_pdm_data_tbs        <=  X"BEEF0009BEEF0008";
    pd_pdm_data_valid_w_tbs  <= "00";
    pd_pdm_data_valid_tbs  <= '0';                      -- Indicates Data is valid
    pd_pdm_data_last_tbs   <= '0';
    
    wait for period;
    wait for period;
    wait for period;
    
    step <= 42;
    dma_ctrl_len_tbs         <= X"0000001C";
    dma_ctrl_start_next_tbs  <= '1';
    wait for period;
    
    step <= 43;
    dma_ctrl_start_next_tbs  <= '0';
    wait for period;
    
    step <= 44;
    wait for period;
    
    step <= 45;
    wait for period;
    
    step <= 46;
    pd_pdm_data_tbs        <= X"BEEF0000BEEF000A";
    pd_pdm_data_valid_tbs  <= '1';
    pd_pdm_data_valid_w_tbs  <= "10";
    wait for period;
    
    step <= 47;
    pd_pdm_data_tbs        <= X"BEEF0002BEEF0001";
    pd_pdm_data_valid_w_tbs  <= "11";
    wait for period;
    
    step <= 48;
    pd_pdm_data_tbs        <= X"BEEF0004BEEF0003";
    wait for period;
    
    step <= 48;
    pd_pdm_data_tbs        <=  X"BEEF0006BEEF0005";
    pd_pdm_data_valid_w_tbs  <= "11";
    pd_pdm_data_last_tbs   <= '1';
    wait for period;       
    
    step <= 49;
    pd_pdm_data_tbs        <=  X"BEEF0009BEEF0008";
    pd_pdm_data_valid_w_tbs  <= "00";
    pd_pdm_data_valid_tbs  <= '0';                      -- Indicates Data is valid
    pd_pdm_data_last_tbs   <= '0';
    
    wait;
		
		
	end process stimuli_p;
	
  -----------------------------------------------------------------------------
  -- P2L DMA master
  -----------------------------------------------------------------------------
  dut1 : p2l_dma_master
	  generic  map(
		-- Enable byte swap module (if false, no swap)
		g_BYTE_SWAP => false
		)
	  port map
		(
		  ---------------------------------------------------------
		  -- GN4124 core clock and reset
		  clk_i   => clk_tbs,
		  rst_n_i => rst_n_tbs,

		  ---------------------------------------------------------
		  -- From the DMA controller
		  dma_ctrl_carrier_addr_i => dma_ctrl_target_addr_tbs,
		  dma_ctrl_host_addr_h_i  => dma_ctrl_host_addr_h_tbs,
		  dma_ctrl_host_addr_l_i  => dma_ctrl_host_addr_l_tbs,
		  dma_ctrl_len_i          => dma_ctrl_len_tbs,
		  dma_ctrl_start_p2l_i    => dma_ctrl_start_p2l_tbs,
		  dma_ctrl_start_next_i   => dma_ctrl_start_next_tbs,
		  dma_ctrl_done_o         => dma_ctrl_done_s,
		  dma_ctrl_error_o        => dma_ctrl_error_s,
		  dma_ctrl_byte_swap_i    => dma_ctrl_byte_swap_tbs,
		  dma_ctrl_abort_i        => dma_ctrl_abort_tbs,

		  ---------------------------------------------------------
		  -- From P2L Decoder (receive the read completion)
		  --
		  -- Header
		  --pd_pdm_hdr_start_i   =>   pd_pdm_hdr_start_tbs, -- Header strobe
		  --pd_pdm_hdr_length_i  =>   pd_pdm_hdr_length_tbs,-- Packet length in 32-bit words multiples
		  --pd_pdm_hdr_cid_i     =>   pd_pdm_hdr_cid_tbs,-- Completion ID
		  pd_pdm_master_cpld_i =>   pd_pdm_master_cpld_tbs, -- Master read completion with data
		  pd_pdm_master_cpln_i =>   pd_pdm_master_cpln_tbs, -- Master read completion without data
		  --
		  -- Data
		  pd_pdm_data_valid_i  =>   pd_pdm_data_valid_tbs, -- Indicates Data is valid
		  pd_pdm_data_valid_w_i =>  pd_pdm_data_valid_w_tbs,  -- Indicates Data is valid
		  pd_pdm_data_last_i   =>   pd_pdm_data_last_tbs, -- Indicates end of the packet
		  pd_pdm_data_i        =>   pd_pdm_data_tbs, -- Data
		  pd_pdm_be_i          =>   pd_pdm_be_tbs, -- Byte Enable for data

		  ---------------------------------------------------------
		  -- P2L control
		  p2l_rdy_o  => p2l_rdy_s, -- De-asserted to pause transfer already in progress
		  rx_error_o => rx_error_s, -- Asserted when transfer is aborted

		  ---------------------------------------------------------
		  -- To the P2L Interface (send the DMA Master Read request)
		  pdm_arb_tvalid_o  => pdm_arb_tvalid_s, -- Read completion signals
		  pdm_arb_tlast_o => pdm_arb_tlast_s, -- Toward the arbiter
		  pdm_arb_tdata_o   => pdm_arb_tdata_s,
		  pdm_arb_tkeep_o   => pdm_arb_tkeep_s,
		  pdm_arb_req_o    => pdm_arb_req_s,
		  arb_pdm_gnt_i    => arb_pdm_gnt_tbs,

		  ---------------------------------------------------------
		  -- DMA Interface (Pipelined Wishbone)
		  p2l_dma_clk_i   =>  clk_tbs,-- Bus clock
		  p2l_dma_adr_o   =>  p2l_dma_adr_s, -- Adress
		  p2l_dma_dat_i   =>  p2l_dma_dat_s2m_s, -- Data in
		  p2l_dma_dat_o   =>  p2l_dma_dat_m2s_s, -- Data out
		  p2l_dma_sel_o   =>  p2l_dma_sel_s, -- Byte select
		  p2l_dma_cyc_o   =>  p2l_dma_cyc_s,      -- Read or write cycle
		  p2l_dma_stb_o   =>  p2l_dma_stb_s,      -- Read or write strobe
		  p2l_dma_we_o    =>  p2l_dma_we_s,       -- Write
		  p2l_dma_ack_i   =>  p2l_dma_ack_s,      -- Acknowledge
		  p2l_dma_stall_i =>  p2l_dma_stall_tbs,  -- for pipelined Wishbone
		  l2p_dma_cyc_i   =>  l2p_dma_cyc_tbs,      -- L2P dma wb cycle (for bus arbitration)

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
	

	
	dut2:bram_wbs
	generic map (
		ADDR_WIDTH => wb_address_width_c,
		DATA_WIDTH => wb_data_width_c 
	)
	port map (
		-- SYS CON
		clk			=> clk_tbs,
		rst			=> rst_tbs,
		
		-- Wishbone Slave in
		wb_adr_i	=> p2l_dma_adr_s(wb_address_width_c - 1 downto 0),
		wb_dat_i	=> p2l_dma_dat_m2s_s,
		wb_we_i		=> p2l_dma_we_s,
		wb_stb_i	=> p2l_dma_stb_s,
		wb_cyc_i	=> p2l_dma_cyc_s,
		wb_lock_i	=> p2l_dma_stb_s,
		
		-- Wishbone Slave out
		wb_dat_o	=> p2l_dma_dat_s2m_s,
		wb_ack_o	=> p2l_dma_ack_s
	);
	
	
end Behavioral;