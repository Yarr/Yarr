--------------------------------------------
-- Project: FE65-P2 addon
-- Author: Timon Heim (timon.heim@cern.ch)
-- Description: Attaches to serial port and controls FE65-P2 adapter 
-- Dependencies: -
--------------------------------------------

library IEEE;
use IEEE.STD_LOGIC_1164.all;
use IEEE.NUMERIC_STD.all;

library UNISIM;
use UNISIM.vcomponents.all;

entity fe65p2_addon is
    port (
        clk_i   : IN std_logic;
        rst_n   : IN std_logic;
        serial_in : IN std_logic;
        
        clk_rx_i : IN std_logic;

        -- TO FMC
        clk_bx_o : out std_logic;
        trig_o : out std_logic;
        clk_cnfg_o : out std_logic;
        en_pix_sr_cnfg_o : out std_logic;
        ld_cnfg_o : out std_logic;
        si_cnfg_o : out std_logic;
        pix_d_cnfg_o : out std_logic;
        clk_data_o : out std_logic;
        rst_0_o : out std_logic;
        rst_1_o : out std_logic;

        dac_sclk_o : out std_logic;
        dac_sdi_o : out std_logic;
        dac_ld_o : out std_logic;
        dac_cs_o : out std_logic;
        inj_sw_o : out std_logic
    );
end fe65p2_addon;

architecture behavioral of fe65p2_addon is
    -- System signals
    signal sys_rst : std_logic;
    signal clk_40 : std_logic;

    -- clocks
    signal clk_bx_t : std_logic;
    signal clk_cnfg_t : std_logic;
    signal clk_data_t : std_logic;
    signal en_bx_clk : std_logic;
    signal en_conf_clk : std_logic;
    signal en_data_clk : std_logic;

    -- cmd deserialiser
	signal yarr_cmd : std_logic;
    signal cmd_count : unsigned(7 downto 0);
	signal cmd_valid : std_logic;
	signal cmd_sreg : std_logic_vector(31 downto 0);

    -- cmd decoder
    signal new_cmd : std_logic;
    signal adr : std_logic_vector(15 downto 0);
    signal payload : std_logic_vector(15 downto 0);

    -- registers
	signal conf_reg : std_logic_vector(159 downto 0);
	signal pix_reg : std_logic_vector(255 downto 0);
	signal static_reg : std_logic_vector(15 downto 0);
	signal pulser_reg : std_logic_vector(15 downto 0);
	signal latency : unsigned(8 downto 0);
	signal dac_setting : std_logic_vector(15 downto 0);
	signal trig_multiplier : unsigned(3 downto 0);
	signal delay_setting : std_logic_vector(7 downto 0);

    -- config serialiser
    signal conf_load : std_logic_vector(7 downto 0);
    signal conf_sreg_cnt : unsigned(7 downto 0);
    signal conf_sreg : std_logic_vector(144 downto 0);
    signal pix_sreg_cnt : unsigned(8 downto 0);
    signal pix_sreg : std_logic_vector(255 downto 0);
    signal en_pix_reg : std_logic;

    -- inject & trigger
    signal dig_inj : std_logic;
    signal trigger : std_logic;
    signal pulser_trig_t : std_logic_vector(40 downto 0);
    signal inject_cnt : unsigned(8 downto 0);
	 signal trig_cnt : unsigned(4 downto 0);
    signal en_inj : std_logic;
	 signal inj_sw_t : std_logic;

    -- DAC
    signal dac_load : std_logic_vector(15 downto 0);
    signal dac_cs_t : std_logic;
    signal dac_sreg_cnt : unsigned(7 downto 0);
    signal dac_sreg : std_logic_vector(15 downto 0);
    signal dac_sclk_t : std_logic;
	 
	component delay_line
    generic (
        width : positive := 8);
    port (
        clk : in std_logic;
        rst : in std_logic;
        input : IN std_logic;
        output : OUT std_logic;
		  setting : IN std_logic_vector(width-1 downto 0) );
	 end component delay_line;
	 
begin
    sys_rst <= not rst_n;
    clk_40 <= clk_i;
    
    clk_bx_t <= clk_40;
    clk_cnfg_t <= clk_40;
    clk_data_t <= clk_rx_i;

    -- Outputs
    trig_o <= trigger;
    en_pix_sr_cnfg_o <= en_pix_reg;
    ld_cnfg_o <= conf_load(4) or conf_load(5) or conf_load(6) or conf_load(7) or dig_inj;
    si_cnfg_o <= conf_sreg(144) or pix_sreg(255);


    dac_sclk_o <= dac_sclk_t;
    dac_sdi_o <= dac_sreg(15);
    dac_ld_o <= not (dac_load(15)  or dac_load(14) or dac_load(13) or dac_load(12) or dac_load(11) or 
					dac_load(10) or dac_load(9) or dac_load(8) or dac_load(7) or dac_load(6) or dac_load(5));
    dac_cs_o <= dac_cs_t;
    inj_sw_t <= '0' when (unsigned(pulser_trig_t) = 0) else '1';

    -- Fine delay for pulser
	cmp_inj_delay: delay_line 
    GENERIC MAP(
        width => 7)
    PORT MAP(
        clk => clk_40,
        rst => sys_rst,
        input => inj_sw_t,
        output => inj_sw_o,
        setting => delay_setting(6 downto 0)
	);
    
    -- Static settings
    en_data_clk <= static_reg(0);
    en_bx_clk <= static_reg(1);
	pix_d_cnfg_o <= static_reg(2);
    en_inj <= static_reg(3);
    rst_0_o <= not static_reg(4);
    rst_1_o <= not static_reg(5);
	
    yarr_cmd <= serial_in;
    cmd_deserialiser: process(clk_40, sys_rst)
	begin
		if (sys_rst = '1') then
         cmd_sreg <= (others => '0');
			cmd_count <= (others => '0');
			cmd_valid <= '0';
		elsif rising_edge(clk_40) then
			cmd_sreg <= cmd_sreg(30 downto 0) & yarr_cmd;
			if (cmd_count = TO_UNSIGNED(31,8)) then
				cmd_count <= (others => '0');
				cmd_valid <= '1';
			elsif (cmd_count > 0) then
				cmd_count <= cmd_count + 1;
				cmd_valid <= '0';
			elsif (yarr_cmd = '1' and cmd_count = TO_UNSIGNED(0,8)) then -- start bit
				cmd_count <= cmd_count + 1;
				cmd_valid <= '0';
			else
				cmd_valid <= '0';
			end if;			
		end if;
	end process cmd_deserialiser;
    

    cmd_decoder: process(clk_40, sys_rst)
    begin
        if (sys_rst = '1') then
            new_cmd <= '0';
            adr <= (others => '0');
            payload <= (others => '0');

            conf_reg <= (others => '0');
            pix_reg <= (others => '0');
            static_reg <= (others => '0');
            pulser_reg <= (others => '0');
            latency <= (others => '0');
            dac_setting <= (others => '0');
            trig_multiplier <= x"5";
			delay_setting <= (others => '0');
        elsif rising_edge(clk_40) then
            new_cmd <= '0';
            if (cmd_valid = '1') then
                adr <= '0' & cmd_sreg(30 downto 16);
                payload <= cmd_sreg(15 downto 0);
                new_cmd <= '1';
            end if;
    
            -- pulses 1 clk cycle
            pulser_reg <= (others => '0');
            -- [0] : start shift conf reg
            -- [1] : inject & trigger
            -- [2] : start shift pixel reg
            -- [3] : pulse load line
            -- [4] : shift SR by one
            -- [5] : load DAC
            -- [6] : switch pulser
				-- [7] : trigger (no inject)
				
            if (new_cmd = '1') then
				case (adr) is
					-- Global Shift reg (145 bit)
					when x"0000" => conf_reg(0) <= payload(0);
					when x"0001" => conf_reg(1) <= payload(0);
					when x"0002" => conf_reg(2) <= payload(0);
					when x"0003" => conf_reg(6 downto 3) <= payload(3 downto 0);
					when x"0004" => conf_reg(8 downto 7) <= payload(1 downto 0);
					when x"0005" => conf_reg(9) <= payload(0);
					when x"0006" => conf_reg(10) <= payload(0);
					when x"0007" => conf_reg(11) <= payload(0);
					when x"0008" => conf_reg(20 downto 12) <= payload(8 downto 0);
					when x"0009" => conf_reg(36 downto 21) <= payload(15 downto 0);
					when x"000a" => conf_reg(52 downto 37) <= payload(15 downto 0);
					when x"000b" => conf_reg(56 downto 53) <= payload(3 downto 0);
					when x"000c" => conf_reg(64 downto 57) <= payload(7 downto 0);
					when x"000d" => conf_reg(72 downto 65) <= payload(7 downto 0);
					when x"000e" => conf_reg(80 downto 73) <= payload(7 downto 0);
					when x"000f" => conf_reg(88 downto 81) <= payload(7 downto 0);
					when x"0010" => conf_reg(96 downto 89) <= payload(7 downto 0);
					when x"0011" => conf_reg(104 downto 97) <= payload(7 downto 0);
					when x"0012" => conf_reg(112 downto 105) <= payload(7 downto 0);
					when x"0013" => conf_reg(120 downto 113) <= payload(7 downto 0);
					when x"0014" => conf_reg(128 downto 121) <= payload(7 downto 0);
					when x"0015" => conf_reg(136 downto 129) <= payload(7 downto 0);
					when x"0016" => conf_reg(144 downto 137) <= payload(7 downto 0);
					-- Pixel Shift reg (256 bit)
					when x"0020" => pix_reg(15 downto 0) <= payload(15 downto 0);
					when x"0021" => pix_reg(31 downto 16) <= payload(15 downto 0);
					when x"0022" => pix_reg(47 downto 32) <= payload(15 downto 0);
					when x"0023" => pix_reg(63 downto 48) <= payload(15 downto 0);
					when x"0024" => pix_reg(79 downto 64) <= payload(15 downto 0);
					when x"0025" => pix_reg(95 downto 80) <= payload(15 downto 0);
					when x"0026" => pix_reg(111 downto 96) <= payload(15 downto 0);
					when x"0027" => pix_reg(127 downto 112) <= payload(15 downto 0);
					when x"0028" => pix_reg(143 downto 128) <= payload(15 downto 0);
					when x"0029" => pix_reg(159 downto 144) <= payload(15 downto 0);
					when x"002a" => pix_reg(175 downto 160) <= payload(15 downto 0);
					when x"002b" => pix_reg(191 downto 176) <= payload(15 downto 0);
					when x"002c" => pix_reg(207 downto 192) <= payload(15 downto 0);
					when x"002d" => pix_reg(223 downto 208) <= payload(15 downto 0);
					when x"002e" => pix_reg(239 downto 224) <= payload(15 downto 0);
					when x"002f" => pix_reg(255 downto 240) <= payload(15 downto 0);
					-- Modes
					when x"0030" => static_reg <= payload;
					when x"0031" => pulser_reg <= payload;
					when x"0032" => latency <= unsigned(payload(8 downto 0));
					when x"0033" => dac_setting <= payload(15 downto 0);
					when x"0034" => trig_multiplier <= unsigned(payload(3 downto 0));
					when x"0035" => delay_setting <= payload(7 downto 0);
					when others => 
				end case;

            end if;
        end if;
    end process cmd_decoder;

    conf_serialiser: process(clk_40, sys_rst)
    begin
        if (sys_rst = '1') then
            conf_sreg_cnt <= (others => '0');
            conf_sreg <= (others => '0');
            en_conf_clk <= '0';
            conf_load <= (others => '0');
            pix_sreg_cnt <= (others => '0');
            pix_sreg <= (others => '0');
            en_pix_reg <= '0';
        elsif rising_edge(clk_40) then
			-- Configuration serialiser
			conf_load(0) <= '0';
			if (pulser_reg(0) = '1') then
				conf_sreg_cnt <= TO_UNSIGNED(145, 8);
				conf_sreg <= conf_reg(144 downto 0);
				en_conf_clk <= '1';
			elsif (conf_sreg_cnt = TO_UNSIGNED(1, 8)) then
				conf_sreg <= conf_sreg(143 downto 0) & '0';
				conf_load(0) <= '1';
				en_conf_clk <= '0';
				conf_sreg_cnt <= conf_sreg_cnt - 1;
			elsif (conf_sreg_cnt > 0) then
				conf_sreg <= conf_sreg(143 downto 0) & '0';
				conf_sreg_cnt <= conf_sreg_cnt - 1;
				en_conf_clk <= '1';
			end if;
			
            -- Pulse load line
			if (pulser_reg(3) = '1') then
				conf_load(0) <= '1';
			end if;
			
            -- Pixel sreg serialiser
			en_pix_reg <= '0';
			if (pulser_reg(2) = '1') then
				pix_sreg_cnt <= TO_UNSIGNED(256, 9);
				pix_sreg <= pix_reg(255 downto 0);
				en_conf_clk <= '1';
				en_pix_reg <= '1';
			elsif (pix_sreg_cnt = TO_UNSIGNED(1, 9)) then
				pix_sreg <= pix_sreg(254 downto 0) & '0';
				--conf_load <= '1';
				en_pix_reg <= '0';
				en_conf_clk <= '0';
				pix_sreg_cnt <= pix_sreg_cnt - 1;
			elsif (pix_sreg_cnt > 0) then
				pix_sreg <= pix_sreg(254 downto 0) & '0';
				pix_sreg_cnt <= pix_sreg_cnt - 1;
				en_conf_clk <= '1';
				en_pix_reg <= '1';
			end if;
            
            conf_load(1) <= conf_load(0);
			conf_load(2) <= conf_load(1);
			conf_load(3) <= conf_load(2);
			conf_load(4) <= conf_load(3);
			conf_load(5) <= conf_load(4);
			conf_load(6) <= conf_load(5);
			conf_load(7) <= conf_load(6);
        end if;
    end process conf_serialiser;


    inject_proc: process(clk_40, sys_rst)
    begin
        if (sys_rst = '1') then
            dig_inj <= '0';
            trigger <= '0';
            pulser_trig_t(0) <= '0';
            inject_cnt <= (others => '0');
				trig_cnt <= (others => '0');
        elsif rising_edge(clk_40) then
			dig_inj <= '0';
			trigger <= '0';
			pulser_trig_t(0) <= '0';
			if (pulser_reg(1) = '1') then
				inject_cnt <= TO_UNSIGNED((TO_INTEGER(latency) + 2), 9); -- Latency vonfig
				if (en_inj = '0') then
					dig_inj <= '1';
				else
					pulser_trig_t(0) <= '1';
				end if;
			elsif (inject_cnt > ((TO_INTEGER(latency) - 6))) then -- TODO change to pulse length
				if (en_inj = '0') then
					dig_inj <= '1';
				end if;
				inject_cnt <= inject_cnt - 1;
			elsif ((inject_cnt <= (TO_INTEGER(trig_multiplier))) and inject_cnt > 1) then -- TODO change to trigger multiplier
				inject_cnt <= inject_cnt - 1;
				dig_inj <= '0';
				trigger <= '1';
			elsif (inject_cnt = 1) then
				inject_cnt <= inject_cnt - 1;
				dig_inj <= '0';
				trigger <= '1';
			elsif (inject_cnt > 0) then
				inject_cnt <= inject_cnt - 1;
				dig_inj <= '0';
			end if;
			
		   if (pulser_reg(7) = '1') then
	         trig_cnt <= TO_UNSIGNED((TO_INTEGER(trig_multiplier) + 1), 5);
         elsif (trig_cnt > 0) then
            trig_cnt <= trig_cnt - 1;
				trigger <= '1';
		   end if;
      end if;
 
    end process inject_proc;

	pulse_delay: for I in 1 to pulser_trig_t'length-1 generate
	begin
		delay_proc: process(clk_40)
		begin
		   if (sys_rst = '1') then
				pulser_trig_t(I) <= '0';
			elsif rising_edge(clk_40) then
				pulser_trig_t(I) <= pulser_trig_t(I-1);
			end if;
		end process delay_proc;
	end generate;

    dac_proc: process(clk_40, sys_rst)
    begin
        if (sys_rst = '1') then
            dac_load <= (others => '0');
            dac_cs_t <= '1';
            dac_sreg_cnt <= (others => '0');
            dac_sreg <= (others => '0');
            dac_sclk_t <= '0';
        elsif rising_edge(clk_40) then
			dac_load(0) <= '0';
			dac_cs_t <= '1';
			if (pulser_reg(5) = '1') then
				dac_sreg_cnt <= TO_UNSIGNED(160, 8);
				dac_sreg <= dac_setting(15 downto 0);
				dac_sclk_t <= '0';
				dac_cs_t <= '0';	
			elsif (dac_sreg_cnt = TO_UNSIGNED(1, 8)) then
				dac_sreg <= dac_sreg(14 downto 0) & '0';
				dac_load(0) <= '1';
				dac_sreg_cnt <= dac_sreg_cnt - 1;
				dac_sclk_t <= '0';
				dac_cs_t <= '0';
			elsif (dac_sreg_cnt > 0) then
				if ((dac_sreg_cnt mod 10) = 1) then
					dac_sreg <= dac_sreg(14 downto 0) & '0';
				end if;
				if ((dac_sreg_cnt mod 5) = 1) then
					dac_sclk_t <= not dac_sclk_t;
				end if;
				dac_sreg_cnt <= dac_sreg_cnt - 1;
				dac_cs_t <= '0';
			end if;			
			
			dac_load(1) <= dac_load(0);
			dac_load(2) <= dac_load(1);
			dac_load(3) <= dac_load(2);
			dac_load(4) <= dac_load(3);
			dac_load(5) <= dac_load(4);
			dac_load(6) <= dac_load(5);
			dac_load(7) <= dac_load(6);
			dac_load(8) <= dac_load(7);
			dac_load(9) <= dac_load(8);
			dac_load(10) <= dac_load(9);
			dac_load(11) <= dac_load(10);
			dac_load(12) <= dac_load(11);
			dac_load(13) <= dac_load(12);
			dac_load(14) <= dac_load(13);
			dac_load(15) <= dac_load(14);

        end if;
    end process dac_proc;
    
    -- clock ddr2 buffers
	conf_clk_buf : ODDR2
    generic map(
        DDR_ALIGNMENT => "NONE", -- Sets output alignment to "NONE", "C0", "C1" 
        INIT => '0', -- Sets initial state of the Q output to '0' or '1'
        SRTYPE => "ASYNC") -- Specifies "SYNC" or "ASYNC" set/reset
    port map (
        Q => clk_cnfg_o, -- 1-bit output data
        C0 => clk_cnfg_t, -- 1-bit clock input
        C1 => not clk_cnfg_t, -- 1-bit clock input
        CE => en_conf_clk,  -- 1-bit clock enable input
        D0 => '0',   -- 1-bit data input (associated with C0)
        D1 => '1',   -- 1-bit data input (associated with C1)
        R => sys_rst,    -- 1-bit reset input
        S => open     -- 1-bit set input
    );

	bx_clk_buf : ODDR2
   generic map(
      DDR_ALIGNMENT => "NONE", -- Sets output alignment to "NONE", "C0", "C1" 
      INIT => '0', -- Sets initial state of the Q output to '0' or '1'
      SRTYPE => "ASYNC") -- Specifies "SYNC" or "ASYNC" set/reset
   port map (
      Q => clk_bx_o, -- 1-bit output data
      C0 => clk_bx_t, -- 1-bit clock input
      C1 => not clk_bx_t, -- 1-bit clock input
      CE => en_bx_clk,  -- 1-bit clock enable input
      D0 => '0',   -- 1-bit data input (associated with C0)
      D1 => '1',   -- 1-bit data input (associated with C1)
      R => sys_rst,    -- 1-bit reset input
      S => open     -- 1-bit set input
   );
	
   data_clk_buf : ODDR2
   generic map(
      DDR_ALIGNMENT => "NONE", -- Sets output alignment to "NONE", "C0", "C1" 
      INIT => '0', -- Sets initial state of the Q output to '0' or '1'
      SRTYPE => "ASYNC") -- Specifies "SYNC" or "ASYNC" set/reset
   port map (
      Q => clk_data_o, -- 1-bit output data
      C0 => clk_data_T, -- 1-bit clock input
      C1 => not clk_data_t, -- 1-bit clock input
      CE => en_data_clk,  -- 1-bit clock enable input
      D0 => '0',   -- 1-bit data input (associated with C0)
      D1 => '1',   -- 1-bit data input (associated with C1)
      R => sys_rst,    -- 1-bit reset input
      S => open     -- 1-bit set input
   );	

end behavioral;
        

