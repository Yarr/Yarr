library IEEE;
use IEEE.STD_LOGIC_1164.all;
use IEEE.NUMERIC_STD.all;

library work;
use work.hw_type_pkg.all;

package board_pkg is
    constant c_FW_IDENT : std_logic_vector(31 downto 0) := c_HW_IDENT & x"030232";
    constant c_TX_ENCODING : string := "OSERDES";
    constant c_TX_CHANNELS : integer := 4;
    constant c_RX_CHANNELS : integer := 16;
    constant c_FE_TYPE : string := "RD53";
    constant c_RX_NUM_LANES : integer := 1;
    constant c_RX_SPEED : string := "0640";
    constant c_TX_IDLE_WORD : std_logic_vector(31 downto 0) := x"AAAAAAAA";
    constant c_TX_SYNC_WORD : std_logic_vector(31 downto 0) := x"817e817e";
    constant c_TX_SYNC_INTERVAL : unsigned(7 downto 0) := to_unsigned(16,8);
    constant c_TX_AZ_WORD : std_logic_vector(31 downto 0) := x"00000000";
    constant c_TX_AZ_INTERVAL : unsigned(15 downto 0) := to_unsigned(500,16);
    constant c_TX_40_DIVIDER : unsigned(3 downto 0) := to_unsigned(4,4);
end board_pkg;

