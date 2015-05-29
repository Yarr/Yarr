-- ####################################
-- # Project: Yarr
-- # Author: Timon Heim
-- # E-Mail: timon.heim at cern.ch
-- # Comments: Round robin arbiter, no priority
-- ####################################

library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use ieee.numeric_std.all;

entity rr_arbiter is
	generic (
        g_CHANNELS : integer := 16
    );
	port (
		-- sys connect
		clk_i : in std_logic;
		rst_i : in std_logic;
		
		-- requests
		req_i : in std_logic_vector(g_CHANNELS-1 downto 0);
		-- grant
		gnt_o : out std_logic_vector(g_CHANNELS-1 downto 0)
	);
		
end rr_arbiter;

architecture behavioral of rr_arbiter is
	signal req_t : std_logic_vector(g_CHANNELS-1 downto 0);
	signal reqs : std_logic_vector(g_CHANNELS-1 downto 0);
	signal gnt_t : std_logic_vector(g_CHANNELS-1 downto 0);
	signal gnt : std_logic_vector(g_CHANNELS-1 downto 0);
	signal gnts : std_logic_vector(g_CHANNELS-1 downto 0);
	signal gnt_d : std_logic_vector(g_CHANNELS-1 downto 0);
	
begin
	-- Tie offs
	gnt_t <= gnts when (unsigned(reqs) /= 0) else gnt;
	gnt <= req_t and(std_logic_vector(unsigned(not req_t)+1));
	reqs <= req_t and not (std_logic_vector(unsigned(gnt_d)-1) or gnt_d);
	gnts <= reqs and (std_logic_vector(unsigned(not reqs)+1));
	
	sampling_proc : process(clk_i, rst_i)
	begin
		if (rst_i = '1') then
			gnt_d <= (others => '0');
			gnt_o <= (others => '0');
			req_t <= (others => '0');
		elsif rising_edge(clk_i) then
			gnt_d <= gnt_t;
			gnt_o <= gnt_t;
			req_t <= req_i;
		end if;
	end process sampling_proc;

end behavioral;

