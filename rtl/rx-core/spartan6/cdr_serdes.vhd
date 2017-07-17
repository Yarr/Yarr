-- CDR with SERDES
library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;

library UNISIM;
use UNISIM.VComponents.all;

entity cdr_serdes is
	port
	(
		-- clocks
		clk160 : in std_logic;
		clk640 : in std_logic;

		-- reset
		reset : in std_logic;

		-- data input
		din : in std_logic;

		-- data output
		data_value : out std_logic_vector(1 downto 0);
		data_valid : out std_logic_vector(1 downto 0);
		data_lock : out std_logic
	);
end cdr_serdes;

architecture rtl of cdr_serdes is
	signal AZ : std_logic_vector(4 downto 0) := (others => '0');
	signal BZ : std_logic_vector(4 downto 0) := (others => '0');
	signal CZ : std_logic_vector(4 downto 0) := (others => '0');
	signal DZ : std_logic_vector(4 downto 0) := (others => '0');

	signal AAP, AAN : std_logic := '0';
	signal BBP, BBN : std_logic := '0';
	signal CCP, CCN : std_logic := '0';
	signal DDP, DDN : std_logic := '0';

	signal use_A : std_logic := '0';
	signal use_B : std_logic := '0';
	signal use_C : std_logic := '0';
	signal use_D : std_logic := '0';

	signal use_A1, use_A2 : std_logic := '0';
	signal use_B1, use_B2 : std_logic := '0';
	signal use_C1, use_C2 : std_logic := '0';
	signal use_D1, use_D2 : std_logic := '0';

	signal use_A_reg : std_logic := '0';
	signal use_B_reg : std_logic := '0';
	signal use_C_reg : std_logic := '0';
	signal use_D_reg : std_logic := '0';

	signal use_A_reg2 : std_logic := '0';
	signal use_B_reg2 : std_logic := '0';
	signal use_C_reg2 : std_logic := '0';
	signal use_D_reg2 : std_logic := '0';

	signal sdata_A : std_logic_vector(1 downto 0) := "00";
	signal sdata_B : std_logic_vector(1 downto 0) := "00";
	signal sdata_C : std_logic_vector(1 downto 0) := "00";
	signal sdata_D : std_logic_vector(1 downto 0) := "00";

	signal pipe_ce0 : std_logic := '0';
	signal pipe_ce1 : std_logic := '0';

	signal valid_int : std_logic_vector(1 downto 0) := "00";

	signal lockcnt : integer range 0 to 31 := 0;
begin

serdes: ISERDES2
		generic map (
			BITSLIP_ENABLE => FALSE,        -- Enable Bitslip Functionality (TRUE/FALSE)
			DATA_RATE => "SDR",             -- Data-rate ("SDR" or "DDR")
			DATA_WIDTH => 4,                -- Parallel data width selection (2-8)
			INTERFACE_TYPE => "RETIMED",    -- "NETWORKING", "NETWORKING_PIPELINED" or "RETIMED" 
			SERDES_MODE => "NONE"           -- "NONE", "MASTER" or "SLAVE" 
		)
		port map (
			CFB0 => open,           -- 1-bit output: Clock feed-through route output
			CFB1 => open,           -- 1-bit output: Clock feed-through route output
			DFB => open,            -- 1-bit output: Feed-through clock output
			FABRICOUT => open, 	-- 1-bit output: Unsynchrnonized data output
			INCDEC => open,         -- 1-bit output: Phase detector output
			-- Q1 - Q4: 1-bit (each) output: Registered outputs to FPGA logic
			Q1 => AZ(0),
			Q2 => BZ(0),
			Q3 => CZ(0),
			Q4 => DZ(0),
			SHIFTOUT => open,       -- 1-bit output: Cascade output signal for master/slave I/O
			VALID => open,        	-- 1-bit output: Output status of the phase detector
			BITSLIP => '0',     	-- 1-bit input: Bitslip enable input
			CE0 => '1',             -- 1-bit input: Clock enable input
			CLK0 => clk640,         -- 1-bit input: I/O clock network input
			CLK1 => '0',           	-- 1-bit input: Secondary I/O clock network input
			CLKDIV => clk160,       -- 1-bit input: FPGA logic domain clock input
			D => din,               -- 1-bit input: Input data
			IOCE => '1',          	-- 1-bit input: Data strobe input
			RST => reset,           -- 1-bit input: Asynchronous reset input
			SHIFTIN => '0'          -- 1-bit input: Cascade input signal for master/slave I/O
		);

process begin
	wait until rising_edge(clk160);

	if reset = '1' then
		AZ(4 downto 1) <= (others => '0');
		BZ(4 downto 1) <= (others => '0');
		CZ(4 downto 1) <= (others => '0');
		DZ(4 downto 1) <= (others => '0');

		AAP <= '0'; AAN <= '0';
		BBP <= '0'; BBN <= '0';
		CCP <= '0'; CCN <= '0';
		DDP <= '0'; DDN <= '0';

		use_A1 <= '0'; use_A2 <= '0'; use_A <= '0';
		use_B1 <= '0'; use_B2 <= '0'; use_B <= '0';
		use_C1 <= '0'; use_C2 <= '0'; use_C <= '0';
		use_D1 <= '0'; use_D2 <= '0'; use_D <= '0';

		use_A_reg <= '0'; use_A_reg2 <= '0';
		use_B_reg <= '0'; use_B_reg2 <= '0';
		use_C_reg <= '0'; use_C_reg2 <= '0';
		use_D_reg <= '0'; use_D_reg2 <= '0';

		sdata_A <= "00";
		sdata_B <= "00";
		sdata_C <= "00";
		sdata_D <= "00";
		valid_int <= "00";
		
		data_value <= "00";
		data_valid <= "00";
		data_lock <= '0';

		lockcnt <= 0;

		pipe_ce0 <= '0';
		pipe_ce1 <= '0';
	else
		-- clock in the data
		AZ(4 downto 1) <= AZ(3 downto 0);
		BZ(4 downto 1) <= BZ(3 downto 0);
		CZ(4 downto 1) <= CZ(3 downto 0);
		DZ(4 downto 1) <= DZ(3 downto 0);

		-- find positive edges
		AAP <= (AZ(2) xor AZ(3)) and not AZ(2);
		BBP <= (BZ(2) xor BZ(3)) and not BZ(2);
		CCP <= (CZ(2) xor CZ(3)) and not CZ(2);
		DDP <= (DZ(2) xor DZ(3)) and not DZ(2);

		-- find negative edges
		AAN <= (AZ(2) xor AZ(3)) and AZ(2);
		BBN <= (BZ(2) xor BZ(3)) and BZ(2);
		CCN <= (CZ(2) xor CZ(3)) and CZ(2);
		DDN <= (DZ(2) xor DZ(3)) and DZ(2);

		-- decision of sampling point
		use_A1 <= (BBP and not CCP and not DDP and AAP);
		use_A2 <= (BBN and not CCN and not DDN and AAN);
		use_B1 <= (CCP and not DDP and AAP and BBP);
		use_B2 <= (CCN and not DDN and AAN and BBN);
		use_C1 <= (DDP and AAP and BBP and CCP);
		use_C2 <= (DDN and AAN and BBN and CCN);
		use_D1 <= (AAP and not BBP and not CCP and not DDP);
		use_D2 <= (AAN and not BBN and not CCN and not DDN);
		use_A <= use_A1 or use_A2;
		use_B <= use_B1 or use_B2;
		use_C <= use_C1 or use_C2;
		use_D <= use_D1 or use_D2;
		
		-- if we found an edge
		if (use_A or use_B or use_C or use_D) = '1' then
			lockcnt <= 31;
			pipe_ce0 <= '1';	-- sync marker
			pipe_ce1 <= '1';
		else
			if lockcnt = 0 then
				pipe_ce0 <= '0';
			else
				lockcnt <= lockcnt - 1;
			end if;
			pipe_ce1 <= '0';
		end if;

		-- register
		use_A_reg <= use_A;
		use_B_reg <= use_B;
		use_C_reg <= use_C;
		use_D_reg <= use_D;

		if pipe_ce1 = '1' then
			use_A_reg2 <= use_A_reg;
			use_B_reg2 <= use_B_reg;
			use_C_reg2 <= use_C_reg;
			use_D_reg2 <= use_D_reg;
		end if;

		-- collect output data
		sdata_A(0) <= AZ(4) and use_A_reg2; sdata_A(1) <= AZ(4) and use_D_reg2;
		sdata_B(0) <= BZ(4) and use_B_reg2; sdata_B(1) <= '0';
		sdata_C(0) <= CZ(4) and use_C_reg2; sdata_C(1) <= '0';
		sdata_D(0) <= DZ(4) and use_D_reg2; sdata_D(1) <= DZ(4) and use_A_reg2;

		-- ouput data if we have seen an edge
		if pipe_ce0 = '1' then
			data_value <= sdata_A or sdata_B or sdata_C or sdata_D;
		end if;

		-- data valid output
		if use_D_reg2 = '1' and use_A_reg = '1' then
			valid_int <= "00";		-- move from A to D: no valid data
		elsif use_A_reg2 = '1' and use_D_reg = '1' then
			valid_int <= "11";		-- move from D to A: 2 bits valid
		else
			valid_int <= "01";		-- only one bit is valid
		end if;
		
		if pipe_ce0 = '1' then
			data_valid <= valid_int;
		else
			data_valid <= "00";
		end if;

		data_lock <= pipe_ce0;
	end if;	
end process;
		
end architecture;
