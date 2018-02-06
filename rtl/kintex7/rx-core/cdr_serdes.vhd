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
		slip : in std_logic;

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

	signal lockcnt : integer range 0 to 128 := 0;
begin

serdes : ISERDESE2
   generic map (
      DATA_RATE => "SDR",           -- DDR, SDR
      DATA_WIDTH => 4,              -- Parallel data width (2-8,10,14)
      DYN_CLKDIV_INV_EN => "FALSE", -- Enable DYNCLKDIVINVSEL inversion (FALSE, TRUE)
      DYN_CLK_INV_EN => "FALSE",    -- Enable DYNCLKINVSEL inversion (FALSE, TRUE)
      -- INIT_Q1 - INIT_Q4: Initial value on the Q outputs (0/1)
      INIT_Q1 => '0',
      INIT_Q2 => '0',
      INIT_Q3 => '0',
      INIT_Q4 => '0',
      INTERFACE_TYPE => "NETWORKING",   -- MEMORY, MEMORY_DDR3, MEMORY_QDR, NETWORKING, OVERSAMPLE
      IOBDELAY => "NONE",           -- NONE, BOTH, IBUF, IFD
      NUM_CE => 2,                  -- Number of clock enables (1,2)
      OFB_USED => "FALSE",          -- Select OFB path (FALSE, TRUE)
      SERDES_MODE => "MASTER",      -- MASTER, SLAVE
      -- SRVAL_Q1 - SRVAL_Q4: Q output values when SR is used (0/1)
      SRVAL_Q1 => '0',
      SRVAL_Q2 => '0',
      SRVAL_Q3 => '0',
      SRVAL_Q4 => '0' 
   )
   port map (
      O => open,                       -- 1-bit output: Combinatorial output
      -- Q1 - Q8: 1-bit (each) output: Registered data outputs
      Q1 => AZ(0),
      Q2 => BZ(0),
      Q3 => CZ(0),
      Q4 => DZ(0),
      Q5 => open,
      Q6 => open,
      Q7 => open,
      Q8 => open,
      -- SHIFTOUT1, SHIFTOUT2: 1-bit (each) output: Data width expansion output ports
      SHIFTOUT1 => open,
      SHIFTOUT2 => open,
      BITSLIP => slip,           -- 1-bit input: The BITSLIP pin performs a Bitslip operation synchronous to
                                    -- CLKDIV when asserted (active High). Subsequently, the data seen on the
                                    -- Q1 to Q8 output ports will shift, as in a barrel-shifter operation, one
                                    -- position every time Bitslip is invoked (DDR operation is different from
                                    -- SDR).

      -- CE1, CE2: 1-bit (each) input: Data register clock enable inputs
      CE1 => '1',
      CE2 => '0',
      CLKDIVP => '0',           -- 1-bit input: TBD
      -- Clocks: 1-bit (each) input: ISERDESE2 clock input ports
      CLK => clk640,                   -- 1-bit input: High-speed clock
      CLKB => '0',                 -- 1-bit input: High-speed secondary clock
      CLKDIV => clk160,             -- 1-bit input: Divided clock
      OCLK => '0',                 -- 1-bit input: High speed output clock used when INTERFACE_TYPE="MEMORY" 
      -- Dynamic Clock Inversions: 1-bit (each) input: Dynamic clock inversion pins to switch clock polarity
      DYNCLKDIVSEL => '0', -- 1-bit input: Dynamic CLKDIV inversion
      DYNCLKSEL => '0',       -- 1-bit input: Dynamic CLK/CLKB inversion
      -- Input Data: 1-bit (each) input: ISERDESE2 data input ports
      D => din,                       -- 1-bit input: Data input
      DDLY => '0',                 -- 1-bit input: Serial data from IDELAYE2
      OFB => '0',                   -- 1-bit input: Data feedback from OSERDESE2
      OCLKB => '0',               -- 1-bit input: High speed negative edge output clock
      RST => reset,                   -- 1-bit input: Active high asynchronous reset
      -- SHIFTIN1, SHIFTIN2: 1-bit (each) input: Data width expansion input ports
      SHIFTIN1 => '0',
      SHIFTIN2 => '0' 
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
			lockcnt <= 127;
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
