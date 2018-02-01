-- ####################################
-- # Project: Yarr
-- # Author: Timon Heim
-- # E-Mail: timon.heim at cern.ch
-- # Comments: Trigger Logic
-- ####################################

library IEEE;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity trigger_unit is
	port (
		clk_i 	: in  std_logic;
		rst_n_i	: in  std_logic;
		
		-- Serial Trigger Out
		--trig_o : out std_logic;
		trig_pulse_o : out std_logic;
		
		-- Trigger In (async)
		ext_trig_i	: in std_logic;
		
		-- Config
		--trig_word_i : in std_logic_vector(127 downto 0); -- Trigger command
		--trig_word_length_i : in std_logic_vector(31 downto 0);
		trig_freq_i : in std_logic_vector(31 downto 0); -- Number of clock cycles between triggers
		trig_time_i : in std_logic_vector(63 downto 0); -- Clock cycles
		trig_count_i : in std_logic_vector(31 downto 0); -- Fixed number of triggers
		trig_conf_i	: in std_logic_vector(3 downto 0); -- Internal, external, pseudo random, 
		trig_en_i : in std_logic;
		trig_abort_i : in std_logic;
		trig_done_o : out std_logic
	);
end trigger_unit;

architecture Behavioral of trigger_unit is
    -- Signals
   signal bit_count : unsigned(7 downto 0);
   signal sreg      : std_logic_vector(127 downto 0);
	signal trig_pulse : std_logic;
	
	-- Registers
	signal trig_word : std_logic_vector(127 downto 0);
	signal trig_word_length : std_logic_vector(31 downto 0);
	signal trig_freq : std_logic_vector(31 downto 0);
	signal trig_time : std_logic_vector(63 downto 0);
	signal trig_count : std_logic_vector(31 downto 0);
	signal trig_conf : stD_logic_vector(3 downto 0);
	signal trig_en : std_logic;
	constant c_DONE_DELAY : integer := 32;
	signal trig_done : std_logic_vector(c_DONE_DELAY-1 downto 0);
	
	-- Counters
	signal stopwatch_cnt : unsigned(63 downto 0);
	signal int_trig_cnt : unsigned(31 downto 0);
	signal freq_cnt : unsigned(31 downto 0);
	
	-- Sync
	signal trig_en_d0 : std_logic;
	signal trig_en_d1 : std_logic;
	signal trig_en_pos : std_logic;
	signal trig_en_neg : std_logic;
	signal ext_trig_d0 : std_logic;
	signal ext_trig_d1 : std_logic;
	signal ext_trig_d2 : std_logic;
	signal ext_trig_d3 : std_logic;
	signal ext_trig_d4 : std_logic;
	signal ext_trig_pos : std_logic;
	
    constant c_DEADTIME : integer := 10; -- Deadtime moved to trigger logic
	signal deadtime : unsigned(7 downto 0);
	
begin
	-- Done conditions
	done_proc : process(clk_i, rst_n_i)
	begin
		if (rst_n_i = '0') then
			trig_done(0) <= '0';
		elsif rising_edge(clk_i) then
			if (trig_en = '0') then -- Reset done on disable
				trig_done(0) <= '0';
			elsif (trig_abort_i = '1') then -- Abort triggering
				trig_done(0) <= '1';
			elsif (trig_conf = x"0") then -- External, abot will set done
				--trig_done(0) <= '0';
			elsif (trig_conf = x"1") then -- Internal time
				if (stopwatch_cnt = unsigned(trig_time)) then
					trig_done(0) <= '1';
				end if;
			elsif (trig_conf = x"2") then -- Internal count
				if (int_trig_cnt = unsigned(trig_count)) then
					trig_done(0) <= '1';
				end if;
			--elsif (trig_conf = x"3") then -- Pseudo Random
			else -- unknown conf
				trig_done(0) <= '1';
			end if;
		end if;
	end process done_proc;
	
	-- Stopwatch
	stopwatch_proc : process (clk_i, rst_n_i)
	begin
		if (rst_n_i = '0') then
			stopwatch_cnt <= (others => '0');
		elsif rising_edge(clk_i) then
			if (trig_done(0) = '1') then
				stopwatch_cnt <= (others => '0');
			elsif (trig_en = '1') then
				stopwatch_cnt <= stopwatch_cnt + 1;
			end if;
		end if;
	end process stopwatch_proc;
	
	-- Trigger count
	int_trig_cnt_proc : process (clk_i, rst_n_i)
	begin
		if (rst_n_i = '0') then
			int_trig_cnt <= (others => '0');
		elsif rising_edge(clk_i) then
			if (trig_done(0) = '1') then
				int_trig_cnt <= (others => '0');
			elsif (trig_en = '1' and trig_pulse = '1') then
				int_trig_cnt <= int_trig_cnt + 1;
			end if;
		end if;
	end process int_trig_cnt_proc;
	
	-- Trigger Pulser
	trig_pulse_o <= trig_pulse;
	trig_pulse_proc : process(clk_i, rst_n_i)
	begin
		if (rst_n_i = '0') then
			trig_pulse <= '0';
			freq_cnt <= (others => '0');
		elsif rising_edge(clk_i) then
			if (trig_conf = x"0") then -- Pusling on External rising edge
				if (trig_en = '1' and ext_trig_pos = '1' and trig_done(0) = '0') then
					trig_pulse <= '1';
				else
					trig_pulse <= '0';
				end if;
			else -- Pulsing on requency counter
				if (trig_done(0) = '1') then
					trig_pulse <= '0';
					freq_cnt <= (others => '0');
				elsif (trig_en = '1') then
					if (freq_cnt = unsigned(trig_freq)) then	
						freq_cnt <= (others => '0');
						trig_pulse <= '1';
					else
						freq_cnt <= freq_cnt + 1;
						trig_pulse <= '0';
					end if;
				end if;
			end if;
		end if;
	end process trig_pulse_proc;

    -- Tie offs
--    trig_o <= sreg(127);
--    -- Serializer proc
--    serialize: process(clk_i, rst_n_i)
--    begin
--		if (rst_n_i = '0') then
--			sreg <= (others => '0');
--			bit_count <= (others => '0');
--		elsif rising_edge(clk_i) then
--			if (trig_pulse = '1') then
--				sreg <= trig_word;
--				bit_count <= (others => '0');
----			elsif (bit_count <= unsigned(trig_word_length(7 downto 0))) then
--			else
--				sreg <= sreg(126 downto 0) & '0';
----				bit_count <= bit_count + 1;
----			else
----				sreg <= (others => '0');
--			end if;
--		end if;
--    end process serialize;
	
	-- Sync proc
	sync_proc : process (clk_i, rst_n_i)
	begin
		if (rst_n_i = '0') then
			--trig_word <= (others => '0');
			--trig_word_length <= (others => '0');
			trig_freq <= (others => '0');
			trig_time <= (others => '0');
			trig_count <= (others => '0');
			trig_conf <= (others => '0');
			trig_en <= '0';
			trig_done_o <= '0';
			trig_done(c_DONE_DELAY-1 downto 1) <= (others => '0');
			ext_trig_d0 <= '0';
			ext_trig_d1 <= '0';
			ext_trig_d2 <= '0';
			ext_trig_d3 <= '0';
			ext_trig_d4 <= '0';
			ext_trig_pos <= '0';
			trig_en_d0 <= '0';
			trig_en_d1 <= '0';
			trig_en_pos <= '0';
			trig_en_neg <= '0';
            deadtime <= (others => '0');
		elsif rising_edge(clk_i) then
			ext_trig_d0 <= ext_trig_i; -- async input
			ext_trig_d1 <= ext_trig_d0;
			ext_trig_d2 <= ext_trig_d1;
			ext_trig_d3 <= ext_trig_d2;
			ext_trig_d4 <= ext_trig_d3;
			-- Triggered on pos edge of external signal and high longer than 25ns
			if (ext_trig_d4 = '0' and ext_trig_d3 = '1' and deadtime = 0) then
				ext_trig_pos <= '1';
                deadtime <= to_unsigned(c_DEADTIME, 8);
			else
				ext_trig_pos <= '0';
			end if;
		
			trig_en_d0 <= trig_en_i;
			trig_en_d1 <= trig_en_d0;
			if (trig_en_d1 = '0' and trig_en_d0 = '1') then
				trig_en_pos <= '1';
				trig_en_neg <= '0';
			elsif (trig_en_d1 = '1' and trig_en_d0 = '0') then
				trig_en_pos <= '0';
				trig_en_neg <= '1';			
			else
				trig_en_neg <= '0';
				trig_en_pos <= '0';
			end if;
			
			if (trig_en_pos = '1') then		
				--trig_word <= trig_word_i;
				--trig_word_length <= trig_word_length_i;
				trig_freq <= trig_freq_i;
				trig_time <= trig_time_i;
				trig_count <= trig_count_i;
				trig_conf <= trig_conf_i;
				trig_en <= '1';
			elsif (trig_en_neg = '1') then
				trig_en <= '0';
			end if;

			for I in 1 to c_DONE_DELAY-1 loop
				trig_done(I) <= trig_done(I-1);
			end loop;
			trig_done_o <= trig_done(c_DONE_DELAY-1);

            if (deadtime > 0) then
                deadtime <= deadtime - 1;
            end if;
                    
		end if;
	end process;
end Behavioral;

