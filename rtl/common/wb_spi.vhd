library IEEE;
use IEEE.STD_LOGIC_1164.all;
use IEEE.NUMERIC_STD.all;

entity wb_spi is
    generic (
        g_CLK_DIVIDER : positive := 10
    );
    port (
        -- Sys Connect
        wb_clk_i : in std_logic;
        rst_n_i : in std_logic;
		-- Wishbone slave interface
		wb_adr_i	: in  std_logic_vector(31 downto 0);
		wb_dat_i	: in  std_logic_vector(31 downto 0);
		wb_dat_o	: out std_logic_vector(31 downto 0);
		wb_cyc_i	: in  std_logic;
		wb_stb_i	: in  std_logic;
		wb_we_i		: in  std_logic;
		wb_ack_o	: out std_logic;
        -- SPI out
        scl_o : out std_logic;
        sda_o : out std_logic;
        sdi_i : in std_logic;
        latch_o : out std_logic
    );
end wb_spi;

architecture rtl of wb_spi is
    signal data_word : std_logic_vector(31 downto 0);
    signal sreg : std_logic_vector(31 downto 0);
    signal shift_cnt : unsigned(7 downto 0);
    signal wait_cnt : unsigned(7 downto 0);
    signal start : std_logic;
    signal busy : std_logic;
    signal data_in : std_logic_vector(31 downto 0);
begin

    wb_proc: process(wb_clk_i, rst_n_i)
    begin
        if (rst_n_i ='0') then
            data_word <= (others => '0');
            start <= '0';
            wb_ack_o <= '0';
            wb_dat_o <= (others => '0');
        elsif rising_edge(wb_clk_i) then
            wb_ack_o <= '0';
            start <= '0';
            if (wb_cyc_i = '1' and wb_stb_i = '1') then
                if (wb_we_i = '1') then
                    case (wb_adr_i(3 downto 0)) is
                        when x"0" =>
                            data_word <= wb_dat_i;
                            wb_ack_o <= '1';
                        when x"1" =>
                            start <= wb_dat_i(0);
                            wb_ack_o <= '1';
                        when others =>
                            wb_ack_o <= '1';
                    end case;
                else
                    case (wb_adr_i(3 downto 0)) is
                        when x"1" =>
                            wb_dat_o(31 downto 1) <= (others => '0');
                            wb_dat_o(0) <= busy;
                            wb_ack_o <= '1';
                        when x"2" =>
                                wb_dat_o <= data_in;
                                wb_ack_o <= '1';
                        when others =>
                            wb_dat_o <= x"DEADBEEF";
                            wb_ack_o <= '1';
                    end case;
                end if;
            end if;
        end if;
    end process wb_proc;

    spi_proc: process(wb_clk_i, rst_n_i)
    begin
        if (rst_n_i = '0') then
            busy <= '0';
            shift_cnt <= (others => '0');
            wait_cnt <= (others => '0');
            sreg <= (others => '0');
            scl_o <= '0';
            sda_o <= '0';
            latch_o <= '0';
            data_in <= (others => '0');
        elsif rising_edge(wb_clk_i) then
            if (start = '1') then
                sreg <= data_word;
                shift_cnt <= to_unsigned(34, 8);
                busy <= '1';
            end if;
            sda_o <= sreg(31);
            latch_o <= '0';
            
            if (shift_cnt > 2) then
                if (wait_cnt = to_unsigned(g_CLK_DIVIDER, 8)) then
                    wait_cnt <= (others => '0');
                    scl_o <= '0';
                    sreg <= sreg(30 downto 0) & '0';
                    shift_cnt <= shift_cnt - 1;
                    data_in <= data_in(30 downto 0) & sdi_i;
                elsif (wait_cnt=to_unsigned(g_CLK_DIVIDER/2, 8)) then
                    scl_o <= '1';
                    wait_cnt <= wait_cnt + 1;
                else
                    wait_cnt <= wait_cnt + 1;
                end if;
            elsif (shift_cnt > 1) then
                if (wait_cnt = to_unsigned(g_CLK_DIVIDER, 8)) then
                    wait_cnt <= (others => '0');
                    shift_cnt <= shift_cnt - 1;
                else
                    wait_cnt <= wait_cnt + 1;
                end if;
            elsif (shift_cnt > 0) then
                if (wait_cnt = to_unsigned(g_CLK_DIVIDER, 8)) then
                    wait_cnt <= (others => '0');
                    shift_cnt <= shift_cnt - 1;
                    latch_o <= '1';
                    busy <= '0';
                else
                    wait_cnt <= wait_cnt + 1;
                    latch_o <= '1';
                end if;
           end if;              

        end if;
    end process;


end rtl;


