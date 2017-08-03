library IEEE;
USE IEEE.STD_LOGIC_1164.all;
USE IEEE.NUMERIC_STD.all;
use IEEE.std_logic_unsigned.all; 


entity l2p_dma_bench is
    generic (
		constant period : time := 100 ns;
		constant axis_data_width_c : integer := 64;

		constant wb_address_width_c : integer := 12;
		constant wb_data_width_c : integer := 64
	);
	--port ();
end l2p_dma_bench;

architecture Behavioral of l2p_dma_bench is
		signal clk_tbs : STD_LOGIC;
		signal rst_tbs : STD_LOGIC;
		signal rst_n_tbs : STD_LOGIC;
		-- Test bench specific signals
		signal step : integer range 1 to 10;
		signal debug_time : integer;
		
		-- From the DMA controller
		signal dma_ctrl_target_addr_tbs : std_logic_vector(32-1 downto 0);
		signal dma_ctrl_host_addr_h_tbs : std_logic_vector(32-1 downto 0);
		signal dma_ctrl_host_addr_l_tbs : std_logic_vector(32-1 downto 0);
		signal dma_ctrl_len_tbs         : std_logic_vector(32-1 downto 0);
		signal dma_ctrl_start_l2p_tbs   : std_logic;
		signal dma_ctrl_done_s        : std_logic;
		signal dma_ctrl_error_s       : std_logic;
		signal dma_ctrl_byte_swap_tbs   : std_logic_vector(2 downto 0);
		signal dma_ctrl_abort_tbs       : std_logic;

		-- To the arbiter (L2P data)
		signal ldm_arb_tvalid_s  : std_logic;
		signal ldm_arb_tlast_s : std_logic;
		signal ldm_arb_tdata_s   : std_logic_vector(axis_data_width_c-1 downto 0);
		signal ldm_arb_tkeep_s   : std_logic_vector(axis_data_width_c/8-1 downto 0);
		signal ldm_arb_tready_tbs : std_logic;
		signal ldm_arb_req_s    : std_logic;
		signal arb_ldm_gnt_tbs    : std_logic;


		-- L2P channel control
		signal l2p_edb_s  : std_logic;                    -- Asserted when transfer is aborted
		signal l2p_rdy_tbs  : std_logic;                    -- De-asserted to pause transdert already in progress
		signal l2p_64b_address_tbs : std_logic;
		signal tx_error_tbs : std_logic;                    -- Asserted when unexpected or malformed paket received

		-- DMA Interface (Pipelined Wishbone)
		signal l2p_dma_adr_s   : std_logic_vector(wb_data_width_c-1 downto 0);
		signal l2p_dma_dat_s2m_s   : std_logic_vector(wb_data_width_c-1 downto 0);
		signal l2p_dma_dat_s2m_tbs   : std_logic_vector(wb_data_width_c-1 downto 0);
		signal l2p_dma_dat_m2s_s   : std_logic_vector(wb_data_width_c-1 downto 0);
		signal l2p_dma_sel_s   : std_logic_vector(3 downto 0);
		signal l2p_dma_cyc_s   : std_logic;
		signal l2p_dma_stb_s   : std_logic;
		signal l2p_dma_we_s    : std_logic;
		signal l2p_dma_ack_s   : std_logic;
		signal l2p_dma_ack_tbs   : std_logic;
		signal l2p_dma_stall_tbs : std_logic;
		signal p2l_dma_cyc_tbs   : std_logic; -- P2L dma WB cycle for bus arbitration
		
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
            addr_fifo_dout_do  : out std_logic_vector(wb_address_width_c-1 downto 0);
            addr_fifo_din_do   : out std_logic_vector(axis_data_width_c-1 downto 0)
        );
	end component;
		
	component bram_wbs is
	generic (
		constant ADDR_WIDTH : integer := 16;
		constant DATA_WIDTH : integer := 32 
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
	
	time_p: process
    begin
        debug_time <= 0;
        wait for period;
        
    loop
        debug_time <= debug_time + 1;
        wait for period;
        end loop;
        
    end process time_p;
    
    wb_stimuli_p : process
        variable counter : unsigned (15 downto 0);
    begin
    
    
    counter := to_unsigned(0,16);

    
    l2p_dma_dat_s2m_tbs              <= (others => '0');
    l2p_dma_ack_tbs                   <= '0';
    
    while counter <= to_unsigned(32,16) loop
    if l2p_dma_cyc_s = '1' and (debug_time < 20 or debug_time > 25) then
        wait for period;
        --step_ddr <= 3;

        l2p_dma_dat_s2m_tbs               <=   X"dead" & std_logic_vector(counter) & X"beef" & std_logic_vector(counter);
        l2p_dma_ack_tbs                   <= '1';
        counter := counter + 1;

        
    else
        wait for period;
        
        l2p_dma_dat_s2m_tbs              <= (others => '0');
        l2p_dma_ack_tbs                   <= '0';
        
    end if;
    end loop;
        l2p_dma_dat_s2m_tbs              <= (others => '0');
        l2p_dma_ack_tbs                   <= '0';
    wait;
    
    
    end process wb_stimuli_p;
	
	stimuli_p: process
	begin
		step <= 1;
		ldm_arb_tready_tbs <= '1'; -- Asserted when GN4124 is ready to receive master write
		l2p_rdy_tbs  <= '1';                    -- De-asserted to pause transdert already in progress
		l2p_64b_address_tbs <= '0';
		tx_error_tbs <= '0';                    -- Asserted when unexpected or malformed paket received
		dma_ctrl_target_addr_tbs <= X"00000000";
		dma_ctrl_host_addr_h_tbs <= X"00000000";
		dma_ctrl_host_addr_l_tbs <= X"00000000";
		dma_ctrl_len_tbs         <= X"00000000";
		dma_ctrl_start_l2p_tbs   <= '0';
		dma_ctrl_byte_swap_tbs   <= "000";
		dma_ctrl_abort_tbs       <= '0';
		arb_ldm_gnt_tbs    <= '1';
		l2p_dma_stall_tbs <= '0';
		p2l_dma_cyc_tbs   <= '0'; -- P2L dma WB cycle for bus arbitration
		
		wait for period;
		
		wait for period;
		step <= 2;
		ldm_arb_tready_tbs <= '1'; -- Asserted when GN4124 is ready to receive master write
		l2p_rdy_tbs  <= '1';                    -- De-asserted to pause transdert already in progress
		tx_error_tbs <= '0';                    -- Asserted when unexpected or malformed paket received
		dma_ctrl_target_addr_tbs <= X"00000000";
		dma_ctrl_host_addr_h_tbs <= X"00000000";
		dma_ctrl_host_addr_l_tbs <= X"c57334d0";
		dma_ctrl_len_tbs         <= X"00000100";
		dma_ctrl_start_l2p_tbs   <= '1';
		dma_ctrl_byte_swap_tbs   <= "000";
		dma_ctrl_abort_tbs       <= '0';
		arb_ldm_gnt_tbs    <= '1';
		l2p_dma_stall_tbs <= '0';
		p2l_dma_cyc_tbs   <= '0'; -- P2L dma WB cycle for bus arbitration
	
		wait for period;
		step <= 3;
		ldm_arb_tready_tbs <= '1'; -- Asserted when GN4124 is ready to receive master write
		l2p_rdy_tbs  <= '1';                    -- De-asserted to pause transdert already in progress
		tx_error_tbs <= '0';                    -- Asserted when unexpected or malformed paket received
		--dma_ctrl_target_addr_tbs <= X"00000010";
		--dma_ctrl_host_addr_h_tbs <= X"00000000";
		--dma_ctrl_host_addr_l_tbs <= X"0000005A";
		--dma_ctrl_len_tbs         <= X"00000080";
		dma_ctrl_start_l2p_tbs   <= '0';
		dma_ctrl_byte_swap_tbs   <= "000";
		dma_ctrl_abort_tbs       <= '0';
		arb_ldm_gnt_tbs    <= '1';
		l2p_dma_stall_tbs <= '0';
		p2l_dma_cyc_tbs   <= '0'; -- P2L dma WB cycle for bus arbitration
		
		
		wait for period;
		step <= 4;
		
		
		wait for period;
		step <= 5;
		
		
		wait for 20*period;
		step <= 6;
		
		wait for period;
		step <= 7;
		--ldm_arb_tready_tbs  <= '0'; 
		
		wait for period;
        step <= 8;
		--ldm_arb_tready_tbs  <= '1';
		
		wait;
		
		
	end process stimuli_p;


	
  -----------------------------------------------------------------------------
  -- L2P DMA master
  -----------------------------------------------------------------------------
  dut1 : l2p_dma_master
    port map
    (
      clk_i   => clk_tbs,
      rst_n_i => rst_n_tbs,
      
      l2p_rid_i => X"0010",
      
      dma_ctrl_target_addr_i => dma_ctrl_target_addr_tbs,
      dma_ctrl_host_addr_h_i => dma_ctrl_host_addr_h_tbs,
      dma_ctrl_host_addr_l_i => dma_ctrl_host_addr_l_tbs,
      dma_ctrl_len_i         => dma_ctrl_len_tbs,
      dma_ctrl_start_l2p_i   => dma_ctrl_start_l2p_tbs,
      dma_ctrl_done_o        => dma_ctrl_done_s,
      dma_ctrl_error_o       => dma_ctrl_error_s,
      dma_ctrl_byte_swap_i   => dma_ctrl_byte_swap_tbs,
      dma_ctrl_abort_i       => dma_ctrl_abort_tbs,

      ldm_arb_tvalid_o  => ldm_arb_tvalid_s,
      ldm_arb_tlast_o => ldm_arb_tlast_s,
      ldm_arb_tdata_o   => ldm_arb_tdata_s,
      ldm_arb_tkeep_o   => ldm_arb_tkeep_s,
      ldm_arb_req_o    => ldm_arb_req_s,
      arb_ldm_gnt_i    => arb_ldm_gnt_tbs,

      l2p_edb_o  => l2p_edb_s,
      ldm_arb_tready_i => ldm_arb_tready_tbs,
      l2p_rdy_i  => l2p_rdy_tbs,
      --l2p_64b_address_i => l2p_64b_address_tbs,
      tx_error_i => tx_error_tbs,

      l2p_dma_clk_i   => clk_tbs,
      l2p_dma_adr_o   => l2p_dma_adr_s,
      l2p_dma_dat_i   => l2p_dma_dat_s2m_tbs,--l2p_dma_dat_s2m_s,
      l2p_dma_dat_o   => l2p_dma_dat_m2s_s,
      l2p_dma_sel_o   => l2p_dma_sel_s,
      l2p_dma_cyc_o   => l2p_dma_cyc_s,
      l2p_dma_stb_o   => l2p_dma_stb_s,
      l2p_dma_we_o    => l2p_dma_we_s,
      l2p_dma_ack_i   => l2p_dma_ack_tbs,--l2p_dma_ack_s,
      l2p_dma_stall_i => l2p_dma_stall_tbs,
      p2l_dma_cyc_i   => p2l_dma_cyc_tbs
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
		wb_adr_i	=> l2p_dma_adr_s(wb_address_width_c - 1 downto 0),
		wb_dat_i	=> l2p_dma_dat_m2s_s,
		wb_we_i		=> l2p_dma_we_s,
		wb_stb_i	=> l2p_dma_stb_s,
		wb_cyc_i	=> l2p_dma_cyc_s,
		wb_lock_i	=> l2p_dma_stb_s,
		
		-- Wishbone Slave out
		wb_dat_o	=> l2p_dma_dat_s2m_s,
		wb_ack_o	=> l2p_dma_ack_s
	);
	
	
end Behavioral;