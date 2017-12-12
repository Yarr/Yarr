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

entity aurora_rx_channel is
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
end aurora_rx_channel;

architecture behavioral of aurora_rx_channel is
    component aurora_rx_lane
        port (
        -- Sys connect
        rst_n_i : in std_logic;
        clk_rx_i : in std_logic;
        clk_serdes_i : in std_logic;

        -- Input
        rx_data_i_p : in std_logic;
        rx_data_i_n : in std_logic;

        -- Output
        rx_data_o : out std_logic_vector(63 downto 0);
        rx_header_o : out std_logic_vector(1 downto 0);
        rx_valid_o : out std_logic;
        rx_stat_o : out std_logic_vector(7 downto 0)
    );
    end component aurora_rx_lane;

    type rx_data_array is array (g_NUM_LANES-1 downto 0) of std_logic_vector(63 downto 0);
    signal rx_data : rx_data_array;
    type rx_header_array is array (g_NUM_LANES-1 downto 0) of std_logic_vector(1 downto 0);
    signal rx_header : rx_header_array;
    signal rx_data_valid : std_logic_vector(g_NUM_LANES-1 downto 0);
            
begin
    
    lane_loop: for I in 0 to g_NUM_LANES-1 generate
        lane_cmp : aurora_rx_lane port map (
            rst_n_i => rst_n_i,
            clk_rx_i => clk_rx_i,
            clk_serdes_i => clk_serdes_i,
            rx_data_i_p => rx_data_i_p(I),
            rx_data_i_n => rx_data_i_n(I),
            rx_data_o => rx_data(I),
            rx_header_o => rx_header(I),
            rx_valid_o => rx_data_valid(I),
            rx_stat_o => open
        );
            
    end generate lane_loop;


end behavioral;
