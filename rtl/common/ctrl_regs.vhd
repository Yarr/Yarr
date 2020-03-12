-- ####################################
-- # Project: Yarr
-- # Author: Timon Heim
-- # E-Mail: timon.heim at cern.ch
-- # Comments: Generic control register block
-- # Mar 2020
-- ####################################

library IEEE;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

library work;
use work.board_pkg.all;
use work.version_pkg.all;

entity ctrl_regs is
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
		
		-- Register Outputs R/W
        ctrl_reg_0_o : out std_logic_vector(31 downto 0);
        ctrl_reg_1_o : out std_logic_vector(31 downto 0);
        ctrl_reg_2_o : out std_logic_vector(31 downto 0);
        ctrl_reg_3_o : out std_logic_vector(31 downto 0);
        ctrl_reg_4_o : out std_logic_vector(31 downto 0);
        ctrl_reg_5_o : out std_logic_vector(31 downto 0);

        -- Static registers RO
        static_reg_0_o : out std_logic_vector(31 downto 0);
        static_reg_1_o : out std_logic_vector(31 downto 0);
        static_reg_2_o : out std_logic_vector(31 downto 0);
        static_reg_3_o : out std_logic_vector(31 downto 0)
	);
end ctrl_regs;

architecture behavioral of ctrl_regs is
    signal ctrl_reg_0 : std_logic_vector(31 downto 0);
    signal ctrl_reg_1 : std_logic_vector(31 downto 0);
    signal ctrl_reg_2 : std_logic_vector(31 downto 0);
    signal ctrl_reg_3 : std_logic_vector(31 downto 0);
    signal ctrl_reg_4 : std_logic_vector(31 downto 0);
    signal ctrl_reg_5 : std_logic_vector(31 downto 0);

    constant static_reg_0 : std_logic_vector(31 downto 0) := c_FW_VERSION;
    constant static_reg_1 : std_logic_vector(31 downto 0) := c_FW_IDENT;
    constant static_reg_2 : std_logic_vector(31 downto 0) := x"00000000";
    constant static_reg_3 : std_logic_vector(31 downto 0) := x"00000000";

begin

    static_reg_0_o <= static_reg_0;
    static_reg_1_o <= static_reg_1;
    static_reg_2_o <= static_reg_2;
    static_reg_3_o <= static_reg_3;

    wb_stall_o <= '0';

	wb_proc: process (wb_clk_i, rst_n_i)
	begin
		if (rst_n_i = '0') then
			wb_dat_o <= (others => '0');
			wb_ack_o <= '0';
            ctrl_reg_0_o <= (others => '0');
            ctrl_reg_1_o <= (others => '0');
            ctrl_reg_2_o <= (others => '0');
            ctrl_reg_3_o <= (others => '0');
            ctrl_reg_4_o <= (others => '0');
            ctrl_reg_5_o <= (others => '0');
		elsif rising_edge(wb_clk_i) then
			wb_ack_o <= '0';

            ctrl_reg_0_o <= ctrl_reg_0;
            ctrl_reg_1_o <= ctrl_reg_1;
            ctrl_reg_2_o <= ctrl_reg_2;
            ctrl_reg_3_o <= ctrl_reg_3;
            ctrl_reg_4_o <= ctrl_reg_4;
            ctrl_reg_5_o <= ctrl_reg_5;

			if (wb_cyc_i = '1' and wb_stb_i = '1') then
				if (wb_we_i = '1') then
					case (wb_adr_i(3 downto 0)) is
						when x"0" =>
                            ctrl_reg_0 <= wb_dat_i;
							wb_ack_o <= '1';
						when x"1" =>
                            ctrl_reg_1 <= wb_dat_i;
							wb_ack_o <= '1';
						when x"2" =>
                            ctrl_reg_2 <= wb_dat_i;
							wb_ack_o <= '1';
						when x"3" =>
                            ctrl_reg_3 <= wb_dat_i;
							wb_ack_o <= '1';
						when x"4" =>
                            ctrl_reg_4 <= wb_dat_i;
							wb_ack_o <= '1';
						when x"5" =>
                            ctrl_reg_5 <= wb_dat_i;
							wb_ack_o <= '1';
                        when others =>
							wb_ack_o <= '1';
					end case;
				else
					case (wb_adr_i(3 downto 0)) is
						when x"0" =>
							wb_dat_o <= ctrl_reg_0;
							wb_ack_o <= '1';
						when x"1" =>
							wb_dat_o <= ctrl_reg_1;
							wb_ack_o <= '1';
						when x"2" =>
							wb_dat_o <= ctrl_reg_2;
							wb_ack_o <= '1';
						when x"3" =>
							wb_dat_o <= ctrl_reg_3;
							wb_ack_o <= '1';
						when x"4" =>
							wb_dat_o <= ctrl_reg_4;
							wb_ack_o <= '1';
						when x"5" =>
							wb_dat_o <= ctrl_reg_5;
							wb_ack_o <= '1';
						when x"6" =>
							wb_dat_o <= static_reg_0;
							wb_ack_o <= '1';
						when x"7" =>
							wb_dat_o <= static_reg_1;
							wb_ack_o <= '1';
						when x"8" =>
							wb_dat_o <= static_reg_2;
							wb_ack_o <= '1';
						when x"9" =>
							wb_dat_o <= static_reg_3;
							wb_ack_o <= '1';
						when others =>
							wb_dat_o <= x"DEADBEEF";
							wb_ack_o <= '1';
					end case;
				end if;
			end if;
		end if;
	end process wb_proc;
end behavioral;
