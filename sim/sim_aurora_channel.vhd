----------------------------------------------------------------------------------
-- Company: 
-- Engineer: 
-- 
-- Create Date: 12/11/2017 04:01:21 PM
-- Design Name: 
-- Module Name: sim_aurora_channel - Behavioral
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
use IEEE.NUMERIC_STD.ALL;

-- Uncomment the following library declaration if instantiating
-- any Xilinx leaf cells in this code.
library UNISIM;
use UNISIM.VComponents.all;

entity sim_aurora_channel is
--  Port ( );
end sim_aurora_channel;

architecture Behavioral of sim_aurora_channel is
    component aurora_rx_channel
        generic (
            g_NUM_LANES : integer range 1 to 4 := 1
        );
        port (
            -- Sys connect
            rst_n_i : in std_logic;
            clk_rx_i : in std_logic; -- Fabric clock (serdes/8)
            clk_serdes_i : in std_logic; -- IO clock
            
            -- Input
            enable_i : in std_logic;
            rx_data_i_p : in std_logic_vector(g_NUM_LANES-1 downto 0);
            rx_data_i_n : in std_logic_vector(g_NUM_LANES-1 downto 0);
            trig_tag_i : in std_logic_vector(63 downto 0);
        
            -- Output
            rx_data_o : out std_logic_vector(63 downto 0);
            rx_valid_o : out std_logic;
            rx_stat_o : out std_logic_vector(7 downto 0)
        );
    end component aurora_rx_channel;
    
    component scrambler
        port (
            data_in : in std_logic_vector(0 to 63);
            data_out : out std_logic_vector(65 downto 0);
            enable : in std_logic;
            sync_info : in std_logic_vector(1 downto 0);
            clk : in std_logic;
            rst : in std_logic
        );
    end component scrambler;
    
    signal rst_n_i : std_logic := '0';
    signal rst2_n_i : std_logic := '0';
    signal clk_rx_i : std_logic := '0';
    signal clk_serdes_i : std_logic := '0';
    signal clk_ddr_i : std_logic := '0';
    signal enable_i : std_logic := '0';
    signal rx_data_i_p : std_logic := '0';
    signal rx_data_i_n : std_logic := '0';
    signal trig_tag_i : std_logic_vector(63 downto 0) := (others => '0');
    signal rx_data_o : std_logic_vector(63 downto 0);
    signal rx_valid_o : std_logic;
    signal rx_stat_o : std_logic_vector(7 downto 0);
    
    constant RX_CLK_PERIOD : time := 6.4ns;
    constant SERDES_CLK_PERIOD : time := 1.6ns;
    constant DDR_CLK_PERIOD : time := 0.8ns;
    constant IDELAY_CLK_PERIOD : time := 3.2ns;
    
    signal tx_data : std_logic_vector(65 downto 0);
    signal tx_data_s : std_logic_vector(65 downto 0);
    signal tx_data_t : std_logic_vector(65 downto 0);
    signal tx_counter : unsigned(31 downto 0);
    signal cnt : unsigned(31 downto 0);
    signal clk_idelay : std_logic;
    signal idelay_rdy : std_logic;
    signal tx_data_valid : std_logic;
    
    attribute IODELAY_GROUP : STRING;
    attribute IODELAY_GROUP of IDELAYCTRL_inst : label is "aurora";

begin

    rx_clk_proc: process
    begin
        clk_rx_i <= '1';
        wait for RX_CLK_PERIOD/2;
        clk_rx_i <= '0';
        wait for RX_CLK_PERIOD/2;
    end process rx_clk_proc;

    serdes_clk_proc: process
    begin
        clk_serdes_i <= '1';
        wait for SERDES_CLK_PERIOD/2;
        clk_serdes_i <= '0';
        wait for SERDES_CLK_PERIOD/2;
    end process serdes_clk_proc;
    
    ddr_clk_proc: process
    begin
        clk_ddr_i <= '1';
        wait for DDR_CLK_PERIOD/2;
        clk_ddr_i <= '0';
        wait for DDR_CLK_PERIOD/2;
    end process ddr_clk_proc;
    
    idelay_clk_proc: process
    begin
        clk_idelay <= '1';
        wait for IDELAY_CLK_PERIOD/2;
        clk_idelay <= '0';
        wait for IDELAY_CLK_PERIOD/2;
    end process idelay_clk_proc;
    
    IDELAYCTRL_inst : IDELAYCTRL
    port map (
       RDY => idelay_rdy,       -- 1-bit output: Ready output
       REFCLK => clk_idelay, -- 1-bit input: Reference clock input
       RST => not rst_n_i       -- 1-bit input: Active high reset input
    );
    
    rst_proc: process
    begin
        rst_n_i <= '0';
        rst2_n_i <= '0';
        wait for 200ns;
        rst2_n_i <= '1';
        wait for 350ns;
        rst_n_i <= '1';
        wait;
    end process rst_proc;

    aurora_cmp: aurora_rx_channel port map (
        rst_n_i => rst_n_i,
        clk_rx_i => clk_rx_i,
        clk_serdes_i => clk_serdes_i,
        enable_i => enable_i,
        rx_data_i_p(0) => rx_data_i_p,
        rx_data_i_n(0) => rx_data_i_n,
        trig_tag_i => trig_tag_i,
        rx_data_o => rx_data_o,
        rx_valid_o => rx_valid_o,
        rx_stat_o => rx_stat_o
        );
        
    piso_proc: process(clk_rx_i, clk_ddr_i, rst2_n_i)
    begin
        if (rst2_n_i = '0') then
            tx_data <= (others => '1');
            tx_counter <= to_unsigned(1, 32);
            cnt <= (others => '0');
            tx_data_t <= (others => '0');
            tx_data_valid <= '0';
        elsif rising_edge(clk_ddr_i) then 
            --tx_data_valid <= '0';
            rx_data_i_p <= tx_data_t(65);-- after 3.3ns;
            rx_data_i_n <= not tx_data_t(65);-- after 3.3ns;
            tx_data_t <= tx_data_t(64 downto 0) & '0';
--            rx_data_i_p <= tx_data_t(0);
--            rx_data_i_n <= not tx_data_t(0);
--            tx_data_t <= '0' & tx_data_t(65 downto 1);
            tx_counter <= tx_counter + 1;
            if (tx_counter = 64) then
                if (to_integer(cnt) mod 64 = 0) then
                    tx_data <= "01" & std_logic_vector(cnt) & std_logic_vector(cnt);
                else
                    tx_data <= "10" & std_logic_vector(cnt) & std_logic_vector(cnt);
                end if;
                tx_data_valid <= '1';
                tx_counter <= tx_counter + 1;
                --tx_data <= "01" & x"0123456789abcdef";
                cnt <= cnt + 1;
            elsif (tx_counter = 65) then
                tx_data_valid <= '0';
                tx_data_t <= tx_data_s;
                tx_counter <= (others => '0');
            end if;
         end if;
     end process piso_proc;
     
     scrambler_cmp: scrambler port map (
            data_in => tx_data(63 downto 0),
            data_out => tx_data_s(65 downto 0),
            enable => tx_data_valid,
            sync_info => tx_data(65 downto 64),
            clk => clk_ddr_i,
            rst => not rst2_n_i        
     );
            
                 
end Behavioral;
