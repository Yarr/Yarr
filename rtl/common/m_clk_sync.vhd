----------------------------------------------------------------------------------
-- Company: 
-- Engineer: 
-- 
-- Create Date: 06/20/2017 02:26:29 PM
-- Design Name: 
-- Module Name: m_clk_sync - Behavioral
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

-- Uncomment the following library declaration if using
-- arithmetic functions with Signed or Unsigned values
--use IEEE.NUMERIC_STD.ALL;

-- Uncomment the following library declaration if instantiating
-- any Xilinx leaf cells in this code.
--library UNISIM;
--use UNISIM.VComponents.all;

entity m_clk_sync is
    Generic (
        data_width_g : integer := 29
    );
    Port ( rst0_i : in STD_LOGIC;
           rst1_i : in STD_LOGIC;
           clk0_i : in STD_LOGIC;
           clk1_i : in STD_LOGIC;
           data0_i : in STD_LOGIC_VECTOR (data_width_g-1 downto 0);
           data1_o : out STD_LOGIC_VECTOR (data_width_g-1 downto 0));
end m_clk_sync;

architecture Behavioral of m_clk_sync is

    COMPONENT cross_clock_fifo
      PORT (
        rst : IN STD_LOGIC;
        wr_clk : IN STD_LOGIC;
        rd_clk : IN STD_LOGIC;
        din : IN STD_LOGIC_VECTOR(28 DOWNTO 0);
        wr_en : IN STD_LOGIC;
        rd_en : IN STD_LOGIC;
        dout : OUT STD_LOGIC_VECTOR(28 DOWNTO 0);
        full : OUT STD_LOGIC;
        empty : OUT STD_LOGIC
      );
    END COMPONENT;


    signal data_s : std_logic_vector(data_width_g - 1 downto 0);
    signal wr_en_s : std_logic;
    signal empty_s : std_logic;
    signal empty_n_s : std_logic;
    
begin
    
    process(rst0_i,clk0_i)
    begin
        if rst0_i = '1' then
                data_s <= (others=>'0');
                wr_en_s <= '0';
        elsif clk0_i'event and clk0_i = '1' then
            data_s <= data0_i;
            if data_s /= data0_i then
                wr_en_s <= '1';
            else
                wr_en_s <= '0';
            end if;
        end if;
    end process;
    
    empty_n_s <= not empty_s;
    
    your_instance_name : cross_clock_fifo
      PORT MAP (
        rst => rst0_i,
        wr_clk => clk0_i,
        rd_clk => clk1_i,
        din => data0_i,
        wr_en => wr_en_s,
        rd_en => empty_n_s,
        dout => data1_o,
        full => open,
        empty => empty_s
      );
    
end Behavioral;
