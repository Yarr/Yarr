library IEEE;
use IEEE.STD_LOGIC_1164.all;
use IEEE.NUMERIC_STD.all;

package board_pkg is
    constant c_TX_ENCODING : string := "NRZ";
    constant c_TX_CHANNELS : integer := 1;
    constant c_RX_CHANNELS : integer := 1;
    constant c_RX_TYPE : string := "RD53";
    constant c_RX_NUM_LANES : integer := 4;
    constant c_TX_IDLE_WORD : std_logic_vector(31 downto 0) := x"69696969";
end board_pkg;

