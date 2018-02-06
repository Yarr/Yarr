library IEEE;
use IEEE.STD_LOGIC_1164.all;
use IEEE.NUMERIC_STD.all;

use work.gn4124_core_pkg.all;

package common_pkg is

  component generic_async_fifo is
  generic (
    g_data_width : natural;
    g_size       : natural;
    g_show_ahead : boolean := false;

    -- Read-side flag selection
    g_with_rd_empty        : boolean := true;   -- with empty flag
    g_with_rd_full         : boolean := false;  -- with full flag
    g_with_rd_almost_empty : boolean := false;
    g_with_rd_almost_full  : boolean := false;
    g_with_rd_count        : boolean := false;  -- with words counter

    g_with_wr_empty        : boolean := false;
    g_with_wr_full         : boolean := true;
    g_with_wr_almost_empty : boolean := false;
    g_with_wr_almost_full  : boolean := false;
    g_with_wr_count        : boolean := false;

    g_almost_empty_threshold : integer;  -- threshold for almost empty flag
    g_almost_full_threshold  : integer   -- threshold for almost full flag
    );

  port (
    rst_n_i : in std_logic := '1';

    -- write port
    clk_wr_i : in std_logic;
    d_i      : in std_logic_vector(g_data_width-1 downto 0);
    we_i     : in std_logic;

    wr_empty_o        : out std_logic;
    wr_full_o         : out std_logic;
    wr_almost_empty_o : out std_logic;
    wr_almost_full_o  : out std_logic;
    wr_count_o        : out std_logic_vector(log2_ceil(g_size)-1 downto 0);

    -- read port
    clk_rd_i : in  std_logic;
    q_o      : out std_logic_vector(g_data_width-1 downto 0);
    rd_i     : in  std_logic;

    rd_empty_o        : out std_logic;
    rd_full_o         : out std_logic;
    rd_almost_empty_o : out std_logic;
    rd_almost_full_o  : out std_logic;
    rd_count_o        : out std_logic_vector(log2_ceil(g_size)-1 downto 0)
    );
  end component generic_async_fifo;
  
    component wb_spi
      generic (
          g_CLK_DIVIDER : positive := 20
      );
      port (
          -- Sys Connect
          wb_clk_i : in std_logic;
          rst_n_i : in std_logic;
          -- Wishbone slave interface
          wb_adr_i    : in  std_logic_vector(31 downto 0) := (others => '0');
          wb_dat_i    : in  std_logic_vector(31 downto 0) := (others => '0');
          wb_dat_o    : out std_logic_vector(31 downto 0);
          wb_cyc_i    : in  std_logic := '0';
          wb_stb_i    : in  std_logic := '0';
          wb_we_i        : in  std_logic := '0';
          wb_ack_o    : out std_logic;
          -- SPI out
          scl_o : out std_logic;
          sda_o : out std_logic;
          sdi_i : in std_logic;
          latch_o : out std_logic
      );
    end component;



end common_pkg;

package body common_pkg is


end common_pkg;

