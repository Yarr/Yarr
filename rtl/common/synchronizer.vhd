-- ####################################
-- # Project: Yarr
-- # Author: Timon Heim
-- # E-Mail: timon.heim at cern.ch
-- # Comments: Deglitches async inputs
-- # Data: 09/2016
-- # Outputs are synchronous to clk_i
-- ####################################

library IEEE;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity synchronizer is
    port (
        -- Sys connect
        clk_i : in std_logic;
        rst_n_i : in std_logic;

        -- Async input
        async_in : in std_logic;
        sync_out : out std_logic
    );
end synchronizer;

architecture rtl of synchronizer is
    signal deglitch_t1 : std_logic;
    signal deglitch_t2 : std_logic;
begin
    deglitch_proc : process(clk_i, rst_n_i)
    begin
        if (rst_n_i = '0') then
            sync_out <= '0';
            deglitch_t1 <= '0';
            deglitch_t2 <= '0';
        elsif rising_edge(clk_i) then
            deglitch_t1 <= async_in;
            deglitch_t2 <= deglitch_t1;
            sync_out <= deglitch_t2;
        end if;
    end process deglitch_proc;
end rtl;
