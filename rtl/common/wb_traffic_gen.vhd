----------------------------------------------------------------------------------
-- Company: 			LBNL
-- Engineer: 			Arnaud Sautaux
-- E-Mail:			asautaux@lbl.gov
--
-- Project:			YARR
-- Module:			Traffic generator
-- Description:			Master generating traffic
----------------------------------------------------------------------------------


library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use ieee.std_logic_arith.all;
use ieee.std_logic_unsigned.all;


entity wb_traffic_gen is
	generic (
		ADDR_WIDTH : integer := 32;
		DATA_WIDTH : integer := 64;
		WRITE : std_logic := '1';
		COUNTER_START : integer := 0
	);
	port (
		-- SYS CON
		clk			: in std_logic;
		rst			: in std_logic;
		en          : in std_logic;
		
		-- Wishbone Master out
		wb_adr_o			: out std_logic_vector(ADDR_WIDTH-1 downto 0);
		wb_dat_o			: out std_logic_vector(DATA_WIDTH-1 downto 0);
		wb_we_o				: out std_logic;
		wb_stb_o			: out std_logic;
		wb_cyc_o			: out std_logic; 
		
		-- Wishbone Master in
		--wb_dat_i			: in std_logic_vector(DATA_WIDTH-1 downto 0);
		--wb_ack_i			: in std_logic;
		wb_stall_i			: in std_logic
	);
end wb_traffic_gen;
	 
architecture Behavioral of wb_traffic_gen is
	signal counter : std_logic_vector(DATA_WIDTH-1 downto 0);
	
	
	 
begin
	
	
	traffic_p: process (clk, rst)
	begin
		if (rst ='1') then
			counter <= conv_std_logic_vector(COUNTER_START,DATA_WIDTH);
			wb_adr_o <= conv_std_logic_vector(COUNTER_START,ADDR_WIDTH);
			wb_dat_o <= conv_std_logic_vector(COUNTER_START,DATA_WIDTH);	
			wb_we_o	<= '0';
			wb_stb_o <= '0';	
			wb_cyc_o <= '0';
		elsif (clk'event and clk = '1') then
			if wb_stall_i = '0' and en = '1' then
				counter <= counter + 1;
				wb_adr_o <= counter(ADDR_WIDTH-1 downto 0);
				wb_dat_o <= counter;
				wb_we_o	<= WRITE;
				wb_stb_o <= '1';	
				wb_cyc_o <= '1';
			else
				counter <= counter;
				wb_adr_o <= (others => '0');
				wb_dat_o <= (others => '0');	
				wb_we_o	<= '0';
				wb_stb_o <= '0';	
				wb_cyc_o <= '0';
				
			end if;
		end if;
	end process traffic_p;

end Behavioral;
