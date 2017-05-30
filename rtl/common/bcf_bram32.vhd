----------------------------------------------------------------------------------
-- Company: 			University of Wuppertal
-- Engineer: 			Timon Heim
-- E-Mail:				heim@physik.uni-wuppertal.de
--
-- Project:				IBL BOC firmware
-- Module:				Block RAM
-- Description:		Block RAM with Wishbone Slave Interface
----------------------------------------------------------------------------------
-- Changelog:
-- 20.02.2011 - Initial Version
----------------------------------------------------------------------------------
-- TODO:
-- 20.02.2011 - Add DMA capability
----------------------------------------------------------------------------------
-- Address Map:
-- 0x020 to 0x02F
----------------------------------------------------------------------------------


library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use ieee.std_logic_arith.all;
use ieee.std_logic_unsigned.all;

--library work;
--use work.bocpack.all;

entity bram_wbs32 is
	generic (
		constant ADDR_WIDTH : integer := 16;
		constant DATA_WIDTH : integer := 32 
	);
	port (
		-- SYS CON
		clk			: in std_logic;
		rst			: in std_logic;
		
		-- Wishbone Slave in
		wb_adr_i			: in std_logic_vector(ADDR_WIDTH-1 downto 0);
		wb_dat_i			: in std_logic_vector(DATA_WIDTH-1 downto 0);
		wb_we_i			: in std_logic;
		wb_stb_i			: in std_logic;
		wb_cyc_i			: in std_logic; 
		wb_lock_i		: in std_logic; -- nyi
		
		-- Wishbone Slave out
		wb_dat_o			: out std_logic_vector(DATA_WIDTH-1 downto 0);
		wb_ack_o			: out std_logic		
	);
end bram_wbs32;
	 
architecture Behavioral of bram_wbs32 is

	
   type ram_type is array (2**ADDR_WIDTH-1 downto 0) of std_logic_vector (DATA_WIDTH-1 downto 0);
   signal RAM: ram_type;
	
	signal ADDR : std_logic_vector(ADDR_WIDTH-1 downto 0);
	 
begin
	
	ADDR <= wb_adr_i(ADDR_WIDTH-1 downto 0);
	
	bram: process (clk, rst)
	begin
		if (rst ='1') then
			wb_ack_o <= '0';
			for i in 0 to 2**ADDR_WIDTH-1 loop
				RAM(i) <= conv_std_logic_vector(i,RAM(i)'length); -- "DEAD0001BEEF0001"
                RAM(i)(DATA_WIDTH-1 downto DATA_WIDTH/2) <= conv_std_logic_vector(i,RAM(i)'length/2);
                RAM(i)(DATA_WIDTH-1 downto DATA_WIDTH-4*4) <= x"DEAD";
                RAM(i)(DATA_WIDTH-1-DATA_WIDTH/2 downto DATA_WIDTH-4*4-DATA_WIDTH/2) <= x"BEEF";
			end loop;
		elsif (clk'event and clk = '1') then
			if (wb_stb_i = '1' and wb_cyc_i = '1') then
				wb_ack_o <= '1';
				if (wb_we_i = '1') then
					RAM(conv_integer(ADDR)) <= wb_dat_i;
				end if;
				wb_dat_o <= RAM(conv_integer(ADDR)) ;
			else
				wb_ack_o <= '0';
			end if;
		end if;
	end process bram;

end Behavioral;
