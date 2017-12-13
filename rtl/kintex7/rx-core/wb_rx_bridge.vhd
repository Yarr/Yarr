-- ####################################
-- # Project: Yarr
-- # Author: Timon Heim
-- # E-Mail: timon.heim at cern.ch
-- # Comments: Bridge between Rx core and Mem
-- ####################################
-- # Address Map:
-- # 0x0000: Start Adr (RO)
-- # 0x0001: Data Cnt (RO)
-- # 0x0002[0]: Loopback (RW)
-- # 0x0003: Data Rate (RO)
-- # 0x0004: Loop Fifo (WO)


library IEEE;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity wb_rx_bridge is
	port (
		-- Sys Connect
		sys_clk_i		: in  std_logic;
		rst_n_i			: in  std_logic;
		
		-- Wishbone slave interface
		wb_adr_i	: in  std_logic_vector(31 downto 0);
		wb_dat_i	: in  std_logic_vector(31 downto 0);
		wb_dat_o	: out std_logic_vector(31 downto 0);
		wb_cyc_i	: in  std_logic;
		wb_stb_i	: in  std_logic;
		wb_we_i		: in  std_logic;
		wb_ack_o	: out std_logic;
		wb_stall_o	: out std_logic;

		-- Wishbone DMA Master Interface
		dma_clk_i	: in  std_logic;
		dma_adr_o	: out std_logic_vector(31 downto 0);
		dma_dat_o	: out std_logic_vector(63 downto 0);
		dma_dat_i	: in  std_logic_vector(63 downto 0);
		dma_cyc_o	: out std_logic;
		dma_stb_o	: out std_logic;
		dma_we_o	: out std_logic;
		dma_ack_i	: in  std_logic;
		dma_stall_i	: in  std_logic;
		
		-- Rx Interface
		rx_data_i 	: in  std_logic_vector(63 downto 0);
		rx_valid_i	: in  std_logic;
		
		-- Status In
		trig_pulse_i : in std_logic;
		
		-- Status out
		irq_o		: out std_logic;
		busy_o		: out std_logic
	);
end wb_rx_bridge;


architecture Behavioral of wb_rx_bridge is
	-- Cmoponents
   COMPONENT rx_bridge_fifo
      PORT (
        rst : IN STD_LOGIC;
        wr_clk : IN STD_LOGIC;
        rd_clk : IN STD_LOGIC;
        din : IN STD_LOGIC_VECTOR(63 DOWNTO 0);
        wr_en : IN STD_LOGIC;
        rd_en : IN STD_LOGIC;
        prog_empty_thresh : IN STD_LOGIC_VECTOR(9 DOWNTO 0);
        prog_full_thresh : IN STD_LOGIC_VECTOR(9 DOWNTO 0);
        dout : OUT STD_LOGIC_VECTOR(63 DOWNTO 0);
        full : OUT STD_LOGIC;
        empty : OUT STD_LOGIC;
        prog_full : OUT STD_LOGIC;
        prog_empty : OUT STD_LOGIC
      );
    END COMPONENT;
	
	COMPONENT rx_bridge_ctrl_fifo
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
	
	-- Constants
	constant c_ALMOST_FULL_THRESHOLD : unsigned(9 downto 0) := TO_UNSIGNED(900, 10);
	constant c_PACKAGE_SIZE : unsigned(31 downto 0) := TO_UNSIGNED((200*256), 32); -- 200kByte
	constant c_TIMEOUT : unsigned(31 downto 0) := TO_UNSIGNED(2**14, 32); -- Counts in 5ns = 0.1ms
	constant c_TIME_FRAME : unsigned(31 downto 0) := TO_UNSIGNED(200000000-1, 32); -- 200MHz clock cycles in 1 sec
	constant c_EMPTY_THRESHOLD : unsigned(9 downto 0) := TO_UNSIGNED(16, 10);
	constant c_EMPTY_TIMEOUT : unsigned(9 downto 0) := TO_UNSIGNED(2000, 10);
	
	-- Signals
	signal data_fifo_din : std_logic_vector(63 downto 0);
	signal data_fifo_dout : std_logic_vector(63 downto 0);
	signal data_fifo_wren : std_logic;
	signal data_fifo_rden : std_logic;
	signal data_fifo_full : std_logic;
	signal data_fifo_empty : std_logic;
	signal data_fifo_almost_full : std_logic;
	signal data_fifo_prog_empty : std_logic;
	
	signal data_fifo_empty_cnt : unsigned(10 downto 0);
	signal data_fifo_empty_true : std_logic;
	signal data_fifo_empty_pressure : std_logic;
	
	signal ctrl_fifo_din : std_logic_vector(63 downto 0);
	signal ctrl_fifo_dout : std_logic_vector(63 downto 0);
	signal ctrl_fifo_wren : std_logic;
	signal ctrl_fifo_rden : std_logic;
	signal ctrl_fifo_full : std_logic;
	signal ctrl_fifo_empty : std_logic;
	
	signal dma_stb_t : std_logic;
	signal dma_stb_valid : std_logic;
	signal dma_adr_cnt : unsigned(31 downto 0);
	signal dma_start_adr : unsigned(31 downto 0);
	signal dma_data_cnt : unsigned(31 downto 0);
	signal dma_data_cnt_d : unsigned(31 downto 0);
	signal dma_timeout_cnt : unsigned(31 downto 0);
	signal dma_ack_cnt : unsigned(7 downto 0);
	
	signal rx_data_local : std_logic_vector(31 downto 0);
	signal rx_valid_local : std_logic;
	signal rx_data_local_d : std_logic_vector(31 downto 0);
	signal rx_valid_local_d : std_logic;
	
	signal ctrl_fifo_dout_tmp : std_logic_vector(31 downto 0);
	
	signal time_cnt : unsigned(31 downto 0);
	signal time_pulse : std_logic;
	signal data_rate_cnt : unsigned(31 downto 0);
	signal trig_cnt : unsigned(31 downto 0);
	
	signal trig_pulse_d0 : std_logic;
	signal trig_pulse_d1 : std_logic;
	signal trig_pulse_pos : std_logic;
	
	
	-- Registers
	signal loopback : std_logic;
	signal data_rate : std_logic_vector(31 downto 0);
	
begin
	--Tie offs
	irq_o <= '0';
	busy_o <= data_fifo_full;

	-- Wishbone Slave
	wb_slave_proc: process(sys_clk_i, rst_n_i)
	begin
		if (rst_n_i = '0') then
			wb_dat_o <= (others => '0');
			wb_ack_o <= '0';
			wb_stall_o <= '0';
			ctrl_fifo_rden <= '0';
			rx_valid_local <= '0';
			ctrl_fifo_dout_tmp <= (others => '0');
			-- Regs
			loopback <= '0';
		elsif rising_edge(sys_clk_i) then
			-- Default
			wb_ack_o <= '0';
			ctrl_fifo_rden <= '0';
			wb_stall_o <= '0';
			rx_valid_local <= '0';
			
			if (wb_cyc_i = '1' and wb_stb_i = '1') then
				if (wb_we_i = '0') then
					-- READ
					if (wb_adr_i(3 downto 0) = x"0") then -- Start Addr
						if (ctrl_fifo_empty = '0') then
							wb_dat_o <= ctrl_fifo_dout(31 downto 0);
							ctrl_fifo_dout_tmp <= ctrl_fifo_dout(63 downto 32);
							wb_ack_o <= '1';
							ctrl_fifo_rden <= '1';
						else
							wb_dat_o <= x"FFFFFFFF";
							ctrl_fifo_dout_tmp <= (others => '0');
							wb_ack_o <= '1';
							ctrl_fifo_rden <= '0';
						end if;						
					elsif (wb_adr_i(3 downto 0) = x"1") then -- Count
						wb_dat_o <= ctrl_fifo_dout_tmp;
						wb_ack_o <= '1';
					elsif (wb_adr_i(3 downto 0) = x"2") then -- Loopback
						wb_dat_o(31 downto 1) <= (others => '0');
						wb_dat_o(0) <= loopback;
						wb_ack_o <= '1';
					elsif (wb_adr_i(3 downto 0) = x"3") then -- Data Rate
						wb_dat_o <= data_rate;
						wb_ack_o <= '1';
					elsif (wb_adr_i(3 downto 0) = x"5") then -- Bridge Empty
						wb_dat_o(31 downto 1) <= (others => '0');
						wb_dat_o(0) <= data_fifo_empty_true;
						wb_ack_o <= '1';	
					elsif (wb_adr_i(3 downto 0) = x"6") then -- Cur Count
						wb_dat_o <= std_logic_vector(dma_data_cnt_d);
						wb_ack_o <= '1';							
					else
						wb_dat_o <= x"DEADBEEF";
						wb_ack_o <= '1';
					end if;
				else
					-- WRITE
					wb_ack_o <= '1';
					if (wb_adr_i(3 downto 0) = x"2") then
						loopback <= wb_dat_i(0);
					elsif (wb_adr_i(3 downto 0) = x"4") then
						rx_valid_local <= '1';
					end if;
				end if;
			end if;
		end if;
	end process wb_slave_proc;

	-- Data from Rx
	data_rec : process (sys_clk_i, rst_n_i)
	begin
		if (rst_n_i <= '0') then
			data_fifo_wren <= '0';
			data_fifo_din <= (others => '0');
		elsif rising_edge(sys_clk_i) then
			if (loopback = '1') then
				data_fifo_wren <= rx_valid_local_d;
				data_fifo_din <= X"03000000" & rx_data_local_d;
			else
				data_fifo_wren <= rx_valid_i;
				data_fifo_din <=  rx_data_i;
			end if;
		end if;
	end process data_rec;
	
	-- Empty logic to produce some backpressure
	data_fifo_empty <= '1' when (data_fifo_empty_true = '1') else data_fifo_empty_pressure;
	empty_proc : process(dma_clk_i, rst_n_i)
	begin
		if (rst_n_i = '0') then
			data_fifo_empty_pressure <= '0';
			data_fifo_empty_cnt <= (others => '0');
		elsif rising_edge(dma_clk_i) then
			-- Timeout Counter
			if (data_fifo_empty_true = '0' and data_fifo_empty_pressure = '1') then
				data_fifo_empty_cnt <= data_fifo_empty_cnt + 1;
			elsif (data_fifo_empty_true = '1') then
				data_fifo_empty_cnt <= (others => '0');
			end if;
			
			if (data_fifo_empty_cnt > c_EMPTY_TIMEOUT) then
				data_fifo_empty_pressure <= '0';
			elsif (data_fifo_prog_empty = '0') then
				data_fifo_empty_pressure <= '0';
			elsif (data_fifo_empty_true = '1') then
				data_fifo_empty_pressure <= '1';
			end if;
		end if;
	end process empty_proc;
	
	-- DMA Master and data control
	dma_stb_valid <= dma_stb_t and not data_fifo_empty;
	
	to_ddr_proc: process(dma_clk_i, rst_n_i)
	begin
		if(rst_n_i = '0') then
			dma_stb_t <= '0';
			data_fifo_rden <= '0';
			dma_adr_o <= (others => '0');
			dma_dat_o <= (others => '0');
			dma_cyc_o <= '0';
			dma_stb_o <= '0';
			dma_we_o <= '1'; -- Write only
		elsif rising_edge(dma_clk_i) then
			if (data_fifo_empty = '0' and dma_stall_i = '0' and ctrl_fifo_full = '0') then
				dma_stb_t <= '1';
				data_fifo_rden <= '1';
			else
				dma_stb_t <= '0';
				data_fifo_rden <= '0';
			end if;
			
			if (data_fifo_empty = '0' or dma_ack_cnt > 0) then
				dma_cyc_o <= '1';
			else
				dma_cyc_o <= '0';
			end if;
				
			dma_adr_o <= std_logic_vector(dma_adr_cnt);
			dma_dat_o <= data_fifo_dout;
			dma_stb_o <= dma_stb_t and not data_fifo_empty;
			dma_we_o <= '1'; -- Write only
		end if;
	end process to_ddr_proc;
	
	adr_proc : process (dma_clk_i, rst_n_i)
	begin
		if (rst_n_i = '0') then
			ctrl_fifo_wren <= '0';
			dma_adr_cnt <= (others => '0');
			dma_start_adr <= (others => '0');
			dma_data_cnt <= (others => '0');
			dma_data_cnt_d <= (others => '0');
			dma_timeout_cnt <= (others => '0');
			ctrl_fifo_din(63 downto 0) <= (others => '0');
			dma_ack_cnt <= (others => '0');
		elsif rising_edge(dma_clk_i) then
			-- Address Counter
			if (dma_stb_valid = '1') then
				dma_adr_cnt <= dma_adr_cnt + 1;
			end if;
			
			if (dma_stb_valid = '1' and dma_ack_i = '0') then
				dma_ack_cnt <= dma_ack_cnt + 1;
			elsif (dma_stb_valid = '0' and dma_ack_i = '1' and dma_ack_cnt > 0) then
				dma_ack_cnt <= dma_ack_cnt - 1;
			end if;
			
			-- Package size counter
			-- Check if Fifo is full
			if (dma_stb_valid = '1' and dma_data_cnt >= c_PACKAGE_SIZE and ctrl_fifo_full = '0') then
				ctrl_fifo_din(63 downto  32) <= std_logic_vector(dma_data_cnt);
				ctrl_fifo_din(31 downto 0) <= std_logic_vector(dma_start_adr);
				dma_start_adr <= dma_start_adr + c_PACKAGE_SIZE;
				dma_data_cnt <= TO_UNSIGNED(2, 32);
				ctrl_fifo_wren <= '1';
			elsif (dma_stb_valid = '0' and dma_data_cnt >= c_PACKAGE_SIZE and ctrl_fifo_full = '0') then
				ctrl_fifo_din(63 downto  32) <= std_logic_vector(dma_data_cnt);
				ctrl_fifo_din(31 downto 0) <= std_logic_vector(dma_start_adr);
				dma_start_adr <= dma_start_adr + c_PACKAGE_SIZE;
				dma_data_cnt <= TO_UNSIGNED(0, 32);
				ctrl_fifo_wren <= '1';
			elsif (dma_stb_valid = '0' and dma_timeout_cnt >= c_TIMEOUT and dma_data_cnt > 0 and ctrl_fifo_full ='0') then
				ctrl_fifo_din(63 downto  32) <= std_logic_vector(dma_data_cnt);
				ctrl_fifo_din(31 downto 0) <= std_logic_vector(dma_start_adr);
				dma_start_adr <= dma_start_adr + dma_data_cnt;
				dma_data_cnt <= TO_UNSIGNED(0, 32);
				ctrl_fifo_wren <= '1';
			elsif (dma_stb_valid = '1') then
				dma_data_cnt <= dma_data_cnt + 2;
				ctrl_fifo_wren <= '0';
			else
				ctrl_fifo_wren <= '0';
			end if;
			dma_data_cnt_d <= dma_data_cnt;
--			if (dma_data_cnt = 0 and ctrl_fifo_wren = '1') then -- New package
--				ctrl_fifo_din(31 downto 0) <= std_logic_vector(dma_adr_cnt);
--			elsif (dma_data_cnt = 1 and ctrl_fifo_wren = '1') then -- Flying take over
--				ctrl_fifo_din(31 downto 0) <= std_logic_vector(dma_adr_cnt-1);
--			end if;
			
			-- Timeout counter
			if (dma_data_cnt > 0 and data_fifo_empty = '1') then
				dma_timeout_cnt <= dma_timeout_cnt + 1;
			elsif (data_fifo_empty = '0') then
				dma_timeout_cnt <= TO_UNSIGNED(0, 32);
			end if;
		end if;
	end process adr_proc;
	
	-- Data Rate maeasurement
	data_rate_proc: process(sys_clk_i, rst_n_i)
	begin
		if (rst_n_i = '0') then
			data_rate_cnt <= (others => '0');
			data_rate <= (others => '0');
			time_cnt <= (others => '0');
			time_pulse <= '0';
		elsif rising_edge(sys_clk_i) then
			-- 1Hz pulser
			if (time_cnt = c_TIME_FRAME) then
				time_cnt <= (others => '0');
				time_pulse <= '1';
			else
				time_cnt <= time_cnt + 1;
				time_pulse <= '0';
			end if;
			
			if (time_pulse = '1') then
				data_rate <= std_logic_vector(data_rate_cnt);
				data_rate_cnt <= (others => '0');
			elsif (data_fifo_wren = '1') then
				data_rate_cnt <= data_rate_cnt + 1;
			end if;
		end if;
	end process data_rate_proc;
	
	-- Loopback delay
	delayproc : process (sys_clk_i, rst_n_i)
	begin
		if (rst_n_i = '0') then
			rx_data_local <= (others => '0');
			rx_data_local_d <= (others => '0');
			rx_valid_local_d <= '0';
		elsif rising_edge(sys_clk_i) then
			rx_data_local_d <= wb_dat_i;
			rx_valid_local_d <= rx_valid_local;
		end if;
	end process;
	
	-- Trigger sync and count
	trig_sync : process (sys_clk_i, rst_n_i)
	begin
		if (rst_n_i = '0') then
			trig_pulse_d0 <= '0';
			trig_pulse_d1 <= '0';
			trig_pulse_pos <= '0';
			trig_cnt <= (others => '0');
		elsif rising_edge(sys_clk_i) then
			trig_pulse_d0 <= trig_pulse_i;
			trig_pulse_d1 <= trig_pulse_d0;
			if (trig_pulse_d0 = '1' and trig_pulse_d1 = '0') then
				trig_pulse_pos <= '1';
			else
				trig_pulse_pos <= '0';
			end if;
			if (trig_pulse_pos = '1') then
				trig_cnt <= trig_cnt + 1;
			end if;
		end if;
	end process trig_sync;

	cmp_rx_bridge_fifo : rx_bridge_fifo PORT MAP (
		rst => not rst_n_i,
		wr_clk => sys_clk_i,
		rd_clk => dma_clk_i,
		din => data_fifo_din,
		wr_en => data_fifo_wren,
		rd_en => data_fifo_rden,
		prog_full_thresh => std_logic_vector(c_ALMOST_FULL_THRESHOLD),
		prog_empty_thresh => std_logic_vector(c_EMPTY_THRESHOLD),
		dout => data_fifo_dout,
		full => data_fifo_full,
		empty => data_fifo_empty_true,
		prog_full => data_fifo_almost_full,
		prog_empty => data_fifo_prog_empty
	);
	
	cmp_rx_bridge_ctrl_fifo : rx_bridge_ctrl_fifo PORT MAP (
		rst => not rst_n_i,
		wr_clk => dma_clk_i,
		rd_clk => sys_clk_i,
		din => ctrl_fifo_din,
		wr_en => ctrl_fifo_wren,
		rd_en => ctrl_fifo_rden,
		dout => ctrl_fifo_dout,
		full => ctrl_fifo_full,
		empty => ctrl_fifo_empty
	);
end Behavioral;

