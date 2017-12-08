library IEEE;
use IEEE.STD_LOGIC_1164.all;
use IEEE.NUMERIC_STD.all;

package board_pkg is
    constant c_TX_CHANNELS : integer := 1;
    constant c_RX_CHANNELS : integer := 1;
    constant c_RX_TYPE : string := "RD53";
    constant c_RX_NUM_LANES : integer := 4;
end board_pkg;

