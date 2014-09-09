-- ####################################
-- # Project: Yarr
-- # Author: Timon Heim
-- # E-Mail: timon.heim at cern.ch
-- # Comments: Serial Port
-- # Outputs are synchronous to clk_i
-- ####################################
-- # Adress Map:
-- # Adr[8:4]: channel number 0 to 31
-- # Adr[3:0]:
-- #   0x0 - FiFo (WO)
-- #   0x1 - Enable (RW)
-- #   0x2 - Underrun (RO)
-- #   0x3 - Overrun (RO)
-- #   0x4 - Empty (RO)

library IEEE;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity wb_tx_core is
	generic (
		g_NUM_TX : integer range 1 to 32 := 1
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
		
		-- TX
		tx_clk_i	: in  std_logic;
		tx_data_o	: out std_logic_vector(g_NUM_TX-1 downto 0)
	);
end wb_tx_core;

architecture behavioral of wb_tx_core is
	component tx_channel
		port (
			-- Sys connect
			wb_clk_i	: in  std_logic;
			rst_n_i		: in  std_logic;
			
			-- Data In
			wb_dat_i	: in std_logic_vector(31 downto 0);
			wb_wr_en_i	: in std_logic;
			
			-- TX
			tx_clk_i		: in  std_logic;
			tx_data_o		: out std_logic;
			tx_enable_i		: in std_logic;
			
			-- Status
			tx_underrun_o	: out std_logic;
			tx_overrun_o	: out std_logic;
			tx_almost_full_o : out std_logic;
			tx_empty_o	: out std_logic
		);
	end component;
	
	-- Signals
	signal tx_enable : std_logic_vector(31 downto 0) := (others => '0');
	signal tx_underrun : std_logic_vector(31 downto 0) := (others => '0');
	signal tx_overrun : std_logic_vector(31 downto 0) := (others => '0');
	signal tx_almost_full : std_logic_vector(31 downto 0) := (others => '0');
	signal tx_empty	: std_logic_vector(31 downto 0) := (others => '0');
	
	signal wb_wr_en	: std_logic_vector(31 downto 0) := (others => '0');
	
	signal channel : integer range 0 to 31;

begin

	channel <= TO_INTEGER(unsigned(wb_adr_i(8 downto 4)));
	wb_stall_o <= '1' when (tx_almost_full /= x"00000000") else '0';
	
	wb_proc: process (wb_clk_i, rst_n_i)
	begin
		if (rst_n_i = '0') then
			wb_dat_o <= (others => '0');
			wb_ack_o <= '0';
			wb_wr_en <= (others => '0');
			tx_enable <= (others => '0');
		elsif rising_edge(wb_clk_i) then
			wb_wr_en <= (others => '0');
			wb_ack_o <= '0';
			if (wb_cyc_i = '1' and wb_stb_i = '1') then
				if (wb_we_i = '1') then
					if (wb_adr_i(3 downto 0) = x"0") then -- Write to fifo
						wb_wr_en(channel) <= '1';
						wb_ack_o <= '1';
					elsif (wb_adr_i(3 downto 0) = x"1") then -- Set enable mask
						tx_enable <= wb_dat_i;
						wb_ack_o <= '1';
					else
						wb_ack_o <= '1';
					end if;
				else
					if (wb_adr_i(3 downto 0) = x"1") then -- Read enable mask
						wb_dat_o <= tx_enable;
						wb_ack_o <= '1';
					elsif (wb_adr_i(3 downto 0) = x"2") then -- Read underrun stat
						wb_dat_o <= tx_underrun;
						wb_ack_o <= '1';
					elsif (wb_adr_i(3 downto 0) = x"3") then -- Read overrun stat
						wb_dat_o <= tx_overrun;
						wb_ack_o <= '1';
					elsif (wb_adr_i(3 downto 0) = x"4") then -- Read empty stat
						wb_dat_o <= tx_empty;
						wb_ack_o <= '1';
					else
						wb_dat_o <= x"DEADBEEF";
						wb_ack_o <= '1';
					end if;
				end if;
			end if;
		end if;
	end process wb_proc;

	tx_channels: for I in 0 to g_NUM_TX-1 generate
	begin
		cmp_tx_channel: tx_channel PORT MAP (
			-- Sys connect
			wb_clk_i => wb_clk_i,
			rst_n_i => rst_n_i,
			-- Data In
			wb_dat_i => wb_dat_i,
			wb_wr_en_i => wb_wr_en(I),
			-- TX
			tx_clk_i => tx_clk_i,
			tx_data_o => tx_data_o(I),
			tx_enable_i => tx_enable(I),
			-- Status
			tx_underrun_o => tx_underrun(I),
			tx_overrun_o => tx_overrun(I),
			tx_almost_full_o => tx_almost_full(I),
			tx_empty_o => tx_empty(I)
		);
	end generate tx_channels;

end behavioral;