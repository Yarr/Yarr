-- ####################################
-- # Project: Yarr
-- # Author: Timon Heim
-- # E-Mail: timon.heim at cern.ch
-- # Comments: Single tx_channel
-- ####################################

library IEEE;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity tx_channel is
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
end tx_channel;

architecture rtl of tx_channel is
	-- Components
	component serial_port
	generic (
        g_PORT_WIDTH : integer := 32
    );
	port (
        -- Sys connect
        clk_i       : in std_logic;
        rst_n_i     : in std_logic;
        -- Input
        enable_i    : in std_logic;
        data_i      : in std_logic_vector(31 downto 0);
        data_valid_i : in std_logic;
        -- Output
        data_o      : out std_logic;
        data_read_o   : out std_logic
    );
	end component;
	
	component tx_fifo
	port (
		rst : IN STD_LOGIC;
		wr_clk : IN STD_LOGIC;
		rd_clk : IN STD_LOGIC;
		din : IN STD_LOGIC_VECTOR(31 DOWNTO 0);
		wr_en : IN STD_LOGIC;
		rd_en : IN STD_LOGIC;
		dout : OUT STD_LOGIC_VECTOR(31 DOWNTO 0);
		full : OUT STD_LOGIC;
		empty : OUT STD_LOGIC;
		prog_full : OUT STD_LOGIC
	);
	end component;
	
	signal tx_fifo_rd : std_logic;
	signal tx_fifo_wr : std_logic;
	signal tx_fifo_din : std_logic_vector(31 downto 0);
	signal tx_fifo_dout : std_logic_vector(31 downto 0);
	signal tx_fifo_full : std_logic;
	signal tx_fifo_empty : std_logic;
	signal tx_fifo_almost_full : std_logic;
	
begin

	-- Write to FiFo
	tx_fifo_wr <= wb_wr_en_i;
	tx_fifo_din <= wb_dat_i;
	
	
	-- Status outputs
	tx_underrun_o <= tx_fifo_rd and tx_fifo_empty;
	tx_overrun_o <= tx_fifo_wr and tx_fifo_full;
	tx_almost_full_o <= tx_fifo_almost_full;
	tx_empty_o <= tx_fifo_empty;
	
	cmp_sport: serial_port PORT MAP(
		clk_i => tx_clk_i,
		rst_n_i => rst_n_i,
		enable_i => tx_enable_i,
		data_i => tx_fifo_dout,
		data_valid_i => not tx_fifo_empty,
		data_o => tx_data_o,
		data_read_o => tx_fifo_rd
	);
	
	cmp_tx_fifo : tx_fifo PORT MAP (
		rst => not rst_n_i,
		wr_clk => wb_clk_i,
		rd_clk => tx_clk_i,
		din => tx_fifo_din,
		wr_en => tx_fifo_wr,
		rd_en => tx_fifo_rd,
		dout => tx_fifo_dout,
		full => tx_fifo_full,
		empty => tx_fifo_empty,
		prog_full => tx_fifo_almost_full
	);
end rtl;