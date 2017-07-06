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
        ddr_wb_rd_mask_dout_do : out std_logic_vector(7 downto 0);
        ddr_wb_rd_mask_addr_dout_do : out std_logic_vector(g_BYTE_ADDR_WIDTH-1 downto 0);
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
    constant c_write_wait_time : unsigned(9 downto 0) := TO_UNSIGNED(15, 10);
    constant c_read_wait_time : unsigned(9 downto 0) := TO_UNSIGNED(15, 10);
    
    constant c_register_shift_size : integer := 8;
    type data_array is array (0 to c_register_shift_size-1) of std_logic_vector(g_DATA_PORT_SIZE - 1 downto 0);
    type mask_array is array (0 to c_register_shift_size-1) of std_logic_vector(g_MASK_SIZE - 1 downto 0);
    type addr_array is array (0 to c_register_shift_size-1) of std_logic_vector(g_BYTE_ADDR_WIDTH - 1 downto 0);
    type row_array is array (0 to c_register_shift_size-1) of std_logic_vector(c_register_shift_size-1 downto 0);

    --------------------------------------
    -- Signals
    --------------------------------------
    signal rst_s : std_logic;
      
    
    

    
    signal wb_wr_data_shift_a : data_array;
    signal wb_wr_data_shift_next_a : data_array;
    signal wb_wr_data_shift_s : std_logic_vector(511 downto 0);
    signal wb_wr_mask_shift_a : mask_array;
    signal wb_wr_mask_shift_next_a : mask_array;
    signal wb_wr_valid_shift_s : std_logic_vector(c_register_shift_size-1 downto 0);
    signal wb_wr_valid_shift_next_s : std_logic_vector(c_register_shift_size-1 downto 0);
    signal wb_wr_addr_shift_a : addr_array;
    signal wb_wr_addr_shift_next_a : addr_array;

    
    signal wb_wr_shifting_s : std_logic;
    signal wb_wr_match_s : std_logic_vector(c_register_shift_size-1 downto 0);
    signal wb_wr_row_a : row_array;
    signal wb_wr_global_row_s : std_logic_vector(c_register_shift_size-1 downto 0); 
    signal wb_wr_first_row_s : std_logic_vector(c_register_shift_size-1 downto 0);
    signal wb_wr_several_row_s : std_logic;
    signal wb_wr_flush_v_s : std_logic_vector(c_register_shift_size-1 downto 0); 
    signal wb_wr_shift_flush_s : std_logic;
    signal wb_wr_shift_flush_1_s : std_logic;
    
    signal fifo_wb_wr_mask_s : std_logic_vector(63 downto 0);
    signal fifo_wb_wr_addr_s : std_logic_vector(g_BYTE_ADDR_WIDTH-1 downto 0);    
    signal fifo_wb_wr_din_s : std_logic_vector(604 downto 0);
    signal fifo_wb_wr_wr_s : std_logic;
    signal fifo_wb_wr_rd_s : std_logic;
    signal fifo_wb_wr_rd_d : std_logic;
    signal fifo_wb_wr_dout_s : std_logic_vector(604 downto 0);
    signal fifo_wb_wr_full_s : std_logic;
    signal fifo_wb_wr_empty_s : std_logic;
    
    signal wb_rd_valid_shift_s : std_logic_vector(c_register_shift_size-1 downto 0);
    signal wb_rd_valid_shift_next_s : std_logic_vector(c_register_shift_size-1 downto 0);
    signal wb_rd_data_shift_a : data_array;
    signal wb_rd_ack_shift_s : std_logic_vector(c_register_shift_size-1 downto 0);
    signal wb_rd_addr_shift_a : addr_array;
    signal wb_rd_addr_shift_next_a : addr_array;
    signal wb_rd_addr_ref_a : addr_array;
    
    signal wb_rd_shifting_s : std_logic;
    signal wb_rd_match_s : std_logic_vector(c_register_shift_size-1 downto 0);
    signal wb_rd_row_a : row_array;
    signal wb_rd_global_row_s : std_logic_vector(c_register_shift_size-1 downto 0); 
    signal wb_rd_first_row_s : std_logic_vector(c_register_shift_size-1 downto 0);
    signal wb_rd_several_row_s : std_logic;
    signal wb_rd_flush_v_s : std_logic_vector(c_register_shift_size-1 downto 0); 
    signal wb_rd_shift_flush_s : std_logic;
    signal wb_rd_shift_flush_1_s : std_logic;
    
    signal fifo_wb_rd_addr_s : std_logic_vector(g_BYTE_ADDR_WIDTH-1 downto 0);
    signal fifo_wb_rd_addr_din_s : std_logic_vector(g_BYTE_ADDR_WIDTH-1 downto 0);
    signal fifo_wb_rd_addr_wr_s : std_logic;
    signal fifo_wb_rd_addr_rd_s : std_logic;
    signal fifo_wb_rd_addr_dout_s : std_logic_vector(g_BYTE_ADDR_WIDTH-1 downto 0);
    signal fifo_wb_rd_addr_full_s : std_logic;
    signal fifo_wb_rd_addr_almost_full_s : std_logic;
    signal fifo_wb_rd_addr_empty_s : std_logic;
    
    signal fifo_wb_rd_mask_s : std_logic_vector(g_BYTE_ADDR_WIDTH + c_register_shift_size-1 downto 0);
    signal fifo_wb_rd_mask_din_s : std_logic_vector(g_BYTE_ADDR_WIDTH + c_register_shift_size-1 downto 0);
    signal fifo_wb_rd_mask_wr_s : std_logic;
    signal fifo_wb_rd_mask_rd_s : std_logic;
    signal fifo_wb_rd_mask_dout_s : std_logic_vector(c_register_shift_size-1 downto 0);
    signal fifo_wb_rd_mask_full_s : std_logic;
    signal fifo_wb_rd_mask_almost_full_s : std_logic;
    signal fifo_wb_rd_mask_empty_s : std_logic;
    
    signal fifo_wb_rd_mask_rd_data_count_s : STD_LOGIC_VECTOR(4 DOWNTO 0);
    
    signal fifo_wb_rd_data_din_s : std_logic_vector(511 downto 0);
    signal fifo_wb_rd_data_wr_s : std_logic;
    signal fifo_wb_rd_data_rd_s : std_logic;
    signal fifo_wb_rd_data_dout_s : std_logic_vector(511 downto 0);
    signal fifo_wb_rd_data_dout_a : data_array;
    signal fifo_wb_rd_data_full_s : std_logic;
    signal fifo_wb_rd_data_almost_full_s : std_logic;
    signal fifo_wb_rd_data_empty_s : std_logic;
    
    signal fifo_wb_rd_data_rd_data_count_s : STD_LOGIC_VECTOR(4 DOWNTO 0);
    
    signal ddr_cmd_s : std_logic; -- '1' = read, '0' = write
    signal ddr_rd_data_end_s : std_logic;
   

    signal wb_stall_s   : std_logic;
    signal wb_stall_d : std_logic;
    signal wb_stall_dd : std_logic;
    signal wb_wr_ack_s : std_logic;
    signal wb_rd_ack_s : std_logic;
    
    --------------------------------------
    -- debug
    --------------------------------------    

    
    
    --------------------------------------
    -- Counter
    --------------------------------------
    signal wb_write_wait_cnt : unsigned(9 downto 0);    
    signal wb_read_wait_cnt : unsigned(9 downto 0);

    
    
    
begin
    rst_s <= not rst_n_i;
    
    ddr_sr_req_o                <= '0';
    ddr_ref_req_o               <= '0';
    ddr_zq_req_o                <= '0';
    
    --------------------------------------
    -- Read FIFOs debug
    --------------------------------------
    
    ddr_wb_rd_mask_dout_do <= fifo_wb_rd_mask_dout_s;
    ddr_rd_fifo_full_do  <=  fifo_wb_rd_data_full_s & fifo_wb_rd_mask_full_s;
    ddr_rd_fifo_empty_do <=  fifo_wb_rd_data_empty_s & fifo_wb_rd_mask_empty_s;
    ddr_rd_fifo_rd_do    <= fifo_wb_rd_data_rd_s & fifo_wb_rd_mask_rd_s;
    ddr_rd_mask_rd_data_count_do <= fifo_wb_rd_mask_rd_data_count_s(3 downto 0);
    ddr_rd_data_rd_data_count_do <= fifo_wb_rd_data_rd_data_count_s(3 downto 0);

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
            wb_write_wait_cnt <= c_write_wait_time;
            wb_wr_shift_flush_1_s <= '0';
            wb_wr_valid_shift_s <= (others => '0');
            
            for i in 0 to c_register_shift_size-1 loop
                wb_wr_addr_shift_a(i) <= (others => '1');
                wb_wr_data_shift_a(i) <= (others => '0');
                wb_wr_mask_shift_a(i) <= (others => '0');
            end loop;

            wb_wr_ack_s <= '0';
        elsif rising_edge(wb_clk_i) then
            wb_wr_shift_flush_1_s <= wb_wr_shift_flush_s;

            if (wb_cyc_i = '1' and wb_stb_i = '1' and wb_we_i = '1') then
                wb_wr_ack_s <= '1';
                wb_write_wait_cnt <= c_write_wait_time;
            else
    
                 wb_wr_ack_s <= '0';
                 if(wb_wr_valid_shift_s /= (wb_wr_valid_shift_s'range => '0')) then
                     
                     if (wb_write_wait_cnt /= 0) then
                        wb_write_wait_cnt <= wb_write_wait_cnt - 1; 
                         
                         
                     end if;
                 end if;         
            end if;
            
            -- Erase the data sent to the FIFO
            --if(wb_wr_shift_flush_1_s = '1') then
            if(wb_wr_shift_flush_s = '1') then
                wb_write_wait_cnt <= c_write_wait_time;
            end if;
            
            
            
            

            wb_wr_addr_shift_a <= wb_wr_addr_shift_next_a;
            wb_wr_data_shift_a <= wb_wr_data_shift_next_a;
            wb_wr_mask_shift_a <= wb_wr_mask_shift_next_a;
            wb_wr_valid_shift_s <= wb_wr_valid_shift_next_s;
              

  
            
        end if;
        
        
        
        
        
    end process p_wb_write;
    
    
    
    p_wb_write_rtl : process (wb_write_wait_cnt,wb_wr_addr_shift_a,wb_wr_valid_shift_s,wb_wr_shift_flush_s,wb_wr_first_row_s,wb_wr_row_a,wb_wr_match_s,wb_wr_global_row_s)
    begin

        fifo_wb_wr_addr_s <= (others => '0');
        wb_wr_first_row_s <= (others => '0');
        for i in (c_register_shift_size-1) downto 0 loop
            if wb_wr_global_row_s(i) = '1' then
                    fifo_wb_wr_addr_s <= wb_wr_addr_shift_a(i)(g_BYTE_ADDR_WIDTH-1 downto 3) & "000"  ;
                    wb_wr_first_row_s <= wb_wr_row_a(i);
            end if;
              
            
        end loop;
        
        if((wb_wr_global_row_s /= wb_wr_first_row_s) and (wb_wr_global_row_s /= (wb_wr_global_row_s'range => '0'))) then
            wb_wr_several_row_s <= '1';
        else
            wb_wr_several_row_s <= '0';
        end if;
       
        
        
    end process p_wb_write_rtl;
    

    
    p_wb_write_shift: process (wb_wr_shifting_s,wb_wr_addr_shift_a,wb_wr_data_shift_a,wb_wr_mask_shift_a,wb_wr_valid_shift_s,wb_addr_i,wb_data_i,wb_sel_i,wb_wr_flush_v_s)
    
    begin
        if(wb_wr_shifting_s = '1') then
            wb_wr_addr_shift_next_a(c_register_shift_size-1) <= wb_addr_i(g_BYTE_ADDR_WIDTH-1 downto 0);
            wb_wr_data_shift_next_a(c_register_shift_size-1) <= wb_data_i;
            wb_wr_mask_shift_next_a(c_register_shift_size-1) <= wb_sel_i;
            wb_wr_valid_shift_next_s(c_register_shift_size-1) <= wb_cyc_i and wb_stb_i and wb_we_i;
            for i in 1 to c_register_shift_size-1 loop
                wb_wr_addr_shift_next_a(i-1) <= wb_wr_addr_shift_a(i);
                wb_wr_data_shift_next_a(i-1) <= wb_wr_data_shift_a(i);
                wb_wr_mask_shift_next_a(i-1) <= wb_wr_mask_shift_a(i);
                if wb_wr_flush_v_s(i) = '0' then
                    wb_wr_valid_shift_next_s(i-1) <= wb_wr_valid_shift_s(i);
                else
                    wb_wr_valid_shift_next_s(i-1) <= '0';                    
                end if;
            end loop;            
        else
            for i in 0 to c_register_shift_size-1 loop
                wb_wr_addr_shift_next_a(i) <= wb_wr_addr_shift_a(i);
                wb_wr_data_shift_next_a(i) <= wb_wr_data_shift_a(i);
                wb_wr_mask_shift_next_a(i) <= wb_wr_mask_shift_a(i);
                if wb_wr_flush_v_s(i) = '0' then
                    wb_wr_valid_shift_next_s(i) <= wb_wr_valid_shift_s(i);
                else
                    wb_wr_valid_shift_next_s(i) <= '0';               
                end if;
            end loop;   
        end if;
        
    
    end process p_wb_write_shift;
    
    wb_wr_shifting_s <= --'0' when wb_wr_several_row_s = '1' else
                        '1' when wb_cyc_i = '1' and wb_stb_i = '1' and wb_we_i = '1' else
                        '1' when wb_write_wait_cnt = 0 else
                        '0';
    
    wb_wr_global_row_s <= wb_wr_match_s and wb_wr_valid_shift_s;
    wb_wr_flush_v_s <= wb_wr_first_row_s;
    
    wb_wr_shift_flush_s <= '1' when wb_wr_flush_v_s /= (wb_wr_flush_v_s'range => '0') else
                           '0'; 

    
    wr_mask_match_g:for i in 0 to c_register_shift_size-1 generate
        wb_wr_match_s(i) <= '1' when wb_wr_addr_shift_a(i)(2 downto 0) = std_logic_vector(to_unsigned(i,3)) else
                            '0';        
        wr_row_g:for j in 0 to c_register_shift_size-1 generate
            wb_wr_row_a(i)(j) <= '1' when wb_wr_addr_shift_a(i)(g_BYTE_ADDR_WIDTH-1 downto 3) = wb_wr_addr_shift_a(j)(g_BYTE_ADDR_WIDTH-1 downto 3) and wb_wr_match_s(i) = '1' and wb_wr_match_s(j) = '1' and wb_wr_valid_shift_s(i) = '1' and wb_wr_valid_shift_s(j) = '1' else
                                 '0';
        end generate;
        fifo_wb_wr_mask_s((i)*8+7 downto (i)*8)   <= wb_wr_mask_shift_a(i) when wb_wr_flush_v_s(i) = '1' else (others=>'0');
    end generate;
    
    
    -- No Little endian conversion
    wb_wr_data_shift_s <= wb_wr_data_shift_a(7) & 
                          wb_wr_data_shift_a(6) & 
                          wb_wr_data_shift_a(5) & 
                          wb_wr_data_shift_a(4) & 
                          wb_wr_data_shift_a(3) & 
                          wb_wr_data_shift_a(2) & 
                          wb_wr_data_shift_a(1) & 
                          wb_wr_data_shift_a(0);
 
    fifo_wr_data_in : process (wb_clk_i, rst_n_i)
    begin
        if (rst_n_i = '0') then
            fifo_wb_wr_din_s <= (others => '0');
            fifo_wb_wr_wr_s <= '0';
        elsif rising_edge(wb_clk_i) then
        
            fifo_wb_wr_din_s <= fifo_wb_wr_addr_s & 
                                fifo_wb_wr_mask_s &
                                wb_wr_data_shift_s;        
            fifo_wb_wr_wr_s <= wb_wr_shift_flush_s;
        end if;
        
     end process;

    

    --fifo_wb_wr_din_s <= fifo_wb_wr_addr_s & 
    --                    fifo_wb_wr_mask_s &
    --                    wb_wr_data_shift_s;
                        
    --fifo_wb_wr_wr_s <= wb_wr_shift_flush_s;

  
    
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
        wb_rd_shift_flush_1_s <= wb_rd_shift_flush_s;
        wb_read_wait_cnt <= c_read_wait_time;
        
        wb_rd_valid_shift_s <= (others => '0');
        
        for i in 0 to c_register_shift_size-1 loop
            wb_rd_addr_shift_a(i) <= (others => '0');
        end loop;
        
    elsif rising_edge(wb_clk_i) then
        wb_rd_shift_flush_1_s <= wb_rd_shift_flush_s;
        -- Register Shift
        if (wb_cyc_i = '1' and wb_stb_i = '1' and wb_we_i = '0') then
            wb_read_wait_cnt <= c_read_wait_time;
        else
          if(wb_rd_valid_shift_s /= (wb_rd_valid_shift_s'range => '0')) then
                             
                if (wb_read_wait_cnt /= 0) then
                    wb_read_wait_cnt <= wb_read_wait_cnt - 1;
                    
                end if;
            end if;
        end if;
        
        
        -- Erase the data sent to the FIFO
        --if(wb_rd_shift_flush_1_s = '1') then
        if(wb_rd_shift_flush_s = '1') then
            wb_read_wait_cnt <= c_read_wait_time;
        end if;
        
        
        wb_rd_addr_shift_a <= wb_rd_addr_shift_next_a;
        wb_rd_valid_shift_s <= wb_rd_valid_shift_next_s;
        
    end if;
    end process p_wb_read;
    

    
    p_wb_read_rtl : process (wb_read_wait_cnt,wb_rd_addr_shift_a,wb_rd_addr_ref_a,wb_rd_valid_shift_s,wb_rd_shift_flush_s,wb_rd_global_row_s,wb_rd_addr_shift_a,wb_rd_row_a,wb_rd_first_row_s)
    begin
        
        fifo_wb_rd_addr_s <= (others => 'X');
        wb_rd_first_row_s <= (others => '0');
        for i in (c_register_shift_size-1) downto 0 loop
            if wb_rd_global_row_s(i) = '1' then
                    fifo_wb_rd_addr_s <= wb_rd_addr_shift_a(i)(g_BYTE_ADDR_WIDTH-1 downto 3) & "000"  ;
                    wb_rd_first_row_s <= wb_rd_row_a(i);
            end if;
              
            
        end loop;
        
        if((wb_rd_global_row_s /= wb_rd_first_row_s) and (wb_rd_global_row_s /= (wb_rd_global_row_s'range => '0'))) then
            wb_rd_several_row_s <= '1';
        else
            wb_rd_several_row_s <= '0';
        end if;

        
    end process p_wb_read_rtl;
    
     wb_rd_shifting_s <= --'0' when wb_rd_several_row_s = '1' else
                        '1' when wb_cyc_i = '1' and wb_stb_i = '1' and wb_we_i = '0' else
                        '1' when wb_read_wait_cnt = 0 else
                        '0';
    
    wb_rd_global_row_s <= wb_rd_match_s and wb_rd_valid_shift_s;
    wb_rd_flush_v_s <= wb_rd_first_row_s;
    


    
    rd_match_g:for i in 0 to c_register_shift_size-1 generate
        wb_rd_match_s(i) <= '1' when wb_rd_addr_shift_a(i)(2 downto 0) = std_logic_vector(to_unsigned(i,3)) else
                            '0';        
        rd_row_g:for j in 0 to c_register_shift_size-1 generate
            wb_rd_row_a(i)(j) <= '1' when wb_rd_addr_shift_a(i)(g_BYTE_ADDR_WIDTH-1 downto 3) = wb_rd_addr_shift_a(j)(g_BYTE_ADDR_WIDTH-1 downto 3) and wb_rd_match_s(i) = '1' and wb_rd_match_s(j) = '1' and wb_rd_valid_shift_s(i) = '1' and wb_rd_valid_shift_s(j) = '1' else
                                 '0';
        end generate;

    end generate;   
    
    p_wb_read_shift: process (wb_rd_shifting_s,wb_rd_addr_shift_a,wb_rd_valid_shift_s,wb_addr_i,wb_data_i,wb_sel_i,wb_rd_flush_v_s)
    
    begin
        if(wb_rd_shifting_s = '1') then
            wb_rd_addr_shift_next_a(c_register_shift_size-1) <= wb_addr_i(g_BYTE_ADDR_WIDTH-1 downto 0);
            wb_rd_valid_shift_next_s(c_register_shift_size-1) <= wb_cyc_i and wb_stb_i and not wb_we_i;
            for i in 1 to c_register_shift_size-1 loop
                wb_rd_addr_shift_next_a(i-1) <= wb_rd_addr_shift_a(i);
                if wb_rd_flush_v_s(i) = '0' then
                    wb_rd_valid_shift_next_s(i-1) <= wb_rd_valid_shift_s(i);
                else
                    wb_rd_valid_shift_next_s(i-1) <= '0';                    
                end if;
            end loop;            
        else
            for i in 0 to c_register_shift_size-1 loop
                wb_rd_addr_shift_next_a(i) <= wb_rd_addr_shift_a(i);
                if wb_rd_flush_v_s(i) = '0' then
                    wb_rd_valid_shift_next_s(i) <= wb_rd_valid_shift_s(i);
                else
                    wb_rd_valid_shift_next_s(i) <= '0';               
                end if;
            end loop;   
        end if;
        
    
    end process p_wb_read_shift;
    
    p_wb_read_data : process(wb_clk_i, rst_n_i)
    begin
        if (rst_n_i = '0') then
            for i in 0 to c_register_shift_size-1 loop
                wb_rd_data_shift_a(i) <= (others => '0');
                wb_rd_ack_shift_s(i) <= '0';
            end loop;
        elsif rising_edge(wb_clk_i) then
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
    
    
    fifo_rd_data_in : process (wb_clk_i, rst_n_i)
    begin
        if (rst_n_i = '0') then
            fifo_wb_rd_addr_din_s <= (others => '0');
            fifo_wb_rd_mask_din_s <= (others => '0');
            fifo_wb_rd_addr_wr_s <= '0';
            fifo_wb_rd_mask_wr_s <= '0';
        elsif rising_edge(wb_clk_i) then
            
            fifo_wb_rd_addr_wr_s <= wb_rd_shift_flush_s;
            fifo_wb_rd_mask_wr_s <= wb_rd_shift_flush_s;         
            fifo_wb_rd_addr_din_s <= fifo_wb_rd_addr_s;
            fifo_wb_rd_mask_din_s <= fifo_wb_rd_addr_s & wb_rd_valid_shift_s;
        end if;
        
     end process;
 
    --fifo_wb_rd_addr_wr_s <= wb_rd_shift_flush_s;
    --fifo_wb_rd_mask_wr_s <= wb_rd_shift_flush_s;   
    
    
    --fifo_wb_rd_mask_din_s <= fifo_wb_rd_addr_s & wb_rd_valid_shift_s;
    
    
    fifo_wb_rd_data_rd_s <= '1' when wb_rd_ack_shift_s(c_register_shift_size-1 downto 1) = "0000000" and fifo_wb_rd_mask_empty_s = '0' and fifo_wb_rd_data_empty_s = '0' else
                            '0';
    fifo_wb_rd_mask_rd_s <= fifo_wb_rd_data_rd_s;
    
    wb_data_o <= wb_rd_data_shift_a(0);
    wb_rd_ack_s <= wb_rd_ack_shift_s(0);
    
    wb_rd_shift_flush_s <= '1' when wb_rd_flush_v_s /= (wb_rd_flush_v_s'range => '0') else
                           '0';
    
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
        almost_full => fifo_wb_rd_addr_almost_full_s,
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
          dout(7 downto 0) => fifo_wb_rd_mask_dout_s,
          dout(36 downto 8) => ddr_wb_rd_mask_addr_dout_do,
          full => fifo_wb_rd_mask_full_s,
          almost_full => fifo_wb_rd_mask_almost_full_s,
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
          almost_full => fifo_wb_rd_data_almost_full_s,
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
            ddr_cmd_o <= "000";
        elsif rising_edge(ddr_ui_clk_i) then
            if(fifo_wb_wr_rd_s = '1') then
                ddr_cmd_en_o <= '1';
                ddr_cmd_o <= "000";
                ddr_cmd_s <= '0';
            elsif (fifo_wb_rd_addr_rd_s = '1') then
                ddr_cmd_en_o <= '1';
                ddr_cmd_o <= "001";
                ddr_cmd_s <= '1';
            elsif (ddr_rdy_i = '1') then
                ddr_cmd_en_o <= '0';
            
            end if;
        end if;
    end process p_ddr_cmd;
    
    ddr_addr_o <= fifo_wb_wr_dout_s(604 downto 576) when ddr_cmd_s = '0' else
                  fifo_wb_rd_addr_dout_s;
    
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
    ddr_wdf_mask_o <= not fifo_wb_wr_dout_s(575 downto 512);
        
    
    --------------------------------------
    -- DDR Data in
    --------------------------------------

    
    
    fifo_wb_wr_rd_s <= ddr_wdf_rdy_i and ddr_rdy_i and not fifo_wb_wr_empty_s;
    fifo_wb_rd_addr_rd_s <= ddr_rdy_i and (not fifo_wb_rd_addr_empty_s) and (not fifo_wb_rd_data_almost_full_s); -- and (not fifo_wb_rd_mask_full_s);
    fifo_wb_rd_data_wr_s <= ddr_rd_data_valid_i and ddr_rd_data_end_i;
    
    fifo_wb_rd_data_din_s <= ddr_rd_data_i;
    
   
	
    --------------------------------------
    -- Stall proc
    --------------------------------------
	wb_stall_s <= fifo_wb_wr_full_s or fifo_wb_rd_addr_almost_full_s or fifo_wb_rd_mask_almost_full_s or wb_wr_several_row_s; --or (not ddr_wdf_rdy_i) or (not ddr_rdy_i);
	wb_stall_o <= wb_stall_s;

end architecture behavioral;

