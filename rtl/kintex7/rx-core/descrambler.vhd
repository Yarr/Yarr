-- 64b66b descrambler

library IEEE;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity descrambler is
    port (
        -- Sys connect
        rst_n_i : in std_logic;
        clk_i : in std_logic
        --Input
        data_i : in std_logic_vector(63 downto 0);
        valid_i : in std_logic;
        -- Output
        data_o : out std_logic_vector(63 downto 0);
        valid_o : out std_logic
    );
end descrambler;

architecture rtl of descrambler is
    signal xorBit : std_logic_vector(63 downto 0);
    signal buf : std_logic_vector(63 downto 0);
begin

    descramble_proc: process(clk_i, rst_n_i)
    begin
        if (rst_n_i) then
            data_o <= (others => '0');
            valid_o <= '0';
        elsif rising_edge(clk_i) then
            if (valid_i = '1') then
                bit_loop: for I in 0 to 63 generate
                    xorBit(I) <= data_i(I) xor buf(38) xor buf(57);
                    data_o(I) <= xorBit(I);
                    buf(I) <= data_i(I);
                end generate;
                valid_o <= '1'
            end if;
        end if;
    end process descramble_proc;

end rtl;
