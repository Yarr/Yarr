-- ####################################
-- # Project: Yarr
-- # Author: Timon Heim
-- # E-Mail: timon.heim at cern.ch
-- # Comments: RX core
-- # Outputs are synchronous to wb_clk_i
-- ####################################
-- # Adress Map:
-- # Adr[3:0]:
-- #     0x0 : RX Enable Mask


library IEEE;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity wb_rx_core is
	generic (
		g_NUM_RX : integer range 1 to 32 := 1
	);
	port (
		-- Sys connect
		wb_clk_i	: in  std_logic;
		rst_n_i		: in  std_logic;
		
		-- Wishbone slave interface
		wb_adr_i	: in  std_logic_vector(31 downto 0);
		wb_dat_i	: in  std_logic_vector(31 downto 0);
		wb_dat_o	: out std_logic_vector(31 downto 0);
		wb_cyc_i	: in  std_logic;
		wb_stb_i	: in  std_logic;
		wb_we_i		: in  std_logic;
		wb_ack_o	: out std_logic;
		wb_stall_o	: out std_logic;
		
		-- RX IN
		rx_clk_i	: in  std_logic;
		rx_serdes_clk_i : in std_logic;
		rx_data_i	: out std_logic_vector(g_NUM_RX-1 downto 0);
		
		-- RX OUT (sync to sys_clk)
		rx_valid_o : std_logic;
		rx_data_o : out std_logic_vector(31 downto 0)
	);
end wb_rx_core;

architecture behavioral of wb_rx_core is

	signal rx_enable : std_logic_vector(31 downto 0);
	
begin
	wb_proc: process (wb_clk_i, rst_n_i)
	begin
		if (rst_n_i = '0') then
			wb_dat_o <= (others => '0');
			wb_ack_o <= '0';
			wb_wr_en <= (others => '0');
			tx_enable <= (others => '0');
			wb_dat_t <= (others => '0');
			trig_en <= '0';
		elsif rising_edge(wb_clk_i) then
			wb_wr_en <= (others => '0');
			wb_ack_o <= '0';
			if (wb_cyc_i = '1' and wb_stb_i = '1') then
				if (wb_we_i = '1') then
					if (wb_adr_i(3 downto 0) = x"0") then -- Set enable mask
						wb_ack_o <= '1';
						rx_enable <= wb_dat_i;
					else
						wb_ack_o <= '1';
					end if;
				else
					if (wb_adr_i(3 downto 0) = x"0") then -- Read enable mask
						wb_dat_o <= rx_enable;
						wb_ack_o <= '1';
					else
						wb_dat_o <= x"DEADBEEF";
						wb_ack_o <= '1';
					end if;
				end if;
			end if;
		end if;
	end process wb_proc;
end behavioral;

