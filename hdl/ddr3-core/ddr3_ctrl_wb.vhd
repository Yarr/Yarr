library IEEE;
use IEEE.STD_LOGIC_1164.all;
--use IEEE.NUMERIC_STD.all;
use IEEE.std_logic_unsigned.all;
use IEEE.std_logic_arith.all;


library work;
use work.ddr3_ctrl_pkg.all;

entity ddr3_ctrl_wb is
    generic (
        g_BYTE_ADDR_WIDTH : integer := 30;
        g_MASK_SIZE       : integer := 4;
        g_DATA_PORT_SIZE  : integer := 32
    );
    port (
        ----------------------------------------------------------------------------
        -- Reset input (active low)
        ----------------------------------------------------------------------------
        rst_n_i : in std_logic;

        ----------------------------------------------------------------------------
        -- Status
        ----------------------------------------------------------------------------

        ----------------------------------------------------------------------------
        -- DDR controller port
        ----------------------------------------------------------------------------
        ddr_cmd_clk_o       : out std_logic;
        ddr_cmd_en_o        : out std_logic;
        ddr_cmd_instr_o     : out std_logic_vector(2 downto 0);
        ddr_cmd_bl_o        : out std_logic_vector(5 downto 0);
        ddr_cmd_byte_addr_o : out std_logic_vector(g_BYTE_ADDR_WIDTH - 1 downto 0);
        ddr_cmd_empty_i     : in  std_logic;
        ddr_cmd_full_i      : in  std_logic;
        ddr_wr_clk_o        : out std_logic;
        ddr_wr_en_o         : out std_logic;
        ddr_wr_mask_o       : out std_logic_vector(g_MASK_SIZE - 1 downto 0);
        ddr_wr_data_o       : out std_logic_vector(g_DATA_PORT_SIZE - 1 downto 0);
        ddr_wr_full_i       : in  std_logic;
        ddr_wr_empty_i      : in  std_logic;
        ddr_wr_count_i      : in  std_logic_vector(6 downto 0);
        ddr_wr_underrun_i   : in  std_logic;
        ddr_wr_error_i      : in  std_logic;
        ddr_rd_clk_o        : out std_logic;
        ddr_rd_en_o         : out std_logic;
        ddr_rd_data_i       : in  std_logic_vector(g_DATA_PORT_SIZE - 1 downto 0);
        ddr_rd_full_i       : in  std_logic;
        ddr_rd_empty_i      : in  std_logic;
        ddr_rd_count_i      : in  std_logic_vector(6 downto 0);
        ddr_rd_overflow_i   : in  std_logic;
        ddr_rd_error_i      : in  std_logic;

        ----------------------------------------------------------------------------
        -- Wishbone bus port
        ----------------------------------------------------------------------------
        wb_clk_i   : in  std_logic;
        wb_sel_i   : in  std_logic_vector(g_MASK_SIZE - 1 downto 0);
        wb_cyc_i   : in  std_logic;
        wb_stb_i   : in  std_logic;
        wb_we_i    : in  std_logic;
        wb_addr_i  : in  std_logic_vector(31 downto 0);
        wb_data_i  : in  std_logic_vector(g_DATA_PORT_SIZE - 1 downto 0);
        wb_data_o  : out std_logic_vector(g_DATA_PORT_SIZE - 1 downto 0);
        wb_ack_o   : out std_logic;
        wb_stall_o : out std_logic
    );
end entity ddr3_ctrl_wb;

architecture behavioral of ddr3_ctrl_wb is
    --------------------------------------
    -- Constants
    --------------------------------------
    constant c_DDR_BURST_LENGTH : std_logic_vector(5 downto 0) := CONV_STD_LOGIC_VECTOR(16, 6);
    constant c_FIFO_ALMOST_FULL : std_logic_vector(6 downto 0) := CONV_STD_LOGIC_VECTOR(53, 7);
    constant c_ADDR_SHIFT : integer := log2_ceil(g_DATA_PORT_SIZE/8);
    constant c_STALL_TIME : std_logic_vector(3 downto 0) := CONV_STD_LOGIC_VECTOR(20, 4);

    --------------------------------------
    -- Signals
    --------------------------------------
    signal ddr_wr_ack : std_logic;
    signal ddr_rd_ack : std_logic;
    signal ddr_rd_en  : std_logic;
    signal ddr_cmd_en : std_logic;

    signal wb_stall   : std_logic;
    signal wb_stall_d : std_logic;
    signal wb_we_d    : std_logic;
    signal wb_addr_d  : std_logic_vector(g_DATA_PORT_SIZE - 1 downto 0);
    signal wb_stall_restart : std_logic;
    
    signal addr_shift : std_logic_vector(c_ADDR_SHIFT-1 downto 0);
    --------------------------------------
    -- Counter
    --------------------------------------
    signal wb_stall_cnt : std_logic_vector(3 downto 0);
    signal ddr_burst_cnt : std_logic_vector(5 downto 0);
    signal ddr_burst_cnt_d : std_logic_vector(5 downto 0);
    
begin
    -- Tie offs
    ddr_wr_clk_o <= wb_clk_i;
    ddr_rd_clk_o <= wb_clk_i;
    ddr_cmd_clk_o <= wb_clk_i;
    wb_ack_o <= ddr_wr_ack or ddr_rd_ack;
    
    --------------------------------------
    -- Wishbone write
    --------------------------------------
    p_wb_write : process (wb_clk_i, rst_n_i)
    begin
        if (rst_n_i = '0') then
            ddr_wr_en_o <= '0';
            ddr_wr_ack <= '0';
            ddr_wr_data_o <= (others => '0');
            ddr_wr_mask_o <= (others => '0');
        elsif rising_edge(wb_clk_i) then
            if (wb_cyc_i = '1' and wb_stb_i = '1' and wb_we_i = '1') then
                ddr_wr_data_o <= wb_data_i;
                ddr_wr_mask_o <= not(wb_sel_i);
                ddr_wr_en_o <= '1';
                ddr_wr_ack <= '1';
            else
                ddr_wr_data_o <= (others => '0');
                ddr_wr_mask_o <= (others => '0');
                ddr_wr_en_o <= '0';
                ddr_wr_ack <= '0';
            end if;
        end if;
    end process p_wb_write;

    --------------------------------------
    -- Wishbone read
    --------------------------------------
    ddr_rd_en_o <= ddr_rd_en;
    p_wb_read : process (wb_clk_i, rst_n_i)
    begin
        if (rst_n_i = '0') then
            ddr_rd_en <= '0';
            ddr_rd_ack <= '0';
            wb_data_o <= (others => '0');
        elsif rising_edge(wb_clk_i) then
            if (wb_cyc_i = '1' and ddr_rd_empty_i = '0') then
                ddr_rd_en <= '1';
            else
                ddr_rd_en <= '0';
            end if;

            if (ddr_rd_en = '1' and ddr_rd_empty_i = '0') then
                ddr_rd_ack <= '1';
                wb_data_o <= ddr_rd_data_i;
            else
                ddr_rd_ack <= '0';
                wb_data_o <= (others => '1');
            end if;
        end if;
    end process p_wb_read;
    
    --------------------------------------
    -- DDR Control
    --------------------------------------
    addr_shift <= (others => '0');
    ddr_cmd_en_o <= ddr_cmd_en;
    p_ddr_ctrl : process (wb_clk_i, rst_n_i)
    begin
        if (rst_n_i = '0') then
            ddr_burst_cnt <= (others => '0');
            ddr_cmd_en <= '0';
            ddr_cmd_byte_addr_o <= (others => '0');
            ddr_cmd_instr_o <= (others => '0');
            ddr_cmd_bl_o <= (others => '0');
            wb_addr_d <= (others => '0');
            wb_we_d <= '0';
            wb_stall_restart <= '0';
        elsif rising_edge(wb_clk_i) then
            if (wb_cyc_i = '1' and wb_stb_i = '1') then
                if (ddr_burst_cnt = c_DDR_BURST_LENGTH) then
                    ddr_burst_cnt <= CONV_STD_LOGIC_VECTOR(1, 6);
                    ddr_cmd_en <= '1';
                else
                    ddr_burst_cnt <= ddr_burst_cnt + 1;
                    ddr_cmd_en <= '0';
                end if;
            elsif (wb_stb_i = '0' and ddr_burst_cnt > 0) then
                ddr_burst_cnt <= (others => '0');
                ddr_cmd_en <= '1';
            else
                ddr_cmd_en <= '0';
            end if;
            ddr_cmd_bl_o <= std_logic_vector(ddr_burst_cnt - 1);
        
            ddr_burst_cnt_d <= ddr_burst_cnt;
            if (wb_stb_i = '1') then
               wb_addr_d <= wb_addr_i;
               wb_we_d <= wb_we_i;
            end if;
            
            
            if (ddr_burst_cnt = 0) then
                ddr_cmd_byte_addr_o <= wb_addr_i(g_BYTE_ADDR_WIDTH-c_ADDR_SHIFT-1 downto 0) & addr_shift;
                ddr_cmd_instr_o <= "00" & not(wb_we_i);
            elsif (ddr_cmd_en = '1') then
                ddr_cmd_byte_addr_o <= wb_addr_d(g_BYTE_ADDR_WIDTH-c_ADDR_SHIFT-1 downto 0) & addr_shift;
                ddr_cmd_instr_o <= "00" & not(wb_we_d);            
            end if;
        end if;
    end process p_ddr_ctrl;
    --------------------------------------
    -- Stall proc
    --------------------------------------
    wb_stall_o <= wb_stall;
    p_wb_stall : process (wb_clk_i, rst_n_i)
    begin
        if (rst_n_i = '0') then
            wb_stall <= '0';
            wb_stall_d <= '0';
            wb_stall_cnt <= (others => '0');
        elsif rising_edge(wb_clk_i) then
            if (ddr_wr_count_i > c_FIFO_ALMOST_FULL or
                    ddr_rd_count_i > c_FIFO_ALMOST_FULL or
                    ddr_wr_full_i = '1' or
                    ddr_rd_full_i = '1' or
                    ddr_cmd_full_i = '1') then
                wb_stall <= '1';
                wb_stall_cnt <= c_STALL_TIME;
            elsif (wb_stall_cnt > 1) then
                wb_stall <= '1';
                wb_stall_cnt <= wb_stall_cnt - 1;
            else
                wb_stall <= '0';
                wb_stall_cnt <= (others =>'0');
            end if;
            wb_stall_d <= wb_stall;
        end if;
    end process p_wb_stall;
end architecture behavioral;

