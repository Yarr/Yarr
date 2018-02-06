-- ####################################
-- # Project: Yarr
-- # Author: Timon Heim
-- # E-Mail: timon.heim at cern.ch
-- # Comments: RX channel
-- # Aurora style rx code
-- ####################################

library IEEE;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity gearbox32to66 is
    port (
        -- Sys connect
        rst_i : in std_logic;
        clk_i : in std_logic;
        -- Input
        data32_i : in std_logic_vector(31 downto 0);
        data32_valid_i : in std_logic;
        slip_i : in std_logic;
        -- Outoput
        data66_o : out std_logic_vector(65 downto 0);
        data66_valid_o : out std_logic
    );
end gearbox32to66;

architecture rtl of gearbox32to66 is
    signal gearbox_cnt : unsigned(7 downto 0);
    signal shift_cnt : std_logic;
    signal buffer128 : std_logic_vector(127 downto 0);
    signal slip_cnt : std_logic;
begin
    shift_proc: process(clk_i, rst_i)
    begin
        if (rst_i = '1') then
            buffer128 <= (others => '0');
            gearbox_cnt <= (others => '0');
            data66_valid_o <= '0';
            data66_o <= (others => '0');
            shift_cnt <= '0';
            slip_cnt <= '0';
        elsif rising_edge(clk_i) then
            data66_valid_o <= '0';
            if (data32_valid_i = '1') then
                shift_cnt <= not shift_cnt;
                buffer128(127 downto 0) <= buffer128(95 downto 0) & data32_i;
                data66_o <= buffer128(128-(to_integer(gearbox_cnt(4 downto 0))*2)-1 downto 62-(to_integer(gearbox_cnt(4 downto 0))*2));
                if (shift_cnt = '1') then
                    if (slip_i = '1') then
                        gearbox_cnt <= gearbox_cnt;
                        data66_valid_o <= '1';
                    elsif (gearbox_cnt = 32) then
                        gearbox_cnt <= (others => '0');
                        data66_valid_o <= '0';
                    else
                        gearbox_cnt <= gearbox_cnt + 1;
                        data66_valid_o <= '1';
                    end if;
                end if;
            end if;
        end if;
    end process shift_proc;
end rtl;
