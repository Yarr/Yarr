-- ####################################
-- # Project: Yarr
-- # Author: Timon Heim
-- # E-Mail: timon.heim at cern.ch
-- # Comments: RX core
-- # Outputs are synchronous to wb_clk_i
-- ####################################
-- # Adress Map:
-- # Adr[3:0]:
-- #     0x0 : RX Enable Mask


library IEEE;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

library UNISIM;
use UNISIM.VComponents.all;

entity wb_rx_core is
	generic (
		g_NUM_RX : integer range 1 to 32 := 1;
        g_TYPE : string := "FEI4";
        g_NUM_LANES : integer range 1 to 4 := 1
	);
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
		
		-- RX IN
		rx_clk_i	: in  std_logic;
		rx_serdes_clk_i : in std_logic;
		rx_data_i_p	: in std_logic_vector((g_NUM_RX*g_NUM_LANES)-1 downto 0);
		rx_data_i_n	: in std_logic_vector((g_NUM_RX*g_NUM_LANES)-1 downto 0);
        trig_tag_i : in std_logic_vector(31 downto 0);
		
		-- RX OUT (sync to sys_clk)
		rx_valid_o : out std_logic;
		rx_data_o : out std_logic_vector(63 downto 0);
		busy_o : out std_logic;
		
		debug_o : out std_logic_vector(31 downto 0)
	);
end wb_rx_core;

architecture behavioral of wb_rx_core is
	function log2_ceil(val : integer) return natural is
		 variable result : natural;
	begin
		 for i in 0 to g_NUM_RX-1 loop
			 if (val <= (2 ** i)) then
				 result := i;
				 exit;
			 end if;
		 end loop;
		 return result;
	end function;

	constant c_ALL_ZEROS : std_logic_vector(g_NUM_RX-1 downto 0) := (others => '0');
	
	component rr_arbiter
		generic (
			g_CHANNELS : integer := g_NUM_RX
		);
		port (
			-- sys connect
			clk_i : in std_logic;
			rst_i : in std_logic;
			-- requests
			req_i : in std_logic_vector(g_CHANNELS-1 downto 0);
			-- grants
			gnt_o : out std_logic_vector(g_CHANNELS-1 downto 0)
		);
	end component rr_arbiter;

	component fei4_rx_channel
		port (
			-- Sys connect
			rst_n_i : in std_logic;
			clk_160_i : in std_logic;
			clk_640_i : in std_logic;
			enable_i : in std_logic;
			-- Input
			rx_data_i : in std_logic;
            trig_tag_i : in std_logic_vector(31 downto 0);
			-- Output
			rx_data_o : out std_logic_vector(25 downto 0);
			rx_valid_o : out std_logic;
			rx_stat_o : out std_logic_vector(7 downto 0);
			rx_data_raw_o : out std_logic_vector(7 downto 0)
		);
	end component;

    component aurora_rx_channel
        generic (
            g_NUM_LANES : integer range 1 to 4 := g_NUM_LANES
        );
        port (
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
    end component aurora_rx_channel;
	
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
	
	type rx_data_array is array (g_NUM_RX-1 downto 0) of std_logic_vector(63 downto 0);
	type rx_data_fifo_array is array (g_NUM_RX-1 downto 0) of std_logic_vector(63 downto 0);
	type rx_stat_array is array (g_NUM_RX-1 downto 0) of std_logic_vector(7 downto 0);
    signal rx_data_i : std_logic_vector((g_NUM_RX*g_NUM_LANES)-1 downto 0);
    signal rx_data : rx_data_array;
	signal rx_valid : std_logic_vector(g_NUM_RX-1 downto 0);
	signal rx_stat : rx_stat_array;
	signal rx_data_raw : rx_stat_array;
	
	signal rx_fifo_dout :rx_data_fifo_array;
	signal rx_fifo_din : rx_data_fifo_array;
	signal rx_fifo_full : std_logic_vector(g_NUM_RX-1 downto 0);
	signal rx_fifo_empty : std_logic_vector(g_NUM_RX-1 downto 0);
	signal rx_fifo_rden : std_logic_vector(g_NUM_RX-1 downto 0);
	signal rx_fifo_rden_t : std_logic_vector(g_NUM_RX-1 downto 0);
	signal rx_fifo_wren : std_logic_vector(g_NUM_RX-1 downto 0);

	signal rx_enable : std_logic_vector(31 downto 0);
	signal rx_enable_d : std_logic_vector(31 downto 0);
    signal rx_status : std_logic_vector(31 downto 0);
    signal rx_status_s : std_logic_vector(31 downto 0);
	
	signal channel : integer range 0 to g_NUM_RX-1;

	signal debug : std_logic_vector(31 downto 0);
	
begin
	debug_o <= debug;
	
	debug(7 downto 0) <= rx_stat(0);
	debug(15 downto 8) <= rx_data_raw(0);
	debug(16) <= rx_valid(0);



    wb_proc: process (wb_clk_i, rst_n_i)
	begin
		if (rst_n_i = '0') then
			wb_dat_o <= (others => '0');
			wb_ack_o <= '0';
			rx_enable <= (others => '0');
			wb_stall_o <= '0';
		elsif rising_edge(wb_clk_i) then
			wb_ack_o <= '0';
			if (wb_cyc_i = '1' and wb_stb_i = '1') then
				if (wb_we_i = '1') then
					if (wb_adr_i(3 downto 0) = x"0") then -- Set enable mask
						wb_ack_o <= '1';
						rx_enable <= wb_dat_i;
					else
						wb_ack_o <= '1';
					end if;
				else
					if (wb_adr_i(3 downto 0) = x"0") then -- Read enable mask
						wb_dat_o <= rx_enable;
						wb_ack_o <= '1';
                    elsif (wb_adr_i(3 downto 0) = x"1") then -- Link status
                        wb_dat_o <= rx_status;
                        wb_ack_o <= '1';
					else
						wb_dat_o <= x"DEADBEEF";
						wb_ack_o <= '1';
					end if;
				end if;
			end if;
		end if;
	end process wb_proc;
	
	-- Arbiter
	cmp_rr_arbiter : rr_arbiter port map (
		clk_i => wb_clk_i,
		rst_i => not rst_n_i,
		req_i => not rx_fifo_empty,
		gnt_o => rx_fifo_rden_t
	);
	
	--rx_valid_o <= '0' when (unsigned(rx_fifo_rden) = 0 or ((rx_fifo_rden and rx_fifo_empty) = rx_fifo_rden)) else '1';
	--rx_data_o <= x"DEADBEEF" when (unsigned(rx_fifo_rden) = 0) else rx_fifo_dout(log2_ceil(to_integer(unsigned(rx_fifo_rden))));
	
	reg_proc : process(wb_clk_i, rst_n_i)
	begin
		if (rst_n_i = '0') then
			rx_fifo_rden <= (others => '0');
			rx_valid_o <= '0';
			channel <= 0;			
		elsif rising_edge(wb_clk_i) then
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
    
    fei4_iobuf: if g_TYPE = "FEI4" generate
        rx_loop: for I in 0 to (g_NUM_RX*g_NUM_LANES)-1 generate
        begin
            rx_buf : IBUFDS
            generic map (
                DIFF_TERM => TRUE, -- Differential Termination 
                IBUF_LOW_PWR => FALSE, -- Low power (TRUE) vs. performance (FALSE) setting for referenced I/O standards
                IOSTANDARD => "LVDS_25")
            port map (
                O => rx_data_i(I),  -- Buffer output
                I => rx_data_i_p(I),  -- Diff_p buffer input (connect directly to top-level port)
                IB => rx_data_i_n(I) -- Diff_n buffer input (connect directly to top-level port)
            );
        end generate;
    end generate fei4_iobuf;
    
    enable_sync: process (rx_clk_i, rst_n_i)
    begin
        if (rst_n_i = '0') then
            rx_enable_d <= (others => '0');
            rx_status <= (others => '0');
        elsif rising_edge(rx_clk_i) then
            rx_enable_d <= rx_enable;
            rx_status <= rx_status_s;
        end if;
   end process enable_sync;
	
    -- Generate Rx Channels
	busy_o <= '0' when (rx_fifo_full = c_ALL_ZEROS) else '1';
	rx_channels: for I in 0 to g_NUM_RX-1 generate
	begin
        fei4_type: if g_TYPE = "FEI4" generate
            cmp_fei4_rx_channel: fei4_rx_channel PORT MAP(
                rst_n_i => rst_n_i,
                clk_160_i => rx_clk_i,
                clk_640_i => rx_serdes_clk_i,
                enable_i => rx_enable_d(I),
                rx_data_i => rx_data_i(I),
                trig_tag_i => trig_tag_i,
                rx_data_o => rx_data(I)(25 downto 0),
                rx_valid_o => rx_valid(I),
                rx_stat_o => rx_stat(I),
                rx_data_raw_o => rx_data_raw(I)
            );
		    rx_fifo_din(I) <= x"03000000" & STD_LOGIC_VECTOR(TO_UNSIGNED(I,6)) & rx_data(I)(25 downto 0);
        end generate fei4_type;
        
        rd53_type: if g_TYPE = "RD53" generate
            cmp_aurora_rx_channel : aurora_rx_channel PORT MAP (
                rst_n_i => rst_n_i,
                clk_rx_i => rx_clk_i,
                clk_serdes_i => rx_serdes_clk_i,
                enable_i => rx_enable(I),
                rx_data_i_p => rx_data_i_p((I+1)*g_NUM_LANES-1 downto (I*g_NUM_LANES)),
                rx_data_i_n => rx_data_i_n((I+1)*g_NUM_LANES-1 downto (I*g_NUM_LANES)),
                trig_tag_i => x"00000000" & trig_tag_i,
                rx_data_o => rx_data(I),
                rx_valid_o => rx_valid(I),
                rx_stat_o => rx_stat(I)
            );
            rx_status_s(I) <= '1' when rx_stat(I)(1) = '1' else
                              '1' when rx_enable(I) = '0' else
                              '0';
		    rx_fifo_din(I) <= rx_data(I);
        end generate rd53_type;
		
		rx_fifo_wren(I) <= rx_valid(I) and rx_enable_d(I);
		cmp_rx_channel_fifo : rx_channel_fifo PORT MAP (
			rst => not rst_n_i,
			wr_clk => rx_clk_i,
			rd_clk => wb_clk_i,
			din => rx_fifo_din(I),
			wr_en => rx_fifo_wren(I),
			rd_en => rx_fifo_rden(I),
			dout => rx_fifo_dout(I),
			full => rx_fifo_full(I),
			empty => rx_fifo_empty(I)
		);
	end generate;
end behavioral;

