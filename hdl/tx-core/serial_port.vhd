-- ####################################
-- # Project: Yarr
-- # Author: Timon Heim
-- # E-Mail: timon.heim at cern.ch
-- # Comments: Serial Port
-- # Outputs are synchronous to clk_i
-- ####################################

library IEEE;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity serial_port is
	generic (
        g_PORT_WIDTH : integer := 32
    );
    port (
        -- Sys connect
        clk_i       : in std_logic;
        rst_n_i     : in std_logic;
        -- Input
        enable_i    : in std_logic;
        data_i      : in std_logic_vector(g_PORT_WIDTH-1 downto 0);
        data_valid_i : in std_logic;
        -- Output
        data_o      : out std_logic;
        data_read_o   : out std_logic
    );
end serial_port;

architecture behavioral of serial_port is
	function log2_ceil(N : natural) return positive is
	begin
		if N <= 2 then
		  return 1;
		elsif N mod 2 = 0 then
		  return 1 + log2_ceil(N/2);
		else
		  return 1 + log2_ceil((N+1)/2);
		end if;
	end;
    -- Signals
    signal bit_count : unsigned(log2_ceil(g_PORT_WIDTH) downto 0);
    signal sreg      : std_logic_vector(g_PORT_WIDTH-1 downto 0);
begin

    -- Tie offs
    data_o <= sreg(g_PORT_WIDTH-1);
    -- Serializer proc
    serialize: process(clk_i, rst_n_i)
    begin
		if (rst_n_i = '0') then
			sreg <= (others => '0');
			bit_count <= (others => '0');
			data_read_o <= '0';
		elsif rising_edge(clk_i) then
			if (enable_i = '1') then
				if (bit_count = g_PORT_WIDTH-1 and data_valid_i = '1') then
					sreg <= data_i;
					data_read_o <= '1';
					bit_count <= (others => '0');
				else
					sreg <= sreg(g_PORT_WIDTH-2 downto 0) & '0';
					data_read_o <= '0';
					bit_count <= bit_count + 1;
				end if;
			else
			   sreg <= (others => '0');
			   data_read_o <= '0';
			   bit_count <= TO_UNSIGNED(g_PORT_WIDTH-1, bit_count'length);
			end if;
		end if;
    end process serialize;
end behavioral;
