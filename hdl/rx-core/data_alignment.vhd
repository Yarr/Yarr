-- word alignment
library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;

entity data_alignment is
	port
	(
		clk : in std_logic := '0';
		reset : in std_logic := '0';
		
		din : in std_logic_vector(1 downto 0) := "00";
		din_valid : in std_logic_vector(1 downto 0) := "00";
		
		dout : out std_logic_vector(9 downto 0) := (others => '0');
		dout_valid : out std_logic := '0';
		dout_sync : out std_logic := '0'
	);
end data_alignment;

architecture Behavioral of data_alignment is
	
	-- FE-I4 Encoded Komma Words used for Alignment of the Decoder
	constant idle_word : std_logic_vector(9 downto 0) := "0011111001";
	constant sof_word : std_logic_vector(9 downto 0) := "0011111000";
	constant eof_word : std_logic_vector(9 downto 0) := "0011111010";
	
	-- Count of Bits before a new sync word is necessary (currently arbitrary)
	constant fei4syncPeriod: integer := 160000000;
	
	signal data_sr : std_logic_vector(10 downto 0) := (others => '0');
	
	signal dcnt : integer range 0 to 11 := 0;
	signal scnt : integer range 0 to fei4syncPeriod := 0;
	signal sync : std_logic := '0';
	
	signal sync_word : std_logic_vector(1 downto 0) := (others => '0');
	
	signal sync_holdoff : std_logic := '0';
begin

-- search for sync words
process (reset, data_sr) begin
	if reset = '1' then
		sync_word <= "00";
	else
		if sync_holdoff = '0' then
			if (data_sr(9 downto 0) = idle_word) or (data_sr(9 downto 0) = not idle_word) or
				(data_sr(9 downto 0) = sof_word) or (data_sr(9 downto 0) = not sof_word) or
				(data_sr(9 downto 0) = eof_word) or (data_sr(9 downto 0) = not eof_word) then
				
				sync_word <= "01";
			elsif (data_sr(10 downto 1) = idle_word) or (data_sr(10 downto 1) = not idle_word) or
					(data_sr(10 downto 1) = sof_word) or (data_sr(10 downto 1) = not sof_word) or
					(data_sr(10 downto 1) = eof_word) or (data_sr(10 downto 1) = not eof_word) then
					
				sync_word <= "10";
			else
				sync_word <= "00";
			end if;
		else
			sync_word <= "00";
		end if;
	end if;
end process;

-- data alignment
process begin
	wait until rising_edge(clk);
	
	if reset = '1' then
		dout <= (others => '0');
		dout_valid <= '0';
		sync <= '0';
		data_sr <= (others => '0');
		dcnt <= 0;
		scnt <= 0;
	else
		-- clear sync flag
		if scnt = fei4syncPeriod then
			sync <= '0';
			scnt <= 0;
		else
			scnt <= scnt + 1;
		end if;
	
		-- shift in new data
		if din_valid = "01" then
			data_sr <= data_sr(9 downto 0) & din(0);
			
			if sync_word = "01" then
				sync <= '1';
				scnt <= 1;
				dcnt <= 1;
				sync_holdoff <= '1';
			elsif sync_word = "10" then
				sync <= '1';
				scnt <= 2;
				dcnt <= 2;
				sync_holdoff <= '1';
			else
				dcnt <= dcnt + 1;
			end if;
			
			if dcnt = 10 then
				dout <= data_sr(9 downto 0);
				dout_valid <= '1';
				sync_holdoff <= '0';
				dcnt <= 1;
			elsif dcnt = 11 then
				dout <= data_sr(10 downto 1);
				dout_valid <= '1';
				sync_holdoff <= '0';
				dcnt <= 2;
			else
				dout_valid <= '0';
			end if;
		elsif din_valid = "11" then
			data_sr <= data_sr(8 downto 0) & din(0) & din(1);
			
			if sync_word = "01" then
				sync <= '1';
				scnt <= 2;
				dcnt <= 2;
			elsif sync_word = "10" then
				sync <= '1';
				scnt <= 3;
				dcnt <= 3;
			else
				dcnt <= dcnt + 2;
			end if;
			
			if dcnt = 10 then
				dout <= data_sr(9 downto 0);
				dout_valid <= '1' and sync;
				sync_holdoff <= '0';
				dcnt <= 2;
			elsif dcnt = 11 then
				dout <= data_sr(10 downto 1);
				dout_valid <= '1' and sync;
				sync_holdoff <= '0';
				dcnt <= 3;
			else
				dout_valid <= '0';
			end if;
		else
			if sync_word = "01" then
				sync <= '1';
				scnt <= 0;
				dcnt <= 0;
			elsif sync_word = "10" then
				sync <= '1';
				scnt <= 1;
				dcnt <= 1;
			end if;
			
			if dcnt = 10 then
				dout <= data_sr(9 downto 0);
				dout_valid <= '1' and sync;
				sync_holdoff <= '0';
				dcnt <= 0;
			elsif dcnt = 11 then
				dout <= data_sr(10 downto 1);
				dout_valid <= '1' and sync;
				sync_holdoff <= '0';
				dcnt <= 1;
			else
				dout_valid <= '0';
			end if;
		end if;
	end if;
end process;

dout_sync <= sync;

end Behavioral;


