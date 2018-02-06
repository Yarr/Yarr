------------------------------------------------------------------------------
-- Copyright (c) 2012 Xilinx, Inc.
-- This design is confidential and proprietary of Xilinx, All Rights Reserved.
------------------------------------------------------------------------------
--   ____  ____
--  /   /\/   /
-- /___/  \  /   Vendor:                Xilinx
-- \   \   \/    Version:               1.0
--  \   \        Filename:              delay_controller_wrap.vhd
--  /   /        Date Last Modified:    Mar 30, 2016
-- /___/   /\    Date Created:          Jan 8, 2013
-- \   \  /  \
--  \___\/\___\
-- 
--Device: 	7 Series
--Purpose:  	Controls delays on a per-bit basis
--		Number of bits from each seres set via an attribute
--
--Reference:	XAPP585.pdf
--    
--Revision History:
--    Rev 1.0 - First created (nicks)
--
------------------------------------------------------------------------------
--
--  Disclaimer: 
--
--		This disclaimer is not a license and does not grant any rights to the materials 
--              distributed herewith. Except as otherwise provided in a valid license issued to you 
--              by Xilinx, and to the maximum extent permitted by applicable law: 
--              (1) THESE MATERIALS ARE MADE AVAILABLE "AS IS" AND WITH ALL FAULTS, 
--              AND XILINX HEREBY DISCLAIMS ALL WARRANTIES AND CONDITIONS, EXPRESS, IMPLIED, OR STATUTORY, 
--              INCLUDING BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, NON-INFRINGEMENT, OR 
--              FITNESS FOR ANY PARTICULAR PURPOSE; and (2) Xilinx shall not be liable (whether in contract 
--              or tort, including negligence, or under any other theory of liability) for any loss or damage 
--              of any kind or nature related to, arising under or in connection with these materials, 
--              including for any direct, or any indirect, special, incidental, or consequential loss 
--              or damage (including loss of data, profits, goodwill, or any type of loss or damage suffered 
--              as a result of any action brought by a third party) even if such damage or loss was 
--              reasonably foreseeable or Xilinx had been advised of the possibility of the same.
--
--  Critical Applications:
--
--		Xilinx products are not designed or intended to be fail-safe, or for use in any application 
--		requiring fail-safe performance, such as life-support or safety devices or systems, 
--		Class III medical devices, nuclear facilities, applications related to the deployment of airbags,
--		or any other applications that could lead to death, personal injury, or severe property or 
--		environmental damage (individually and collectively, "Critical Applications"). Customer assumes 
--		the sole risk and liability of any use of Xilinx products in Critical Applications, subject only 
--		to applicable laws and regulations governing limitations on product liability.
--
--  THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS PART OF THIS FILE AT ALL TIMES.
--
------------------------------------------------------------------------------

library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.std_logic_unsigned.all ;

library unisim ;
use unisim.vcomponents.all ;

entity delay_controller_wrap is generic (
	S 			: integer := 4) ;				-- Set the number of bits
port 	(
	m_datain		:  in std_logic_vector(S-1 downto 0) ;		-- Inputs from master serdes
	s_datain		:  in std_logic_vector(S-1 downto 0) ;		-- Inputs from slave serdes
	enable_phase_detector	:  in std_logic ;				-- Enables the phase detector logic when high
	enable_monitor		:  in std_logic ;				-- Enables the eye monitoring logic when high
	reset			:  in std_logic ;				-- Reset line synchronous to clk
	clk			:  in std_logic ;				-- Global/Regional clock 
	c_delay_in		:  in std_logic_vector(4 downto 0) ;  		-- delay value found on clock line
	m_delay_out		: out std_logic_vector(4 downto 0) ;  		-- Master delay control value
	s_delay_out		: out std_logic_vector(4 downto 0) ;  		-- Master delay control value
	data_out		: out std_logic_vector(S-1 downto 0) ;  	-- Output data
	results			: out std_logic_vector(31 downto 0) ;  		-- eye monitor result data
	m_delay_1hot		: out std_logic_vector(31 downto 0) ;  		-- Master delay control value as a one-hot vector
	debug 			: out std_logic_vector(1 downto 0) ;		-- debug data
	del_mech		:  in std_logic ;				-- changes delay mechanism slightly at higher bit rates
	bt_val			:  in std_logic_vector(4 downto 0)) ;		-- Calculated bit time value for slave devices
		
end delay_controller_wrap ;

architecture arch_delay_controller_wrap of delay_controller_wrap is
	
signal	mdataouta		: std_logic_vector(S-1 downto 0) ;			
signal	mdataoutb		: std_logic  ;			
signal	mdataoutc		: std_logic_vector(S-1 downto 0) ;			
signal	sdataouta		: std_logic_vector(S-1 downto 0) ;			
signal	sdataoutb		: std_logic ;			
signal	sdataoutc		: std_logic_vector(S-1 downto 0) ;			
signal	s_ovflw 		: std_logic ;		
signal	m_delay_mux		: std_logic_vector(1 downto 0) ;					
signal	s_delay_mux		: std_logic_vector(1 downto 0) ;					
signal	data_mux		: std_logic ;					
signal	dec_run			: std_logic  ;					
signal	inc_run			: std_logic ;					
signal	eye_run			: std_logic ;					
signal	s_state			: std_logic_vector(4 downto 0) ;					
signal	pdcount			: std_logic_vector(5 downto 0) ;					
signal	m_delay_val_int		: std_logic_vector(4 downto 0) ;
signal	s_delay_val_int		: std_logic_vector(4 downto 0) ;	
signal	s_delay_val_eye		: std_logic_vector(4 downto 0) ;	
signal	meq_max			: std_logic ;					
signal	meq_min			: std_logic ;					
signal	pd_max			: std_logic ;					
signal	pd_min			: std_logic ;	
signal	delay_change		: std_logic ;					
signal	msxoria			: std_logic_vector(7 downto 0) ;					
signal	msxorda			: std_logic_vector(7 downto 0) ;					
signal	action			: std_logic_vector(1 downto 0) ;					
signal	msxor_cti		: std_logic_vector(1 downto 0) ;					
signal	msxor_ctd		: std_logic_vector(1 downto 0) ;					
signal	msxor_ctix		: std_logic_vector(1 downto 0) ;					
signal	msxor_ctdx		: std_logic_vector(1 downto 0) ;					
signal	msxor_ctiy		: std_logic_vector(2 downto 0) ;					
signal	msxor_ctdy		: std_logic_vector(2 downto 0) ;					
signal	match			: std_logic_vector(7 downto 0) ;					
signal	shifter			: std_logic_vector(31 downto 0) := (0=>'1', others => '0') ;					
signal	pd_hold			: std_logic_vector(7 downto 0) ;					
signal	res_int			: std_logic_vector(31 downto 0) := (others => '0') ;					
signal	bt_val_d2		: std_logic_vector(4 downto 0) ;
		
begin

m_delay_out <= m_delay_val_int ;
s_delay_out <= s_delay_val_int ;
results <= res_int ;
debug <= action ;
bt_val_d2 <= '0' & bt_val(4 downto 1) ;

loop2 : if S /= 8 generate		-- phase detector filter, works on changes in data only
  loop3 : for i in S to 7 generate
  msxoria(i) <= '0' ;			-- unused early bits
  msxorda(i) <= '0' ;			-- unused late bits
  end generate ;
end generate ;

loop0 : for i in 0 to S-2 generate

msxoria(i+1) <= ((not s_ovflw and ((mdataouta(i) and not mdataouta(i+1) and not sdataouta(i))   or (not mdataouta(i) and mdataouta(i+1) and     sdataouta(i)))) or 
	         (    s_ovflw and ((mdataouta(i) and not mdataouta(i+1) and not sdataouta(i+1)) or (not mdataouta(i) and mdataouta(i+1) and     sdataouta(i+1))))) ;  	-- early bits                   
msxorda(i+1) <= ((not s_ovflw and ((mdataouta(i) and not mdataouta(i+1) and     sdataouta(i))   or (not mdataouta(i) and mdataouta(i+1) and not sdataouta(i))))) or 
	         (    s_ovflw and ((mdataouta(i) and not mdataouta(i+1) and     sdataouta(i+1)) or (not mdataouta(i) and mdataouta(i+1) and not sdataouta(i+1)))) ;	-- late bits
end generate ;

msxoria(0) <= ((not s_ovflw and ((mdataoutb and not mdataouta(0) and not sdataoutb)    or (not mdataoutb and mdataouta(0) and     sdataoutb))) or 			-- first early bit
	       (    s_ovflw and ((mdataoutb and not mdataouta(0) and not sdataouta(0)) or (not mdataoutb and mdataouta(0) and     sdataouta(0))))) ;
msxorda(0) <= ((not s_ovflw and ((mdataoutb and not mdataouta(0) and     sdataoutb)    or (not mdataoutb and mdataouta(0) and not sdataoutb)))) or 			-- first late bit
	       (    s_ovflw and ((mdataoutb and not mdataouta(0) and     sdataouta(0)) or (not mdataoutb and mdataouta(0) and not sdataouta(0)))) ;

process (clk) begin
if clk'event and clk = '1' then				-- generate number of incs or decs for low 4 bits
	case (msxoria(3 downto 0)) is
		when X"0"    => msxor_cti <= "00" ;
		when X"1"    => msxor_cti <= "01" ;
		when X"2"    => msxor_cti <= "01" ;
		when X"3"    => msxor_cti <= "10" ;
		when X"4"    => msxor_cti <= "01" ;
		when X"5"    => msxor_cti <= "10" ;
		when X"6"    => msxor_cti <= "10" ;
		when X"8"    => msxor_cti <= "01" ;
		when X"9"    => msxor_cti <= "10" ;
		when X"A"    => msxor_cti <= "10" ;
		when X"C"    => msxor_cti <= "10" ;
		when others  => msxor_cti <= "11" ;
	end case ;
	case (msxorda(3 downto 0)) is
		when X"0"    => msxor_ctd <= "00" ;
		when X"1"    => msxor_ctd <= "01" ;
		when X"2"    => msxor_ctd <= "01" ;
		when X"3"    => msxor_ctd <= "10" ;
		when X"4"    => msxor_ctd <= "01" ;
		when X"5"    => msxor_ctd <= "10" ;
		when X"6"    => msxor_ctd <= "10" ;
		when X"8"    => msxor_ctd <= "01" ;
		when X"9"    => msxor_ctd <= "10" ;
		when X"A"    => msxor_ctd <= "10" ;
		when X"C"    => msxor_ctd <= "10" ;
		when others  => msxor_ctd <= "11" ;
	end case ;
	case (msxoria(7 downto 4)) is				-- generate number of incs or decs for high n bits, max 4
		when X"0"    => msxor_ctix <= "00" ;
		when X"1"    => msxor_ctix <= "01" ;
		when X"2"    => msxor_ctix <= "01" ;
		when X"3"    => msxor_ctix <= "10" ;
		when X"4"    => msxor_ctix <= "01" ;
		when X"5"    => msxor_ctix <= "10" ;
		when X"6"    => msxor_ctix <= "10" ;
		when X"8"    => msxor_ctix <= "01" ;
		when X"9"    => msxor_ctix <= "10" ;
		when X"A"    => msxor_ctix <= "10" ;
		when X"C"    => msxor_ctix <= "10" ;
		when others  => msxor_ctix <= "11" ;
	end case ;
	case (msxorda(7 downto 4)) is
		when X"0"    => msxor_ctdx <= "00" ;
		when X"1"    => msxor_ctdx <= "01" ;
		when X"2"    => msxor_ctdx <= "01" ;
		when X"3"    => msxor_ctdx <= "10" ;
		when X"4"    => msxor_ctdx <= "01" ;
		when X"5"    => msxor_ctdx <= "10" ;
		when X"6"    => msxor_ctdx <= "10" ;
		when X"8"    => msxor_ctdx <= "01" ;
		when X"9"    => msxor_ctdx <= "10" ;
		when X"A"    => msxor_ctdx <= "10" ;
		when X"C"    => msxor_ctdx <= "10" ;
		when others  => msxor_ctdx <= "11" ;
	end case ;
end if ;
end process ;

msxor_ctiy <= ('0' & msxor_cti) + ('0' & msxor_ctix) ;
msxor_ctdy <= ('0' & msxor_ctd) + ('0' & msxor_ctdx) ;

process (clk) begin            
if clk'event and clk = '1' then 
	if msxor_ctiy = msxor_ctdy then
		action <= "00" ;
	elsif msxor_ctiy > msxor_ctdy then
		action <= "01" ;
	else 
		action <= "10" ;
	end if ;
end if ;
end process ;

process (clk) begin
if clk'event and clk = '1' then 
	mdataouta <= m_datain ;
	mdataoutb <= mdataouta(S-1) ;
	sdataouta <= s_datain ;
	sdataoutb <= sdataouta(S-1) ;
end if ;
end process ;

process (clk) begin
if clk'event and clk = '1' then 					-- per bit delay shift state machine
	if reset = '1' then
		s_ovflw <= '0' ;
		pdcount <= "100000" ;
		m_delay_val_int <= c_delay_in ; 			-- initial master delay
		s_delay_val_int <= "00000" ; 				-- initial slave delay
		data_mux <= '0' ;
		m_delay_mux <= "01" ;
		s_delay_mux <= "01" ;
		s_state <= "00000" ;
		inc_run <= '0' ;
		dec_run <= '0' ;
		eye_run <= '0' ;
		pd_hold <= "00000000" ;
		s_delay_val_eye <= "00000" ;	
	else
		case (m_delay_mux) is
			when "00"   => mdataoutc <= mdataouta(S-2 downto 0) & mdataoutb ;
			when "10"   => mdataoutc <= m_datain(0) &             mdataouta(S-1 downto 1) ;
			when others => mdataoutc <= mdataouta ;
		end case ;
		case (s_delay_mux) is 
			when "00"   => sdataoutc <= sdataouta(S-2 downto 0) & sdataoutb ;
			when "10"   => sdataoutc <= s_datain(0) &             sdataouta(S-1 downto 1) ;
			when others => sdataoutc <= sdataouta ;
		end case ;
		if m_delay_val_int = bt_val then
			meq_max <= '1' ;
		else 
			meq_max <= '0' ;
		end if ;
		if m_delay_val_int = "00000" then
			meq_min <= '1' ;
		else
			meq_min <= '0' ;
		end if ;
		if pdcount = "111111" and pd_max = '0' and delay_change = '0' then
			pd_max <= '1' ;
		else 
			pd_max <= '0' ;
		end if ;
		if pdcount = "000000" and pd_min = '0' and delay_change = '0' then
			pd_min <= '1' ;
		else 
			pd_min <= '0' ;
		end if ;
		if delay_change = '1' or inc_run = '1' or dec_run = '1' or eye_run = '1' then
			pd_hold <= "11111111" ;			
			pdcount <= "100000" ; 
		elsif pd_hold(7) = '1' then
			pdcount <= "100000" ; 
			pd_hold <= pd_hold(6 downto 0) & '0' ;
		elsif action(0) = '1' and pdcount /= "111111" then							-- increment filter count
			pdcount <= pdcount + 1 ; 
		elsif action(1) = '1' and pdcount /= "000000" then							-- decrement filter count
			pdcount <= pdcount - 1 ; 
		end if ;
		if ((enable_phase_detector = '1' and pd_max = '1' and delay_change = '0') or inc_run = '1') then	-- increment delays, check for master delay = max
			delay_change <= '1' ;
			if meq_max = '0' and inc_run = '0' then
				m_delay_val_int <= m_delay_val_int + 1 ;
			else 												-- master is max
				s_state(3 downto 0) <= s_state(3 downto 0) + 1 ;
				case (s_state(3 downto 0)) is
				when "0000" => inc_run <= '1' ; s_delay_val_int <= bt_val ; 				-- indicate state machine running and set slave delay to bit time 
				when "0110" => data_mux <= '1' ; m_delay_val_int <= "00000" ;				-- change data mux over to forward slave data and set master delay to zero
				when "1001" => m_delay_mux <= m_delay_mux - 1 ;  					-- change master delay mux over to forward with a 1-bit less advance
				when "1110" => data_mux <= '0' ;  							-- change data mux over to forward master data
				when "1111" => s_delay_mux <= m_delay_mux ; inc_run <= '0' ;				-- change slave delay mux over to forward with a 1-bit less advance
				when others => inc_run <= '1' ;
				end case ;
			end if ;
		elsif ((enable_phase_detector = '1' and pd_min = '1' and delay_change = '0') or dec_run = '1') then	-- decrement delays, check for master delay = 0
			delay_change <= '1' ;
			if meq_min = '0' and dec_run = '0' then
				m_delay_val_int <= m_delay_val_int - 1 ;
			else 												-- master is zero
				s_state(3 downto 0) <= s_state(3 downto 0) + 1 ;
				case (s_state(3 downto 0)) is
				when "0000" => dec_run <= '1' ; s_delay_val_int <= "00000" ; 				-- indicate state machine running and set slave delay to zero 
				when "0110" => data_mux <= '1' ;  m_delay_val_int <= bt_val ;				-- change data mux over to forward slave data and set master delay to bit time 
				when "1001" => m_delay_mux <= m_delay_mux + 1 ;  					-- change master delay mux over to forward with a 1-bit more advance
				when "1110" => data_mux <= '0' ;  							-- change data mux over to forward master data
				when "1111" => s_delay_mux <= m_delay_mux ; dec_run <= '0' ;				-- change slave delay mux over to forward with a 1-bit less advance
				when others => dec_run <= '1' ;
				end case ;
			end if ;
		elsif enable_monitor = '1' and (eye_run = '1' or delay_change = '1') then
			delay_change <= '0' ;
			s_state <= s_state + 1 ;
			case (s_state) is
				when "00000" => eye_run <= '1' ; s_delay_val_int <= s_delay_val_eye ; 			-- indicate state machine running and set slave delay to monitor value 
				when "10110" => if match = "11111111" then res_int <= res_int or shifter ; 		-- set or clear result bit
				                else res_int <= res_int and not shifter ; end if ; 							 
				          	if s_delay_val_eye = bt_val then 					-- only monitor active taps, ie as far as btval
				          		shifter <= (0=>'1',others=>'0') ; s_delay_val_eye <= "00000" ;
				          	else shifter <= shifter(30 downto 0) & shifter(31) ; 
				          		s_delay_val_eye <= s_delay_val_eye + 1 ; end if ;		 
				          	eye_run <= '0' ; s_state <= "00000" ; 
				when others => 	eye_run <= '1' ; 
			end case ;
		else
			delay_change <= '0' ;
			if (m_delay_val_int >= bt_val_d2) and del_mech = '0' then 					-- set slave delay to 1/2 bit period beyond or behind the master delay
				s_delay_val_int <= m_delay_val_int - bt_val_d2 ;					
				s_ovflw <= '0' ;
			else 
				s_delay_val_int <= m_delay_val_int + bt_val_d2 ;          				-- slave always ahead when del_mech is '1'
				s_ovflw <= '1' ;
			end if ;
		end if ;
		if enable_phase_detector = '0' and delay_change = '0' then
			delay_change <= '1' ;
		end if ;
	end if ;
	if enable_phase_detector = '1' then
		if data_mux = '0' then
			data_out <= mdataoutc ;
		else 
			data_out <= sdataoutc ;
		end if ;
	else
		data_out <= m_datain ;	
	end if ;
end if ;
end process ;

process (clk) begin
if clk'event and clk = '1' then
	if mdataouta = sdataouta then
		match <= match(6 downto 0) & '1' ;
	else
		match <= match(6 downto 0) & '0' ;
	end if ;
end if ;
end process ;

m_delay_1hot <= X"00000001" when m_delay_val_int = "00000" else
	    	X"00000002" when m_delay_val_int = "00001" else
	    	X"00000004" when m_delay_val_int = "00010" else
	    	X"00000008" when m_delay_val_int = "00011" else
	    	X"00000010" when m_delay_val_int = "00100" else
	    	X"00000020" when m_delay_val_int = "00101" else
	    	X"00000040" when m_delay_val_int = "00110" else
	    	X"00000080" when m_delay_val_int = "00111" else
	    	X"00000100" when m_delay_val_int = "01000" else
	    	X"00000200" when m_delay_val_int = "01001" else
	    	X"00000400" when m_delay_val_int = "01010" else
	    	X"00000800" when m_delay_val_int = "01011" else
	    	X"00001000" when m_delay_val_int = "01100" else
	    	X"00002000" when m_delay_val_int = "01101" else
	    	X"00004000" when m_delay_val_int = "01110" else
	    	X"00008000" when m_delay_val_int = "01111" else
            	X"00010000" when m_delay_val_int = "10000" else
            	X"00020000" when m_delay_val_int = "10001" else
            	X"00040000" when m_delay_val_int = "10010" else
            	X"00080000" when m_delay_val_int = "10011" else
            	X"00100000" when m_delay_val_int = "10100" else
            	X"00200000" when m_delay_val_int = "10101" else
            	X"00400000" when m_delay_val_int = "10110" else
            	X"00800000" when m_delay_val_int = "10111" else
            	X"01000000" when m_delay_val_int = "11000" else
            	X"02000000" when m_delay_val_int = "11001" else
            	X"04000000" when m_delay_val_int = "11010" else
            	X"08000000" when m_delay_val_int = "11011" else
            	X"10000000" when m_delay_val_int = "11100" else
            	X"20000000" when m_delay_val_int = "11101" else
            	X"40000000" when m_delay_val_int = "11110" else
            	X"80000000" ; 
            	
end arch_delay_controller_wrap ;
