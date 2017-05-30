library IEEE;
USE IEEE.STD_LOGIC_1164.all;
USE IEEE.NUMERIC_STD.all;
use IEEE.std_logic_unsigned.all; 


entity top_bench is
    generic (
		constant period : time := 100 ns;
		constant axis_data_width_c : integer := 64;
		constant axis_rx_tkeep_width_c : integer := 64/8;
		constant axis_rx_tuser_width_c : integer := 22;
		constant wb_address_width_c : integer := 15;
		constant wb_data_width_c : integer := 32
	);
	--port ();
end top_bench;

architecture Behavioral of top_bench is
	signal clk_tbs : STD_LOGIC;
	signal rst_tbs : STD_LOGIC;
	
	signal usr_sw_tbs : STD_LOGIC_VECTOR (2 downto 0);
	signal usr_led_s : STD_LOGIC_VECTOR (3 downto 0);
	signal front_led_s : STD_LOGIC_VECTOR (3 downto 0);
	
	-- Slave AXI-Stream
	signal s_axis_rx_tdata_tbs : STD_LOGIC_VECTOR (axis_data_width_c - 1 downto 0);
	signal s_axis_rx_tkeep_tbs : STD_LOGIC_VECTOR (axis_rx_tkeep_width_c - 1 downto 0);
	signal s_axis_rx_tlast_tbs : STD_LOGIC;
	signal s_axis_rx_tready_s : STD_LOGIC;
	signal s_axis_rx_tuser_tbs : STD_LOGIC_VECTOR (axis_rx_tuser_width_c - 1 downto 0);
	signal s_axis_rx_tvalid_tbs : STD_LOGIC;
	-- Master AXI-Stream
	signal m_axis_tx_tdata_s : STD_LOGIC_VECTOR (axis_data_width_c - 1 downto 0);
	signal m_axis_tx_tkeep_s : STD_LOGIC_VECTOR (axis_data_width_c/8 - 1 downto 0);
	signal m_axis_tx_tuser_s : STD_LOGIC_VECTOR (3 downto 0);
	signal m_axis_tx_tlast_s : STD_LOGIC;
	signal m_axis_tx_tvalid_s : STD_LOGIC;
	signal m_axis_tx_tready_tbs : STD_LOGIC;
    
    -- PCIE signals
    signal user_lnk_up_s : STD_LOGIC;
    signal user_app_rdy_s : STD_LOGIC;
    signal cfg_interrupt_s : STD_LOGIC;
    signal cfg_interrupt_rdy_s : STD_LOGIC;
    signal cfg_interrupt_assert_s : STD_LOGIC;
    signal cfg_interrupt_di_s : STD_LOGIC_VECTOR(7 DOWNTO 0);
    signal cfg_interrupt_do_s : STD_LOGIC_VECTOR(7 DOWNTO 0);
    signal cfg_interrupt_mmenable_s : STD_LOGIC_VECTOR(2 DOWNTO 0);
    signal cfg_interrupt_msienable_s : STD_LOGIC;
    signal cfg_interrupt_msixenable_s : STD_LOGIC;
    signal cfg_interrupt_msixfm_s : STD_LOGIC;
    signal cfg_interrupt_stat_s : STD_LOGIC;
    signal cfg_pciecap_interrupt_msgnum_s : STD_LOGIC_VECTOR(4 DOWNTO 0);
	
	-- Test bench specific signals
	signal step : integer;
	
	type tlp_type_t is (MRd,MRdLk,MWr,IORd,IOWr,CfgRd0,CfgWr0,CfgRd1,CfgWr1,TCfgRd,TCfgWr,Msg,MsgD,Cpl,CplD,CplLk,CplDLk,LPrfx,unknown);
	type header_t is (H3DW,H4DW);
	
	procedure axis_data_p (
		tlp_type_i : in tlp_type_t;
		header_type_i : in header_t;
		address_i : in STD_LOGIC_VECTOR(64-1 downto 0); 
		data_i : in STD_LOGIC_VECTOR(64-1 downto 0);
		length_i : in STD_LOGIC_VECTOR(10-1 downto 0); 
		rx_data_0 : out STD_LOGIC_VECTOR (axis_data_width_c - 1 downto 0);
		rx_data_1 : out STD_LOGIC_VECTOR (axis_data_width_c - 1 downto 0);
		rx_data_2 : out STD_LOGIC_VECTOR (axis_data_width_c - 1 downto 0)
		) is
	begin
		rx_data_0(63 downto 48) := X"0000"; --H1 Requester ID
		rx_data_0(47 downto 40) := X"00"; --H1 Tag 
		
		if length_i = "00" & X"01" then
			rx_data_0(39 downto 32) := X"0f"; --H1 Tag and Last DW BE and 1st DW BE see ch. 2.2.5 pcie spec
		else
			rx_data_0(39 downto 32) := X"ff";
		end if;
		
		case tlp_type_i is
			when MRd =>
				if header_type_i = H3DW then
					rx_data_0(31 downto 29) := "000"; -- H0 FMT
				else
					rx_data_0(31 downto 29) := "001"; -- H0 FMT
				end if;
				rx_data_0(28 downto 24) := "00000"; -- H0 type Memory request
			when MWr =>
				if header_type_i = H3DW then
					rx_data_0(31 downto 29) := "010"; -- H0 FMT
				else
					rx_data_0(31 downto 29) := "011"; -- H0 FMT
				end if;
				rx_data_0(28 downto 24) := "00000"; -- H0 type Memory request
			when CplD =>
				rx_data_0(31 downto 29) := "010"; -- H0 FMT
				rx_data_0(28 downto 24) := "01010"; -- H0 type Memory request
			when others =>
			
			
		end case;
		
		
		
		
		rx_data_0(23 downto 16) := X"00";   -- some unused bits
		rx_data_0(15 downto 10) := "000000"; --H0 unused bits 
		rx_data_0(9 downto 0) := length_i;  --H0 length H & length L
		
		if header_type_i = H3DW then
			rx_data_1(63 downto 32) := data_i(31 downto 0); --D0 Data
			rx_data_1(31 downto 0)	:= address_i(31 downto 0);  --H2 Adress	
			rx_data_2 := (others => '0');
		else
			rx_data_1(63 downto 32) := address_i(31 downto 0); --H3 Adress L (Last 4 bit must always pull at zero, byte to 8 byte)
			rx_data_1(31 downto 0)	:= address_i(63 downto 32);  --H2 Adress H
			rx_data_2 := data_i;
		end if;
		

		

		
	end axis_data_p;
	
	component app is
		Generic(
			AXI_BUS_WIDTH : integer := 64
			);
		Port ( 
		   clk_i : in STD_LOGIC;
		   rst_i : in STD_LOGIC;
		   user_lnk_up_i : in STD_LOGIC;
		   user_app_rdy_i : in STD_LOGIC;
		   
		   -- AXI-Stream bus
		   m_axis_tx_tready_i : in STD_LOGIC;
		   m_axis_tx_tdata_o : out STD_LOGIC_VECTOR(AXI_BUS_WIDTH-1 DOWNTO 0);
		   m_axis_tx_tkeep_o : out STD_LOGIC_VECTOR(AXI_BUS_WIDTH/8-1 DOWNTO 0);
		   m_axis_tx_tlast_o : out STD_LOGIC;
		   m_axis_tx_tvalid_o : out STD_LOGIC;
		   m_axis_tx_tuser_o : out STD_LOGIC_VECTOR(3 DOWNTO 0);
		   s_axis_rx_tdata_i : in STD_LOGIC_VECTOR(AXI_BUS_WIDTH-1 DOWNTO 0);
		   s_axis_rx_tkeep_i : in STD_LOGIC_VECTOR(AXI_BUS_WIDTH/8-1 DOWNTO 0);
		   s_axis_rx_tlast_i : in STD_LOGIC;
		   s_axis_rx_tvalid_i : in STD_LOGIC;
		   s_axis_rx_tready_o : out STD_LOGIC;
		   s_axis_rx_tuser_i : in STD_LOGIC_VECTOR(21 DOWNTO 0);
		   
		   -- PCIe interrupt config
		   cfg_interrupt_o : out STD_LOGIC;
		   cfg_interrupt_rdy_i : in STD_LOGIC;
		   cfg_interrupt_assert_o : out STD_LOGIC;
		   cfg_interrupt_di_o : out STD_LOGIC_VECTOR(7 DOWNTO 0);
		   cfg_interrupt_do_i : in STD_LOGIC_VECTOR(7 DOWNTO 0);
		   cfg_interrupt_mmenable_i : in STD_LOGIC_VECTOR(2 DOWNTO 0);
		   cfg_interrupt_msienable_i : in STD_LOGIC;
		   cfg_interrupt_msixenable_i : in STD_LOGIC;
		   cfg_interrupt_msixfm_i : in STD_LOGIC;
		   cfg_interrupt_stat_o : out STD_LOGIC;
		   cfg_pciecap_interrupt_msgnum_o : out STD_LOGIC_VECTOR(4 DOWNTO 0);
		   
		   -- PCIe debug
           cfg_dstatus_i : in STD_LOGIC_VECTOR(15 DOWNTO 0);
		   
		   --I/O
		   usr_sw_i : in STD_LOGIC_VECTOR (2 downto 0);
		   usr_led_o : out STD_LOGIC_VECTOR (3 downto 0);
		   front_led_o : out STD_LOGIC_VECTOR (3 downto 0)
		);
	end component;
	
begin

	clk_p: process
	begin
		clk_tbs <= '1';
		wait for period/2;
		clk_tbs <= '0';
		wait for period/2;
	end process clk_p;
	
	reset_p: process
	begin
	   rst_tbs <= '1';
	   wait for period;
	   rst_tbs <= '0';
	   wait;
	end process reset_p;
	
	stimuli_p: process
		variable data_0 : STD_LOGIC_VECTOR (axis_data_width_c - 1 downto 0);
		variable data_1 : STD_LOGIC_VECTOR (axis_data_width_c - 1 downto 0);
		variable data_2 : STD_LOGIC_VECTOR (axis_data_width_c - 1 downto 0);
	begin
		step <= 0;
		s_axis_rx_tdata_tbs <= (others => '0');
		s_axis_rx_tkeep_tbs <= (others => '0');
		s_axis_rx_tlast_tbs <= '0';
		s_axis_rx_tuser_tbs <= (others => '0');
		s_axis_rx_tvalid_tbs <= '0';
		m_axis_tx_tready_tbs <= '1';
		wait for period;
		wait for period;
		-- DMACSTARTR = X"00000010"
		step <= 10;
		axis_data_p (MWr,H3DW,
		X"0000000000000008",
		X"00000000" & X"00000010",
		"00" & X"01",
		data_0,
		data_1,
		data_2);
		s_axis_rx_tdata_tbs <= data_0;
		s_axis_rx_tkeep_tbs <= X"FF";
		s_axis_rx_tlast_tbs <= '0';
		s_axis_rx_tuser_tbs <= "11" & X"e4004";
		s_axis_rx_tvalid_tbs <= '1';	
		wait for period;
		
		step <= 11;
		s_axis_rx_tdata_tbs <= data_1;
		s_axis_rx_tkeep_tbs <= X"FF";
		s_axis_rx_tlast_tbs <= '1';
		s_axis_rx_tuser_tbs <= "10" & X"e4004";
		s_axis_rx_tvalid_tbs <= '1';
		wait for period;
		
		step <= 12;
		s_axis_rx_tdata_tbs <= X"0000000000000001";
		s_axis_rx_tkeep_tbs <= X"FF";
		s_axis_rx_tlast_tbs <= '0';
		s_axis_rx_tuser_tbs <= "11" & X"60000";
		s_axis_rx_tvalid_tbs <= '0';
		wait for period;
		
		step <= 13;
		s_axis_rx_tdata_tbs <= X"0000000000000001";
		s_axis_rx_tkeep_tbs <= X"FF";
		s_axis_rx_tlast_tbs <= '0';
		s_axis_rx_tuser_tbs <= "11" & X"60000";
		s_axis_rx_tvalid_tbs <= '0';
		wait for period;
		
		wait for 6*period;
		
		
		--DMAHSTARTLR = 0xA0
		wait for period;
		step <= 20;
		axis_data_p (MWr,H3DW,
		X"000000000000000C",
		X"00000000" & X"000000A0",
		"00" & X"01",
		data_0,
		data_1,
		data_2);
		s_axis_rx_tdata_tbs <= data_0;
		s_axis_rx_tkeep_tbs <= X"FF";
		s_axis_rx_tlast_tbs <= '0';
		s_axis_rx_tuser_tbs <= "11" & X"e4004";
		s_axis_rx_tvalid_tbs <= '1';	

		wait for period;
		step <= 21;
		s_axis_rx_tdata_tbs <= data_1;
		s_axis_rx_tkeep_tbs <= X"FF";
		s_axis_rx_tlast_tbs <= '1';
		s_axis_rx_tuser_tbs <= "10" & X"e4004";
		s_axis_rx_tvalid_tbs <= '1';
		
		
		wait for period;
		step <= 22;
		s_axis_rx_tdata_tbs <= X"0000000000000001";
		s_axis_rx_tkeep_tbs <= X"FF";
		s_axis_rx_tlast_tbs <= '0';
		s_axis_rx_tuser_tbs <= "11" & X"60000";
		s_axis_rx_tvalid_tbs <= '0';
		wait for period;
		step <= 23;
		s_axis_rx_tdata_tbs <= X"0000000000000001";
		s_axis_rx_tkeep_tbs <= X"FF";
		s_axis_rx_tlast_tbs <= '0';
		s_axis_rx_tuser_tbs <= "11" & X"60000";
		s_axis_rx_tvalid_tbs <= '0';
		wait for period;
		
		wait for 6*period;
		
		--DMALENR = 0x20
		wait for period;
		step <= 30;
		axis_data_p (MWr,H3DW,
		X"0000000000000014",
		X"00000000" & X"00000020",
		"00" & X"01",
		data_0,
		data_1,
		data_2);
		s_axis_rx_tdata_tbs <= data_0;
		s_axis_rx_tkeep_tbs <= X"FF";
		s_axis_rx_tlast_tbs <= '0';
		s_axis_rx_tuser_tbs <= "11" & X"e4004";
		s_axis_rx_tvalid_tbs <= '1';	
		wait for period;
		step <= 31;
		s_axis_rx_tdata_tbs <= data_1;
		s_axis_rx_tkeep_tbs <= X"FF";
		s_axis_rx_tlast_tbs <= '1';
		s_axis_rx_tuser_tbs <= "10" & X"e4004";
		s_axis_rx_tvalid_tbs <= '1';
		wait for period;
		step <= 32;
		s_axis_rx_tdata_tbs <= X"0000000000000001";
		s_axis_rx_tkeep_tbs <= X"FF";
		s_axis_rx_tlast_tbs <= '0';
		s_axis_rx_tuser_tbs <= "11" & X"60000";
		s_axis_rx_tvalid_tbs <= '0';
		wait for period;
		step <= 33;
		s_axis_rx_tdata_tbs <= X"0000000000000001";
		s_axis_rx_tkeep_tbs <= X"FF";
		s_axis_rx_tlast_tbs <= '0';
		s_axis_rx_tuser_tbs <= "11" & X"60000";
		s_axis_rx_tvalid_tbs <= '0';
		wait for period;
		
		wait for 6*period;
		
		--DMAATTRIBR = 0x2 last item and P2L
		wait for period;
		step <= 40;
		axis_data_p (MWr,H3DW,
		X"0000000000000020",
		X"00000000" & X"00000002",
		"00" & X"01",
		data_0,
		data_1,
		data_2);
		s_axis_rx_tdata_tbs <= data_0;
		s_axis_rx_tkeep_tbs <= X"FF";
		s_axis_rx_tlast_tbs <= '0';
		s_axis_rx_tuser_tbs <= "11" & X"e4004";
		s_axis_rx_tvalid_tbs <= '1';	
		wait for period;
		step <= 41;
		s_axis_rx_tdata_tbs <= data_1;
		s_axis_rx_tkeep_tbs <= X"FF";
		s_axis_rx_tlast_tbs <= '1';
		s_axis_rx_tuser_tbs <= "10" & X"e4004";
		s_axis_rx_tvalid_tbs <= '1';
		wait for period;
		step <= 42;
		s_axis_rx_tdata_tbs <= X"0000000000000001";
		s_axis_rx_tkeep_tbs <= X"FF";
		s_axis_rx_tlast_tbs <= '0';
		s_axis_rx_tuser_tbs <= "11" & X"60000";
		s_axis_rx_tvalid_tbs <= '0';
		wait for period;
		step <= 43;
		s_axis_rx_tdata_tbs <= X"0000000000000001";
		s_axis_rx_tkeep_tbs <= X"FF";
		s_axis_rx_tlast_tbs <= '0';
		s_axis_rx_tuser_tbs <= "11" & X"60000";
		s_axis_rx_tvalid_tbs <= '0';
		wait for period;
		
		wait for 6*period;
		
		--DMACTRLR = 0x1 to start the transfert
		wait for period;
		step <= 40;
		axis_data_p (MWr,H3DW,
		X"0000000000000000",
		X"00000000" & X"00000001",
		"00" & X"01",
		data_0,
		data_1,
		data_2);
		s_axis_rx_tdata_tbs <= data_0;
		s_axis_rx_tkeep_tbs <= X"FF";
		s_axis_rx_tlast_tbs <= '0';
		s_axis_rx_tuser_tbs <= "11" & X"e4004";
		s_axis_rx_tvalid_tbs <= '1';	
		wait for period;
		step <= 41;
		s_axis_rx_tdata_tbs <= data_1;
		s_axis_rx_tkeep_tbs <= X"FF";
		s_axis_rx_tlast_tbs <= '1';
		s_axis_rx_tuser_tbs <= "10" & X"e4004";
		s_axis_rx_tvalid_tbs <= '1';
		wait for period;
		step <= 42;
		s_axis_rx_tdata_tbs <= X"0000000000000001";
		s_axis_rx_tkeep_tbs <= X"FF";
		s_axis_rx_tlast_tbs <= '0';
		s_axis_rx_tuser_tbs <= "11" & X"60000";
		s_axis_rx_tvalid_tbs <= '0';
		wait for period;
		step <= 43;
		s_axis_rx_tdata_tbs <= X"0000000000000001";
		s_axis_rx_tkeep_tbs <= X"FF";
		s_axis_rx_tlast_tbs <= '0';
		s_axis_rx_tuser_tbs <= "11" & X"60000";
		s_axis_rx_tvalid_tbs <= '0';
		wait for period;
		
		wait for 14*period;		
		
		step <= 50;
		axis_data_p (CplD,H3DW,X"0000000000000010",X"BEEF5A5A" & X"BEEF0001","00" & X"04",data_0,data_1,data_2);
		s_axis_rx_tdata_tbs <= data_0;
		s_axis_rx_tkeep_tbs <= X"FF";
		s_axis_rx_tlast_tbs <= '0';
		s_axis_rx_tuser_tbs <= "11" & X"60004";
		s_axis_rx_tvalid_tbs <= '1';
		m_axis_tx_tready_tbs <= '1';
		wait for period;
		
		step <= 51;
		s_axis_rx_tdata_tbs <= data_1;
		wait for period;
		step <= 52;
		s_axis_rx_tdata_tbs <=  X"BEEF0002" & X"DEAD0001";
		wait for period;
		step <= 53;
		s_axis_rx_tdata_tbs <=  X"CACA0003" & X"DEAD0002";
		s_axis_rx_tkeep_tbs <= X"0F";
		s_axis_rx_tlast_tbs <= '1';
		wait for period;
		step <= 54;
		s_axis_rx_tdata_tbs <= X"0000000000000001";
		s_axis_rx_tkeep_tbs <= X"FF";
		s_axis_rx_tlast_tbs <= '0';
		s_axis_rx_tuser_tbs <= "11" & X"60000";
		s_axis_rx_tvalid_tbs <= '0';
		wait for period;
		
		wait for 6*period;
		
		--DMAATTRIBR = 0x0 last item and L2P
		wait for period;
		step <= 60;
		axis_data_p (MWr,H3DW,
		X"0000000000000020",
		X"00000000" & X"00000000",
		"00" & X"01",
		data_0,
		data_1,
		data_2);
		s_axis_rx_tdata_tbs <= data_0;
		s_axis_rx_tkeep_tbs <= X"FF";
		s_axis_rx_tlast_tbs <= '0';
		s_axis_rx_tuser_tbs <= "11" & X"e4004";
		s_axis_rx_tvalid_tbs <= '1';	
		wait for period;
		step <= 61;
		s_axis_rx_tdata_tbs <= data_1;
		s_axis_rx_tkeep_tbs <= X"FF";
		s_axis_rx_tlast_tbs <= '1';
		s_axis_rx_tuser_tbs <= "10" & X"e4004";
		s_axis_rx_tvalid_tbs <= '1';
		wait for period;
		step <= 62;
		s_axis_rx_tdata_tbs <= X"0000000000000001";
		s_axis_rx_tkeep_tbs <= X"FF";
		s_axis_rx_tlast_tbs <= '0';
		s_axis_rx_tuser_tbs <= "11" & X"60000";
		s_axis_rx_tvalid_tbs <= '0';
		wait for period;
		step <= 63;
		s_axis_rx_tdata_tbs <= X"0000000000000001";
		s_axis_rx_tkeep_tbs <= X"FF";
		s_axis_rx_tlast_tbs <= '0';
		s_axis_rx_tuser_tbs <= "11" & X"60000";
		s_axis_rx_tvalid_tbs <= '0';
		wait for period;
		
		wait for 6*period;
		
		--DMACTRLR = 0x1 to start the transfert
		wait for period;
		step <= 70;
		axis_data_p (MWr,H3DW,
		X"0000000000000000",
		X"00000000" & X"00000001",
		"00" & X"01",
		data_0,
		data_1,
		data_2);
		s_axis_rx_tdata_tbs <= data_0;
		s_axis_rx_tkeep_tbs <= X"FF";
		s_axis_rx_tlast_tbs <= '0';
		s_axis_rx_tuser_tbs <= "11" & X"e4004";
		s_axis_rx_tvalid_tbs <= '1';	
		wait for period;
		step <= 71;
		s_axis_rx_tdata_tbs <= data_1;
		s_axis_rx_tkeep_tbs <= X"FF";
		s_axis_rx_tlast_tbs <= '1';
		s_axis_rx_tuser_tbs <= "10" & X"e4004";
		s_axis_rx_tvalid_tbs <= '1';
		wait for period;
		step <= 72;
		s_axis_rx_tdata_tbs <= X"0000000000000001";
		s_axis_rx_tkeep_tbs <= X"FF";
		s_axis_rx_tlast_tbs <= '0';
		s_axis_rx_tuser_tbs <= "11" & X"60000";
		s_axis_rx_tvalid_tbs <= '0';
		wait for period;
		step <= 73;
		s_axis_rx_tdata_tbs <= X"0000000000000001";
		s_axis_rx_tkeep_tbs <= X"FF";
		s_axis_rx_tlast_tbs <= '0';
		s_axis_rx_tuser_tbs <= "11" & X"60000";
		s_axis_rx_tvalid_tbs <= '0';
		wait for period;
		
		wait for 6*period;
		
		--- Read DMACSTARTR
		-- step <= 60;
		-- axis_data_p (MRd,H3DW,X"0000000000000008",X"00000000" & X"BEEF5050","00" & X"00",data_0,data_1,data_2);
		-- s_axis_rx_tdata_tbs <= data_0;
		-- s_axis_rx_tkeep_tbs <= X"FF";
		-- s_axis_rx_tlast_tbs <= '0';
		-- s_axis_rx_tuser_tbs <= "00" & X"e4004";
		-- s_axis_rx_tvalid_tbs <= '1';
		-- wait for period;
		-- step <= 61;
		-- s_axis_rx_tdata_tbs <= data_1;
		-- s_axis_rx_tkeep_tbs <= X"FF";
		-- s_axis_rx_tlast_tbs <= '1';
		-- s_axis_rx_tuser_tbs <= "11" & X"60004";
		-- s_axis_rx_tvalid_tbs <= '1';
		-- wait for period;
		-- step <= 62;
		-- s_axis_rx_tdata_tbs <= X"0000000000A00001";
		-- s_axis_rx_tkeep_tbs <= X"FF";
		-- s_axis_rx_tlast_tbs <= '0';
		-- s_axis_rx_tuser_tbs <= "11" & X"60000";
		-- s_axis_rx_tvalid_tbs <= '0';
		-- wait for period;
		
		-- wait for 6*period;
		
		step <= 100;


		wait;
		
		
	end process stimuli_p;
	

	app_0:app
	generic map(
		AXI_BUS_WIDTH => 64
	)
	port map(
		clk_i => clk_tbs,
		rst_i => rst_tbs,
		user_lnk_up_i => user_lnk_up_s,
		user_app_rdy_i => user_app_rdy_s,

		-- AXI-Stream bus
		m_axis_tx_tready_i => m_axis_tx_tready_tbs,
		m_axis_tx_tdata_o => m_axis_tx_tdata_s,
		m_axis_tx_tkeep_o => m_axis_tx_tkeep_s,
		m_axis_tx_tlast_o => m_axis_tx_tlast_s,
		m_axis_tx_tvalid_o => m_axis_tx_tvalid_s,
		m_axis_tx_tuser_o => m_axis_tx_tuser_s,
		s_axis_rx_tdata_i => s_axis_rx_tdata_tbs,
		s_axis_rx_tkeep_i => s_axis_rx_tkeep_tbs,
		s_axis_rx_tlast_i => s_axis_rx_tlast_tbs,
		s_axis_rx_tvalid_i => s_axis_rx_tvalid_tbs,
		s_axis_rx_tready_o => s_axis_rx_tready_s,
		s_axis_rx_tuser_i => s_axis_rx_tuser_tbs,

		-- PCIe interrupt config
		cfg_interrupt_o => cfg_interrupt_s,
		cfg_interrupt_rdy_i => cfg_interrupt_rdy_s,
		cfg_interrupt_assert_o => cfg_interrupt_assert_s,
		cfg_interrupt_di_o => cfg_interrupt_di_s,
		cfg_interrupt_do_i => cfg_interrupt_do_s,
		cfg_interrupt_mmenable_i => cfg_interrupt_mmenable_s,
		cfg_interrupt_msienable_i => cfg_interrupt_msienable_s,
		cfg_interrupt_msixenable_i => cfg_interrupt_msixenable_s,
		cfg_interrupt_msixfm_i => cfg_interrupt_msixfm_s,
		cfg_interrupt_stat_o => cfg_interrupt_stat_s,
		cfg_pciecap_interrupt_msgnum_o => cfg_pciecap_interrupt_msgnum_s,
        
        cfg_dstatus_i => (others => '0'),
        
		--I/O
		usr_sw_i => usr_sw_tbs,
		usr_led_o => usr_led_s,
		front_led_o => front_led_s
	);
	
	
	
end Behavioral;