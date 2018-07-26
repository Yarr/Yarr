
-- ####################################
-- # Project: Yarr
-- # Author: Timon Heim
-- # E-Mail: timon.heim at cern.ch
-- # Comments: EUDET TLU interface
-- # Data: 09/2016
-- # Outputs are synchronous to clk_i
-- ####################################

library IEEE;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity eudet_tlu is
    port (
        -- Sys connect
        clk_i : IN std_logic;
        rst_n_i : IN std_logic;
        
        -- Eudet signals
        eudet_trig_i : IN std_logic;
        eudet_rst_i : IN std_logic;
        eudet_busy_o : OUT std_logic;
        eudet_clk_o : OUT std_logic;

        -- From logic
        busy_i : IN std_logic;
        simple_mode_i : IN std_logic;
        -- To logic
        trig_o : OUT std_logic;
        rst_o : OUT std_logic;
        trig_tag_o : OUT std_logic_vector(15 downto 0)
    );
end eudet_tlu;

architecture rtl of eudet_tlu is
    -- Components
    component synchronizer
        port (
            -- Sys connect
            clk_i : in std_logic;
            rst_n_i : in std_logic;

            -- Async input
            async_in : in std_logic;
            sync_out : out std_logic
        );
    end component;

    -- constants
    signal C_DEADTIME : integer := 300; -- clk_i cycles
    signal C_CLKDIVIDER : integer := 4; -- 40 MHz -> 10Mhz

    -- State machine
    type state_type is (IDLE, TRIGGER, RECEIVE, DEAD);
    signal state : state_type;

    -- Sync inputs
    signal sync_eudet_trig_t : std_logic;
    signal sync_eudet_rst_i : std_logic;

    signal trig_tag_t : std_logic_vector(15 downto 0); -- only 15:1 good
    signal eudet_busy_t : std_logic;
    signal eudet_clk_t : std_logic;
    signal eudet_bust_t : std_logic;
    signal clk_counter : unsigned (3 downto 0);
    signal bit_counter : unsigned (4 downto 0);
    signal dead_counter : unsigned (9 downto 0);
begin
    -- Sync async inputs
    trig_sync: synchronizer port map(clk_i => clk_i, rst_n_i => rst_n_i, async_in => eudet_trig_i, sync_out => sync_eudet_trig_t);
    rst_sync: synchronizer port map(clk_i => clk_i, rst_n_i => rst_n_i, async_in => eudet_rst_i, sync_out => sync_eudet_rst_i);

    eudet_busy_o <= eudet_busy_t;
    eudet_clk_o <= eudet_clk_t;
    rst_o <= '0';

    state_machine: process(clk_i, rst_n_i)
    begin
        if (rst_n_i = '0') then
            state <= IDLE;
            eudet_busy_t <= '0';
            eudet_clk_t <= '0';
            clk_counter <= (others => '0');
            bit_counter <= (others => '0');
            dead_counter <= (others => '0');
            trig_tag_t <= (others => '0');
            trig_tag_o <= (others => '0');
            trig_o <= '0';
        elsif rising_edge(clk_i) then
            case state is
                when IDLE =>
                    eudet_busy_t <= '0';
                    eudet_clk_t <= '0';
                    clk_counter <= (others => '0');
                    bit_counter <= (others => '0');
                    trig_o <= '0';
                    if (sync_eudet_trig_t = '1') then
                        state <= TRIGGER;
                    end if;

                when TRIGGER =>
                    -- Raise busy and wit until trigger is negated
                    eudet_busy_t <= '1';
                    eudet_clk_t <= '0';
                    trig_o <= '0';
                    clk_counter <= (others => '0');
                    bit_counter <= (others => '0');
                    trig_tag_t <= (others => '0');
                    dead_counter <= (others => '0');
                    if (sync_eudet_trig_t = '0' and simple_mode_i = '0') then
                        state <= RECEIVE;
                    elsif (sync_eudet_trig_t = '0' and simple_mode_i = '1') then
                        state <= DEAD;
                    end if;

                when RECEIVE =>
                    eudet_busy_t <= '1';
                    trig_o <= '0';
                    clk_counter <= clk_counter + 1;
                    dead_counter <= (others => '0');
                    if (clk_counter = (C_CLKDIVIDER-1)) then
                        clk_counter <= (others => '0');
                        eudet_clk_t <= not eudet_clk_t;
                        if (eudet_clk_t = '1') then --sampling on negative edge
                            bit_counter <= bit_counter + 1;
                            trig_tag_t <= eudet_trig_i & trig_tag_t(15 downto 1); -- do not need synced vers here
                        end if;
                    end if;
                    if (bit_counter = "10000") then
                        state <= DEAD;
                        trig_tag_o <= '0' & trig_tag_t(14 downto 0);
                    end if;
                
                when DEAD =>
                    eudet_busy_t <= '1';
                    eudet_clk_t <= '0';
                    trig_o <= '0';
                    if (dead_counter = 0) then
                        trig_o <= '1'; -- Trigger now (16 clock cycles after the inital trigger?)
                    end if;
                    dead_counter <= dead_counter + 1;
                    if (dead_counter = C_DEADTIME) then
                        state <= IDLE;
                    end if;

                when others =>
                    eudet_busy_t <= '0';
                    eudet_clk_t <= '0';
                    trig_o <= '0';
                    clk_counter <= (others => '0');
                    bit_counter <= (others => '0');
                    state <= IDLE;
            end case;
        end if;
    end process state_machine;

end rtl;
