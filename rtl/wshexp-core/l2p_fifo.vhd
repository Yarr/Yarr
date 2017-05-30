LIBRARY ieee;
USE ieee.std_logic_1164.ALL;


ENTITY l2p_fifo IS
  PORT (
    rst : IN STD_LOGIC;
    wr_clk : IN STD_LOGIC;
    rd_clk : IN STD_LOGIC;
    din : IN STD_LOGIC_VECTOR(63 DOWNTO 0);
    wr_en : IN STD_LOGIC;
    rd_en : IN STD_LOGIC;
    prog_full_thresh_assert : IN STD_LOGIC_VECTOR(9 DOWNTO 0);
    prog_full_thresh_negate : IN STD_LOGIC_VECTOR(9 DOWNTO 0);
    dout : OUT STD_LOGIC_VECTOR(63 DOWNTO 0);
    full : OUT STD_LOGIC;
    empty : OUT STD_LOGIC;
    valid : OUT STD_LOGIC;
    prog_full : OUT STD_LOGIC
  );
END l2p_fifo;

ARCHITECTURE l2p_fifo_a OF l2p_fifo IS

COMPONENT l2p_fifo64
  PORT (
    rst : IN STD_LOGIC;
    wr_clk : IN STD_LOGIC;
    rd_clk : IN STD_LOGIC;
    din : IN STD_LOGIC_VECTOR(63 DOWNTO 0);
    wr_en : IN STD_LOGIC;
    rd_en : IN STD_LOGIC;
    prog_full_thresh_assert : IN STD_LOGIC_VECTOR(9 DOWNTO 0);
    prog_full_thresh_negate : IN STD_LOGIC_VECTOR(9 DOWNTO 0);
    dout : OUT STD_LOGIC_VECTOR(63 DOWNTO 0);
    full : OUT STD_LOGIC;
    empty : OUT STD_LOGIC;
    valid : OUT STD_LOGIC;
    prog_full : OUT STD_LOGIC
  );
END COMPONENT;



BEGIN


U0 : l2p_fifo64
  PORT MAP (
    rst => rst,
    wr_clk => wr_clk,
    rd_clk => rd_clk,
    din => din,
    wr_en => wr_en,
    rd_en => rd_en,
    prog_full_thresh_assert => prog_full_thresh_assert,
    prog_full_thresh_negate => prog_full_thresh_negate,
    dout => dout,
    full => full,
    empty => empty,
    valid => valid,
    prog_full => prog_full
  );

END l2p_fifo_a;
