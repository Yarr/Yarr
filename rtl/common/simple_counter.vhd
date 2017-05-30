----------------------------------------------------------------------------------
-- Company: 
-- Engineer: 
-- 
-- Create Date: 09/26/2016 05:31:29 PM
-- Design Name: 
-- Module Name: simple_counter - Behavioral
-- Project Name: 
-- Target Devices: 
-- Tool Versions: 
-- Description: 
-- 
-- Dependencies: 
-- 
-- Revision:
-- Revision 0.01 - File Created
-- Additional Comments:
-- 
----------------------------------------------------------------------------------


library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.STD_LOGIC_ARITH.ALL;
use IEEE.STD_LOGIC_UNSIGNED.ALL;

-- Uncomment the following library declaration if using
-- arithmetic functions with Signed or Unsigned values
--use IEEE.NUMERIC_STD.ALL;

-- Uncomment the following library declaration if instantiating
-- any Xilinx leaf cells in this code.
--library UNISIM;
--use UNISIM.VComponents.all;

entity simple_counter is
    Port ( 
           rst_i : in STD_LOGIC;
           clk_i : in STD_LOGIC;
           count_o : out STD_LOGIC_VECTOR (28 downto 0)
            );
end simple_counter;

architecture Behavioral of simple_counter is
   
begin
    process(clk_i,rst_i)
         variable count_int : std_logic_vector(28 downto 0) := (others =>'0');
     begin
     
        if rst_i = '1' then
            count_int := (others =>'0');
        elsif clk_i='1' and clk_i'event then
            count_int := count_int + 1;
        end if;
        count_o <= count_int;
    end process;
  
    
end Behavioral;
