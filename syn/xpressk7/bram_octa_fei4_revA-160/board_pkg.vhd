library IEEE;
use IEEE.STD_LOGIC_1164.all;
use IEEE.NUMERIC_STD.all;

package board_pkg is
    constant c_TX_CHANNELS : integer := 8;
    constant c_RX_CHANNELS : integer := 8;
    constant c_TYPE : string = "FEI4";
    constant c_NUM_LANES : integer := 1;
end board_pkg;

