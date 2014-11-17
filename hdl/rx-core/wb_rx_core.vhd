-- ####################################
-- # Project: Yarr
-- # Author: Timon Heim
-- # E-Mail: timon.heim at cern.ch
-- # Comments: RX core
-- # Outputs are synchronous to wb_clk_i
-- ####################################
-- # Adress Map:
-- # Adr[3:0]:
-- #     0x0 : RX Enable Mask


library IEEE;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity wb_rx_core is
	generic (
		g_NUM_RX : integer range 1 to 32 := 1
	);
	port (
		-- Sys connect
		wb_clk_i	: in  std_logic;
		rst_n_i		: in  std_logic;
		
		-- Wishbone slave interface
		wb_adr_i	: in  std_logic_vector(31 downto 0);
		wb_dat_i	: in  std_logic_vector(31 downto 0);
		wb_dat_o	: out std_logic_vector(31 downto 0);
		wb_cyc_i	: in  std_logic;
		wb_stb_i	: in  std_logic;
		wb_we_i		: in  std_logic;
		wb_ack_o	: out std_logic;
		wb_stall_o	: out std_logic;
		
		-- RX IN
		rx_clk_i	: in  std_logic;
		rx_serdes_clk_i : in std_logic;
		rx_data_i	: in std_logic_vector(g_NUM_RX-1 downto 0);
		
		-- RX OUT (sync to sys_clk)
		rx_valid_o : out std_logic;
		rx_data_o : out std_logic_vector(31 downto 0);
		busy_o : out std_logic;
		
		debug_o : out std_logic_vector(31 downto 0)
	);
end wb_rx_core;

architecture behavioral of wb_rx_core is

	constant c_ALL_ZEROS : std_logic_vector(g_NUM_RX-1 downto 0) := (others => '0');

	component fei4_rx_channel
		port (
			-- Sys connect
			rst_n_i : in std_logic;
			clk_160_i : in std_logic;
			clk_640_i : in std_logic;
			enable_i : in std_logic;
			-- Input
			rx_data_i : in std_logic;
			-- Output
			rx_data_o : out std_logic_vector(23 downto 0);
			rx_valid_o : out std_logic;
			rx_stat_o : out std_logic_vector(7 downto 0);
			rx_data_raw_o : out std_logic_vector(7 downto 0)
		);
	end component;
	
	COMPONENT rx_channel_fifo
		PORT (
			rst : IN STD_LOGIC;
			wr_clk : IN STD_LOGIC;
			rd_clk : IN STD_LOGIC;
			din : IN STD_LOGIC_VECTOR(31 DOWNTO 0);
			wr_en : IN STD_LOGIC;
			rd_en : IN STD_LOGIC;
			dout : OUT STD_LOGIC_VECTOR(31 DOWNTO 0);
			full : OUT STD_LOGIC;
			empty : OUT STD_LOGIC
		);
	END COMPONENT;
	
	type rx_data_array is array (g_NUM_RX-1 downto 0) of std_logic_vector(23 downto 0);
	type rx_data_fifo_array is array (g_NUM_RX-1 downto 0) of std_logic_vector(31 downto 0);
	type rx_stat_array is array (g_NUM_RX-1 downto 0) of std_logic_vector(7 downto 0);
	signal rx_data : rx_data_array;
	signal rx_valid : std_logic_vector(g_NUM_RX-1 downto 0);
	signal rx_stat : rx_stat_array;
	signal rx_data_raw : rx_stat_array;
	
	signal rx_fifo_dout :rx_data_fifo_array;
	signal rx_fifo_din : rx_data_fifo_array;
	signal rx_fifo_full : std_logic_vector(g_NUM_RX-1 downto 0);
	signal rx_fifo_empty : std_logic_vector(g_NUM_RX-1 downto 0);
	signal rx_fifo_rden : std_logic_vector(g_NUM_RX-1 downto 0);
	signal rx_fifo_wren : std_logic_vector(g_NUM_RX-1 downto 0);
	signal rx_fifo_dout_t : std_logic_vector(31 downto 0);

	signal rx_enable : std_logic_vector(31 downto 0);
	
	signal rx_fifo_cur : std_logic_vector(g_NUM_RX-1 downto 0);
	signal rx_fifo_cur_d : std_logic_vector(g_NUM_RX-1 downto 0);
	signal rx_fifo_act : std_logic_vector(g_NUM_RX-1 downto 0);
	signal rx_fifo_cnt : unsigned(5 downto 0);
	signal rx_fifo_cnt_d : unsigned(5 downto 0);
	signal rx_fifo_cnt_dd : unsigned(5 downto 0);
	
	signal debug : std_logic_vector(31 downto 0);
	
begin
	debug_o <= debug;
	
	debug(7 downto 0) <= rx_stat(0);
	debug(15 downto 8) <= rx_data_raw(0);
	debug(16) <= rx_valid(0);

	wb_proc: process (wb_clk_i, rst_n_i)
	begin
		if (rst_n_i = '0') then
			wb_dat_o <= (others => '0');
			wb_ack_o <= '0';
			rx_enable <= (others => '0');
			wb_stall_o <= '0';
		elsif rising_edge(wb_clk_i) then
			wb_ack_o <= '0';
			if (wb_cyc_i = '1' and wb_stb_i = '1') then
				if (wb_we_i = '1') then
					if (wb_adr_i(3 downto 0) = x"0") then -- Set enable mask
						wb_ack_o <= '1';
						rx_enable <= wb_dat_i;
					else
						wb_ack_o <= '1';
					end if;
				else
					if (wb_adr_i(3 downto 0) = x"0") then -- Read enable mask
						wb_dat_o <= rx_enable;
						wb_ack_o <= '1';
					else
						wb_dat_o <= x"DEADBEEF";
						wb_ack_o <= '1';
					end if;
				end if;
			end if;
		end if;
	end process wb_proc;
	
	-- Arbiter
	arbiter_proc : process(wb_clk_i, rst_n_i)
	begin
		if (rst_n_i = '0') then
			rx_data_o <= (others => '0');
			rx_valid_o <= '0';
			rx_fifo_cnt <= (others => '0');
			rx_fifo_cnt_d <= (others => '0');
			rx_fifo_cnt_dd <= (others => '0');
			rx_fifo_act <= (others => '0');
		elsif rising_edge(wb_clk_i) then
			-- Read active Fifo
			rx_fifo_act <= (others => '0');
			rx_fifo_act(TO_INTEGER(rx_fifo_cnt)) <= '1';
			
			rx_fifo_rden <= rx_fifo_act;
			rx_data_o <= x"DEADBEEF";
			rx_valid_o <= '0';
			--for I in 0 to g_NUM_RX-1 loop
				--rx_data_o <= rx_fifo_dout(I) when (rx_fifo_rden(I) = '1') else rx_fifo_dout(I+1);
				if (rx_fifo_rden(TO_INTEGER(rx_fifo_cnt_dd)) = '1' and rx_fifo_empty(TO_INTEGER(rx_fifo_cnt_dd)) = '0') then
					rx_data_o <= rx_fifo_dout(TO_INTEGER(rx_fifo_cnt_dd));
					rx_valid_o <= '1';
				end if;
			--end loop;
			
			rx_fifo_cnt_dd <= rx_fifo_cnt_d;
			rx_fifo_cnt_d <= rx_fifo_cnt;
			if(rx_fifo_cnt >= g_NUM_RX-1) then
				rx_fifo_cnt <= (others => '0');
			else
				rx_fifo_cnt <= rx_fifo_cnt + 1;
			end if;
		end if;
	end process arbiter_proc;
	
	-- Bit shifter
	single_shift_gen: if (g_NUM_RX = 1) generate
	begin
		rx_fifo_cur <= (0 => '1', others => '0');
		rx_fifo_cur_d <= rx_fifo_cur;
	end generate;
	multi_shift_gen: if (g_NUM_RX > 1) generate
	begin
		process(wb_clk_i, rst_n_i)
		begin
			if (rst_n_i = '0') then
				rx_fifo_cur <= (0 => '1', others => '0');
				rx_fifo_cur_d <= (g_NUM_RX-1 => '1', others => '0');
			elsif rising_edge(wb_clk_i) then
				rx_fifo_cur <= rx_fifo_cur(g_NUM_RX-2 downto 0) & rx_fifo_cur(g_NUM_RX-1);
				rx_fifo_cur_d <= rx_fifo_cur;
			end if;
		end process;
	end generate;
	
	-- Generate Rx Channels
	busy_o <= '0' when (rx_fifo_full = c_ALL_ZEROS) else '1';
	rx_channels: for I in 0 to g_NUM_RX-1 generate
	begin
		cmp_fei4_rx_channel: fei4_rx_channel PORT MAP(
			rst_n_i => rst_n_i,
			clk_160_i => rx_clk_i,
			clk_640_i => rx_serdes_clk_i,
			enable_i => rx_enable(I),
			rx_data_i => rx_data_i(I),
			rx_data_o => rx_data(I),
			rx_valid_o => rx_valid(I),
			rx_stat_o => rx_stat(I),
			rx_data_raw_o => rx_data_raw(I)
		);
		
		rx_fifo_din(I) <= STD_LOGIC_VECTOR(TO_UNSIGNED(I,8)) & rx_data(0);
		rx_fifo_wren(I) <= rx_valid(0) and rx_enable(I);
		cmp_rx_channel_fifo : rx_channel_fifo PORT MAP (
			rst => not rst_n_i,
			wr_clk => rx_clk_i,
			rd_clk => wb_clk_i,
			din => rx_fifo_din(I),
			wr_en => rx_fifo_wren(I),
			rd_en => rx_fifo_rden(I),
			dout => rx_fifo_dout(I),
			full => rx_fifo_full(I),
			empty => rx_fifo_empty(I)
		);
	end generate;
end behavioral;

