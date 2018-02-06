-- ####################################
-- # Project: Yarr
-- # Author: Timon Heim
-- # E-Mail: timon.heim at cern.ch
-- # Comments: RX channel
-- # FE-I4 Style Rx Channel; Sync, Align & Decode
-- ####################################


library IEEE;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

library decode_8b10b;

entity fei4_rx_channel is
	port (
		-- Sys connect
		rst_n_i : in std_logic;
		clk_160_i : in std_logic;
		clk_640_i : in std_logic;
		
		enable_i : in std_logic;
		
		-- Input
		rx_data_i : in std_logic;
        trig_tag_i : in std_logic_vector(31 downto 0);
		
		-- Output
		rx_data_o : out std_logic_vector(25 downto 0);
		rx_valid_o : out std_logic;
		rx_stat_o : out std_logic_vector(7 downto 0);
		rx_data_raw_o : out std_logic_vector(7 downto 0)
	);
end fei4_rx_channel;

architecture behavioral of fei4_rx_channel is
	component cdr_serdes 
		port (
			-- clocks
			clk160 : in std_logic;
			clk640 : in std_logic;

			-- reset
			reset : in std_logic;

			-- data input
			din : in std_logic;
			slip : in std_logic;

			-- data output
			data_value : out std_logic_vector(1 downto 0);
			data_valid : out std_logic_vector(1 downto 0);
			data_lock : out std_logic
		);
	end component;
	
	component data_alignment
		port
		(
			clk : in std_logic;
			reset : in std_logic;
			
			din : in std_logic_vector(1 downto 0);
			din_valid : in std_logic_vector(1 downto 0);
			
			dout : out std_logic_vector(9 downto 0);
			dout_valid : out std_logic;
			dout_sync : out std_logic
		);
	end component;
	
	component decode_8b10b_wrapper
		port(
			CLK : IN std_logic;
			DIN : IN std_logic_vector(9 downto 0);
			CE : IN std_logic;
			SINIT : IN std_logic;          
			DOUT : OUT std_logic_vector(7 downto 0);
			KOUT : OUT std_logic;
			CODE_ERR : OUT std_logic;
			DISP_ERR : OUT std_logic;
			ND : OUT std_logic
		);
	end component;
	
	constant c_SOF : std_logic_vector(7 downto 0) := x"fc";
	constant c_EOF : std_logic_vector(7 downto 0) := x"bc";
	constant c_IDLE : std_logic_vector(7 downto 0) := x"3c";

	signal data_raw_value : std_logic_vector(1 downto 0);
	signal data_raw_valid : std_logic_vector(1 downto 0);
	signal data_raw_lock : std_logic;
	
	signal data_enc_value : std_logic_vector(9 downto 0);
	signal data_enc_valid : std_logic;
	signal data_enc_sync : std_logic;
	
	signal data_enc_value_rev : std_logic_vector(9 downto 0);
	signal data_enc_valid_rev : std_logic;
	signal data_enc_valid_rev_d : std_logic;
	
	signal data_dec_value : std_logic_vector(7 downto 0);
	signal data_dec_valid : std_logic;
	signal data_dec_kchar : std_logic;
	signal data_dec_decerr : std_logic;
	signal data_dec_disperr : std_logic;
	
	signal data_fram_cnt : unsigned(1 downto 0);
	signal data_frame_flag : std_logic;
	signal data_frame_value : std_logic_vector(25 downto 0);
	signal data_frame_valid : std_logic;
	
	signal status : std_logic_vector(7 downto 0);

begin
	-- Status Output
	rx_stat_o <= status;
	status(0) <= data_raw_lock;
	status(1) <= data_enc_sync;
	status(2) <= data_dec_decerr;
	status(3) <= data_dec_disperr;
	status(5 downto 4) <= data_raw_value;
	status(7 downto 6) <= data_raw_valid;
	rx_data_raw_o <= data_dec_value;

	-- Frame collector
	rx_data_o <= data_frame_value;
	rx_valid_o <= data_frame_valid and data_raw_lock and data_enc_sync and enable_i;
	framing_proc : process(clk_160_i, rst_n_i)
	begin
		if (rst_n_i = '0') then
			data_fram_cnt <= (others => '0');
			data_frame_flag <= '0';
			data_frame_value <= (others => '0');
			data_frame_valid <= '0';
		elsif rising_edge(clk_160_i) then	
			-- Count bytes
			if (data_frame_flag = '1' and data_dec_valid = '1' and data_fram_cnt = 2) then
				data_fram_cnt <= (others => '0');
				data_frame_valid <= '1';
			elsif (data_frame_flag = '1' and data_dec_valid = '1' and data_fram_cnt < 2) then
				data_fram_cnt <= data_fram_cnt + 1;
				data_frame_valid <= '0';
			elsif (data_frame_flag = '0') then
				data_fram_cnt <= (others => '0');
				data_frame_valid <= '0';
			else
				data_frame_valid <= '0';
			end if;
			
            -- Mark Start and End of Frame
			if (data_dec_valid = '1' and data_dec_kchar = '1' and data_dec_value = c_SOF and data_enc_sync = '1') then
				data_frame_flag <= '1';
                data_frame_value(25 downto 24) <= "01"; -- tag code
                data_frame_value(23 downto 0) <= trig_tag_i(23 downto 0);
                data_frame_valid <= '1';
			elsif (data_dec_valid = '1' and data_dec_kchar = '1' and (data_dec_value = c_EOF or data_dec_value = c_IDLE)) then
				data_frame_flag <= '0';
			end if;
            
			-- Build Frame
			if (data_frame_flag = '1' and data_dec_valid = '1' and data_dec_kchar = '0' ) then
                data_frame_value(25 downto 24) <= "00"; -- no special code
				data_frame_value(23 downto 16) <= data_frame_value(15 downto 8);
				data_frame_value(15 downto 8) <= data_frame_value(7 downto 0);
				data_frame_value(7 downto 0) <= data_dec_value;
			end if;
		end if;
	end process framing_proc;

	-- Reverse bit order to make it standard
	reverse_proc : process (clk_160_i, rst_n_i)
	begin
		if (rst_n_i = '0') then
			data_enc_value_rev <= (others => '0');
			data_enc_valid_rev <= '0';
			data_enc_valid_rev_d <= '0';
			data_dec_valid <= '0';
		elsif rising_edge(clk_160_i) then
			for I in 0 to 9 loop
				data_enc_value_rev(I) <= data_enc_value(9-I);
			end loop;
			data_enc_valid_rev <= data_enc_valid;
			data_enc_valid_rev_d <= data_enc_valid_rev;
			data_dec_valid <= data_enc_valid_rev_d;
		end if;
	end process reverse_proc;

	cmp_cdr_serdes : cdr_serdes port map (
		clk160 => clk_160_i,
		clk640 => clk_640_i, 
		reset => not rst_n_i,
		din => rx_data_i,
		slip => '0',
		data_value => data_raw_value,
		data_valid => data_raw_valid,
		data_lock => data_raw_lock
	);
	
	cmp_data_align : data_alignment port map (
		clk => clk_160_i,
		reset => not rst_n_i,
		din => data_raw_value,
		din_valid => data_raw_valid,
		dout => data_enc_value,
		dout_valid => data_enc_valid,
		dout_sync => data_enc_sync
	);
	
	cmp_decoder: decode_8b10b_wrapper PORT MAP(
		CLK => clk_160_i,
		DIN => data_enc_value_rev,
		CE => data_enc_valid_rev,
		SINIT => '0',
		DOUT => data_dec_value,
		KOUT => data_dec_kchar,
		CODE_ERR => data_dec_decErr,
		DISP_ERR => data_dec_dispErr,
		ND => open
	);
		
end behavioral;

