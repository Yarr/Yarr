-- ####################################
-- # Project: Yarr
-- # Author: Timon Heim
-- # E-Mail: timon.heim at cern.ch
-- # Comments: Forced Round robin arbiter
-- ####################################

library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use ieee.numeric_std.all;

entity frr_arbiter is
	generic (
        g_CHANNELS : integer := 16
    );
	port (
		-- sys connect
		clk_i : in std_logic;
		rst_i : in std_logic;
		
		-- requests
		req_i : in std_logic_vector(g_CHANNELS-1 downto 0);
        en_i : in std_logic_vector(g_CHANNELS-1 downto 0);

		-- grant
		gnt_o : out std_logic_vector(g_CHANNELS-1 downto 0)
	);
		
end frr_arbiter;

architecture behavioral of frr_arbiter is
	constant c_ALL_ZEROS : std_logic_vector(g_CHANNELS-1 downto 0) := (others => '0');
	constant c_ALL_ONES : std_logic_vector(g_CHANNELS-1 downto 0) := (others => '1');
	signal gnt_t : std_logic_vector(g_CHANNELS-1 downto 0);
	signal dis_t : std_logic_vector(g_CHANNELS-1 downto 0);
	signal req_d0 : std_logic_vector(g_CHANNELS-1 downto 0);
	signal req_d1 : std_logic_vector(g_CHANNELS-1 downto 0);

begin

	gnt_o <= gnt_t;
    dis_t <= not en_i;
	
	arb_proc : process(clk_i, rst_i, req_i, gnt_t)
	begin
		if (rst_i = '1') then
			gnt_t <= (others => '0');
			req_d0 <= (others => '0');
			req_d1 <= (others => '0');
		elsif rising_edge(clk_i) then
			if (((en_i and req_i) /= c_ALL_ZEROS) and 
            ((gnt_t = c_ALL_ZEROS))) then --or ((en_i and std_logic_vector(unsigned(dis_t) + unsigned(gnt_t(g_CHANNELS-2 downto 0) & '0'))) = c_ALL_ZEROS) )) then
				gnt_t <=en_i and std_logic_vector( unsigned(dis_t) + unsigned(c_ALL_ZEROS(g_CHANNELS-2 downto 0) & '1'));
			elsif (gnt_t /= c_ALL_ZEROS) then
				gnt_t <=en_i and std_logic_vector( unsigned(dis_t) + unsigned(gnt_t(g_CHANNELS-2 downto 0) & '0'));
			end if;

		end if;
	end process arb_proc;

end behavioral;

