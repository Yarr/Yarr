--------------------------------------------------------------------------------
-- CERN (BE-CO-HT)
-- Generic asynchronous FIFO wrapper
-- http://www.ohwr.org/projects/fmc-adc-100m14b4cha
--------------------------------------------------------------------------------
--
-- unit name: generic_async_fifo (generic_async_fifo_wrapper.vhd)
--
-- author: Matthieu Cattin (matthieu.cattin@cern.ch)
--
-- date: 05-12-2011
--
-- version: 1.0
--
-- description: Wrapper to use Xilinx Coregen FIFOs instead of generic FIFOs
--              from Generics RAMs and FIFOs collection.
--
-- dependencies:
--
--------------------------------------------------------------------------------
-- last changes: see svn log.
--------------------------------------------------------------------------------
-- TODO: - 
--------------------------------------------------------------------------------

library IEEE;
use IEEE.STD_LOGIC_1164.all;
use IEEE.NUMERIC_STD.all;

--library work;
use work.wshexp_core_pkg.all;


entity generic_async_fifo is

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

end generic_async_fifo;


architecture syn of generic_async_fifo is


  component fifo_32x512
    port (
      rst                     : in  std_logic;
      wr_clk                  : in  std_logic;
      rd_clk                  : in  std_logic;
      din                     : in  std_logic_vector(31 downto 0);
      wr_en                   : in  std_logic;
      rd_en                   : in  std_logic;
      prog_full_thresh_assert : in  std_logic_vector(9 downto 0);
      prog_full_thresh_negate : in  std_logic_vector(9 downto 0);
      dout                    : out std_logic_vector(31 downto 0);
      full                    : out std_logic;
      empty                   : out std_logic;
      valid                   : out std_logic;
      prog_full               : out std_logic);
  end component fifo_32x512;

  component fifo_64x512
    port (
      rst                     : in  std_logic;
      wr_clk                  : in  std_logic;
      rd_clk                  : in  std_logic;
      din                     : in  std_logic_vector(63 downto 0);
      wr_en                   : in  std_logic;
      rd_en                   : in  std_logic;
      prog_full_thresh_assert : in  std_logic_vector(8 downto 0);
      prog_full_thresh_negate : in  std_logic_vector(8 downto 0);
      dout                    : out std_logic_vector(63 downto 0);
      full                    : out std_logic;
      empty                   : out std_logic;
      valid                   : out std_logic;
      prog_full               : out std_logic);
  end component fifo_64x512;

  component fifo_96x512
    port (
      rst                     : in  std_logic;
      wr_clk                  : in  std_logic;
      rd_clk                  : in  std_logic;
      din                     : in  std_logic_vector(95 downto 0);
      wr_en                   : in  std_logic;
      rd_en                   : in  std_logic;
      prog_full_thresh_assert : in  std_logic_vector(8 downto 0);
      prog_full_thresh_negate : in  std_logic_vector(8 downto 0);
      dout                    : out std_logic_vector(95 downto 0);
      full                    : out std_logic;
      empty                   : out std_logic;
      --valid                   : out std_logic;
      prog_full               : out std_logic);
  end component fifo_96x512;  
  
  component fifo_128x512
    port (
      rst                     : in  std_logic;
      wr_clk                  : in  std_logic;
      rd_clk                  : in  std_logic;
      din                     : in  std_logic_vector(127 downto 0);
      wr_en                   : in  std_logic;
      rd_en                   : in  std_logic;
      prog_full_thresh_assert : in  std_logic_vector(8 downto 0);
      prog_full_thresh_negate : in  std_logic_vector(8 downto 0);
      dout                    : out std_logic_vector(127 downto 0);
      full                    : out std_logic;
      empty                   : out std_logic;
      valid                   : out std_logic;
      prog_full               : out std_logic);
  end component fifo_128x512;  


  signal rst : std_logic;


begin

  -- Active high reset for FIFOs
  rst <= not(rst_n_i);

  -- Assign unused outputs
  wr_empty_o <= '0';
  wr_almost_empty_o <= '0';
  wr_count_o <= (others => '0');
  rd_full_o <= '0';
  rd_almost_full_o <= '0';
  rd_almost_empty_o <= '0';
  rd_count_o <= (others => '0');

  gen_fifo_32bit : if g_data_width = 32 generate
    cmp_fifo_32x512 : fifo_32x512
      port map (
        rst                     => rst,
        wr_clk                  => clk_wr_i,
        rd_clk                  => clk_rd_i,
        din                     => d_i,
        wr_en                   => we_i,
        rd_en                   => rd_i,
        prog_full_thresh_assert => std_logic_vector(to_unsigned(g_almost_full_threshold, 10)),
        prog_full_thresh_negate => std_logic_vector(to_unsigned(g_almost_full_threshold, 10)),
        dout                    => q_o,
        full                    => wr_full_o,
        empty                   => rd_empty_o,
        valid                   => open,
        prog_full               => wr_almost_full_o);
  end generate gen_fifo_32bit;

  gen_fifo_64bit : if g_data_width = 64 generate
    cmp_fifo_64x512 : fifo_64x512
      port map (
        rst                     => rst,
        wr_clk                  => clk_wr_i,
        rd_clk                  => clk_rd_i,
        din                     => d_i,
        wr_en                   => we_i,
        rd_en                   => rd_i,
        prog_full_thresh_assert => std_logic_vector(to_unsigned(g_almost_full_threshold, 9)),
        prog_full_thresh_negate => std_logic_vector(to_unsigned(g_almost_full_threshold, 9)),
        dout                    => q_o,
        full                    => wr_full_o,
        empty                   => rd_empty_o,
        --valid                   => open,
        prog_full               => wr_almost_full_o);
  end generate gen_fifo_64bit;

  gen_fifo_96bit : if g_data_width = 96 generate
    cmp_fifo_96x512 : fifo_96x512
      port map (
        rst                     => rst,
        wr_clk                  => clk_wr_i,
        rd_clk                  => clk_rd_i,
        din                     => d_i,
        wr_en                   => we_i,
        rd_en                   => rd_i,
        prog_full_thresh_assert => std_logic_vector(to_unsigned(g_almost_full_threshold, 9)),
        prog_full_thresh_negate => std_logic_vector(to_unsigned(g_almost_full_threshold, 9)),
        dout                    => q_o,
        full                    => wr_full_o,
        empty                   => rd_empty_o,
        --valid                   => open,
        prog_full               => wr_almost_full_o);
  end generate gen_fifo_96bit;

  gen_fifo_128bit : if g_data_width = 128 generate
    cmp_fifo_128x512 : fifo_128x512
      port map (
        rst                     => rst,
        wr_clk                  => clk_wr_i,
        rd_clk                  => clk_rd_i,
        din                     => d_i,
        wr_en                   => we_i,
        rd_en                   => rd_i,
        prog_full_thresh_assert => std_logic_vector(to_unsigned(g_almost_full_threshold, 9)),
        prog_full_thresh_negate => std_logic_vector(to_unsigned(g_almost_full_threshold, 9)),
        dout                    => q_o,
        full                    => wr_full_o,
        empty                   => rd_empty_o,
        valid                   => open,
        prog_full               => wr_almost_full_o);
  end generate gen_fifo_128bit;

end syn;
