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
        input : IN std_logic;
        output : OUT std_logic;
		setting : IN std_logic_vector(width-1 downto 0)
    );
end delay_line;

architecture Behavioral of delay_line is
    signal chain_t : std_logic_vector(((2**width)*2-1) downto 0);
    signal chain : std_logic_vector(((2**width)*2-1) downto 0);

    signal input_t : std_logic;
    signal output_t : std_logic;

    attribute KEEP : string;
    attribute KEEP of chain_t : signal is "true";

    attribute RLOC : string;
	--attribute RLOC of first_carry4 : label is "X" & INTEGER'image(0) & "Y" & INTEGER'image(0);
    
    constant delay_time: time := 300 ps;
begin

    input_t <= input;
    output <= output_t;

    
--    first_carry4: CARRY4 port map (
--        CO => chain_t(3 downto 0),
--        CI => '0',
--        CYINIT => input_t,
--        DI => "0000",
--        S => "1111"
--    );
--
--    chain_gen: for i in 1 to ((2**width)/2)-1 generate
--        --attribute RLOC of cmp_carry4 : label is "X" &INTEGER'image(0) & "Y" & INTEGER'image(i);
--    begin
--        cmp_carry4: CARRY4 port map (
--            CO => chain_t(4*(i+1)-1 downto 4*i),
--            CI => chain_t((4*i)-1),
--            CYINIT => '0',
--            DI => "0000",
--            S => "1111"
--        );
--    end generate;

	delay_mux: muxf7 port map (s => '0', i0 => input_t, i1 => '0', o => chain_t(0));
	chain(0) <= chain_t(0) after delay_time;
    
	delay_loop: for j in 1 to ((2**WIDTH)-1) generate
		--attribute RLOC of mux : label is "X"  & INTEGER'image((j-1)/3) & "Y" & INTEGER'image(((j-1)) rem 3);
	begin
		mux: muxf6 port map (s => '0', i0 => chain(j - 1), i1 => '0', o => chain_t(j));
        chain(j) <= chain_t(j) after delay_time;
	end generate;

    output_t <= chain(to_integer(unsigned(setting)));

end Behavioral;

