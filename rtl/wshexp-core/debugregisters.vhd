----------------------------------------------------------------------------------
-- Company: 
-- Engineer: 
-- 
-- Create Date: 04/28/2017 01:43:25 PM
-- Design Name: 
-- Module Name: debugregisters - Behavioral
-- Project Name: 
-- Target Devices: 
-- Tool Versions: 
-- Description: 
-- 
-- Dependencies: 
-- 
-- Revision:
-- Revision 0.01 - File Created
-- Additional Comments:
-- 
----------------------------------------------------------------------------------


library IEEE;
use IEEE.STD_LOGIC_1164.ALL;

-- Uncomment the following library declaration if using
-- arithmetic functions with Signed or Unsigned values
--use IEEE.NUMERIC_STD.ALL;
use ieee.std_logic_arith.all;
use ieee.std_logic_unsigned.all;

-- Uncomment the following library declaration if instantiating
-- any Xilinx leaf cells in this code.
--library UNISIM;
--use UNISIM.VComponents.all;

entity debugregisters is
    generic (
        constant ADDR_WIDTH : integer := 4;
        constant DATA_WIDTH : integer := 32
    );
    Port ( 
    -- SYS CON
    clk            : in std_logic;
    rst            : in std_logic;
    
    -- Wishbone Slave in
    wb_adr_i            : in std_logic_vector(ADDR_WIDTH-1 downto 0);
    wb_dat_i            : in std_logic_vector(DATA_WIDTH-1 downto 0);
    wb_we_i            : in std_logic;
    wb_stb_i            : in std_logic;
    wb_cyc_i            : in std_logic; 
    
    -- Wishbone Slave out
    wb_dat_o            : out std_logic_vector(DATA_WIDTH-1 downto 0);
    wb_ack_o            : out std_logic  ;
    
    -- input/ouput  
    dummyram_sel_o      : out std_logic;
    ddr3ram_sel_o       : out std_logic;
    dummyaddress_sel_o      : out std_logic;
    dummydeadbeef_sel_o     : out std_logic;
    
    usr_led_o : out STD_LOGIC_VECTOR (3 downto 0);
    usr_sw_i : in STD_LOGIC_VECTOR (2 downto 0)--;
    
    --ddr_init_calib_complete_i : in std_logic
    );
end debugregisters;

architecture Behavioral of debugregisters is
  type ram_type is array (2**ADDR_WIDTH-1 downto 0) of std_logic_vector (DATA_WIDTH-1 downto 0);
  signal RAM: ram_type;
  
 signal ADDR : std_logic_vector(ADDR_WIDTH-1 downto 0);
 
 signal chipselect_s : std_logic;
 
 constant whisbonecompselect_addr : integer := 0;
 constant io_addr : integer := 1;
 constant ddr_status : integer := 2;
begin
	ADDR <= wb_adr_i(ADDR_WIDTH-1 downto 0);
    
    --chipselect_s <= '1' when wb_adr_i(WSH_ADDR_WIDTH-1 downto RAM_ADDR_WIDTH) = BASE_ADDRESS (WSH_ADDR_WIDTH-1 downto RAM_ADDR_WIDTH);
    
    bram: process (clk, rst)
    begin
        if (rst ='1') then
            wb_ack_o <= '0';
            for i in 0 to 2**ADDR_WIDTH-1 loop
                RAM(i) <= conv_std_logic_vector(0,RAM(i)'length);
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
            
            -- input bits
            RAM(io_addr)(6 downto 4) <= usr_sw_i;
            --RAM(ddr_status)(0) <= ddr_init_calib_complete_i;
        end if;
        
    end process bram;
    
    --output bits
    dummyram_sel_o      <= '1' when RAM(whisbonecompselect_addr)(1 downto 0) = "00" else '0';
    ddr3ram_sel_o       <= '1' when RAM(whisbonecompselect_addr)(1 downto 0) = "01" else '0';
    dummyaddress_sel_o  <= '1' when RAM(whisbonecompselect_addr)(1 downto 0) = "10" else '0';
    dummydeadbeef_sel_o <= '1' when RAM(whisbonecompselect_addr)(1 downto 0) = "11" else '0';
    
    usr_led_o <= RAM(io_addr)(3 downto 0);
    
end Behavioral;
