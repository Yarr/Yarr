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

entity pos_edge_finder is
    port (
        clk_i : in std_logic;
        dat_i : in std_logic;
        dat_o : out std_logic
    );
end pos_edge_finder;

architecture pos of pos_edge_finder is
    signal found_edge : std_logic;
begin
    find_pos : process(dat_i)
    begin
        found_edge <= '0';
        if rising_edge(dat_i) then
            found_edge <= '1';
        end if;
    end process find_pos;
    
    output : process(clk_i)
    begin
        if rising_edge(clk_i) then
            if found_edge = '1' then dat_o <= '1'; end if;
        else
            dat_o <= '0';
        end if;
    end process output;
end pos;


entity neg_edge_finder is
    port (
        clk_i : in std_logic;
        dat_i : in std_logic;
        dat_o : out std_logic
    );
end neg_edge_finder;

architecture neg of neg_edge_finder is
    signal found_edge : std_logic;
begin
    find_neg : process(dat_i)
    begin
        found_edge <= '0';
        if falling_edge(dat_i) then
            found_edge <= '1';
        end if;
    end process find_neg;
    
    output : process(clk_i)
    begin
        if rising_edge(clk_i) then
            if found_edge = '1' then dat_o <= '1'; end if;
        else
            dat_o <= '0';
        end if;
    end process output;
end neg;

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
    component pos_edge_finder
        port (
            clk_i : in std_logic;
            dat_i : in std_logic;
            dat_o : out std_logic
        );
    end component;
    component neg_edge_finder
        port (
            clk_i : in std_logic;
            dat_i : in std_logic;
            dat_o : out std_logic
        );
    end component;
begin
    cmp_pos_edge_finder : pos_edge_finder
        port map(clk_i => clk_i, dat_i => dat_i, dat_o => rising_o);
    cmp_neg_edge_finder : neg_edge_finder
        port map(clk_i => clk_i, dat_i => dat_i, dat_o => falling_o);
    
end rtl;
