-- ####################################
-- # Project: Yarr
-- # Author: Timon Heim
-- # E-Mail: timon.heim at cern.ch
-- # Comments: Trigger logic core
-- # Data: 09/2016
-- # Outputs are synchronous to clk_i
-- ####################################
-- # Adress Map:
-- # 0x0 - Trigger Mask [3:0] ext, [7:4] int, [8] eudet
-- # 0x1 - Trigger tag mode
-- #    0 = trigger counter
-- #    1 = clk_i timestamp
-- #    2 = eudet input

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
        int_trig_i : in std_logic_vector(3 downto 0);
        int_trig_o : out std_logic;
        int_busy_i : in std_logic;
        trig_tag : out std_logic_vector(31 downto 0)
    );
end wb_trigger_logic;

architecture rtl of wb_trigger_logic is
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

    signal C_DEADTIME : integer := 300; -- clk_i cycles
    
    -- Registers
    signal trig_mask : std_logic_vector(31 downto 0);
    signal trig_tag_mode : std_logic_vector(7 downto 0);
    
    -- Local signals
    signal sync_ext_trig_i : std_logic_vector(3 downto 0);
    signal sync_ext_busy_i : std_logic;
    signal master_trig_t : std_logic;
    signal master_trig_d1 : std_logic;
    signal master_trig_d2 : std_logic;
    signal master_trig_sel_edge : std_logic;
    signal master_trig_pos_edge : std_logic;
    signal master_trig_neg_edge : std_logic;
    signal master_busy_t : std_logic;
    signal eudet_trig_t : std_logic;
    signal eudet_trig_tag_t : std_logic_vector(15 downto 0);
    signal trig_counter : unsigned (31 downto 0);
    signal timestamp_cnt : unsigned(31 downto 0);
    signal local_reset : std_logic;
    signal deadtime_cnt : unsigned(15 downto 0);
    signal busy_t : std_logic;

begin
    -- WB interface
    wb_proc: process(wb_clk_i, rst_n_i)
    begin
        if (rst_n_i = '0') then
            wb_dat_o <= (others => '0');
            wb_ack_o <= '0';
            trig_mask <= x"00000010"; -- auto enable internal
            trig_tag_mode <= x"01";
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
                        when x"FF" =>
                            local_reset <= '1'; -- Pulse local reset
                        when others =>
                    end case;
                else
                    case (wb_adr_i(7 downto 0)) is
                        when x"00" =>
                            wb_dat_o <= trig_mask;
                        when others =>
                            wb_dat_o <= x"DEADBEEF";
                    end case;
                end if;
            end if;
        end if;
    end process wb_proc;

    -- Sync inputs
    trig_inputs: for I in 0 to 3 generate
    begin
        cmp_sync_trig: synchronizer port map(clk_i => clk_i, rst_n_i => rst_n_i, async_in => ext_trig_i(I), sync_out => sync_ext_trig_i(I));
    end generate trig_inputs;
    cmp_sync_busy: synchronizer port map(clk_i => clk_i, rst_n_i => rst_n_i, async_in => ext_busy_i, sync_out => sync_ext_busy_i);
    
    master_busy_t <= sync_ext_busy_i or int_busy_i or busy_t;

    -- Apply trigger mask to inputs
    master_trig_t <= (sync_ext_trig_i(0) and trig_mask(0))
                    or (sync_ext_trig_i(1) and trig_mask(1))
                    or (sync_ext_trig_i(2) and trig_mask(2))
                    or (sync_ext_trig_i(3) and trig_mask(3))
                    or (int_trig_i(0) and trig_mask(4))
                    or (int_trig_i(1) and trig_mask(5))
                    or (int_trig_i(2) and trig_mask(6))
                    or (int_trig_i(3) and trig_mask(7))
                    or (eudet_trig_t and trig_mask(8));
                    
    -- find edge
    master_trig_sel_edge <= master_trig_pos_edge; -- TODO hardcoded
    edge_proc: process(clk_i, rst_n_i)
    begin
        if (rst_n_i = '0') then
            master_trig_d1 <= '0';
            master_trig_d2 <= '0';
            master_trig_pos_edge <= '0';
            master_trig_neg_edge <= '0';
        elsif rising_edge(clk_i) then
            master_trig_d1 <= master_trig_t;
            master_trig_d2 <= master_trig_d1;
            master_trig_pos_edge <= '0';
            master_trig_neg_edge <= '0';
            if (master_trig_d2 = '0' and master_trig_d1 = '1') then
                master_trig_pos_edge <= '1';
            end if;
            if (master_trig_d1 = '0' and master_trig_d2 = '1') then
                master_trig_neg_edge <= '1';
            end if;
        end if;
    end process edge_proc; 
    
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
            elsif (master_trig_sel_edge = '1') then
                trig_counter <= trig_counter + 1;
            end if;

            if (local_reset = '1') then
                timestamp_cnt <= (others => '0');
            else
                timestamp_cnt <= timestamp_cnt + 1;
            end if;
            
            if (master_trig_sel_edge = '1' and master_busy_t = '0') then
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
            ext_busy_o <= '0';
            int_trig_o <= '0';
            deadtime_cnt <= (others => '0');
            busy_t <= '0';
        elsif rising_edge(clk_i) then
            if (master_busy_t = '0') then
                ext_trig_o <= master_trig_sel_edge;
                int_trig_o <= master_trig_sel_edge;
                ext_busy_o <= '0';
            else
                ext_busy_o <= '1';
            end if;

            if (master_trig_sel_edge = '1') then
                deadtime_cnt <= TO_UNSIGNED(C_DEADTIME, 16);
            end if;
            if (deadtime_cnt > 0) then
                deadtime_cnt <= deadtime_cnt - 1;
                busy_t <= '1';
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
        trig_o => eudet_trig_t,
        rst_o => open,
        trig_tag_o => eudet_trig_tag_t
    );
end rtl;
