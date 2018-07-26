-- ####################################
-- # Project: Yarr
-- # Author: Vyassa Baratham
-- # E-Mail: vbaratham at berkeley.edu
-- # Comments: Allows configurable delay of up to N clk cycles
-- # Data: 09/2017
-- # Outputs are synchronous to clk_i
-- ####################################

library IEEE;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity delayer is

    generic (N : integer); -- shift register width
    port (
        clk_i : in std_logic;
        rst_n_i : in std_logic;
        dat_i : in std_logic;
        dat_o : out std_logic;
        delay : in std_logic_vector(N-1 downto 0)
    );

end delayer;

architecture rtl of delayer is
    signal shift_reg : std_logic_vector(2**N-1 downto 0);
begin
    proc : process(clk_i, rst_n_i)
    begin
        if (rst_n_i = '0') then
            dat_o <= '0';
            shift_reg <= (others => '0');
        elsif rising_edge(clk_i) then
            shift_reg(2**N-1 downto 1) <= shift_reg(2**N-2 downto 0);
            shift_reg(0) <= dat_i;
            dat_o <= shift_reg(to_integer(unsigned(delay)));
        end if;
    end process proc;
end rtl;
