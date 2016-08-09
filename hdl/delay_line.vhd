library IEEE;
use IEEE.STD_LOGIC_1164.all;
use IEEE.NUMERIC_STD.all;

library UNISIM;
use UNISIM.VComponents.all;

entity delay_line is
    generic (
        width : positive := 8
	 );
    port (
        clk : in std_logic;
        rst : in std_logic;
        input : IN std_logic;
        output : OUT std_logic;
		setting : IN std_logic_vector(width-1 downto 0)
    );
end delay_line;

architecture Behavioral of delay_line is
    signal chain : std_logic_vector(((2**width)-1) downto 0);
    signal chain_t : std_logic_vector(((2**width)-1) downto 0);
    signal select_t : std_logic_vector(((2**width)-1) downto 0);
    

    signal input_t : std_logic_vector(((2**width)/4)-1 downto 0);
    signal output_t : std_logic;

    attribute KEEP : string;
    attribute KEEP of chain_t : signal is "true";
    attribute KEEP of input_t : signal is "true";

    attribute RLOC : string;
	attribute RLOC of first_mux : label is "X" & INTEGER'image(0) & "Y" & INTEGER'image(0);
    
    constant delay_time: time := 300 ps;
begin

    
    output <= input when (unsigned(setting) = 0) else output_t;

    reg: process(clk, rst, input, setting)
    begin
        if (rst = '1') then
            select_t <= (others => '0');
        elsif rising_edge(clk) then
            select_t <= (others => '0');
            select_t <= std_logic_vector(to_unsigned(1, 2**width) sll 2**width-1-to_integer(unsigned(setting)));
        end if;
    end process reg;
    
	first_mux: muxf6 port map (s => '0', i0 => input_t(0), i1 => '0', o => chain_t(0));
	chain(0) <= chain_t(0) after delay_time;
    
	delay_loop: for j in 1 to ((2**WIDTH)-1) generate
	begin
        even_cols: if ((j/32) rem 2) = 0 generate
            -- four mux go into one slice
            constant row : integer := (j/4) rem 8;
            constant column : integer := j/32;
            attribute RLOC of mux : label is "X"  & INTEGER'image(column) & "Y" & INTEGER'image(row);        
        begin
            mux: muxf6 port map (s => select_t(j), i0 => chain(j - 1), i1 => input_t(j/4), o => chain_t(j));
            chain(j) <= chain_t(j) after delay_time;
        end generate;
        odd_cols: if ((j/32) rem 2) = 1 generate
            -- four mux go into one slice
            constant row : integer := 7-((j/4) rem 8);
            constant column : integer := j/32;
            attribute RLOC of mux : label is "X"  & INTEGER'image(column) & "Y" & INTEGER'image(row);        
        begin
            mux: muxf6 port map (s => select_t(j+(3-((j rem 4)*2))), i0 => chain(j+(3-((j rem 4)*2)) - 1), i1 => input_t(j/4), o => chain_t(j+(3-((j rem 4)*2))));
            chain(j) <= chain_t(j) after delay_time;
        end generate;
	end generate;
    
    reg_loop: for j in 0 to ((2**width)/4)-1 generate
    begin
        even_cols: if ((j/8) rem 2) = 0 generate
            constant row : integer := (j) rem 8;
            constant column : integer := j/8;
            attribute RLOC of input_reg : label is "X"  & INTEGER'image(column-1) & "Y" & INTEGER'image(row);
        begin
            input_reg : FDCE generic map (INIT => '0')port map (Q => input_t(j), C => clk,CE => '1', CLR => rst, D => input);
        end generate;
        odd_cols: if ((j/8) rem 2) = 1 generate
            constant row : integer := 7-((j) rem 8);
            constant column : integer := j/8;
            attribute RLOC of input_reg : label is "X"  & INTEGER'image(column-1) & "Y" & INTEGER'image(row);
        begin
            input_reg : FDCE generic map (INIT => '0')port map (Q => input_t(j), C => clk,CE => '1', CLR => rst, D => input);
        end generate;
    end generate;
    
    
    
    output_t <= chain(2**width-1);

end Behavioral;

