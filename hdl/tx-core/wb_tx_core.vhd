-- ####################################
-- # Project: Yarr
-- # Author: Timon Heim
-- # E-Mail: timon.heim at cern.ch
-- # Comments: Serial Port
-- # Outputs are synchronous to clk_i
-- ####################################
-- # Adress Map:
-- # Adr[8:4]: channel number 0 to 31
-- # Adr[3:0]:
-- #   0x0 - FiFo (WO) (Write to enabled channels)
-- #   0x1 - CMD Enable (RW)
-- #   0x2 - CMD Empty (RO)
-- #   0x3 - Trigger Enable (RW)
-- #   0x4 - Trigger Done (RO)
-- #   0x5 - Trigger Conf (RW) : 
-- #          0 = External
-- #          1 = Internal Time
-- #          2 = Internal Count
-- #   0x6 - Trigger Frequency (RW)
-- #   0x7 - Trigger Time_L (RW)
-- #   0x8 - Trigger Time_H (RW)
-- #   0x9 - Trigger Count (RW)
-- #   0xA - Trigger Word Length (RW)
-- #   0xB - Trigger Word [31:0] (RW)
-- #   0xC - Trigger Word [63:32] (RW)
-- #   0xD - Trigger Word [95:64] (RW)
-- #   0xE - Trigger Word [127:96] (RW)

library IEEE;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity wb_tx_core is
	generic (
		g_NUM_TX : integer range 1 to 32 := 1
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
		
		-- TX
		tx_clk_i	: in  std_logic;
		tx_data_o	: out std_logic_vector(g_NUM_TX-1 downto 0);
		trig_pulse_o : out std_logic
	);
end wb_tx_core;

architecture behavioral of wb_tx_core is
	component tx_channel
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
			
			-- Status
			tx_underrun_o	: out std_logic;
			tx_overrun_o	: out std_logic;
			tx_almost_full_o : out std_logic;
			tx_empty_o	: out std_logic
		);
	end component;
	
	component trigger_unit
		port (
			clk_i 	: in  std_logic;
			rst_n_i	: in  std_logic;
			
			-- Serial Trigger Out
			trig_o : out std_logic;
			trig_pulse_o : out std_logic;
			
			-- Trigger In (async)
			ext_trig_i	: in std_logic;
			
			-- Config
			trig_word_i : in std_logic_vector(127 downto 0); -- Trigger command
			trig_word_length_i : in std_logic_vector(31 downto 0); -- Trigger command length
			trig_freq_i : in std_logic_vector(31 downto 0); -- Number of clock cycles between triggers
			trig_time_i : in std_logic_vector(63 downto 0); -- Clock cycles
			trig_count_i : in std_logic_vector(31 downto 0); -- Fixed number of triggers
			trig_conf_i	: in std_logic_vector(3 downto 0); -- Internal, external, pseudo random, 
			trig_en_i : in std_logic;
			trig_abort_i : in std_logic;
			trig_done_o : out std_logic
		);
	end component;
	
	-- Signals
	signal tx_data_cmd : std_logic_vector(g_NUM_TX-1 downto 0);
	signal tx_data_trig : std_logic;
	signal tx_trig_pulse : std_logic;
	
	-- Registers
	signal tx_enable : std_logic_vector(31 downto 0) := (others => '0');
	
	signal tx_underrun : std_logic_vector(31 downto 0) := (others => '0');
	signal tx_overrun : std_logic_vector(31 downto 0) := (others => '0');
	signal tx_almost_full : std_logic_vector(31 downto 0) := (others => '0');
	signal tx_empty	: std_logic_vector(31 downto 0) := (others => '0');
	
	-- Trigger command
	signal trig_freq : std_logic_vector(31 downto 0); -- Number of clock cycles between triggers
	signal trig_time : std_logic_vector(63 downto 0); -- Clock cycles
	signal trig_time_l : std_logic_vector(31 downto 0);
	signal trig_time_h : std_logic_vector(31 downto 0);
	signal trig_count : std_logic_vector(31 downto 0); -- Fixed number of triggers
	signal trig_conf : std_logic_vector(3 downto 0); -- Internal, external, pseudo random, 
	signal trig_en : std_logic;
	signal trig_done : std_logic;
	signal trig_word_length : std_logic_vector(31 downto 0);
	signal trig_word : std_logic_vector(127 downto 0);
	signal trig_abort : std_logic;
	
	signal wb_wr_en	: std_logic_vector(31 downto 0) := (others => '0');
	signal wb_dat_t : std_logic_vector(31 downto 0);
	
	signal channel : integer range 0 to 31;

begin

	channel <= TO_INTEGER(unsigned(wb_adr_i(8 downto 4)));
	wb_stall_o <= '1' when (tx_almost_full /= x"00000000") else '0';
	
	wb_proc: process (wb_clk_i, rst_n_i)
	begin
		if (rst_n_i = '0') then
			wb_dat_o <= (others => '0');
			wb_ack_o <= '0';
			wb_wr_en <= (others => '0');
			tx_enable <= (others => '0');
			wb_dat_t <= (others => '0');
			trig_en <= '0';
			trig_abort  <= '0';
		elsif rising_edge(wb_clk_i) then
			wb_wr_en <= (others => '0');
			wb_ack_o <= '0';
			trig_time <= trig_time_h & trig_time_l;
			trig_abort  <= '0';
			if (wb_cyc_i = '1' and wb_stb_i = '1') then
				if (wb_we_i = '1') then
					if (wb_adr_i(3 downto 0) = x"0") then -- Write to fifo
						wb_wr_en <= tx_enable;
						wb_ack_o <= '1';
						wb_dat_t <= wb_dat_i;
					elsif (wb_adr_i(3 downto 0) = x"1") then -- Set enable mask
						tx_enable <= wb_dat_i;
						wb_ack_o <= '1';
					elsif (wb_adr_i(3 downto 0) = x"3") then -- Set trigger enable
						trig_en <= wb_dat_i(0);
						wb_ack_o <= '1';
					elsif (wb_adr_i(3 downto 0) = x"5") then -- Set trigger conf
						trig_conf <= wb_dat_i(3 downto 0);
						wb_ack_o <= '1';
					elsif (wb_adr_i(3 downto 0) = x"6") then -- Set trigger frequency
						trig_freq <= wb_dat_i;
						wb_ack_o <= '1';
					elsif (wb_adr_i(3 downto 0) = x"7") then -- Set trigger time low
						trig_time_l(31 downto 0) <= wb_dat_i;
						wb_ack_o <= '1';
					elsif (wb_adr_i(3 downto 0) = x"8") then -- Set trigger time high
						trig_time_h(31 downto 0) <= wb_dat_i;
						wb_ack_o <= '1';
					elsif (wb_adr_i(3 downto 0) = x"9") then -- Set trigger count
						trig_count <= wb_dat_i;
						wb_ack_o <= '1';
					elsif (wb_adr_i(3 downto 0) = x"A") then -- Set trigger word length (bits)
						trig_word_length <= wb_dat_i;
						wb_ack_o <= '1';
					elsif (wb_adr_i(3 downto 0) = x"B") then -- Set trigger word [31:0]
						trig_word(31 downto 0) <= wb_dat_i;
						wb_ack_o <= '1';
					elsif (wb_adr_i(3 downto 0) = x"C") then -- Set trigger word [63:32]
						trig_word(63 downto 32) <= wb_dat_i;
						wb_ack_o <= '1';
					elsif (wb_adr_i(3 downto 0) = x"D") then -- Set trigger word [95:64]
						trig_word(95 downto 64) <= wb_dat_i;
						wb_ack_o <= '1';
					elsif (wb_adr_i(3 downto 0) = x"E") then -- Set trigger word [127:96]
						trig_word(127 downto 96) <= wb_dat_i;
						wb_ack_o <= '1';
					elsif (wb_adr_i(3 downto 0) = x"F") then -- Toggle trigger abort
						trig_abort <= wb_dat_i(0);
						wb_ack_o <= '1';
					else
						wb_ack_o <= '1';
					end if;
				else
					if (wb_adr_i(3 downto 0) = x"1") then -- Read enable mask
						wb_dat_o <= tx_enable;
						wb_ack_o <= '1';
					elsif (wb_adr_i(3 downto 0) = x"2") then -- Read empty stat
						wb_dat_o <= tx_empty;
						wb_ack_o <= '1';
					elsif (wb_adr_i(3 downto 0) = x"3") then -- Read trigger enable
						wb_dat_o(0) <= trig_en;
						wb_dat_o(31 downto 1) <= (others => '0');
						wb_ack_o <= '1';
					elsif (wb_adr_i(3 downto 0) = x"4") then -- Read trigger done
						wb_dat_o(0) <= trig_done;
						wb_dat_o(31 downto 1) <= (others => '0');
						wb_ack_o <= '1';
					elsif (wb_adr_i(3 downto 0) = x"5") then -- Read trigger conf
						wb_dat_o(3 downto 0) <= trig_conf;
						wb_dat_o(31 downto 4) <= (others => '0');
						wb_ack_o <= '1';
					elsif (wb_adr_i(3 downto 0) = x"6") then -- Read trigger freq
						wb_dat_o <= trig_freq;
						wb_ack_o <= '1';
					elsif (wb_adr_i(3 downto 0) = x"7") then -- Read trigger time low
						wb_dat_o <= trig_time(31 downto 0);
						wb_ack_o <= '1';
					elsif (wb_adr_i(3 downto 0) = x"8") then -- Read trigger time high
						wb_dat_o <= trig_time(63 downto 32);
						wb_ack_o <= '1';
					elsif (wb_adr_i(3 downto 0) = x"9") then -- Read trigger count
						wb_dat_o <= trig_count;
						wb_ack_o <= '1';
					elsif (wb_adr_i(3 downto 0) = x"A") then -- Set trigger word length (bits)
						wb_dat_o <= trig_word_length;
						wb_ack_o <= '1';
					elsif (wb_adr_i(3 downto 0) = x"B") then -- Set trigger word [31:0]
						wb_dat_o <= trig_word(31 downto 0);
						wb_ack_o <= '1';
					elsif (wb_adr_i(3 downto 0) = x"C") then -- Set trigger word [63:32]
						wb_dat_o <= trig_word(63 downto 32);
						wb_ack_o <= '1';
					elsif (wb_adr_i(3 downto 0) = x"D") then -- Set trigger word [95:64]
						wb_dat_o <= trig_word(95 downto 64);
						wb_ack_o <= '1';
					elsif (wb_adr_i(3 downto 0) = x"E") then -- Set trigger word [127:96]
						wb_dat_o <= trig_word(127 downto 96);
						wb_ack_o <= '1';
					else
						wb_dat_o <= x"DEADBEEF";
						wb_ack_o <= '1';
					end if;
				end if;
			end if;
		end if;
	end process wb_proc;

	tx_channels: for I in 0 to g_NUM_TX-1 generate
	begin
		cmp_tx_channel: tx_channel PORT MAP (
			-- Sys connect
			wb_clk_i => wb_clk_i,
			rst_n_i => rst_n_i,
			-- Data In
			wb_dat_i => wb_dat_t,
			wb_wr_en_i => wb_wr_en(I),
			-- TX
			tx_clk_i => tx_clk_i,
			tx_data_o => tx_data_cmd(I),
			tx_enable_i => tx_enable(I),
			-- Status
			tx_underrun_o => tx_underrun(I),
			tx_overrun_o => tx_overrun(I),
			tx_almost_full_o => tx_almost_full(I),
			tx_empty_o => tx_empty(I)
		);
		
		tx_mux : process(tx_clk_i, rst_n_i)
		begin
			if (rst_n_i = '0') then
				tx_data_o(I) <= '0';
			elsif rising_edge(tx_clk_i) then
				if (tx_enable(I) = '1' and trig_en = '1') then
					tx_data_o(I) <= tx_data_trig;
				else
					tx_data_o(I) <= tx_data_cmd(I);
				end if;
			end if;
		end process;
	end generate tx_channels;
	
	trig_pulse_o <= tx_trig_pulse;
	cmp_trig_unit : trigger_unit PORT MAP (
		clk_i => tx_clk_i,
		rst_n_i => rst_n_i,
		-- Serial Trigger Out
		trig_o => tx_data_trig,
		trig_pulse_o=> tx_trig_pulse,
		-- Trigger In (async)
		ext_trig_i => '0',
		-- Config
		trig_word_i => trig_word,
		trig_word_length_i => trig_word_length,
		trig_freq_i => trig_freq,
		trig_time_i => trig_time,
		trig_count_i => trig_count,
		trig_conf_i => trig_conf,
		trig_en_i => trig_en,
		trig_abort_i => trig_abort,
		trig_done_o => trig_done
	);

end behavioral;