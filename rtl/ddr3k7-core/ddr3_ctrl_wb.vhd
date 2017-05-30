library IEEE;
use IEEE.STD_LOGIC_1164.all;
use IEEE.NUMERIC_STD.all;


library work;
use work.ddr3_ctrl_pkg.all;

entity ddr3_ctrl_wb is
    generic (
           g_BYTE_ADDR_WIDTH : integer := 29;
           g_MASK_SIZE       : integer := 8;
           g_DATA_PORT_SIZE  : integer := 64
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
        
        ddr_addr_o                  : out    std_logic_vector(g_BYTE_ADDR_WIDTH-1 downto 0);
        ddr_cmd_o                   : out    std_logic_vector(2 downto 0);
        ddr_cmd_en_o                : out    std_logic;
        ddr_wdf_data_o              : out    std_logic_vector(511 downto 0);
        ddr_wdf_end_o               : out    std_logic;
        ddr_wdf_mask_o              : out    std_logic_vector(63 downto 0);
        ddr_wdf_wren_o              : out    std_logic;
        ddr_rd_data_i               : in   std_logic_vector(511 downto 0);
        ddr_rd_data_end_i           : in   std_logic;
        ddr_rd_data_valid_i         : in   std_logic;
        ddr_rdy_i                   : in   std_logic;
        ddr_wdf_rdy_i               : in   std_logic;
        ddr_sr_req_o                : out    std_logic;
        ddr_ref_req_o               : out    std_logic;
        ddr_zq_req_o                : out    std_logic;
        ddr_sr_active_i             : in   std_logic;
        ddr_ref_ack_i               : in   std_logic;
        ddr_zq_ack_i                : in   std_logic;
        ddr_ui_clk_i                  : in   std_logic;
        ddr_ui_clk_sync_rst_i           : in   std_logic;
        ddr_init_calib_complete_i       : in   std_logic;

        ----------------------------------------------------------------------------
        -- Wishbone bus port
        ----------------------------------------------------------------------------
        wb_clk_i   : in  std_logic;
        wb_sel_i   : in  std_logic_vector(g_MASK_SIZE - 1 downto 0);
        wb_cyc_i   : in  std_logic;
        wb_stb_i   : in  std_logic;
        wb_we_i    : in  std_logic;
        wb_addr_i  : in  std_logic_vector(32 - 1 downto 0);
        wb_data_i  : in  std_logic_vector(g_DATA_PORT_SIZE - 1 downto 0);
        wb_data_o  : out std_logic_vector(g_DATA_PORT_SIZE - 1 downto 0);
        wb_ack_o   : out std_logic;
        wb_stall_o : out std_logic;
        
        ----------------------------------------------------------------------------
        -- Debug ports
        ----------------------------------------------------------------------------
        ddr_rd_mask_rd_data_count_do : out std_logic_vector(3 downto 0);
        ddr_rd_data_rd_data_count_do : out std_logic_vector(3 downto 0);
        ddr_rd_fifo_full_do : out std_logic_vector(1 downto 0);
        ddr_rd_fifo_empty_do : out std_logic_vector(1 downto 0);
        ddr_rd_fifo_rd_do : out std_logic_vector(1 downto 0)
    );
end entity ddr3_ctrl_wb;

architecture behavioral of ddr3_ctrl_wb is
    
    
    
    
    --------------------------------------
    -- Constants
    --------------------------------------
    --constant c_DDR_BURST_LENGTH : unsigned(5 downto 0) := TO_UNSIGNED(16, 6);
	--constant c_READ_STALL_ASSERT : unsigned(6 downto 0) := TO_UNSIGNED(54, 7);
	--constant c_READ_STALL_NEGATE : unsigned(6 downto 0) := TO_UNSIGNED(42, 7);
	--constant c_WRITE_STALL_ASSERT : unsigned(6 downto 0) := TO_UNSIGNED(52, 7);
	--constant c_WRITE_STALL_NEGATE : unsigned(6 downto 0) := TO_UNSIGNED(42, 7);
    --constant c_ADDR_SHIFT : integer := log2_ceil(g_DATA_PORT_SIZE/8);
    --constant c_STALL_TIME : unsigned(3 downto 0) := TO_UNSIGNED(15, 4);
    constant c_write_wait_time : unsigned(9 downto 0) := TO_UNSIGNED(15, 10);
    constant c_read_wait_time : unsigned(9 downto 0) := TO_UNSIGNED(15, 10);
    
    constant c_in_order_detection : string := "OFF";
    constant c_endianness : string := "LITTLE";
    
    constant c_register_shift_size : integer := 8;
    type data_array is array (0 to c_register_shift_size-1) of std_logic_vector(g_DATA_PORT_SIZE - 1 downto 0);
    type mask_array is array (0 to c_register_shift_size-1) of std_logic_vector(g_MASK_SIZE - 1 downto 0);
    type addr_array is array (0 to c_register_shift_size-1) of std_logic_vector(g_BYTE_ADDR_WIDTH - 1 downto 0);

    --------------------------------------
    -- Signals
    --------------------------------------
    signal rst_s : std_logic;
    

    signal wb_wr_addr_row_s : std_logic_vector(c_register_shift_size-1 downto 0);
    signal wb_wr_row_size_s : integer;
    --signal wb_wr_addr_row_next_s : std_logic_vector(3 downto 0);
    signal wb_wr_row_size_next_s : integer;    
    
    

    
    signal wb_wr_shift_flush_s : std_logic;
    signal wb_wr_shift_flush_vs : std_logic_vector(c_register_shift_size-1 downto 0);
    
    signal wb_wr_data_shift_a : data_array;
    signal wb_wr_data_shift_s : std_logic_vector(511 downto 0);
    --signal wb_wr_data_shift_swp_s : std_logic_vector(511 downto 0);
    signal wb_wr_mask_shift_a : mask_array;
    signal wb_wr_qmask_shift_s : std_logic_vector(c_register_shift_size-1 downto 0);
    signal wb_wr_addr_shift_a : addr_array;
    

    
    signal wb_wr_addr_ref_a : addr_array;
    signal wb_wr_addr_ref_next_a : addr_array;

    
    signal fifo_wb_wr_mask_din_s : std_logic_vector(63 downto 0);
    signal fifo_wb_wr_addr_din_s : std_logic_vector(g_BYTE_ADDR_WIDTH-1 downto 0);
    signal fifo_wb_wr_din_s : std_logic_vector(604 downto 0);
    signal fifo_wb_wr_wr_s : std_logic;
    signal fifo_wb_wr_rd_s : std_logic;
    signal fifo_wb_wr_rd_d : std_logic;
    signal fifo_wb_wr_dout_s : std_logic_vector(604 downto 0);
    signal fifo_wb_wr_full_s : std_logic;
    signal fifo_wb_wr_empty_s : std_logic;
    
    signal wb_rd_shift_flush_s : std_logic;
    
    signal wb_rd_row_size_s : integer;
    signal wb_rd_addr_row_s : std_logic_vector(c_register_shift_size-1 downto 0);
    signal wb_rd_qmask_shift_s : std_logic_vector(c_register_shift_size-1 downto 0);
    signal wb_rd_data_shift_a : data_array;
    signal wb_rd_ack_shift_s : std_logic_vector(c_register_shift_size-1 downto 0);
    signal wb_rd_addr_shift_a : addr_array;
    signal wb_rd_addr_ref_a : addr_array;
    
    --signal fifo_wb_rd_addr_din_a : addr_array;
    
    signal fifo_wb_rd_addr_din_s : std_logic_vector(g_BYTE_ADDR_WIDTH-1 downto 0);
    signal fifo_wb_rd_addr_wr_s : std_logic;
    signal fifo_wb_rd_addr_rd_s : std_logic;
    signal fifo_wb_rd_addr_rd_d : std_logic;
    signal fifo_wb_rd_addr_dout_s : std_logic_vector(g_BYTE_ADDR_WIDTH-1 downto 0);
    signal fifo_wb_rd_addr_full_s : std_logic;
    signal fifo_wb_rd_addr_empty_s : std_logic;
    
    --signal fifo_wb_rd_mask_din_a : mask_array;
    --signal wb_rd_mask_din_s : std_logic_vector(3 downto 0);
    signal fifo_wb_rd_mask_din_s : std_logic_vector(c_register_shift_size-1 downto 0);
    signal fifo_wb_rd_mask_wr_s : std_logic;
    signal fifo_wb_rd_mask_rd_s : std_logic;
    --signal fifo_wb_rd_addr_rd_d : std_logic;
    signal fifo_wb_rd_mask_dout_s : std_logic_vector(c_register_shift_size-1 downto 0);
    signal fifo_wb_rd_mask_full_s : std_logic;
    signal fifo_wb_rd_mask_empty_s : std_logic;
    
    signal fifo_wb_rd_mask_rd_data_count_s : STD_LOGIC_VECTOR(3 DOWNTO 0);
    
    signal fifo_wb_rd_data_din_s : std_logic_vector(511 downto 0);
    signal fifo_wb_rd_data_wr_s : std_logic;
    signal fifo_wb_rd_data_rd_s : std_logic;
    signal fifo_wb_rd_data_rd_d : std_logic;
    signal fifo_wb_rd_data_dout_s : std_logic_vector(511 downto 0);
    signal fifo_wb_rd_data_dout_a : data_array;
    signal fifo_wb_rd_data_full_s : std_logic;
    signal fifo_wb_rd_data_empty_s : std_logic;
    
    signal fifo_wb_rd_data_rd_data_count_s : STD_LOGIC_VECTOR(3 DOWNTO 0);
    
    signal ddr_cmd_s : std_logic; -- '1' = read, '0' = write
    signal ddr_rd_data_end_s : std_logic;
    
    --signal wb_rd_read_addr_rec_nb_s : unsigned(1 downto 0); -- received addresses number

    signal wb_stall_s   : std_logic;
    signal wb_stall_d : std_logic;
    signal wb_stall_dd : std_logic;
    signal wb_wr_ack_s : std_logic;
    signal wb_rd_ack_s : std_logic;
    
    --------------------------------------
    -- debug
    --------------------------------------    
    constant qword_swap_ds : std_logic_vector(2 downto 0) := "111";
    constant c_BYTE_SWAP : boolean := true;
    
    
    --------------------------------------
    -- Counter
    --------------------------------------
    signal wb_write_cnt : integer range 0 to c_register_shift_size;
    --signal wb_write_cnt_d : integer range 0 to c_register_shift_size;
    signal wb_write_wait_cnt : unsigned(9 downto 0);
    
    --signal wb_read_cnt : integer range 0 to 3;
    signal wb_read_wait_cnt : unsigned(9 downto 0);
    --signal wb_read_data_cnt : integer range 0 to 3;
    signal wb_read_data_cnt_d : integer range 0 to 3;
    
    --signal wb_read_test : integer;
    
    
    
begin
    rst_s <= not rst_n_i;
    
    ddr_sr_req_o                <= '0';
    ddr_ref_req_o               <= '0';
    ddr_zq_req_o                <= '0';
    
    --------------------------------------
    -- Read FIFOs debug
    --------------------------------------
    
    ddr_rd_fifo_full_do  <=  fifo_wb_rd_data_full_s & fifo_wb_rd_mask_full_s;
    ddr_rd_fifo_empty_do <=  fifo_wb_rd_data_empty_s & fifo_wb_rd_mask_empty_s;
    ddr_rd_fifo_rd_do    <= fifo_wb_rd_data_rd_s & fifo_wb_rd_mask_rd_s;
    ddr_rd_mask_rd_data_count_do <= fifo_wb_rd_mask_rd_data_count_s;
    ddr_rd_data_rd_data_count_do <= fifo_wb_rd_data_rd_data_count_s;

    --------------------------------------
    -- QWORD swap debug
    --------------------------------------
    
    
    
    
    --------------------------------------
    -- Wishbone ack
    --------------------------------------    
    
    wb_ack_o <= wb_wr_ack_s or wb_rd_ack_s;
    

    
    --------------------------------------
    -- Wishbone write
    --------------------------------------
    
    p_wb_write : process (wb_clk_i, rst_n_i)
    begin
        if (rst_n_i = '0') then
            wb_write_cnt <= 0;
            wb_write_wait_cnt <= c_write_wait_time;
            --fifo_wb_wr_addr_din_s <= (others => '0');
            --wb_wr_addr_s <= (others => '0');
            wb_wr_qmask_shift_s <= (others => '0');
            
            for i in 0 to c_register_shift_size-1 loop
                wb_wr_addr_shift_a(i) <= (others => '1');
                wb_wr_data_shift_a(i) <= (others => '0');
                wb_wr_mask_shift_a(i) <= (others => '0');
            end loop;

            --fifo_wb_wr_wr_s <= '0';
            wb_wr_ack_s <= '0';
        elsif rising_edge(wb_clk_i) then

            -- Register Shift
            if (wb_cyc_i = '1' and wb_stb_i = '1' and wb_we_i = '1') then
                for i in 0 to c_register_shift_size-2 loop
                    wb_wr_addr_shift_a(i) <= wb_wr_addr_shift_a(i+1);
                    wb_wr_data_shift_a(i) <= wb_wr_data_shift_a(i+1);
                    wb_wr_mask_shift_a(i) <= wb_wr_mask_shift_a(i+1);
                    wb_wr_qmask_shift_s(i) <= wb_wr_qmask_shift_s(i+1);
                end loop;   
            else
    
                 wb_wr_ack_s <= '0';
                 if(wb_wr_qmask_shift_s /= (wb_wr_qmask_shift_s'range => '0')) then
                     
                     if (wb_write_wait_cnt = 0) then
                         
                         for i in 0 to c_register_shift_size-2 loop
                             wb_wr_addr_shift_a(i) <= wb_wr_addr_shift_a(i+1);
                             wb_wr_data_shift_a(i) <= wb_wr_data_shift_a(i+1);
                             wb_wr_mask_shift_a(i) <= wb_wr_mask_shift_a(i+1);
                             wb_wr_qmask_shift_s(i) <= wb_wr_qmask_shift_s(i+1);
                         end loop;
                         
                         wb_wr_addr_shift_a(c_register_shift_size-1) <= (others => '1');
                         wb_wr_data_shift_a(c_register_shift_size-1) <= (others => '1');
                         wb_wr_mask_shift_a(c_register_shift_size-1) <= (others => '0');
                         wb_wr_qmask_shift_s(c_register_shift_size-1) <= '0';
                         
                     else
                        wb_write_wait_cnt <= wb_write_wait_cnt - 1;
                     end if;
                 end if;         
            end if;
            
            -- Erase the data sent to the FIFO
            if(wb_wr_shift_flush_s = '1') then
                --wb_write_cnt <= 1;
                wb_write_wait_cnt <= c_write_wait_time;
                --for i in 0 to (wb_wr_row_size_s-1) loop
                for i in 0 to (c_register_shift_size-1) loop
                    if i <= (wb_wr_row_size_s-1) then
                        wb_wr_addr_shift_a(i) <= (others => '1');
                        wb_wr_data_shift_a(i) <= (others => '1');
                        wb_wr_mask_shift_a(i) <= (others => '0');
                        wb_wr_qmask_shift_s(i) <= '0';
                    end if;
                end loop;
            end if;
            
            
            -- Input data going in the shift register
            if (wb_cyc_i = '1' and wb_stb_i = '1' and wb_we_i = '1') then
                wb_wr_addr_shift_a(c_register_shift_size-1) <= wb_addr_i(g_BYTE_ADDR_WIDTH-1 downto 0);
                wb_wr_data_shift_a(c_register_shift_size-1) <= wb_data_i;
                wb_wr_mask_shift_a(c_register_shift_size-1) <= wb_sel_i;
                wb_wr_qmask_shift_s(c_register_shift_size-1) <= '1';
                
                wb_wr_ack_s <= '1';
                wb_write_wait_cnt <= c_write_wait_time;
            end if;
            
            
        end if;
        
        
        
    end process p_wb_write;
    
    
    
    p_wb_write_rtl : process (wb_write_wait_cnt,wb_wr_addr_shift_a,wb_wr_addr_ref_a,wb_wr_qmask_shift_s,wb_wr_addr_row_s,wb_wr_shift_flush_s)
    begin
        wb_wr_shift_flush_s <= '0';
        --fifo_wb_wr_addr_din_s <= wb_wr_addr_ref_a(0);
        fifo_wb_wr_addr_din_s <= (others => '0');
        for i in 0 to (c_register_shift_size-1) loop
            if wb_wr_qmask_shift_s(i) = '1' and wb_wr_addr_ref_a(i)(2 downto 0) = "000" then
                    fifo_wb_wr_addr_din_s <= wb_wr_addr_ref_a(i);
                    wb_wr_shift_flush_s <= '1';
            end if;
            
        end loop;
        
        
        fifo_wb_wr_wr_s <= wb_wr_shift_flush_s;
        
        for i in 0 to (c_register_shift_size-1) loop
                wb_wr_addr_ref_a(i) <= std_logic_vector(unsigned(wb_wr_addr_shift_a(i)) - i); -- little endian
        end loop;
        
    end process p_wb_write_rtl;
    
    
    in_order_detection_wr: if c_in_order_detection = "ON" generate
    p_wb_write_address_row : process(wb_wr_addr_ref_a,wb_wr_qmask_shift_s)
    begin

        wb_wr_addr_row_s(0) <= '1';
        for i in 1 to c_register_shift_size-1 loop
            if wb_wr_addr_ref_a(i) = wb_wr_addr_ref_a(0) and wb_wr_qmask_shift_s(i) = '1' then
                wb_wr_addr_row_s(i) <= '1';
            else
                wb_wr_addr_row_s(i) <= '0';
            end if;
          
            
        end loop;
        
    end process p_wb_write_address_row;
    end generate in_order_detection_wr;
    
    without_in_order_detection_wr: if c_in_order_detection /= "ON" generate
    p_wb_write_address_row : process(wb_wr_addr_ref_a,wb_wr_qmask_shift_s)
    begin
        for i in 0 to c_register_shift_size-1 loop
               wb_wr_addr_row_s(i) <= '1';
        end loop;
    end process p_wb_write_address_row;
    end generate without_in_order_detection_wr;
    
    with wb_wr_addr_row_s select wb_wr_row_size_s <=
        8 when "11111111",
        7 when "01111111",
        6 when "00111111",
        5 when "00011111",
        4 when "00001111",
        3 when "00000111",
        2 when "00000011",
        1 when "00000001",
        0 when others;
   
    
    
    
    wr_mask_g:for i in 0 to c_register_shift_size-1 generate
        fifo_wb_wr_mask_din_s((i)*8+7 downto (i)*8)   <= wb_wr_mask_shift_a(i) when wb_wr_qmask_shift_s(i) = '1' and wb_wr_addr_row_s(i) = '1' else (others=>'0');
    end generate wr_mask_g;
    
    
    -- No Little endian conversion
    wb_wr_data_shift_s <= wb_wr_data_shift_a(7) & 
                          wb_wr_data_shift_a(6) & 
                          wb_wr_data_shift_a(5) & 
                          wb_wr_data_shift_a(4) & 
                          wb_wr_data_shift_a(3) & 
                          wb_wr_data_shift_a(2) & 
                          wb_wr_data_shift_a(1) & 
                          wb_wr_data_shift_a(0);
 

    
--    qword_wr_swap_comp:qword_swap_512
--    port map(
--        qword_swap => "000",
--        din => wb_wr_data_shift_s,
--        dout => wb_wr_data_shift_swp_s
--    );
    

    fifo_wb_wr_din_s <= fifo_wb_wr_addr_din_s & 
                        fifo_wb_wr_mask_din_s &
                        wb_wr_data_shift_s;
   

    
    fifo_wb_write : fifo_315x16
    PORT MAP (
        rst => rst_s,
        wr_clk => wb_clk_i,
        rd_clk => ddr_ui_clk_i,
        din => fifo_wb_wr_din_s,
        wr_en => fifo_wb_wr_wr_s,
        rd_en => fifo_wb_wr_rd_s,
        dout => fifo_wb_wr_dout_s,
        full => fifo_wb_wr_full_s,
        empty => fifo_wb_wr_empty_s
    );
    


    
    
    

    --------------------------------------
    -- Wishbone read
    --------------------------------------

    p_wb_read : process (wb_clk_i, rst_n_i)
        
    begin
    if (rst_n_i = '0') then
        --wb_read_test <= 0;
        --wb_read_cnt <= 0;
        wb_read_wait_cnt <= c_read_wait_time;
        
        wb_rd_qmask_shift_s <= (others => '0');
        
        for i in 0 to c_register_shift_size-1 loop
            wb_rd_addr_shift_a(i) <= (others => '1');
        end loop;
        
    elsif rising_edge(wb_clk_i) then
        -- Register Shift
        if (wb_cyc_i = '1' and wb_stb_i = '1' and wb_we_i = '0') then
            for i in 0 to c_register_shift_size-2 loop
                wb_rd_addr_shift_a(i) <= wb_rd_addr_shift_a(i+1);
                wb_rd_qmask_shift_s(i) <= wb_rd_qmask_shift_s(i+1);
            end loop; 
        else
          if(wb_rd_qmask_shift_s /= (wb_rd_qmask_shift_s'range => '0')) then
                             
                if (wb_read_wait_cnt = 0) then
                
                    for i in 0 to c_register_shift_size-2 loop
                        wb_rd_addr_shift_a(i) <= wb_rd_addr_shift_a(i+1);
                        wb_rd_qmask_shift_s(i) <= wb_rd_qmask_shift_s(i+1);
                    end loop;
                    
                    wb_rd_addr_shift_a(c_register_shift_size-1) <= (others => '1');
                    wb_rd_qmask_shift_s(c_register_shift_size-1) <= '0';
                
                else
                    wb_read_wait_cnt <= wb_read_wait_cnt - 1;
                end if;
            end if;
        end if;
        
        
        -- Erase the data sent to the FIFO
        if(wb_rd_shift_flush_s = '1') then

            wb_read_wait_cnt <= c_read_wait_time;
            for i in 0 to (c_register_shift_size-1) loop
                if i <= (wb_rd_row_size_s-1) then
                    wb_rd_addr_shift_a(i) <= (others => '1');
                    wb_rd_qmask_shift_s(i) <= '0';
                end if;
            end loop;
        end if;
        
        -- Input data going in the shift register
        if (wb_cyc_i = '1' and wb_stb_i = '1' and wb_we_i = '0') then
            wb_rd_addr_shift_a(c_register_shift_size-1) <= wb_addr_i(g_BYTE_ADDR_WIDTH-1 downto 0);
            wb_rd_qmask_shift_s(c_register_shift_size-1) <= '1';
            
            wb_read_wait_cnt <= c_read_wait_time;
        else

        end if;
    end if;
    end process p_wb_read;
    

    
    p_wb_read_rtl : process (wb_read_wait_cnt,wb_rd_addr_shift_a,wb_rd_addr_ref_a,wb_rd_qmask_shift_s,wb_rd_addr_row_s,wb_rd_shift_flush_s)
    begin
        
        wb_rd_shift_flush_s <= wb_rd_qmask_shift_s(0);
        fifo_wb_rd_addr_wr_s <= wb_rd_shift_flush_s;
        fifo_wb_rd_mask_wr_s <= wb_rd_shift_flush_s;
        
        wb_rd_shift_flush_s <= '0';
        fifo_wb_rd_addr_din_s <= (others => '0');
        for i in 0 to (c_register_shift_size-1) loop
            if wb_rd_qmask_shift_s(i) = '1' and wb_rd_addr_ref_a(i)(2 downto 0) = "000" then
                    fifo_wb_rd_addr_din_s <= wb_rd_addr_ref_a(i);
                    wb_rd_shift_flush_s <= '1';
            end if;
            
        end loop;
        
        for i in 0 to (c_register_shift_size-1) loop
                wb_rd_addr_ref_a(i) <= std_logic_vector(unsigned(wb_rd_addr_shift_a(i)) - i); -- little endian
        end loop;
        
    end process p_wb_read_rtl;
    
    
    in_order_detection_rd: if c_in_order_detection = "ON" generate
    p_wb_read_address_row : process(wb_rd_addr_ref_a)
    begin
        wb_rd_addr_row_s(0) <= '1';
        for i in 1 to c_register_shift_size-1 loop
            if wb_rd_addr_ref_a(i) = wb_rd_addr_ref_a(0) then
                wb_rd_addr_row_s(i) <= '1';
            else
                wb_rd_addr_row_s(i) <= '0';
            end if;
            
        end loop;
        
    end process p_wb_read_address_row;
    end generate in_order_detection_rd;
    
    without_in_order_detection_rd: if c_in_order_detection /= "ON" generate
    p_wb_write_address_row : process(wb_wr_addr_ref_a,wb_wr_qmask_shift_s)
    begin
        for i in 0 to c_register_shift_size-1 loop
               wb_rd_addr_row_s(i) <= '1';
        end loop;
    end process p_wb_write_address_row;
    end generate without_in_order_detection_rd;
    
    with wb_rd_addr_row_s select wb_rd_row_size_s <=
        8 when "11111111",
        7 when "01111111",
        6 when "00111111",
        5 when "00011111",
        4 when "00001111",
        3 when "00000111",
        2 when "00000011",
        1 when "00000001",
        0 when others;
    
    
    p_wb_read_data : process(wb_clk_i, rst_n_i)
    begin
        if (rst_n_i = '0') then
            for i in 0 to c_register_shift_size-1 loop
                wb_rd_data_shift_a(i) <= (others => '0');
                wb_rd_ack_shift_s(i) <= '0';
                fifo_wb_rd_data_rd_d <= '0';
            end loop;
        elsif rising_edge(wb_clk_i) then
            fifo_wb_rd_data_rd_d <= fifo_wb_rd_data_rd_s;
            if(fifo_wb_rd_data_rd_s = '1') then
                for i in 0 to c_register_shift_size-1 loop
                    wb_rd_data_shift_a(i) <= fifo_wb_rd_data_dout_s(63+(i*64) downto 0+(i*64));
                    wb_rd_ack_shift_s(i) <= fifo_wb_rd_mask_dout_s(i); -- The data are reversed
                end loop;
            else
                wb_rd_data_shift_a(c_register_shift_size-1) <= (others => '0');
                wb_rd_ack_shift_s(c_register_shift_size-1) <= '0';
                for i in 0 to c_register_shift_size-2 loop
                    wb_rd_data_shift_a(i) <= wb_rd_data_shift_a(i+1);
                    wb_rd_ack_shift_s(i) <= wb_rd_ack_shift_s(i+1);
                end loop;                
            end if;
        end if;
    end process p_wb_read_data;
    
 
    
    
    fifo_wb_rd_mask_din_s <= wb_rd_qmask_shift_s;
    
    
    fifo_wb_rd_data_rd_s <= '1' when wb_rd_ack_shift_s(c_register_shift_size-1 downto 1) = "0000000" and fifo_wb_rd_mask_empty_s = '0' and fifo_wb_rd_data_empty_s = '0' else
                            '0';
    fifo_wb_rd_mask_rd_s <= fifo_wb_rd_data_rd_s;
    
    wb_data_o <= wb_rd_data_shift_a(0);
    wb_rd_ack_s <= wb_rd_ack_shift_s(0);
    

    
    fifo_wb_read_addr : fifo_27x16
      PORT MAP (
        rst => rst_s,
        wr_clk => wb_clk_i,
        rd_clk => ddr_ui_clk_i,
        din => fifo_wb_rd_addr_din_s,
        wr_en => fifo_wb_rd_addr_wr_s,
        rd_en => fifo_wb_rd_addr_rd_s,
        dout => fifo_wb_rd_addr_dout_s,
        full => fifo_wb_rd_addr_full_s,
        empty => fifo_wb_rd_addr_empty_s
      );
    
    fifo_wb_read_mask : fifo_4x16
        PORT MAP (
          rst => rst_s,
          wr_clk => wb_clk_i,
          rd_clk => wb_clk_i,
          din => fifo_wb_rd_mask_din_s,
          wr_en => fifo_wb_rd_mask_wr_s,
          rd_en => fifo_wb_rd_mask_rd_s,
          dout => fifo_wb_rd_mask_dout_s,
          full => fifo_wb_rd_mask_full_s,
          empty => fifo_wb_rd_mask_empty_s,
          rd_data_count => fifo_wb_rd_mask_rd_data_count_s
        );
    
    fifo_wb_read_data : fifo_256x16
        PORT MAP (
          rst => rst_s,
          wr_clk => ddr_ui_clk_i,
          rd_clk => wb_clk_i,
          din => fifo_wb_rd_data_din_s,
          wr_en => fifo_wb_rd_data_wr_s,
          rd_en => fifo_wb_rd_data_rd_s,
          dout => fifo_wb_rd_data_dout_s,
          full => fifo_wb_rd_data_full_s,
          empty => fifo_wb_rd_data_empty_s,
          rd_data_count => fifo_wb_rd_data_rd_data_count_s
        );    
    
    --------------------------------------
    -- DDR CMD
    --------------------------------------
    
    p_ddr_cmd : process (ddr_ui_clk_i, rst_n_i)
    begin
        if (rst_n_i = '0') then
            ddr_cmd_en_o <= '0';
            --ddr_addr_o <= (others => '0');
            ddr_cmd_o <= "000";
        elsif rising_edge(ddr_ui_clk_i) then
            if(fifo_wb_wr_rd_s = '1') then
                ddr_cmd_en_o <= '1';
                ddr_cmd_o <= "000";
                ddr_cmd_s <= '0';
                --ddr_addr_o <= fifo_wb_wr_dout_s(604 downto 576);
            elsif (fifo_wb_rd_addr_rd_s = '1') then
                ddr_cmd_en_o <= '1';
                ddr_cmd_o <= "001";
                ddr_cmd_s <= '1';
                --ddr_addr_o <= fifo_wb_rd_addr_dout_s;
            elsif (ddr_rdy_i = '1') then
                ddr_cmd_en_o <= '0';
            
            end if;
        end if;
    end process p_ddr_cmd;
    
    ddr_addr_o <= fifo_wb_wr_dout_s(604 downto 576) when ddr_cmd_s = '0' else
                  fifo_wb_rd_addr_dout_s;
                  
    --ddr_cmd_o <= "000" when  fifo_wb_wr_rd_d = '1' else
    --                           "001";
                               
    --ddr_cmd_en_o <= fifo_wb_wr_rd_d or fifo_wb_rd_addr_rd_d;
    
    --------------------------------------
    -- DDR Data out
    --------------------------------------
    p_ddr_data_out : process (ddr_ui_clk_i, rst_n_i)
    begin
        if (rst_n_i = '0') then
            ddr_wdf_wren_o <= '0';
            ddr_wdf_end_o  <= '0';        
        elsif rising_edge(ddr_ui_clk_i) then
            if (fifo_wb_wr_rd_s = '1') then
                ddr_wdf_wren_o <= '1';
                ddr_wdf_end_o  <= '1';
            elsif (ddr_wdf_rdy_i = '1') then
                ddr_wdf_wren_o <= '0';
                ddr_wdf_end_o  <= '0';
            end if;
        end if;
        
    end process p_ddr_data_out;
    
    ddr_wdf_data_o <= fifo_wb_wr_dout_s(511 downto 0);
    --ddr_wdf_wren_o <= fifo_wb_wr_rd_d;
    --ddr_wdf_end_o  <= fifo_wb_wr_rd_d;
    ddr_wdf_mask_o <= not fifo_wb_wr_dout_s(575 downto 512);
        
    
    --------------------------------------
    -- DDR Data in
    --------------------------------------

    
    
    fifo_wb_wr_rd_s <= ddr_wdf_rdy_i and ddr_rdy_i and not fifo_wb_wr_empty_s;
    fifo_wb_rd_addr_rd_s <= ddr_rdy_i and not fifo_wb_rd_addr_empty_s;
    fifo_wb_rd_data_wr_s <= ddr_rd_data_valid_i and ddr_rd_data_end_i; -- TODO : check ddr_clk
    
    fifo_wb_rd_data_din_s <= ddr_rd_data_i;
    
--    qword_rd_swap_comp:qword_swap_512
--    port map(
--        qword_swap => "000",
--        din => ddr_rd_data_i,
--        dout => fifo_wb_rd_data_din_s
--    );
    
--    process(ddr_rd_data_i,qword_swap_ds)
--    begin
--    fifo_wb_rd_data_din_s <= f_qword_swap_512(c_BYTE_SWAP, ddr_rd_data_i, qword_swap_ds);
--    end process;

    
    --fifo_wb_rd_data_din_s <= ddr_rd_data_i; -- big endian
    --rd_data_g:for i in 0 to c_register_shift_size-1 generate
        --little endian conversion
        --fifo_wb_rd_data_din_s(i*64+63 downto i*64) <= ddr_rd_data_i(((c_register_shift_size-1)-i)*64+63 downto ((c_register_shift_size-1)-i)*64);
        --fifo_wb_wr_mask_din_s(((c_register_shift_size-1)-i)*8+7 downto ((c_register_shift_size-1)-i)*8)   <= wb_wr_mask_shift_a(i) when wb_wr_qmask_shift_s(i) = '1' and wb_wr_addr_row_s(i) = '1' else (others=>'0');
    --end generate rd_data_g;
    
	
    --------------------------------------
    -- Stall proc
    --------------------------------------
	wb_stall_s <= fifo_wb_wr_full_s or fifo_wb_rd_addr_full_s; --or (not ddr_wdf_rdy_i) or (not ddr_rdy_i);
	wb_stall_o <= wb_stall_s;
    p_wb_stall : process (wb_clk_i, rst_n_i)
    begin
        if (rst_n_i = '0') then
            --wb_stall_s <= '0';
            wb_stall_d <= '0';
			wb_stall_dd <= '0';

        elsif rising_edge(wb_clk_i) then

			
            wb_stall_d <= wb_stall_s;
            wb_stall_dd <= wb_stall_d;
        end if;
    end process p_wb_stall;
end architecture behavioral;

