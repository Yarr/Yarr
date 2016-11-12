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
		trig_pulse_o : out std_logic;
		
		-- Sync
		ext_trig_i : in std_logic
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
	signal trig_time_l_d : std_logic_vector(31 downto 0);
	signal trig_time_h : std_logic_vector(31 downto 0);
	signal trig_time_h_d : std_logic_vector(31 downto 0);
	signal trig_count : std_logic_vector(31 downto 0); -- Fixed number of triggers
	signal trig_conf : std_logic_vector(3 downto 0); -- Internal, external, pseudo random, 
	signal trig_en : std_logic;
	signal trig_done : std_logic;
	signal trig_word_length : std_logic_vector(31 downto 0);
	signal trig_word : std_logic_vector(127 downto 0);
    
    -- Trig input freq counter
    signal ext_trig_t1 : std_logic;
    signal ext_trig_t2 : std_logic;
    signal ext_trig_t3 : std_logic;
    signal trig_in_freq_cnt : unsigned(31 downto 0);
    signal trig_in_freq : std_logic_vector(31 downto 0);
    signal per_second : std_logic;
    signal per_second_cnt : unsigned(31 downto 0);
    constant ticks_per_second : integer := 40000000; -- 40 MHz clock rate
    
    
    
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
            tx_enable <= (others => '0');
            trig_conf <= (others => '0');
            trig_time_h <= (others => '0');
            trig_time_h_d <= (others => '0');
            trig_time_h <= (others => '0');
            trig_time_l_d <= (others => '0');
            trig_count <= (others => '0');
            trig_word <= (others => '0');
            trig_abort <= '0';
		elsif rising_edge(wb_clk_i) then
			wb_wr_en <= (others => '0');
			wb_ack_o <= '0';
            trig_time_h_d <= trig_time_h;
            trig_time_l_d <= trig_time_l;
			trig_time <= trig_time_h_d & trig_time_l_d; -- delay for more flexible routing
			trig_abort  <= '0';
			if (wb_cyc_i = '1' and wb_stb_i = '1') then
				if (wb_we_i = '1') then
					case (wb_adr_i(3 downto 0)) is
						when x"0" => -- Write to fifo
							wb_wr_en <= tx_enable;
							wb_ack_o <= '1';
							wb_dat_t <= wb_dat_i;
						when x"1" => -- Set enable mask
							tx_enable <= wb_dat_i;
							wb_ack_o <= '1';
						when x"3" => -- Set trigger enable
							trig_en <= wb_dat_i(0);
							wb_ack_o <= '1';
						when x"5" => -- Set trigger conf
							trig_conf <= wb_dat_i(3 downto 0);
							wb_ack_o <= '1';
						when x"6" => -- Set trigger frequency
							trig_freq <= wb_dat_i;
							wb_ack_o <= '1';
						when x"7" => -- Set trigger time low
							trig_time_l(31 downto 0) <= wb_dat_i;
							wb_ack_o <= '1';
						when x"8" => -- Set trigger time high
							trig_time_h(31 downto 0) <= wb_dat_i;
							wb_ack_o <= '1';
						when x"9" => -- Set trigger count
							trig_count <= wb_dat_i;
							wb_ack_o <= '1';
						when x"A" => -- Set trigger word length (bits)
							trig_word_length <= wb_dat_i;
							wb_ack_o <= '1';
						when x"B" => -- Set trigger word [31:0]
							trig_word(31 downto 0) <= wb_dat_i;
							wb_ack_o <= '1';
						when x"C" => -- Set trigger word [63:32]
							trig_word(63 downto 32) <= wb_dat_i;
							wb_ack_o <= '1';
						when x"D" => -- Set trigger word [95:64]
							trig_word(95 downto 64) <= wb_dat_i;
							wb_ack_o <= '1';
						when x"E" => -- Set trigger word [127:96]
							trig_word(127 downto 96) <= wb_dat_i;
							wb_ack_o <= '1';
						when x"F" => -- Toggle trigger abort
							trig_abort <= wb_dat_i(0);
							wb_ack_o <= '1';
						when others =>
							wb_ack_o <= '1';
					end case;
				else
					case (wb_adr_i(3 downto 0)) is
						when x"0" => -- Read enable mask
							wb_dat_o <= tx_enable;
							wb_ack_o <= '1';
						when x"2" => -- Read empty stat
							wb_dat_o <= tx_empty;
							wb_ack_o <= '1';
						when x"3" => -- Read trigger enable
							wb_dat_o(0) <= trig_en;
							wb_dat_o(31 downto 1) <= (others => '0');
							wb_ack_o <= '1';
						when x"4" => -- Read trigger done
							wb_dat_o(0) <= trig_done;
							wb_dat_o(31 downto 1) <= (others => '0');
							wb_ack_o <= '1';
						when x"5" => -- Read trigger conf
							wb_dat_o(3 downto 0) <= trig_conf;
							wb_dat_o(31 downto 4) <= (others => '0');
							wb_ack_o <= '1';
						when x"6" => -- Read trigger freq
							wb_dat_o <= trig_freq;
							wb_ack_o <= '1';
						when x"7" => -- Read trigger time low
							wb_dat_o <= trig_time(31 downto 0);
							wb_ack_o <= '1';
						when x"8" => -- Read trigger time high
							wb_dat_o <= trig_time(63 downto 32);
							wb_ack_o <= '1';
						when x"9" => -- Read trigger count
							wb_dat_o <= trig_count;
							wb_ack_o <= '1';
						when x"A" => -- Set trigger word length (bits)
							wb_dat_o <= trig_word_length;
							wb_ack_o <= '1';
						when x"B" => -- Set trigger word [31:0]
							wb_dat_o <= trig_word(31 downto 0);
							wb_ack_o <= '1';
						when x"C" => -- Set trigger word [63:32]
							wb_dat_o <= trig_word(63 downto 32);
							wb_ack_o <= '1';
						when x"D" => -- Set trigger word [95:64]
							wb_dat_o <= trig_word(95 downto 64);
							wb_ack_o <= '1';
						when x"E" => -- Set trigger word [127:96]
							wb_dat_o <= trig_word(127 downto 96);
							wb_ack_o <= '1';
						when x"F" => -- Trigger in frequency
							wb_dat_o <= trig_in_freq;
							wb_ack_o <= '1';
						when others =>
							wb_dat_o <= x"DEADBEEF";
							wb_ack_o <= '1';
					end case;
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
		ext_trig_i => ext_trig_i,
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
    
    -- Create 1 tick per second for counter
    per_sec_proc : process(tx_clk_i, rst_n_i)
    begin
        if (rst_n_i = '0') then
            per_second <= '0';
            per_second_cnt <= (others => '0');
        elsif rising_edge(tx_clk_i) then
            if (per_second_cnt = ticks_per_second) then
                per_second <= '1';
                per_second_cnt <= (others => '0');
            else
                per_second <= '0';
                per_second_cnt <= per_second_cnt + 1;
            end if;
        end if;
    end process per_sec_proc;
    
    -- Count incoming trig frequency
    trig_in_freq_proc : process(tx_clk_i, rst_n_i)
    begin
        if (rst_n_i = '0') then
            trig_in_freq_cnt <= (others => '0');
            ext_trig_t1 <= '0';
            ext_trig_t2 <= '0';
            ext_trig_t3 <= '0';
        elsif rising_edge(tx_clk_i) then
            ext_trig_t1 <= ext_trig_i;
            ext_trig_t2 <= ext_trig_t1;        
            ext_trig_t3 <= ext_trig_t2;        
            if (trig_done = '1') then -- reset when trigger module is done
                trig_in_freq_cnt <= (others => '0');
            else
                if (ext_trig_t2 = '1' and ext_trig_t3 = '0') then -- positive edge
                    trig_in_freq_cnt <= trig_in_freq_cnt + 1;
                end if;
                trig_in_freq <= std_logic_vector(trig_in_freq_cnt);
            end if;
        end if;
    end process trig_in_freq_proc;

end behavioral;