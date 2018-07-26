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

library unisim ;
use unisim.vcomponents.all ;

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
    
    constant c_AURORA_IDLE : std_logic_vector(7 downto 0) := x"78";
    constant c_AURORA_SEP : std_logic_vector(7 downto 0) := x"1E";
    
    signal rx_data_s : std_logic_vector(63 downto 0);
    signal rx_valid_s : std_logic;

    type rx_data_array is array (g_NUM_LANES-1 downto 0) of std_logic_vector(63 downto 0);
    signal rx_data : rx_data_array;
    type rx_header_array is array (g_NUM_LANES-1 downto 0) of std_logic_vector(1 downto 0);
    signal rx_header : rx_header_array;
    type rx_status_array is array (g_NUM_LANES-1 downto 0) of std_logic_vector(7 downto 0);
    signal rx_status : rx_status_array;
    signal rx_data_valid : std_logic_vector(g_NUM_LANES-1 downto 0);
    
    signal rx_fifo_dout :rx_data_array;
    signal rx_fifo_din : rx_data_array;
    signal rx_fifo_full : std_logic_vector(g_NUM_LANES-1 downto 0);
    signal rx_fifo_empty : std_logic_vector(g_NUM_LANES-1 downto 0);
    signal rx_fifo_rden : std_logic_vector(g_NUM_LANES-1 downto 0);
    signal rx_fifo_rden_t : std_logic_vector(g_NUM_LANES-1 downto 0);
    signal rx_fifo_wren : std_logic_vector(g_NUM_LANES-1 downto 0);
    
    signal channel : integer range 0 to g_NUM_LANES-1;
    
    COMPONENT ila_rx_dma_wb
    PORT (
        clk : IN STD_LOGIC;
        probe0 : IN STD_LOGIC_VECTOR(31 DOWNTO 0); 
        probe1 : IN STD_LOGIC_VECTOR(63 DOWNTO 0); 
        probe2 : IN STD_LOGIC_VECTOR(63 DOWNTO 0); 
        probe3 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
        probe4 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
        probe5 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
        probe6 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
        probe7 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
        probe8 : IN STD_LOGIC_VECTOR(31 DOWNTO 0); 
        probe9 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
        probe10 : IN STD_LOGIC_VECTOR(0 DOWNTO 0);
        probe11 : IN STD_LOGIC_VECTOR(0 DOWNTO 0)
    );
    END COMPONENT  ;
            
begin

    rx_data_o <= rx_data_s;
    rx_valid_o <= rx_valid_s;
	
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
            rx_data_s <= (others => '0');
            rx_valid_s <= '0';
            channel <= 0;            
        elsif rising_edge(clk_rx_i) then
            rx_fifo_rden <= rx_fifo_rden_t;
            channel <= log2_ceil(to_integer(unsigned(rx_fifo_rden_t)));
            if (unsigned(rx_fifo_rden) = 0 or ((rx_fifo_rden and rx_fifo_empty) = rx_fifo_rden)) then
                rx_valid_s <= '0';
                rx_data_s <= x"DEADBEEFDEADBEEF";
            else
                rx_valid_s <= '1';
                rx_data_s <= rx_fifo_dout(channel);
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
            rx_stat_o => rx_status(I)
        );
        rx_stat_o(I) <= rx_status(I)(1);
        
        -- TODO need to save register reads!
        -- TODO use 
        
        -- We expect these types of data:
        -- b01 - D[63:0] - 64 bit data
        -- b10 - 0x1E - 0x04 - 0xXXXX - D[31:0] - 32 bit data
        -- b10 - 0x1E - 0x00 - 0x0000 - 0x00000000 - 0 bit data
        -- b10 - 0x78 - Flag[7:0] - 0xXXXX - 0xXXXXXXXX - Idle
        -- b10 - 0xB4 - D[55:0] - Register read (MM)
        
        -- Swapping [63:32] and [31:0] to reverse swapping by casting 64-bit to uint32_t
        rx_fifo_din(I) <= rx_data(I)(31 downto 0) & rx_data(I)(63 downto 32) when (rx_header(I) = "01") else
                          rx_data(I)(31 downto 0) & x"FFFFFFFF" when (rx_data(I)(63 downto 56) = c_AURORA_SEP) else
                          rx_data(I)(31 downto 0) & rx_data(I)(63 downto 32) when ((rx_header(I) = "10") and (rx_data(I)(63 downto 56) = x"55")) else
                          rx_data(I)(31 downto 0) & rx_data(I)(63 downto 32) when ((rx_header(I) = "10") and (rx_data(I)(63 downto 56) = x"99")) else
                          rx_data(I)(31 downto 0) & rx_data(I)(63 downto 32) when ((rx_header(I) = "10") and (rx_data(I)(63 downto 56) = x"D2")) else
                          x"FFFFFFFFFFFFFFFF";
        rx_fifo_wren(I) <= rx_data_valid(I) when (rx_header(I) = "01") else
                           rx_data_valid(I) when ((rx_data(I)(63 downto 56) = c_AURORA_SEP) and (rx_data(I)(55 downto 48) = x"04")) else
                           rx_data_valid(I) when ((rx_header(I) = "10") and (rx_data(I)(63 downto 56) = x"55")) else
                           rx_data_valid(I) when ((rx_header(I) = "10") and (rx_data(I)(63 downto 56) = x"99")) else
                           rx_data_valid(I) when ((rx_header(I) = "10") and (rx_data(I)(63 downto 56) = x"D2")) else
                           '0';
                           
        cmp_lane_fifo : rx_channel_fifo PORT MAP (
            rst => not rst_n_i,
            wr_clk => clk_rx_i,
            rd_clk => clk_rx_i,
            din => rx_fifo_din(I),
            wr_en => rx_fifo_wren(I) and enable_i,
            rd_en => rx_fifo_rden(I),
            dout => rx_fifo_dout(I),
            full => rx_fifo_full(I),
            empty => rx_fifo_empty(I)
        );        
    end generate lane_loop;
    
    aurora_channel_debug : ila_rx_dma_wb
    PORT MAP (
      clk => clk_rx_i,
      probe0 => x"00000000", 
      probe1 => rx_data_s, 
      probe2 => rx_fifo_din(0), 
      probe3(0) => rx_fifo_wren(0),
      probe4(0) => rx_fifo_wren(1),
      probe5(0) => rx_fifo_wren(2), 
      probe6(0) => rx_fifo_wren(3),
      probe7(0) => rx_valid_s,
      probe8 => x"00000000",
      probe9(0) => rx_fifo_rden(0),
      probe10(0) => rx_fifo_empty(0),
      probe11(0) => '0'
    );
    
end behavioral;
