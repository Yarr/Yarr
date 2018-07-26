-- ####################################
-- # Project: Yarr
-- # Author: Vyassa Baratham
-- # E-Mail: vbaratham at berkeley.edu
-- # Comments: assert the output for the duration of the
-- #           clock cycle following an edge on the input
-- # Data: 09/2017
-- # Outputs are synchronous to clk_i
-- ####################################

library IEEE;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity edge_detector is
    port (
        clk_i : in std_logic;
        rst_n_i : in std_logic;
        
        dat_i : in std_logic;
        rising_o : out std_logic;
        falling_o : out std_logic
    );
end edge_detector;

architecture rtl of edge_detector is
    signal prev_dat_i : std_logic;
begin
    proc : process(clk_i, rst_n_i)
    begin
        if (rst_n_i = '0') then
            prev_dat_i <= '0';
            rising_o <= '0';
            falling_o <= '0';
        elsif rising_edge(clk_i) then
            if (dat_i /= prev_dat_i) then
                falling_o <= prev_dat_i;
                rising_o <= dat_i;
            else
                falling_o <= '0';
                rising_o <= '0';
            end if;
            prev_dat_i <= dat_i;
        end if;
    end process proc;
end rtl;
