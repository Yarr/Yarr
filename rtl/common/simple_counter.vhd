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
use IEEE.NUMERIC_STD.ALL;

-- Uncomment the following library declaration if instantiating
-- any Xilinx leaf cells in this code.
--library UNISIM;
--use UNISIM.VComponents.all;

entity simple_counter is
    Port ( 
           enable_i : in STD_LOGIC;
	   rst_i : in STD_LOGIC;
           clk_i : in STD_LOGIC;
           count_o : out STD_LOGIC_VECTOR (28 downto 0);
           gray_count_o : out STD_LOGIC_VECTOR (28 downto 0)
            );
end simple_counter;


-- Implementation:

-- There is an imaginary bit in the counter, at q(0), that resets to 1
-- (unlike the rest of the bits of the counter) and flips every clock cycle.
-- The decision of whether to flip any non-imaginary bit in the counter
-- depends solely on the bits below it, down to the imaginary bit.	It flips
-- only if all these bits, taken together, match the pattern 10* (a one
-- followed by any number of zeros).

-- Almost every non-imaginary bit has a component instance that sets the 
-- bit based on the values of the lower-order bits, as described above.
-- The rules have to differ slightly for the most significant bit or else 
-- the counter would saturate at it's highest value, 1000...0.

architecture Behavioral of simple_counter is
    signal count_s : std_logic_vector(28 downto 0) := (others =>'0');
    -- q contains all the values of the counter, plus the imaginary bit
    -- (values are shifted to make room for the imaginary bit at q(0))
    signal q  : std_logic_vector (29 downto 0);
    
    -- no_ones_below(x) = 1 iff there are no 1's in q below q(x)
    signal no_ones_below  : std_logic_vector (29 downto 0);
    
    -- q_msb is a modification to make the msb logic work
    signal q_msb : std_logic;

begin
    q_msb <= q(28) or q(29);
    
    process(clk_i,rst_i)
         
     begin
     
        if rst_i = '1' then
            count_s <= (others =>'0');
            q(0) <= '1';
            q(29 downto 1) <= (others => '0');

            
        elsif clk_i='1' and clk_i'event and enable_i = '1' then
            count_s <= count_s + 1;
			-- Toggle the imaginary bit
            q(0) <= not q(0);
            
            for i in 1 to 29 loop
            
                -- Flip q(i) if lower bits are a 1 followed by all 0's
                q(i) <= q(i) xor (q(i-1) and no_ones_below(i-1));
            
            end loop;  -- i
            
            q(29) <= q(29) xor (q_msb and no_ones_below(28));
            
        end if;
        
    end process;
    

    
    process(q, no_ones_below)
    begin
    	-- There are never any 1's beneath the lowest bit
        no_ones_below(0) <= '1';
        
        for j in 1 to 29 loop
            no_ones_below(j) <= no_ones_below(j-1) and not q(j-1);
        end loop;
    end process;
    
    -- Copy over everything but the imaginary bit
    gray_count_o <= q(29 downto 1);
    
    count_o <= count_s;
    
end Behavioral;
