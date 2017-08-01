library IEEE;
USE IEEE.STD_LOGIC_1164.all;
USE IEEE.NUMERIC_STD.all;
use IEEE.std_logic_unsigned.all; 

use work.wshexp_core_pkg.all;

entity p2l_decoder_bench is
    generic (
		constant period : time := 100 ns;
		constant axis_data_width_c : integer := 64;
		constant axis_rx_tkeep_width_c : integer := 64/8;
		constant axis_rx_tuser_width_c : integer := 22;
		constant wb_address_width_c : integer := 64;
		constant wb_data_width_c : integer := 32
	);
	--port ();
end p2l_decoder_bench;

architecture Behavioral of p2l_decoder_bench is
		signal clk_tbs : STD_LOGIC;
		signal rst_tbs : STD_LOGIC;
		-- Slave AXI-Stream
		signal s_axis_rx_tdata_tbs : STD_LOGIC_VECTOR (axis_data_width_c - 1 downto 0);
		signal s_axis_rx_tkeep_tbs : STD_LOGIC_VECTOR (axis_rx_tkeep_width_c - 1 downto 0);
		signal s_axis_rx_tlast_tbs : STD_LOGIC;
		signal s_axis_rx_ready_s : STD_LOGIC;
		signal s_axis_rx_tuser_tbs : STD_LOGIC_VECTOR (axis_rx_tuser_width_c - 1 downto 0);
		signal s_axis_rx_tvalid_tbs : STD_LOGIC;
        -- To the wishbone master
        signal pd_wbm_address_s : STD_LOGIC_VECTOR(63 downto 0);
        signal pd_wbm_data_s : STD_LOGIC_VECTOR(31 downto 0);
        signal pd_wbm_valid_s : std_logic;
        signal wbm_pd_ready_tbs : std_logic;
        signal pd_op_s : STD_LOGIC_VECTOR(2 downto 0);
        signal pd_header_type_s : STD_LOGIC;
        signal pd_payload_length_s : STD_LOGIC_VECTOR(9 downto 0);
		-- L2P DMA
		signal pd_pdm_data_valid_s  : std_logic;                      -- Indicates Data is valid
        signal pd_pdm_data_last_s   : std_logic;                      -- Indicates end of the packet
        signal pd_pdm_data_s        : std_logic_vector(63 downto 0);  -- Data
        signal pd_pdm_keep_s	    : std_logic_vector(7 downto 0);

		type tlp_type_t is (MRd,MRdLk,MWr,IORd,IOWr,CfgRd0,CfgWr0,CfgRd1,CfgWr1,TCfgRd,TCfgWr,Msg,MsgD,Cpl,CplD,CplLk,CplDLk,LPrfx,unknown);
		type header_t is (H3DW,H4DW);
		signal byte_swap_c : STD_LOGIC_VECTOR (1 downto 0);
		type bool_t is (false,true);
		-- Test bench specific signals
		signal step : integer;
		
		procedure axis_data_p (
			tlp_type_i : in tlp_type_t;
			header_type_i : in header_t;
			address_i : in STD_LOGIC_VECTOR(wb_address_width_c-1 downto 0); 
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
		
		Component p2l_decoder is
		Port (
            clk_i : in STD_LOGIC;
            rst_i : in STD_LOGIC;
            -- Slave AXI-Stream
            s_axis_rx_tdata_i : in STD_LOGIC_VECTOR (64 - 1 downto 0);
            s_axis_rx_tkeep_i : in STD_LOGIC_VECTOR (64/8 - 1 downto 0);
            s_axis_rx_tuser_i : in STD_LOGIC_VECTOR (21 downto 0);
            s_axis_rx_tlast_i : in STD_LOGIC;
            s_axis_rx_tvalid_i : in STD_LOGIC;
            s_axis_rx_tready_o : out STD_LOGIC;
            -- To the wishbone master
            pd_wbm_address_o : out STD_LOGIC_VECTOR(63 downto 0);
            pd_wbm_data_o : out STD_LOGIC_VECTOR(31 downto 0);
            pd_wbm_valid_o : out std_logic;
            pd_wbm_hdr_rid_o    : out std_logic_vector(15 downto 0);  -- Requester ID
            pd_wbm_hdr_cid_o    : out std_logic_vector(15 downto 0);  -- Completer ID
            pd_wbm_hdr_tag_o    : out std_logic_vector(7 downto 0);
            pd_wbm_target_mrd_o : out std_logic;                      -- Target memory read
            pd_wbm_target_mwr_o : out std_logic;                      -- Target memory write
            
            wbm_pd_ready_i : in std_logic;
            --wbm_pd_done_i : in std_logic;
            pd_op_o : out STD_LOGIC_VECTOR(2 downto 0);
            pd_header_type_o : out STD_LOGIC;
            pd_payload_length_o : out STD_LOGIC_VECTOR(9 downto 0);
            -- L2P DMA
            pd_pdm_data_valid_o  : out std_logic;                      -- Indicates Data is valid
            pd_pdm_data_valid_w_o  : out std_logic_vector(1 downto 0);
            pd_pdm_data_last_o   : out std_logic;                      -- Indicates end of the packet
            pd_pdm_keep_o         : out std_logic_vector(7 downto 0);
            pd_pdm_data_o        : out std_logic_vector(63 downto 0);  -- Data
            --debug outputs
            states_do : out STD_LOGIC_VECTOR(3 downto 0)
		);
		end component;
		
begin
    byte_swap_c <= "11";
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
		step <= 1;
		s_axis_rx_tdata_tbs <= (others => '0');
		s_axis_rx_tkeep_tbs <= (others => '0');
		s_axis_rx_tlast_tbs <= '0';
		s_axis_rx_tuser_tbs <= (others => '0');
		s_axis_rx_tvalid_tbs <= '0';
		wbm_pd_ready_tbs <= '1';
		wait for period;
		
		wait for period;
		step <= 2;
		axis_data_p (MWr,H3DW,X"0000000000000000",X"00000000" & X"BEEF5A5A","00" & X"01",data_0,data_1,data_2);
		s_axis_rx_tdata_tbs <= data_0;

		s_axis_rx_tkeep_tbs <= X"FF";
		s_axis_rx_tlast_tbs <= '0';
		s_axis_rx_tuser_tbs <= "11" & X"e4004";
		s_axis_rx_tvalid_tbs <= '1';

		wait for period;
		step <= 3;
		s_axis_rx_tdata_tbs <= data_1;
		s_axis_rx_tkeep_tbs <= X"FF";
		s_axis_rx_tlast_tbs <= '1';
		s_axis_rx_tuser_tbs <= "10" & X"e4004";
		s_axis_rx_tvalid_tbs <= '1';
		
		
		wait for period;
		step <= 4;
		s_axis_rx_tdata_tbs <= X"0000000000000001";
		s_axis_rx_tkeep_tbs <= X"FF";
		s_axis_rx_tlast_tbs <= '0';
		s_axis_rx_tuser_tbs <= "11" & X"60000";
		s_axis_rx_tvalid_tbs <= '0';
		wbm_pd_ready_tbs <= '0';

		wait for period;
		step <= 5;
		s_axis_rx_tdata_tbs <= X"0000000000000001";
		s_axis_rx_tkeep_tbs <= X"FF";
		s_axis_rx_tlast_tbs <= '0';
		s_axis_rx_tuser_tbs <= "11" & X"60000";
		s_axis_rx_tvalid_tbs <= '0';

		wait for period;
		wait for period;
		wbm_pd_ready_tbs <= '1';
		wait for period;
		step <= 6;
		axis_data_p (MRd,H3DW,X"0000000000000000",X"00000000" & X"5A5AEFBE","00" & X"00",data_0,data_1,data_2);
		s_axis_rx_tdata_tbs <= data_0;
		s_axis_rx_tkeep_tbs <= X"FF";
		s_axis_rx_tlast_tbs <= '0';
		s_axis_rx_tuser_tbs <= "00" & X"e4004";
		s_axis_rx_tvalid_tbs <= '1';
		wait for period;
		step <= 7;
		
		s_axis_rx_tdata_tbs <= data_1;
		s_axis_rx_tkeep_tbs <= X"FF";
		s_axis_rx_tlast_tbs <= '1';
		s_axis_rx_tuser_tbs <= "11" & X"60004";
		s_axis_rx_tvalid_tbs <= '1';
		wait for period;
		s_axis_rx_tdata_tbs <= X"0000000000A00001";
		s_axis_rx_tkeep_tbs <= X"FF";
		s_axis_rx_tlast_tbs <= '0';
		s_axis_rx_tuser_tbs <= "11" & X"60000";
		s_axis_rx_tvalid_tbs <= '0';
		wait for period;
		wait for period;
		step <= 8;
		wait for period;
		wait for period;

		wait for period;

		step <= 9;
		wbm_pd_ready_tbs <= '0';
		wait for period;
		
		step <= 10;
        axis_data_p (MWr,H3DW,X"0000000000000000",X"00000000" & X"BEEF5A5A","00" & X"01",data_0,data_1,data_2);
        s_axis_rx_tdata_tbs <= data_0;

        s_axis_rx_tkeep_tbs <= X"FF";
        s_axis_rx_tlast_tbs <= '0';
        s_axis_rx_tuser_tbs <= "11" & X"e4004";
        s_axis_rx_tvalid_tbs <= '1';
        
        wait for period;
        wait for period;
        wbm_pd_ready_tbs <= '1';
        wait for period;
        step <= 11;
        s_axis_rx_tdata_tbs <= data_1;
        s_axis_rx_tkeep_tbs <= X"FF";
        s_axis_rx_tlast_tbs <= '1';
        s_axis_rx_tvalid_tbs <= '1';
        s_axis_rx_tuser_tbs <= "10" & X"e4004";
        
        
        
        wait for period;
        step <= 12;
        s_axis_rx_tdata_tbs <= X"0000000000000001";
        s_axis_rx_tkeep_tbs <= X"FF";
        s_axis_rx_tlast_tbs <= '0';
        s_axis_rx_tuser_tbs <= "11" & X"60000";
        s_axis_rx_tvalid_tbs <= '0';
        wbm_pd_ready_tbs <= '0';

        wait for period;
        step <= 13;
        s_axis_rx_tdata_tbs <= X"0000000000000001";
        s_axis_rx_tkeep_tbs <= X"FF";
        s_axis_rx_tlast_tbs <= '0';
        s_axis_rx_tuser_tbs <= "11" & X"60000";
        s_axis_rx_tvalid_tbs <= '0';

        wait for period;
        
        wait for period;
        wbm_pd_ready_tbs <= '1';
        wait for period;
		

	    wait for period;
		step <= 17;
		axis_data_p (CplD,H3DW,X"0000000000000010",X"5A5AEFBE" & X"0000EFBE","00" & X"04",data_0,data_1,data_2);
		s_axis_rx_tdata_tbs <= data_0;
		s_axis_rx_tkeep_tbs <= X"FF";
		s_axis_rx_tlast_tbs <= '0';
		s_axis_rx_tuser_tbs <= "11" & X"60004";
		s_axis_rx_tvalid_tbs <= '1';
		wait for period;
		step <= 18;
		s_axis_rx_tdata_tbs <= data_1;
		wait for period;
		s_axis_rx_tdata_tbs <=  X"0200EFBE" & X"0100EFBE";
		wait for period;
		s_axis_rx_tdata_tbs <=  X"0400CACA" & X"0300EFBE";
		s_axis_rx_tkeep_tbs <= X"0F";
		s_axis_rx_tlast_tbs <= '1';
		wait for period;
        
        
        
        step <= 19;
        axis_data_p (CplD,H3DW,X"0000000000000010",X"5A5AEFBE" & X"0000EFBE","00" & X"02",data_0,data_1,data_2);
        s_axis_rx_tdata_tbs <= data_0;
        s_axis_rx_tkeep_tbs <= X"FF";
        s_axis_rx_tlast_tbs <= '0';
        s_axis_rx_tuser_tbs <= "11" & X"60004";
        s_axis_rx_tvalid_tbs <= '1';
        wait for period;
        step <= 20;
        s_axis_rx_tdata_tbs <= data_1;
        --wait for period;
        --s_axis_rx_tdata_tbs <=  X"0200EFBE" & X"0100EFBE";
        wait for period;
        s_axis_rx_tdata_tbs <=  X"0400CACA" & X"0300EFBE";
        s_axis_rx_tkeep_tbs <= X"0F";
        s_axis_rx_tlast_tbs <= '1';
		wait for period;
		
        step <= 30;
        axis_data_p (CplD,H3DW,X"0000000000000010",X"5A5AEFBE" & X"0000EFBE","00" & X"01",data_0,data_1,data_2);
        s_axis_rx_tdata_tbs <= data_0;
        s_axis_rx_tkeep_tbs <= X"FF";
        s_axis_rx_tlast_tbs <= '0';
        s_axis_rx_tuser_tbs <= "11" & X"60004";
        s_axis_rx_tvalid_tbs <= '1';
        wait for period;
        
        step <= 31;
        s_axis_rx_tdata_tbs <= data_1;
        --wait for period;
        --s_axis_rx_tdata_tbs <=  X"0200EFBE" & X"0100EFBE";
        s_axis_rx_tlast_tbs <= '1';
		wait for period;
        step <= 40;
        axis_data_p (CplD,H3DW,X"0000000000000010",X"5A5AEFBE" & X"0000EFBE","00" & X"05",data_0,data_1,data_2);
        s_axis_rx_tdata_tbs <= data_0;
        s_axis_rx_tkeep_tbs <= X"FF";
        s_axis_rx_tlast_tbs <= '0';
        s_axis_rx_tuser_tbs <= "11" & X"60004";
        s_axis_rx_tvalid_tbs <= '1';
        wait for period;
        step <= 41;
        s_axis_rx_tdata_tbs <= data_1;
        wait for period;
        s_axis_rx_tdata_tbs <=  X"0200EFBE" & X"0100EFBE";
        wait for period;
        s_axis_rx_tdata_tbs <=  X"0400EFBE" & X"0300EFBE";
        s_axis_rx_tkeep_tbs <= X"FF";
        s_axis_rx_tlast_tbs <= '1';
        wait for period;
		step <= 42;
		s_axis_rx_tdata_tbs <= X"000F000000A00001";
        s_axis_rx_tkeep_tbs <= X"FF";
        s_axis_rx_tlast_tbs <= '0';
        s_axis_rx_tuser_tbs <= "11" & X"60000";
        s_axis_rx_tvalid_tbs <= '0';
		wait for period;
		

        
        step <= 60;
        s_axis_rx_tdata_tbs <= X"000000204a000002";
        s_axis_rx_tkeep_tbs <= X"FF";
        s_axis_rx_tlast_tbs <= '0';
        s_axis_rx_tvalid_tbs <= '1';
        wait for period;
        
        step <= 61;
        s_axis_rx_tdata_tbs <= X"701b000001000038";
        wait for period;
        
        step <= 62;
        s_axis_rx_tdata_tbs <= X"2b55629a0060bfbd";
        s_axis_rx_tkeep_tbs <= X"0F";
        s_axis_rx_tlast_tbs <= '1';
        wait for period;
        
		step <= 63;
        s_axis_rx_tdata_tbs <= X"0000000000A00001";
        s_axis_rx_tkeep_tbs <= X"FF";
        s_axis_rx_tlast_tbs <= '0';
        s_axis_rx_tvalid_tbs <= '0';
        wait for 2*period;
        
        step <= 64;
        s_axis_rx_tdata_tbs <= X"000000184a000006";
        s_axis_rx_tkeep_tbs <= X"FF";
        s_axis_rx_tlast_tbs <= '0';
        s_axis_rx_tvalid_tbs <= '1';
        wait for period;
        step <= 65;
        s_axis_rx_tdata_tbs <= X"0300000001000040";
        wait for period;
        step <= 66;
        s_axis_rx_tdata_tbs <= X"00000000d0040000";
        wait for period;
        step <= 67;
        s_axis_rx_tdata_tbs <= X"0200000000000000";
        wait for period;
        step <= 68;
        s_axis_rx_tdata_tbs <= X"0d1e7fc900000000";
        s_axis_rx_tkeep_tbs <= X"0F";
        s_axis_rx_tlast_tbs <= '1';
        wait for period;

        step <= 0;
		s_axis_rx_tdata_tbs <= X"000F000000A00001";
		s_axis_rx_tkeep_tbs <= X"FF";
		s_axis_rx_tlast_tbs <= '0';
		s_axis_rx_tuser_tbs <= "11" & X"60000";
		s_axis_rx_tvalid_tbs <= '0';
		wait for period;
               
        step <= 10;
        axis_data_p (CplD,H3DW,X"0000000000000010",X"5A5AEFBE" & X"0000EFBE","00" & X"01",data_0,data_1,data_2);
        s_axis_rx_tdata_tbs <= data_0;
        s_axis_rx_tkeep_tbs <= X"FF";
        s_axis_rx_tlast_tbs <= '0';
        s_axis_rx_tvalid_tbs <= '1';
        wait for period;
        
        step <= 11;
        s_axis_rx_tdata_tbs <= data_1;
        s_axis_rx_tlast_tbs <= '1';
        wait for period;
        
        step <= 0;
        s_axis_rx_tdata_tbs <= X"000F000000A00001";
        s_axis_rx_tkeep_tbs <= X"FF";
        s_axis_rx_tlast_tbs <= '0';
        s_axis_rx_tuser_tbs <= "11" & X"60000";
        s_axis_rx_tvalid_tbs <= '0';
        wait for period;
        
        step <= 20;
        axis_data_p (CplD,H3DW,X"0000000000000010",X"5A5AEFBE" & X"0000EFBE","00" & X"02",data_0,data_1,data_2);
        s_axis_rx_tdata_tbs <= data_0;
        s_axis_rx_tkeep_tbs <= X"FF";
        s_axis_rx_tlast_tbs <= '0';
        s_axis_rx_tvalid_tbs <= '1';
        wait for period;
        
        step <= 21;
        s_axis_rx_tdata_tbs <= data_1;
        --s_axis_rx_tlast_tbs <= '1';
        wait for period;
        
        step <= 22;
        s_axis_rx_tdata_tbs <=  X"0200EFBE" & X"0100EFBE";
        s_axis_rx_tkeep_tbs <= X"0F";
        s_axis_rx_tlast_tbs <= '1';
        wait for period;
        
        step <= 0;
        s_axis_rx_tdata_tbs <= X"000F000000A00001";
        s_axis_rx_tkeep_tbs <= X"FF";
        s_axis_rx_tlast_tbs <= '0';
        s_axis_rx_tuser_tbs <= "11" & X"60000";
        s_axis_rx_tvalid_tbs <= '0';
        wait for period;
        
        step <= 30;
        axis_data_p (CplD,H3DW,X"0000000000000010",X"5A5AEFBE" & X"0000EFBE","00" & X"03",data_0,data_1,data_2);
        s_axis_rx_tdata_tbs <= data_0;
        s_axis_rx_tkeep_tbs <= X"FF";
        s_axis_rx_tlast_tbs <= '0';
        s_axis_rx_tvalid_tbs <= '1';
        wait for period;
        
        step <= 31;
        s_axis_rx_tdata_tbs <= data_1;
        --s_axis_rx_tlast_tbs <= '1';
        wait for period;
        
        step <= 32;
        s_axis_rx_tdata_tbs <=  X"0200EFBE" & X"0100EFBE";
        s_axis_rx_tkeep_tbs <= X"FF";
        s_axis_rx_tlast_tbs <= '1';
        wait for period;     
        
        step <= 0;
        s_axis_rx_tdata_tbs <= X"000F000000A00001";
        s_axis_rx_tkeep_tbs <= X"FF";
        s_axis_rx_tlast_tbs <= '0';
        s_axis_rx_tuser_tbs <= "11" & X"60000";
        s_axis_rx_tvalid_tbs <= '0';
        wait for period;
        
        step <= 40;
        axis_data_p (CplD,H3DW,X"0000000000000010",X"5A5AEFBE" & X"0000EFBE","00" & X"04",data_0,data_1,data_2);
        s_axis_rx_tdata_tbs <= data_0;
        s_axis_rx_tkeep_tbs <= X"FF";
        s_axis_rx_tlast_tbs <= '0';
        s_axis_rx_tuser_tbs <= "11" & X"60004";
        s_axis_rx_tvalid_tbs <= '1';
        wait for period;
        step <= 41;
        s_axis_rx_tdata_tbs <= data_1;
        wait for period;
        s_axis_rx_tdata_tbs <=  X"0200EFBE" & X"0100EFBE";
        wait for period;
        s_axis_rx_tdata_tbs <=  X"0400EFBE" & X"0300EFBE";
        s_axis_rx_tkeep_tbs <= X"0F";
        s_axis_rx_tlast_tbs <= '1';
        wait for period;

        step <= 0;
        s_axis_rx_tdata_tbs <= X"000F000000A00001";
        s_axis_rx_tkeep_tbs <= X"FF";
        s_axis_rx_tlast_tbs <= '0';
        s_axis_rx_tuser_tbs <= "11" & X"60000";
        s_axis_rx_tvalid_tbs <= '0';
        wait for period;
        
        step <= 50;
        axis_data_p (CplD,H3DW,X"0000000000000010",X"5A5AEFBE" & X"0000EFBE","00" & X"05",data_0,data_1,data_2);
        s_axis_rx_tdata_tbs <= data_0;
        s_axis_rx_tkeep_tbs <= X"FF";
        s_axis_rx_tlast_tbs <= '0';
        s_axis_rx_tuser_tbs <= "11" & X"60004";
        s_axis_rx_tvalid_tbs <= '1';
        wait for period;
        step <= 51;
        s_axis_rx_tdata_tbs <= data_1;
        wait for period;
        s_axis_rx_tdata_tbs <=  X"0200EFBE" & X"0100EFBE";
        wait for period;
        s_axis_rx_tdata_tbs <=  X"0400EFBE" & X"0300EFBE";
        s_axis_rx_tkeep_tbs <= X"FF";
        s_axis_rx_tlast_tbs <= '1';
        wait for period;
        
        step <= 60;
        axis_data_p (CplD,H3DW,X"0000000000000010",X"5A5AEFBE" & X"0000EFBE","00" & X"06",data_0,data_1,data_2);
        s_axis_rx_tdata_tbs <= data_0;
        s_axis_rx_tkeep_tbs <= X"FF";
        s_axis_rx_tlast_tbs <= '0';
        s_axis_rx_tuser_tbs <= "11" & X"60004";
        s_axis_rx_tvalid_tbs <= '1';
        wait for period;
        step <= 61;
        s_axis_rx_tdata_tbs <= data_1;
        wait for period;
        s_axis_rx_tdata_tbs <=  X"0200EFBE" & X"0100EFBE";
        wait for period;
        s_axis_rx_tdata_tbs <=  X"0400EFBE" & X"0300EFBE";
        wait for period;
        s_axis_rx_tdata_tbs <=  X"0600EFBE" & X"0500EFBE";
        s_axis_rx_tkeep_tbs <= X"0F";
        s_axis_rx_tlast_tbs <= '1';
        wait for period;

        step <= 0;
        s_axis_rx_tdata_tbs <= X"000F000000A00001";
        s_axis_rx_tkeep_tbs <= X"FF";
        s_axis_rx_tlast_tbs <= '0';
        s_axis_rx_tuser_tbs <= "11" & X"60000";
        s_axis_rx_tvalid_tbs <= '0';
        wait for period;
		
		step <= 100;
		s_axis_rx_tdata_tbs <= X"000F000000A00001";
		s_axis_rx_tkeep_tbs <= X"FF";
		s_axis_rx_tlast_tbs <= '0';
		s_axis_rx_tuser_tbs <= "11" & X"60000";
		s_axis_rx_tvalid_tbs <= '0';
		wait;
		
	end process stimuli_p;
	
	dut1:p2l_decoder
	port map(
		clk_i => clk_tbs,
		rst_i => rst_tbs,
		-- Slave AXI-Stream
		s_axis_rx_tdata_i => s_axis_rx_tdata_tbs,
		s_axis_rx_tkeep_i => s_axis_rx_tkeep_tbs,
		s_axis_rx_tlast_i => s_axis_rx_tlast_tbs,
		s_axis_rx_tready_o => s_axis_rx_ready_s,
		s_axis_rx_tuser_i => s_axis_rx_tuser_tbs,
		s_axis_rx_tvalid_i => s_axis_rx_tvalid_tbs,
		-- To the wishbone master
        pd_wbm_address_o => pd_wbm_address_s,
        pd_wbm_data_o => pd_wbm_data_s,
        pd_pdm_keep_o => pd_pdm_keep_s,
        pd_wbm_valid_o => pd_wbm_valid_s,
        wbm_pd_ready_i => wbm_pd_ready_tbs,
        pd_op_o => pd_op_s,
        pd_header_type_o => pd_header_type_s,
        pd_payload_length_o => pd_payload_length_s,

		-- L2P DMA
		pd_pdm_data_valid_o => pd_pdm_data_valid_s,
        pd_pdm_data_last_o => pd_pdm_data_last_s,
        pd_pdm_data_o => pd_pdm_data_s
	);
	
	
	
end Behavioral;
