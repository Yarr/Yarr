-- ####################################
-- # Project: Yarr
-- # Author: Timon Heim
-- # E-Mail: timon.heim at cern.ch
-- # Comments: RX lane
-- # Aurora style single rx lane
-- ####################################

library IEEE;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity aurora_rx_lane is 
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
end aurora_rx_lane;

architecture behavioral of aurora_rx_lane is
    component serdes_1_to_468_idelay_ddr
        generic (
            S 			: integer := 8 ;				-- Set the serdes factor to 4, 6 or 8
            D 			: integer := 1 ;				-- Set the number of inputs
            CLKIN_PERIOD		: real := 1.5625 ;				-- clock period (ns) of input clock on clkin_p
            REF_FREQ 		: real := 300.0 ;   				-- Parameter to set reference frequency used by idelay controller
            HIGH_PERFORMANCE_MODE 	: string := "TRUE" ;				-- Parameter to set HIGH_PERFORMANCE_MODE of input delays to reduce jitter
            DATA_FORMAT 		: string := "PER_CLOCK"			-- Used to determine method for mapping input parallel word to output serial words
        );
        port (
            datain_p		:  in std_logic_vector(D-1 downto 0) ;		-- Input from LVDS receiver pin
            datain_n		:  in std_logic_vector(D-1 downto 0) ;		-- Input from LVDS receiver pin
            enable_phase_detector	:  in std_logic ;				-- Enables the phase detector logic when high
            enable_monitor		:  in std_logic ;				-- Enables the monitor logic when high, note time-shared with phase detector function
            reset			:  in std_logic ;				-- Reset line
            bitslip			:  in std_logic ;				-- bitslip 
            idelay_rdy		:  in std_logic ;				-- input delays are ready
            rxclk			:  in std_logic ;				-- Global/BUFIO rx clock network
            system_clk		:  in std_logic ;				-- Global/Regional clock output
            rx_lckd			: out std_logic ;				-- 
            rx_data			: out std_logic_vector((S*D)-1 downto 0) ;  	-- Output data
            bit_rate_value		:  in std_logic_vector(15 downto 0) ;	 	-- Bit rate in Mbps, eg X"0585
            dcd_correct		:  in std_logic ;	 			-- '0' = square, '1' = assume 10% DCD
            bit_time_value		: out std_logic_vector(4 downto 0) ;		-- Calculated bit time value for slave devices
            debug			: out std_logic_vector(10*D+18 downto 0) ;  	-- Debug bus
            eye_info		: out std_logic_vector(32*D-1 downto 0) ;  	-- Eye info
            m_delay_1hot		: out std_logic_vector(32*D-1 downto 0) ;  	-- Master delay control value as a one-hot vector
            clock_sweep		: out std_logic_vector(31 downto 0)  	-- clock Eye info
        );
    end component serdes_1_to_468_idelay_ddr;

    component gearbox32to66
        port (
            -- Sys connect
            rst_i : in std_logic;
            clk_i : in std_logic;
            -- Input
            data32_i : in std_logic_vector(31 downto 0);
            data32_valid_i : in std_logic;
            slip_i : in std_logic;
            -- Outoput
            data66_o : out std_logic_vector(65 downto 0);
            data66_valid_o : out std_logic
        );
    end component gearbox32to66;
     
    constant c_DATA_HEADER : std_logic_vector(1 downto 0) := "01";
    constant c_CMD_HEADER : std_logic_vector(1 downto 0) := "10";
    constant c_SYNC_MAX : unsigned(7 downto 0) := to_unsigned(16, 8);

    -- Serdes
    signal serdes_slip : std_logic;
    signal serdes_idelay_rdy : std_logic;
    signal serdes_data8 : std_logic_vector(7 downto 0);

    -- 8 to 32
    signal serdes_data32_shift : std_logic_vector(31 downto 0);
    signal serdes_data32 : std_logic_vector(31 downto 0);
    signal serdes_data32_valid : std_logic;
    signal serdes_cnt : unsigned(1 downto 0);

    -- Gearbox
    signal gearbox_data66 : std_logic_vector(65 downto 0);
    signal gearbox_data66_valid : std_logic;
    signal gearbox_slip : std_logic;

    -- Block Sync
    signal sync_cnt : unsigned(7 downto 0);
    signal slip_cnt : unsigned(2 downto 0);

begin

    serdes_cmp: serdes_1_to_468_idelay_ddr port map (
        datain_p(0) => rx_data_i_p,
        datain_n(0) => rx_data_i_n,
        enable_phase_detector => '1',
        enable_monitor => '0',
        reset => not rst_n_i,
        bitslip => serdes_slip,
        idelay_rdy => serdes_idelay_rdy,
        rxclk => clk_serdes_i,
        system_clk => clk_rx_i,
        rx_lckd => open,
        rx_data => serdes_data8,
        bit_rate_value => x"1280", -- TODO make generic
        dcd_correct => '0',
        bit_time_value => open,
        debug => open,
        eye_info => open,
        m_delay_1hot => open,
        clock_sweep => open
    );

    serdes_8to32_proc : process(clk_rx_i, rst_n_i)
    begin
        if (rst_n_i = '0') then
            serdes_data32 <= (others => '0');
            serdes_data32_shift <= (others => '0');
            serdes_data32_valid <= '0';
            serdes_cnt <= (others => '0');
        elsif rising_edge(clk_rx_i) then
            serdes_cnt <= serdes_cnt + 1;
            serdes_data32_valid <= '0';
            serdes_data32_shift(31 downto 8) <= serdes_data32_shift(23 downto 0);
            serdes_data32_shift(7 downto 0) <= serdes_data8;
            if (serdes_cnt = "11") then
                serdes_data32 <= serdes_data32_shift;
                serdes_data32_valid <= '1';
            end if;
        end if;
    end process serdes_8to32_proc;

    gearbox32to66_cmp : gearbox32to66 port map (
        rst_i => not rst_n_i,
        clk_i => clk_rx_i,
        data32_i => serdes_data32,
        data32_valid_i => serdes_data32_valid,
        slip_i => gearbox_slip,
        data66_o => gearbox_data66,
        data66_valid_o => gearbox_data66_valid
    );

    block_sync_proc: process(clk_rx_i, rst_n_i)
    begin
        if (rst_n_i = '0') then
            sync_cnt <= (others => '0');
            slip_cnt <= (others => '0');
            rx_valid_o <= '0';
            rx_data_o <= (others => '0');
            rx_header_o <= "00";
        elsif rising_edge(clk_rx_i) then
            serdes_slip <= '0';
            rx_valid_o <= '1';
            if (gearbox_data66_valid = '1') then
                gearbox_slip <= '1'; -- Keep high until next valid so gearbox sees it
                if ((gearbox_data66(65 downto 64) = c_DATA_HEADER) or
                    (gearbox_data66(65 downto 64) = c_CMD_HEADER)) then
                    if (sync_cnt < c_SYNC_MAX) then
                        sync_cnt <= sync_cnt + 1;
                    end if;
                else
                    sync_cnt <= (others => '0');
                    if (slip_cnt = 7) then
                        gearbox_slip <= '1';
                        serdes_slip <= '1';
                        slip_cnt <= (others => '1');
                    else
                        serdes_slip <= '1';
                        slip_cnt <= slip_cnt + 1;
                    end if;
                end if;
                -- Output proc
                if (sync_cnt = c_SYNC_MAX) then
                    rx_data_o <= gearbox_data66(63 downto 0);
                    rx_header_o <= gearbox_data66(65 downto 64);
                    rx_valid_o <= '1';
                end if;
            end if;
        end if;
    end process block_sync_proc;



end behavioral;
