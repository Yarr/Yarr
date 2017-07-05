library IEEE;
USE IEEE.STD_LOGIC_1164.all;
USE IEEE.NUMERIC_STD.all;

use work.wshexp_core_pkg.all;

entity p2l_decoder is
	Port (
		clk_i : in STD_LOGIC;
		rst_i : in STD_LOGIC;
		-- From Slave AXI-Stream
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
        pd_wbm_hdr_tag_o    : out std_logic_vector(7 downto 0);
        pd_wbm_target_mrd_o : out std_logic;                      -- Target memory read
        pd_wbm_target_mwr_o : out std_logic;                      -- Target memory write
		wbm_pd_ready_i : in std_logic;
		-- to L2P DMA
		pd_pdm_data_valid_o  : out std_logic;                      -- Indicates Data is valid
		pd_pdm_data_valid_w_o  : out std_logic_vector(1 downto 0);
        pd_pdm_data_last_o   : out std_logic;                      -- Indicates end of the packet
		pd_pdm_keep_o		 : out std_logic_vector(7 downto 0);
        pd_pdm_data_o        : out std_logic_vector(63 downto 0);  -- Data
		--debug outputs
		states_do : out STD_LOGIC_VECTOR(3 downto 0);
		pd_op_o : out STD_LOGIC_VECTOR(2 downto 0);
        pd_header_type_o : out STD_LOGIC;
        pd_payload_length_o : out STD_LOGIC_VECTOR(9 downto 0)
	);
end p2l_decoder;

architecture Behavioral of p2l_decoder is

	constant axis_data_width_c : integer := 64;
	constant address_mask_c : STD_LOGIC_VECTOR(64-1 downto 0) := X"00000000" & X"000FFFFF"; -- depends on pcie memory size
		
	constant fmt_h3dw_nodata_c : STD_LOGIC_VECTOR (3 - 1 downto 0):= "000";
	constant fmt_h3dw_data_c : STD_LOGIC_VECTOR (3 - 1 downto 0):= "001";
	constant fmt_h4dw_nodata_c : STD_LOGIC_VECTOR (3 - 1 downto 0):= "010";
	constant fmt_h4dw_data_c : STD_LOGIC_VECTOR (3 - 1 downto 0):= "011";
	constant fmt_tlp_prefix_c : STD_LOGIC_VECTOR (3 - 1 downto 0):= "100";
	
	constant tlp_type_Mr_c : STD_LOGIC_VECTOR (5 - 1 downto 0):= "00000";
	constant tlp_type_Cpl_c : STD_LOGIC_VECTOR (5 - 1 downto 0):= "01010";
	
	type state_t is (idle,hd0_rx, hd1_rx, lastdata_rx, data_rx);
	signal state_s : state_t;
	signal previous_state_s : state_t;
	signal payload_length_s : STD_LOGIC_VECTOR(9 downto 0);
	signal bar_hit_s : STD_LOGIC_VECTOR(6 downto 0);
	type tlp_type_t is (MRd,MRdLk,MWr,IORd,IOWr,CfgRd0,CfgWr0,CfgRd1,CfgWr1,TCfgRd,TCfgWr,Msg,MsgD,Cpl,CplD,CplLk,CplDLk,LPrfx,unknown);
	signal tlp_type_s : tlp_type_t;
	type header_t is (H3DW,H4DW);
	signal header_type_s : header_t;
	type bool_t is (false,true);
	signal payload_s : bool_t;
	signal tlp_prefix : bool_t;
	signal address_s : STD_LOGIC_VECTOR(64-1 downto 0); 
	signal data_s : STD_LOGIC_VECTOR(32-1 downto 0);
	
	signal s_axis_rx_tdata_s   : STD_LOGIC_VECTOR (axis_data_width_c - 1 downto 0);
	signal s_axis_rx_tkeep_s   : STD_LOGIC_VECTOR (axis_data_width_c/8 - 1 downto 0);
	signal s_axis_rx_tkeep_1_s : STD_LOGIC_VECTOR (axis_data_width_c/8 - 1 downto 0);
	signal s_axis_rx_tuser_s   : STD_LOGIC_VECTOR (21 downto 0);
	signal s_axis_rx_tvalid_s  : STD_LOGIC;
	signal s_axis_rx_tlast_s   : STD_LOGIC;
	signal s_axis_rx_tready_s  : STD_LOGIC;
	signal s_axis_rx_tlast_1_s : STD_LOGIC;
	signal s_axis_rx_tdata_0_s : STD_LOGIC_VECTOR (axis_data_width_c - 1 downto 0);
	signal s_axis_rx_tdata_1_s : STD_LOGIC_VECTOR (axis_data_width_c - 1 downto 0);
	signal pd_pdm_keep_0_s     : std_logic_vector(7 downto 0);
    signal pd_pdm_keep_1_s     : std_logic_vector(7 downto 0);
    signal pd_op_s             : STD_LOGIC_VECTOR(2 downto 0);
    signal pd_wbm_hdr_rid_s    : std_logic_vector(15 downto 0);  -- Requester ID
    --signal pd_wbm_hdr_cid_s    : std_logic_vector(15 downto 0);  -- Completer ID
    signal pd_wbm_hdr_tag_s    : std_logic_vector(7 downto 0);
    signal pd_wbm_target_mrd_s : std_logic;                      -- Target memory read
    signal pd_wbm_target_mwr_s : std_logic;
    
    signal data_cnt_s : unsigned(9 downto 0);
	
	signal byte_swap_c : STD_LOGIC_VECTOR (1 downto 0);
begin
    byte_swap_c <= "11";

	state_p:process(rst_i,clk_i) 
	begin
		if rst_i = '1' then
			--if s_axis_rx_tvalid_i = '1' then
			     state_s <= idle;
			--end if;
		elsif clk_i = '1' and clk_i'event then
			case state_s is
				when idle =>
				    state_s <= hd0_rx;
				when hd0_rx =>
					if s_axis_rx_tvalid_s = '1' then
							state_s <= hd1_rx; -- 
					end if;
				when hd1_rx =>
					--if s_axis_rx_tvalid_s = '0' then
						--state_s <= hd1_rx;
					if s_axis_rx_tvalid_s = '1' then
					   if s_axis_rx_tvalid_i = '1' and s_axis_rx_tlast_i = '1' then
                            state_s <= lastdata_rx;
                        elsif s_axis_rx_tlast_s = '0' then
                            state_s <= data_rx; -- TODO: MORE DATA
                        elsif s_axis_rx_tlast_s = '1' then
                            --if (tlp_type_s = MRd or tlp_type_s = MWr) then
                                --state_s <= wait_done;
                            --else
                                state_s <= hd0_rx;
                            --end if;
                            
                        end if;
					end if;
				when lastdata_rx => 
					state_s <= hd0_rx;
				when data_rx =>
					if s_axis_rx_tlast_i = '1' then
						state_s <= lastdata_rx;
					end if;
			end case;
		end if;		
	end process state_p;
	
	delay_p: process(clk_i,rst_i)
	begin
		if rst_i = '1' then
		    s_axis_rx_tdata_s <= (others => '0');
            s_axis_rx_tkeep_s <= (others => '0');
            s_axis_rx_tuser_s <= (others => '0');
            s_axis_rx_tvalid_s <= '0';
            s_axis_rx_tlast_s <= '0';
            previous_state_s <= hd1_rx;
		elsif clk_i = '1' and clk_i'event then
			s_axis_rx_tdata_s <= s_axis_rx_tdata_i;
			s_axis_rx_tkeep_s <= s_axis_rx_tkeep_i;
			s_axis_rx_tkeep_1_s <= s_axis_rx_tkeep_s;
			s_axis_rx_tuser_s <= s_axis_rx_tuser_i;
			s_axis_rx_tvalid_s <= s_axis_rx_tvalid_i;
			s_axis_rx_tlast_s <= s_axis_rx_tlast_i;
			s_axis_rx_tlast_1_s <= s_axis_rx_tlast_s;
			previous_state_s <= state_s;
		end if;
	end process delay_p;
	
	data_counter_p : process(clk_i,rst_i)
    begin
        if rst_i = '1' then
            data_cnt_s <= (others => '0');
        elsif clk_i = '1' and clk_i'event then
            case state_s is
                when hd0_rx =>
                   data_cnt_s <= unsigned(s_axis_rx_tdata_s(9 downto 0));
                when hd1_rx =>
                   data_cnt_s <= data_cnt_s - 1;
                when data_rx =>
                    if s_axis_rx_tvalid_s = '1' then
                        --if s_axis_rx_tkeep_s = X"0F" then
                            --data_cnt_s <= data_cnt_s - 1;
                        --elsif s_axis_rx_tkeep_s = X"FF" then
                            data_cnt_s <= data_cnt_s - 2;
                        --end if;
                    end if; 
                
                when lastdata_rx =>
                    if s_axis_rx_tvalid_s = '1' then
                        if s_axis_rx_tkeep_s = X"0F" then
                            data_cnt_s <= data_cnt_s - 1;
                        elsif s_axis_rx_tkeep_s = X"FF" then
                            data_cnt_s <= data_cnt_s - 2;
                        end if;
                    end if; 
                when others =>
            end case;
            
  
        end if;
    end process data_counter_p;
	
	reg_p: process(rst_i,clk_i)
	begin
		if rst_i = '1' then
			address_s <= (others => '0');
			tlp_type_s <= unknown;
			header_type_s <= H4DW;
			data_s <= (others => '0');
			pd_wbm_hdr_rid_s <= (others => '0');
			pd_wbm_hdr_tag_s <= (others => '0');
			pd_wbm_target_mrd_s <= '0';
			pd_wbm_target_mwr_s <= '0';
		elsif clk_i = '1' and clk_i'event then
			case state_s is
				when idle =>
				    address_s <= (others => '0');
                    tlp_type_s <= unknown;
                    header_type_s <= H4DW;
                    data_s <= (others => '0');
                    pd_wbm_hdr_rid_s <= (others => '0');
                    pd_wbm_hdr_tag_s <= (others => '0');
                    pd_wbm_target_mrd_s <= '0';
                    pd_wbm_target_mwr_s <= '0';
				when hd0_rx =>
					bar_hit_s <= s_axis_rx_tuser_s(8 downto 2);
					payload_length_s <= s_axis_rx_tdata_s(9 downto 0);
					pd_wbm_hdr_rid_s <= s_axis_rx_tdata_s(63 downto 48);
					pd_wbm_target_mrd_s <= '0';
                    pd_wbm_target_mwr_s <= '0';
					case s_axis_rx_tdata_s(31 downto 24) is
						when "00000000" =>
							tlp_type_s <= MRd;
							header_type_s <= H3DW;
							pd_wbm_target_mrd_s <= '1';
							
						when "00100000" =>
							tlp_type_s <= MRd;
							header_type_s <= H4DW;

						when "00000001" =>
							tlp_type_s <= MRdLk;
							header_type_s <= H3DW;

						when "00100001" =>
							tlp_type_s <= MRdLk;
							header_type_s <= H4DW;

						when "01000000" =>
							tlp_type_s <= MWr;
							header_type_s <= H3DW;
							pd_wbm_target_mwr_s <= '1';

						when "01100000" =>
							tlp_type_s <= MWr;	
							header_type_s <= H4DW;

						when "00000010" =>
							tlp_type_s <= IORd;
							header_type_s <= H3DW;

						when "01000010" =>
							tlp_type_s <= IOWr;
							header_type_s <= H3DW;

						when "00000100" =>
							tlp_type_s <= CfgRd0;
							header_type_s <= H3DW;

						when "01000100" =>
							tlp_type_s <= CfgWr0;
							header_type_s <= H3DW;

						when "00000101" =>
							tlp_type_s <= CfgRd1;
							header_type_s <= H3DW;

						when "01000101" =>
							tlp_type_s <= CfgWr1;
							header_type_s <= H3DW;
						when "00011011" =>
							tlp_type_s <= TCfgRd;
							header_type_s <= H3DW;

						when "01011011" =>
							tlp_type_s <= TCfgWr;
							header_type_s <= H3DW;

						when "00001010" =>
							tlp_type_s <= Cpl;
							header_type_s <= H3DW;

						when "01001010" =>
							tlp_type_s <= CplD;
							header_type_s <= H3DW;

						when "00001011" =>
							tlp_type_s <= CplLk;
							header_type_s <= H3DW;

						when "01001011" =>
							tlp_type_s <= CplDLk;
							header_type_s <= H3DW;

						when others =>
							if s_axis_rx_tdata_s(31 downto 27) = "00110" then
								tlp_type_s <= Msg;
								header_type_s <= H4DW;
							elsif s_axis_rx_tdata_s(31 downto 27) = "01110" then
								tlp_type_s <= MsgD;
								header_type_s <= H4DW;
							elsif s_axis_rx_tdata_s(31 downto 28) = "1000" then
								tlp_type_s <= LPrfx;
								header_type_s <= H3DW;
							else
								tlp_type_s <= unknown;
								header_type_s <= H4DW;
							end if;
					end case;
				when hd1_rx =>
					if header_type_s = H3DW then -- d0h2_rx
						--wb_dat_o_s <= X"00000000" & s_axis_rx_tdata_s(63 downto 32); -- 64bit
						address_s <= X"00000000" & s_axis_rx_tdata_s(31 downto 0) and address_mask_c; -- see 2.2.4.1. in pcie spec
						address_s(1 downto 0) <= "00";
						data_s <= s_axis_rx_tdata_s(63 downto 32);
					else -- H4DW h3h2_rx
						address_s(63 downto 32) <= s_axis_rx_tdata_s(31 downto 0) and address_mask_c(63 downto 32);
						address_s(31 downto 0) <= s_axis_rx_tdata_s(63 downto 36) & "0000" and address_mask_c(31 downto 0); 
					end if;

				when data_rx =>
				
				when lastdata_rx =>
				
				when others =>

			end case;
		end if;
	end process reg_p;
	
    pd_header_type_o <= '1' when header_type_s = H4DW else '0';
    
    pd_wbm_hdr_rid_o    <= pd_wbm_hdr_rid_s;
    pd_wbm_hdr_tag_o    <= pd_wbm_hdr_tag_s;
    pd_wbm_target_mrd_o <= pd_wbm_target_mrd_s;
    pd_wbm_target_mwr_o <= pd_wbm_target_mwr_s;
	
	
	p2l_data_delay_p : process(clk_i)
	begin
		if (clk_i'event and clk_i = '1') then
			--if (s_axis_rx_tvalid_i = '1') then
				s_axis_rx_tdata_0_s <= s_axis_rx_tdata_i;
				s_axis_rx_tdata_1_s <= s_axis_rx_tdata_0_s;
				pd_pdm_keep_0_s <= s_axis_rx_tkeep_i;
                pd_pdm_keep_1_s <= pd_pdm_keep_0_s;
			--end if;
		end if;
	end process p2l_data_delay_p;
  
	
	pd_pdm_data_o <= 
          f_byte_swap(true,data_s, byte_swap_c) & f_byte_swap(true,data_s, byte_swap_c) when payload_length_s = "0000000001" else 
          f_byte_swap(true, s_axis_rx_tdata_0_s(31 downto 0), byte_swap_c) & f_byte_swap(true, s_axis_rx_tdata_1_s(63 downto 32), byte_swap_c);
    pd_pdm_data_valid_o <= s_axis_rx_tvalid_s when (state_s = data_rx or state_s = lastdata_rx) and pd_op_s = "011"
                          else '0';
    pd_pdm_data_last_o <= 
       s_axis_rx_tlast_s when ( state_s = lastdata_rx and s_axis_rx_tkeep_i = X"0F" and pd_op_s = "011") else
       '1' when s_axis_rx_tkeep_1_s = X"FF" and s_axis_rx_tlast_1_s = '1'and pd_op_s = "011" else
       '0';
    
    pd_pdm_data_valid_w_o(1) <= 
      s_axis_rx_tvalid_s when (state_s = data_rx or state_s = lastdata_rx) and pd_op_s = "011" and pd_pdm_keep_0_s(3 downto 0) = X"F" else
      '0';
      
    pd_pdm_data_valid_w_o(0) <= 
      s_axis_rx_tvalid_s when (state_s = data_rx or state_s = lastdata_rx) and pd_op_s = "011" and pd_pdm_keep_1_s(7 downto 4) = X"F" else
      '1' when pd_op_s = "011" and s_axis_rx_tlast_s = '1' and  state_s = hd0_rx else
      '1' when s_axis_rx_tkeep_1_s = X"FF" and s_axis_rx_tlast_1_s = '1'and pd_op_s = "011" else 
      '0';

	pd_pdm_keep_o <= pd_pdm_keep_0_s(3 downto 0) & pd_pdm_keep_1_s(7 downto 4);
	
	pd_wbm_valid_o <= '1' when previous_state_s = hd1_rx and s_axis_rx_tlast_1_s = '1' and (pd_op_s = "001" or pd_op_s = "010") else '0';
	
	
	s_axis_rx_tready_s <= wbm_pd_ready_i;
	s_axis_rx_tready_o <= s_axis_rx_tready_s;
	
	pd_payload_length_o <= payload_length_s;
	pd_wbm_address_o <= address_s;
	pd_wbm_data_o <= f_byte_swap(true,data_s, byte_swap_c);
	
	pd_op_o <= pd_op_s;
	
    debug_output_p:process (state_s,header_type_s,tlp_type_s)
    begin
        case tlp_type_s is
            when MRd =>
                pd_op_s <= "001";
            when MWr =>
                pd_op_s <= "010";
            when CplD =>
                pd_op_s <= "011";
            when others =>
                pd_op_s <= "000";
        end case;
        case state_s is
            when idle =>
                states_do <= "0000";
            when hd0_rx =>
                states_do <= "0001";
            when hd1_rx =>
                states_do <= "0010";
            when data_rx =>
                states_do <= "0011";
            when lastdata_rx => 
                states_do <= "0100";
         end case;
    end process debug_output_p;
	

end;
