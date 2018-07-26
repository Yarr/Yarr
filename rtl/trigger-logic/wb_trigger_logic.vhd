-- ####################################
-- # Project: Yarr
-- # Author: Timon Heim
-- # E-Mail: timon.heim at cern.ch
-- # Comments: Trigger logic core
-- # Data: 09/2016
-- # Outputs are synchronous to clk_i
-- ####################################
-- # Adress Map:
-- #
-- # 0x0      - Trigger mask [3:0] ext, [4] eudet
-- #              0 = off
-- #              1 = on
-- # 0x1      - Trigger tag mode
-- #              0 = trigger counter
-- #              1 = clk_i timestamp
-- #              2 = eudet input
-- # 0x2      - Concidence/veto logic (entire config word used
-- #            as selector of multiplexor)
-- # 0x3      - Trigger edge [3:0] ext, [:4] ignored
-- #              0 = rising
-- #              1 = falling
-- # 0x4..0x7 - Per-channel delay (clk_i cycles, max 8)
-- #              0x4 = ext[0] ... 0x7 = ext[3]
-- # 0x8      - deadtime (clk_i cycles)
-- # 0xFF     - local reset (reset trigger tag values)
-- #
-- # See ./README.md for more detailed instructions

library IEEE;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;


entity wb_trigger_logic is
    port (
		-- Sys connect
		wb_clk_i	: in  std_logic;
		rst_n_i		: in  std_logic;
		
		-- Wishbone slave interface
		wb_adr_i	: in  std_logic_vector(31 downto 0);
		wb_dat_i	: in  std_logic_vector(31 downto 0);
		wb_dat_o	: out std_logic_vector(31 downto 0);
		wb_cyc_i	: in  std_logic;
		wb_stb_i	: in  std_logic;
		wb_we_i		: in  std_logic;
		wb_ack_o	: out std_logic;
        
        -- To/From outside world
        ext_trig_i : in std_logic_vector(3 downto 0);
        ext_trig_o : out std_logic;
        ext_busy_i : in std_logic;
        ext_busy_o : out std_logic;
	    
        -- Eudet TLU
        eudet_clk_o : out std_logic;
        eudet_busy_o : out std_logic;
        eudet_trig_i : in std_logic;
        eudet_rst_i : in std_logic;

        -- To/From inside world
        clk_i : in std_logic;
        trig_tag : out std_logic_vector(31 downto 0);

        debug_o : out std_logic_vector(31 downto 0)
    );
end wb_trigger_logic;

architecture rtl of wb_trigger_logic is
    -- Components
    component edge_detector
        port (
            clk_i : in std_logic;
            rst_n_i : in std_logic;

            dat_i : in std_logic;
            rising_o : out std_logic;
            falling_o : out std_logic
        );
    end component;
    
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
    
    component delayer
        generic (N : integer);
        port (
            clk_i : in std_logic;
            rst_n_i : in std_logic;
            dat_i : in std_logic;
            dat_o : out std_logic;
            delay : in std_logic_vector
        );
    end component;
	
    component eudet_tlu
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
    end component;

    constant delay_width : integer := 3;
    
    -- Registers
    signal trig_mask : std_logic_vector(31 downto 0);
    signal trig_tag_mode : std_logic_vector(7 downto 0);
    signal trig_logic : std_logic_vector(31 downto 0);
    signal trig_edge : std_logic_vector(3 downto 0);
    signal ch0_delay : std_logic_vector(delay_width-1 downto 0);
    signal ch1_delay : std_logic_vector(delay_width-1 downto 0);
    signal ch2_delay : std_logic_vector(delay_width-1 downto 0);
    signal ch3_delay : std_logic_vector(delay_width-1 downto 0);
    signal deadtime : std_logic_vector(15 downto 0); -- clk_i cycles
    
    -- Local signals
    signal edge_r : std_logic_vector(3 downto 0);
    signal edge_f : std_logic_vector(3 downto 0);
    signal sync_ext_trig_i : std_logic_vector(3 downto 0);
    signal edge_ext_trig_i : std_logic_vector(3 downto 0);
    signal del_ext_trig_i : std_logic_vector(3 downto 0);
    signal sync_ext_busy_i : std_logic;
    signal master_trig_t : std_logic;
    signal prev_master_trig_t : std_logic; -- delay output one clk to sync w/ busy signal
    signal master_busy_t : std_logic;
    signal lcl_eudet_trig_t : std_logic;
    signal eudet_trig_tag_t : std_logic_vector(15 downto 0);
    signal trig_counter : unsigned (31 downto 0);
    signal timestamp_cnt : unsigned(31 downto 0);
    signal local_reset : std_logic;
    signal deadtime_cnt : unsigned(15 downto 0);
    signal busy_t : std_logic;

begin

    -- Debug port
    debug_o(3 downto 0) <= ext_trig_i;
    debug_o(7 downto 4) <= sync_ext_trig_i;
    debug_o(11 downto 8) <= edge_ext_trig_i;
    debug_o(15 downto 12) <= del_ext_trig_i;
    debug_o(16) <= master_trig_t;
    debug_o(17) <= master_busy_t;
    debug_o(22 downto 18) <= trig_mask(4 downto 0);
    debug_o(31 downto 23) <= trig_logic(8 downto 0);
  
    -- WB interface
    wb_proc: process(wb_clk_i, rst_n_i)
    begin
        if (rst_n_i = '0') then
            wb_dat_o <= (others => '0');
            wb_ack_o <= '0';
            trig_mask <= x"00000001"; -- auto enable internal
            trig_tag_mode <= x"01";
            trig_logic <= (1 => '1', others => '0'); -- auto enable internal
            trig_edge <= (others => '0');
            ch0_delay <= (others => '0');
            ch1_delay <= (others => '0');
            ch2_delay <= (others => '0');
            ch3_delay <= (others => '0');
            deadtime <= std_logic_vector(to_unsigned(300, 16));
        elsif rising_edge(wb_clk_i) then
            wb_ack_o <= '0';
            wb_dat_o <= (others => '0');
            local_reset <= '0';
            if (wb_cyc_i = '1' and wb_stb_i = '1') then
                wb_ack_o <= '1';
                if (wb_we_i = '1') then
                    case (wb_adr_i(7 downto 0)) is
                        when x"00" =>
                            trig_mask <= wb_dat_i;
                        when x"01" =>
                            trig_tag_mode <= wb_dat_i(7 downto 0);
                        when x"02" =>
                            trig_logic <= wb_dat_i;
                        when x"03" =>
                            trig_edge <= wb_dat_i(3 downto 0);
                        when x"04" =>
                            ch0_delay <= wb_dat_i(delay_width-1 downto 0);
                        when x"05" =>
                            ch1_delay <= wb_dat_i(delay_width-1 downto 0);
                        when x"06" =>
                            ch2_delay <= wb_dat_i(delay_width-1 downto 0);
                        when x"07" =>
                            ch3_delay <= wb_dat_i(delay_width-1 downto 0);
                        when x"08" =>
                            deadtime <= wb_dat_i(15 downto 0);
                        when x"FF" =>
                            local_reset <= '1'; -- Pulse local reset
                        when others =>
                    end case;
                else
                    case (wb_adr_i(7 downto 0)) is
                        when x"00" =>
                            wb_dat_o <= trig_mask;
                        when x"01" =>
                            wb_dat_o <= std_logic_vector(resize(unsigned(trig_tag_mode), 32));
                        when x"02" =>
                            wb_dat_o <= trig_logic;
                        when x"03" =>
                            wb_dat_o <= std_logic_vector(resize(unsigned(trig_edge), 32));
                        when x"04" =>
                            wb_dat_o <= std_logic_vector(resize(unsigned(ch0_delay), 32));
                        when x"05" =>
                            wb_dat_o <= std_logic_vector(resize(unsigned(ch1_delay), 32));
                        when x"06" =>
                            wb_dat_o <= std_logic_vector(resize(unsigned(ch2_delay), 32));
                        when x"07" =>
                            wb_dat_o <= std_logic_vector(resize(unsigned(ch3_delay), 32));
                        when x"08" =>
                            wb_dat_o <= std_logic_vector(resize(unsigned(deadtime), 32));
                        when others =>
                            wb_dat_o <= x"DEADBEEF";
                    end case;
                end if;
            end if;
        end if;
    end process wb_proc;

    -- Sync/edge detector inputs
    trig_inputs: for I in 0 to 3 generate
    begin
        cmp_sync_trig: synchronizer
            port map(clk_i => clk_i, rst_n_i => rst_n_i, async_in => ext_trig_i(I), sync_out => sync_ext_trig_i(I));
        cmp_edge_trig: edge_detector
            port map(clk_i => clk_i, rst_n_i => rst_n_i, dat_i => sync_ext_trig_i(I),
                     falling_o => edge_f(I), rising_o => edge_r(I) );
        edge_ext_trig_i(I) <= edge_f(I) when trig_edge(I) = '1' else edge_r(I);
    end generate trig_inputs;
    
    cmp_delay_trig0: delayer
        generic map(N => delay_width)
        port map(clk_i => clk_i, rst_n_i => rst_n_i, dat_i => edge_ext_trig_i(0),
                 dat_o => del_ext_trig_i(0), delay => ch0_delay);
    cmp_delay_trig1: delayer
        generic map(N => delay_width)
        port map(clk_i => clk_i, rst_n_i => rst_n_i, dat_i => edge_ext_trig_i(1),
                 dat_o => del_ext_trig_i(1), delay => ch1_delay);
    cmp_delay_trig2: delayer
        generic map(N => delay_width)
        port map(clk_i => clk_i, rst_n_i => rst_n_i, dat_i => edge_ext_trig_i(2),
                 dat_o => del_ext_trig_i(2), delay => ch2_delay);
    cmp_delay_trig3: delayer
        generic map(N => delay_width)
        port map(clk_i => clk_i, rst_n_i => rst_n_i, dat_i => edge_ext_trig_i(3),
                 dat_o => del_ext_trig_i(3), delay => ch3_delay);
                 
    cmp_sync_busy: synchronizer port map(clk_i => clk_i, rst_n_i => rst_n_i, async_in => ext_busy_i, sync_out => sync_ext_busy_i);
    
    master_busy_t <= sync_ext_busy_i or busy_t;
    ext_busy_o <= master_busy_t;
	
    -- Apply coincidence/veto logic
    master_trig_t <= trig_logic(to_integer(unsigned((lcl_eudet_trig_t & del_ext_trig_i) and trig_mask(4 downto 0))));
    
    -- trig tag gen
    trig_tag_proc: process(clk_i, rst_n_i)
    begin
        if (rst_n_i = '0') then
            trig_tag <= (others => '0');
            trig_counter <= (others => '0');
            timestamp_cnt <= (others => '0');
        elsif rising_edge(clk_i) then
            -- TODO need reset
            if (local_reset = '1') then
                trig_counter <= (others => '0');    
            elsif (master_trig_t = '1') then
                trig_counter <= trig_counter + 1;
            end if;

            if (local_reset = '1') then
                timestamp_cnt <= (others => '0');
            else
                timestamp_cnt <= timestamp_cnt + 1;
            end if;
            
            if (master_trig_t = '1' and master_busy_t = '0') then
                case (trig_tag_mode) is
                    when x"00" =>
                        trig_tag <= std_logic_vector(trig_counter);
                    when x"01" =>
                        trig_tag <= std_logic_vector(timestamp_cnt);
                    when x"02" =>
                        trig_tag <= x"0000" & eudet_trig_tag_t;
                    when others =>
                        trig_tag <= x"DEADBEEF";
                end case;
            end if;
        end if;
    end process trig_tag_proc;
    
    -- Output proc
    out_proc: process(clk_i, rst_n_i)
    begin
        if (rst_n_i = '0') then
            ext_trig_o <= '0';
            deadtime_cnt <= (others => '0');
            busy_t <= '0';
            prev_master_trig_t <= '0';
        elsif rising_edge(clk_i) then
            if (master_busy_t = '0') then
                ext_trig_o <= prev_master_trig_t;
                prev_master_trig_t <= master_trig_t;
            end if;
            
            if (prev_master_trig_t = '1') then
                ext_trig_o <= '1' and not master_busy_t;
            else
                ext_trig_o <= '0';
            end if;
            
            if (deadtime_cnt > 0) then
                -- This happens on the clk cycle after master_trig_t pulses (ie, when
                -- ext_trig_o pulses), immediately setting ext_busy_o to '1'
                deadtime_cnt <= deadtime_cnt - 1;
                busy_t <= '1';
            elsif (master_trig_t = '1') then
                -- This happens on the clk cycle before ext_trig_o pulses
                deadtime_cnt <= UNSIGNED(deadtime);
            else
                busy_t <= '0';
            end if;

        end if;
    end process out_proc;

    cmp_eudet_tlu: eudet_tlu
    port map (
        clk_i => clk_i,
        rst_n_i => rst_n_i,
        eudet_trig_i => eudet_trig_i,
        eudet_rst_i => eudet_rst_i,
        eudet_busy_o => eudet_busy_o,
        eudet_clk_o => eudet_clk_o,
        busy_i => '0',
        simple_mode_i => '0',
        trig_o => lcl_eudet_trig_t,
        rst_o => open,
        trig_tag_o => eudet_trig_tag_t
    );
		    
 end rtl;
