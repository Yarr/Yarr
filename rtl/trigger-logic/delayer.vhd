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
use ieee.math_real.all;

entity delayer is

    generic (N : integer);
    port (
        clk_i : in std_logic;
        rst_n_i : in std_logic;
        dat_i : in std_logic;
        dat_o : out std_logic;
        delay : in std_logic_vector(integer(ceil(log2(real(N))))-1 downto 0)  -- TODO: make it log2_ceil(N)-1 downto 0
    );
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

end delayer;

architecture rtl of delayer is
    signal shift_reg : std_logic_vector(N-1 downto 0);
begin
    proc : process(clk_i, rst_n_i)
    begin
        if (rst_n_i = '0') then
            dat_o <= '0';
            shift_reg <= (others => '0');
        elsif rising_edge(clk_i) then
            shift_reg(N-1 downto 1) <= shift_reg(N-2 downto 0);
            shift_reg(0) <= dat_i;
            dat_o <= shift_reg(to_integer(unsigned(delay)));
        end if;
    end process proc;
end rtl;
