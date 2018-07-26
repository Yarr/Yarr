-- ####################################
-- # Project: Yarr
-- # Author: Timon Heim
-- # E-Mail: timon.heim at cern.ch
-- # Comments: Single tx_channel
-- ####################################

library IEEE;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

library work;
use work.board_pkg.all;

entity tx_channel is
	port (
		-- Sys connect
		wb_clk_i	: in  std_logic;
		rst_n_i		: in  std_logic;
		
		-- Data In
		wb_dat_i	: in std_logic_vector(31 downto 0);
		wb_wr_en_i	: in std_logic;
		
		-- TX
		tx_clk_i		: in  std_logic;
		tx_data_o		: out std_logic;
		tx_enable_i		: in std_logic;
		
		-- Word Looper
		loop_pulse_i    : in std_logic;
		loop_mode_i     : in std_logic; -- (WB clk domain)
		loop_word_i     : in std_logic_vector(511 downto 0); -- (WB clk domain)
		loop_word_bytes_i : in std_logic_vector(7 downto 0); -- (WB clk domain)
		
		-- Status
		tx_underrun_o	: out std_logic;
		tx_overrun_o	: out std_logic;
		tx_almost_full_o : out std_logic;
		tx_empty_o	: out std_logic
	);
end tx_channel;

architecture rtl of tx_channel is
	-- Components
	component serial_port
	generic (
        g_PORT_WIDTH : integer := 32
    );
	port (
        -- Sys connect
        clk_i       : in std_logic;
        rst_n_i     : in std_logic;
        -- Input
        enable_i    : in std_logic;
        data_i      : in std_logic_vector(31 downto 0);
        idle_i      : in std_logic_vector(31 downto 0);
        sync_i      : in std_logic_vector(31 downto 0);
        sync_interval_i : in std_logic_vector(7 downto 0);
        data_valid_i : in std_logic;
        -- Output
        data_o      : out std_logic;
        data_read_o   : out std_logic
    );
	end component;
	
	component tx_fifo
	port (
		rst : IN STD_LOGIC;
		wr_clk : IN STD_LOGIC;
		rd_clk : IN STD_LOGIC;
		din : IN STD_LOGIC_VECTOR(31 DOWNTO 0);
		wr_en : IN STD_LOGIC;
		rd_en : IN STD_LOGIC;
		dout : OUT STD_LOGIC_VECTOR(31 DOWNTO 0);
		full : OUT STD_LOGIC;
		empty : OUT STD_LOGIC;
		prog_full : OUT STD_LOGIC
	);
	end component;
	
	--constant c_MAX_LOOP_CNT : unsigned(7 downto 0) := to_unsigned(4,8);
	
	signal tx_fifo_rd : std_logic;
	signal tx_fifo_wr : std_logic;
	signal tx_fifo_din : std_logic_vector(31 downto 0);
	signal tx_fifo_dout : std_logic_vector(31 downto 0);
	signal tx_fifo_full : std_logic;
	signal tx_fifo_empty : std_logic;
	signal tx_fifo_almost_full : std_logic;
	
	signal sport_data_valid : std_logic;
	signal sport_data : std_logic_vector(31 downto 0);
	signal sport_data_read : std_logic;
	
	signal loop_cnt : unsigned(7 downto 0);
	signal loop_empty : std_logic;
	signal loop_mode_s : std_logic;
	signal loop_word_s : std_logic_vector(511 downto 0);
    signal loop_word_bytes_s : std_logic_vector(7 downto 0);
	
begin

	-- Write to FiFo
	tx_fifo_wr <= wb_wr_en_i;
	tx_fifo_din <= wb_dat_i;
	
	
	-- Status outputs
	tx_underrun_o <= tx_fifo_rd and tx_fifo_empty;
	tx_overrun_o <= tx_fifo_wr and tx_fifo_full;
	tx_almost_full_o <= tx_fifo_almost_full;
	tx_empty_o <= tx_fifo_empty;
	
	loop_proc: process(tx_clk_i, rst_n_i)
	begin
	   if (rst_n_i = '0') then
	       loop_cnt <= (others => '0');
	       loop_empty <= '1';
	       loop_mode_s <= '0';
           loop_word_s <= (others => '0');
           loop_word_bytes_s <= (others => '0');
	   elsif rising_edge(tx_clk_i) then
	       loop_empty <= '1';
	       loop_mode_s <= loop_mode_i;
	       loop_word_s <= loop_word_i;
	       loop_word_bytes_s <= loop_word_bytes_i;
	       if (loop_mode_s = '1') then
	           loop_empty <= '1';
	           if (loop_pulse_i = '1') then
	               loop_cnt <= unsigned(loop_word_bytes_s); -- reload counter
	     	       loop_empty <= '0';      
               elsif (sport_data_read = '1' and loop_cnt /= to_unsigned(0, 8)) then
	               loop_cnt <= loop_cnt - 1; -- sport read one word
	     	       loop_empty <= '0';      
	           elsif (loop_cnt > to_unsigned(0,8)) then
	               loop_empty <= '0';
	           end if;
	       end if;	   
	   end if;
	end process loop_proc;
	
	sport_data_valid <= not tx_fifo_empty when (loop_mode_s = '0') else not loop_empty;
	tx_fifo_rd <= sport_data_read when (loop_mode_s = '0') else '0';
	sport_data <= tx_fifo_dout when (loop_mode_s = '0') else 
	           loop_word_s(511 downto 480) when (loop_cnt = to_unsigned(16, 8)) else -- MSB first
	           loop_word_s(479 downto 448) when (loop_cnt = to_unsigned(15, 8)) else
	           loop_word_s(447 downto 416) when (loop_cnt = to_unsigned(14, 8)) else
	           loop_word_s(415 downto 384) when (loop_cnt = to_unsigned(13, 8)) else
	           loop_word_s(383 downto 352) when (loop_cnt = to_unsigned(12, 8)) else
	           loop_word_s(351 downto 320) when (loop_cnt = to_unsigned(11, 8)) else
	           loop_word_s(319 downto 288) when (loop_cnt = to_unsigned(10, 8)) else
	           loop_word_s(287 downto 256) when (loop_cnt = to_unsigned(9, 8)) else
	           loop_word_s(255 downto 224) when (loop_cnt = to_unsigned(8, 8)) else
	           loop_word_s(223 downto 192) when (loop_cnt = to_unsigned(7, 8)) else
	           loop_word_s(191 downto 160) when (loop_cnt = to_unsigned(6, 8)) else
	           loop_word_s(159 downto 128) when (loop_cnt = to_unsigned(5, 8)) else
	           loop_word_s(127 downto 96) when (loop_cnt = to_unsigned(4, 8)) else
	           loop_word_s(95 downto 64) when (loop_cnt = to_unsigned(3, 8)) else
	           loop_word_s(63 downto 32) when (loop_cnt = to_unsigned(2, 8)) else
	           loop_word_s(31 downto 0) when (loop_cnt = to_unsigned(1, 8));
	
	cmp_sport: serial_port PORT MAP(
		clk_i => tx_clk_i,
		rst_n_i => rst_n_i,
		enable_i => tx_enable_i,
		data_i => sport_data,
		idle_i => c_TX_IDLE_WORD,
		sync_i => c_TX_SYNC_WORD,
		sync_interval_i => std_logic_vector(c_TX_SYNC_INTERVAL),
		data_valid_i => sport_data_valid,
		data_o => tx_data_o,
		data_read_o => sport_data_read
	);
	
	cmp_tx_fifo : tx_fifo PORT MAP (
		rst => not rst_n_i,
		wr_clk => wb_clk_i,
		rd_clk => tx_clk_i,
		din => tx_fifo_din,
		wr_en => tx_fifo_wr,
		rd_en => tx_fifo_rd,
		dout => tx_fifo_dout,
		full => tx_fifo_full,
		empty => tx_fifo_empty,
		prog_full => tx_fifo_almost_full
	);
end rtl;
