-- ####################################
-- # Project: Yarr
-- # Author: Timon Heim
-- # E-Mail: timon.heim at cern.ch
-- # Comments: RX channel
-- # Aurora style rx code
-- ####################################

library IEEE;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity aurora_rx_channel is
    generic (
        g_NUM_LANES : integer range 1 to 4 := 1
    );
    port (
        -- Sys connect
        rst_n_i : in std_logic;
        clk_rx_i : in std_logic; -- Fabric clock (serdes/8)
        clk_serdes_i : in std_logic; -- IO clock
        
        -- Input
        enable_i : in std_logic;
        rx_data_i_p : in std_logic_vector(g_NUM_LANES-1 downto 0);
        rx_data_i_n : in std_logic_vector(g_NUM_LANES-1 downto 0);
        trig_tag_i : in std_logic_vector(63 downto 0);

        -- Output
        rx_data_o : out std_logic_vector(63 downto 0);
        rx_valid_o : out std_logic;
        rx_stat_o : out std_logic_vector(7 downto 0)
    );
end aurora_rx_channel;

architecture behavioral of aurora_rx_channel is

	function log2_ceil(val : integer) return natural is
		 variable result : natural;
	begin
		 for i in 0 to g_NUM_LANES-1 loop
			 if (val <= (2 ** i)) then
				 result := i;
				 exit;
			 end if;
		 end loop;
		 return result;
	end function;

	constant c_ALL_ZEROS : std_logic_vector(g_NUM_LANES-1 downto 0) := (others => '0');

    component aurora_rx_lane
        port (
        -- Sys connect
        rst_n_i : in std_logic;
        clk_rx_i : in std_logic;
        clk_serdes_i : in std_logic;

        -- Input
        rx_data_i_p : in std_logic;
        rx_data_i_n : in std_logic;

        -- Output
        rx_data_o : out std_logic_vector(63 downto 0);
        rx_header_o : out std_logic_vector(1 downto 0);
        rx_valid_o : out std_logic;
        rx_stat_o : out std_logic_vector(7 downto 0)
    );
    end component aurora_rx_lane;
    
    component rr_arbiter
        generic (
            g_CHANNELS : integer := g_NUM_LANES
        );
        port (
            -- sys connect
            clk_i : in std_logic;
            rst_i : in std_logic;
            -- requests
            req_i : in std_logic_vector(g_NUM_LANES-1 downto 0);
            -- grants
            gnt_o : out std_logic_vector(g_NUM_LANES-1 downto 0)
        );
    end component rr_arbiter;
    
    COMPONENT rx_channel_fifo
        PORT (
            rst : IN STD_LOGIC;
            wr_clk : IN STD_LOGIC;
            rd_clk : IN STD_LOGIC;
            din : IN STD_LOGIC_VECTOR(63 DOWNTO 0);
            wr_en : IN STD_LOGIC;
            rd_en : IN STD_LOGIC;
            dout : OUT STD_LOGIC_VECTOR(63 DOWNTO 0);
            full : OUT STD_LOGIC;
            empty : OUT STD_LOGIC
        );
    END COMPONENT;
    
    constant c_RD53_IDLE : std_logic_vector(7 downto 0) := x"78";
    

    type rx_data_array is array (g_NUM_LANES-1 downto 0) of std_logic_vector(63 downto 0);
    signal rx_data : rx_data_array;
    type rx_header_array is array (g_NUM_LANES-1 downto 0) of std_logic_vector(1 downto 0);
    signal rx_header : rx_header_array;
    signal rx_data_valid : std_logic_vector(g_NUM_LANES-1 downto 0);
    
    signal rx_fifo_dout :rx_data_array;
    signal rx_fifo_din : rx_data_array;
    signal rx_fifo_full : std_logic_vector(g_NUM_LANES-1 downto 0);
    signal rx_fifo_empty : std_logic_vector(g_NUM_LANES-1 downto 0);
    signal rx_fifo_rden : std_logic_vector(g_NUM_LANES-1 downto 0);
    signal rx_fifo_rden_t : std_logic_vector(g_NUM_LANES-1 downto 0);
    signal rx_fifo_wren : std_logic_vector(g_NUM_LANES-1 downto 0);
    
    signal channel : integer range 0 to g_NUM_LANES-1;
            
begin

	
	-- Arbiter
	cmp_rr_arbiter : rr_arbiter port map (
		clk_i => clk_rx_i,
		rst_i => not rst_n_i,
		req_i => not rx_fifo_empty,
		gnt_o => rx_fifo_rden_t
	);
	
	reg_proc : process(clk_rx_i, rst_n_i)
    begin
        if (rst_n_i = '0') then
            rx_fifo_rden <= (others => '0');
            rx_valid_o <= '0';
            channel <= 0;            
        elsif rising_edge(clk_rx_i) then
            rx_fifo_rden <= rx_fifo_rden_t;
            channel <= log2_ceil(to_integer(unsigned(rx_fifo_rden_t)));
            if (unsigned(rx_fifo_rden) = 0 or ((rx_fifo_rden and rx_fifo_empty) = rx_fifo_rden)) then
                rx_valid_o <= '0';
                rx_data_o <= x"DEADBEEFDEADBEEF";
            else
                rx_valid_o <= '1';
                rx_data_o <= rx_fifo_dout(channel);
            end if;
        end if;
    end process reg_proc;
    
    lane_loop: for I in 0 to g_NUM_LANES-1 generate
        lane_cmp : aurora_rx_lane port map (
            rst_n_i => rst_n_i,
            clk_rx_i => clk_rx_i,
            clk_serdes_i => clk_serdes_i,
            rx_data_i_p => rx_data_i_p(I),
            rx_data_i_n => rx_data_i_n(I),
            rx_data_o => rx_data(I),
            rx_header_o => rx_header(I),
            rx_valid_o => rx_data_valid(I),
            rx_stat_o => open
        );
        rx_fifo_din(I) <= rx_data(I);
        rx_fifo_wren(I) <= rx_data_valid(I) when (rx_data(I)(63 downto 56) /= c_RD53_IDLE) else '0';
        cmp_rx_channel_fifo : rx_channel_fifo PORT MAP (
            rst => not rst_n_i,
            wr_clk => clk_rx_i,
            rd_clk => clk_rx_i,
            din => rx_fifo_din(I),
            wr_en => rx_fifo_wren(I),
            rd_en => rx_fifo_rden(I),
            dout => rx_fifo_dout(I),
            full => rx_fifo_full(I),
            empty => rx_fifo_empty(I)
        );        
    end generate lane_loop;


end behavioral;
