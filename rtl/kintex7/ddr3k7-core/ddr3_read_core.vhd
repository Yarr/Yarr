----------------------------------------------------------------------------------
-- Company: LBNL
-- Engineer: Arnaud Sautaux
-- 
-- Create Date: 07/27/2017 10:50:41 AM
-- Design Name: ddr3k7-core
-- Module Name: ddr3_read_core - Behavioral
-- Project Name: YARR
-- Target Devices: xc7k160t
-- Tool Versions: Vivado v2016.2 (64 bit)
-- Description: 
-- 
-- Dependencies: 
-- 
-- Revision:
-- Revision 0.01 - File Created
-- Additional Comments:
-- 
----------------------------------------------------------------------------------


library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;

entity ddr3_read_core is
    generic (
       g_BYTE_ADDR_WIDTH : integer := 29;
       g_MASK_SIZE       : integer := 8;
       g_DATA_PORT_SIZE  : integer := 64;
       g_NOT_CONSECUTIVE_DETECTION : boolean := false
    );
    Port ( 
    
           ----------------------------------------------------------------------------
           -- Reset input (active low)
           ----------------------------------------------------------------------------
           rst_n_i : in std_logic;
           wb_clk_i : in STD_LOGIC;
           wb_sel_i : in STD_LOGIC_VECTOR (g_MASK_SIZE - 1 downto 0);
           wb_stb_i : in STD_LOGIC;
           wb_cyc_i : in STD_LOGIC;
           wb_we_i : in STD_LOGIC;
           wb_adr_i : in STD_LOGIC_VECTOR (32 - 1 downto 0);
           wb_dat_i : in STD_LOGIC_VECTOR (g_DATA_PORT_SIZE - 1 downto 0);
           wb_dat_o : out STD_LOGIC_VECTOR (g_DATA_PORT_SIZE - 1 downto 0);
           wb_ack_o : out STD_LOGIC;
           wb_stall_o : out STD_LOGIC;
           
           ddr_addr_o                  : out    std_logic_vector(g_BYTE_ADDR_WIDTH-1 downto 0);
           ddr_cmd_o                   : out    std_logic_vector(2 downto 0);
           ddr_cmd_en_o                : out    std_logic;
           ddr_rd_data_i               : in   std_logic_vector(511 downto 0);
           ddr_rd_data_end_i           : in   std_logic;
           ddr_rd_data_valid_i         : in   std_logic;
           ddr_rdy_i                   : in   std_logic;
           ddr_ui_clk_i                  : in   std_logic;
           
           ddr_req_o                    : out std_logic;
           ddr_gnt_i                    : in std_logic
           );
end ddr3_read_core;

architecture Behavioral of ddr3_read_core is
    --------------------------------------
    -- Components
    --------------------------------------

    COMPONENT fifo_29x32
      PORT (
        rst : IN STD_LOGIC;
        wr_clk : IN STD_LOGIC;
        rd_clk : IN STD_LOGIC;
        din : IN STD_LOGIC_VECTOR(28 DOWNTO 0);
        wr_en : IN STD_LOGIC;
        rd_en : IN STD_LOGIC;
        dout : OUT STD_LOGIC_VECTOR(28 DOWNTO 0);
        full : OUT STD_LOGIC;
        almost_full : OUT STD_LOGIC;
        empty : OUT STD_LOGIC
      );
    END COMPONENT;
    
    COMPONENT fifo_8x32
      PORT (
        rst : IN STD_LOGIC;
        wr_clk : IN STD_LOGIC;
        rd_clk : IN STD_LOGIC;
        din : IN STD_LOGIC_VECTOR(7 DOWNTO 0);
        wr_en : IN STD_LOGIC;
        rd_en : IN STD_LOGIC;
        dout : OUT STD_LOGIC_VECTOR(7 DOWNTO 0);
        full : OUT STD_LOGIC;
        almost_full : OUT STD_LOGIC;
        empty : OUT STD_LOGIC;
        rd_data_count : OUT STD_LOGIC_VECTOR(4 DOWNTO 0)
      );
    END COMPONENT;
    
    COMPONENT fifo_256x16
      PORT (
        rst : IN STD_LOGIC;
        wr_clk : IN STD_LOGIC;
        rd_clk : IN STD_LOGIC;
        din : IN STD_LOGIC_VECTOR(511 DOWNTO 0);
        wr_en : IN STD_LOGIC;
        rd_en : IN STD_LOGIC;
        dout : OUT STD_LOGIC_VECTOR(511 DOWNTO 0);
        full : OUT STD_LOGIC;
        almost_full : OUT STD_LOGIC;
        empty : OUT STD_LOGIC;
        rd_data_count : OUT STD_LOGIC_VECTOR(4 DOWNTO 0)
      );
    END COMPONENT;

    --------------------------------------
    -- Constants
    --------------------------------------
    constant c_read_wait_time : unsigned(7 downto 0) := TO_UNSIGNED(15, 8);
    constant c_register_shift_size : integer := 8;

    --------------------------------------
    -- Types
    --------------------------------------   
    type data_array is array (0 to c_register_shift_size-1) of std_logic_vector(g_DATA_PORT_SIZE - 1 downto 0);
    type mask_array is array (0 to c_register_shift_size-1) of std_logic_vector(g_MASK_SIZE - 1 downto 0);
    type addr_array is array (0 to c_register_shift_size-1) of std_logic_vector(g_BYTE_ADDR_WIDTH - 1 downto 0);
    type row_array is array (0 to c_register_shift_size-1) of std_logic_vector(c_register_shift_size-1 downto 0);
    
    --------------------------------------
    -- Signals
    --------------------------------------
    signal rst_s : std_logic;
    
    signal wb_sel_s   : std_logic_vector(g_MASK_SIZE - 1 downto 0);
    signal wb_cyc_s   : std_logic;
    signal wb_stb_s   : std_logic;
    signal wb_we_s    : std_logic;
    signal wb_adr_s  : std_logic_vector(32 - 1 downto 0);
    signal wb_dat_s  : std_logic_vector(g_DATA_PORT_SIZE - 1 downto 0);
    signal wb_ack_s : std_logic;
    signal wb_stall_s : std_logic;

    signal wb_rd_valid_shift_s : std_logic_vector(c_register_shift_size-1 downto 0);
    signal wb_rd_valid_shift_next_s : std_logic_vector(c_register_shift_size-1 downto 0);
    signal wb_rd_data_shift_a : data_array;
    signal wb_ack_shift_s : std_logic_vector(c_register_shift_size-1 downto 0);
    signal wb_rd_addr_shift_a : addr_array;
    signal wb_rd_addr_shift_next_a : addr_array;
    signal wb_rd_addr_ref_a : addr_array;
    
    signal wb_rd_shifting_s : std_logic;
    signal wb_rd_aligned_s : std_logic_vector(c_register_shift_size-1 downto 0);
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
    
    --------------------------------------
    -- Counter
    --------------------------------------
    signal wb_read_wait_cnt : unsigned(7 downto 0);  

begin
    rst_s <= not rst_n_i;
    --------------------------------------
    -- Wishbone input delay
    --------------------------------------
    
    p_wb_in : process (wb_clk_i, rst_n_i)
    begin    
        if (rst_n_i = '0') then
            wb_sel_s   <= (others =>'0');
            wb_cyc_s   <= '0';
            wb_stb_s   <= '0';
            wb_we_s    <= '0';
            wb_adr_s  <= (others =>'0');
            wb_dat_s  <= (others =>'0');  
        elsif rising_edge(wb_clk_i) then
             wb_sel_s   <= wb_sel_i;
             wb_cyc_s   <= wb_cyc_i;
             wb_stb_s   <= wb_stb_i;
             wb_we_s    <= wb_we_i;
             wb_adr_s  <= wb_adr_i;
             wb_dat_s  <= wb_dat_i;       
        end if;
    end process p_wb_in;

    --------------------------------------
    -- Wishbone ouput
    --------------------------------------    
    
    wb_ack_o <= wb_ack_s;
    detection_gen : if (g_NOT_CONSECUTIVE_DETECTION = true) generate
        wb_stall_s <= fifo_wb_rd_addr_almost_full_s or fifo_wb_rd_mask_almost_full_s or wb_rd_several_row_s;
    end generate;
    no_dectection_gen : if (g_NOT_CONSECUTIVE_DETECTION = false) generate
        wb_stall_s <= fifo_wb_rd_addr_almost_full_s or fifo_wb_rd_mask_almost_full_s;
    end generate;
    wb_stall_o <= wb_stall_s;

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
        if (wb_cyc_s = '1' and wb_stb_s = '1' and wb_we_s = '0') then
            wb_read_wait_cnt <= c_read_wait_time;
        else
          if(wb_rd_valid_shift_s /= (wb_rd_valid_shift_s'range => '0')) then
                             
                if (wb_read_wait_cnt /= 0) then
                    wb_read_wait_cnt <= wb_read_wait_cnt - 1;
                    
                end if;
            end if;
        end if;
        
        
        if(wb_rd_shift_flush_s = '1') then
            wb_read_wait_cnt <= c_read_wait_time;
        end if;
        
        
        wb_rd_addr_shift_a <= wb_rd_addr_shift_next_a;
        wb_rd_valid_shift_s <= wb_rd_valid_shift_next_s;
        
    end if;
    end process p_wb_read;
    

    
    p_wb_read_rtl : process (wb_read_wait_cnt,wb_rd_addr_shift_a,wb_rd_addr_ref_a,wb_rd_valid_shift_s,wb_rd_shift_flush_s,wb_rd_global_row_s,wb_rd_addr_shift_a,wb_rd_row_a,wb_rd_first_row_s)
    begin
        
        fifo_wb_rd_addr_s <= (others => '0');
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
                        '1' when wb_cyc_s = '1' and wb_stb_s = '1' and wb_we_s = '0' else--and wb_stall_s = '0' else
                        '1' when wb_read_wait_cnt = 0 else
                        '0';
    
    wb_rd_global_row_s <= wb_rd_aligned_s and wb_rd_valid_shift_s;
    wb_rd_flush_v_s <= wb_rd_first_row_s;
    


    
    rd_match_g:for i in 0 to c_register_shift_size-1 generate
        wb_rd_aligned_s(i) <= '1' when wb_rd_addr_shift_a(i)(2 downto 0) = std_logic_vector(to_unsigned(i,3)) else
                            '0';        
        rd_row_g:for j in 0 to c_register_shift_size-1 generate
            wb_rd_row_a(i)(j) <= '1' when wb_rd_addr_shift_a(i)(g_BYTE_ADDR_WIDTH-1 downto 3) = wb_rd_addr_shift_a(j)(g_BYTE_ADDR_WIDTH-1 downto 3) and wb_rd_aligned_s(i) = '1' and wb_rd_aligned_s(j) = '1' and wb_rd_valid_shift_s(i) = '1' and wb_rd_valid_shift_s(j) = '1' else
                                 '0';
        end generate;

    end generate;   
    
    p_wb_read_shift: process (wb_rd_shifting_s,wb_rd_addr_shift_a,wb_rd_valid_shift_s,wb_adr_s,wb_dat_s,wb_sel_s,wb_rd_flush_v_s)
    
    begin
        if(wb_rd_shifting_s = '1') then
            wb_rd_addr_shift_next_a(c_register_shift_size-1) <= wb_adr_s(g_BYTE_ADDR_WIDTH-1 downto 0);
            wb_rd_valid_shift_next_s(c_register_shift_size-1) <= wb_cyc_s and wb_stb_s and not wb_we_s;
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
                wb_ack_shift_s(i) <= '0';
            end loop;
        elsif rising_edge(wb_clk_i) then
            if(fifo_wb_rd_data_rd_s = '1') then
                for i in 0 to c_register_shift_size-1 loop
                    wb_rd_data_shift_a(i) <= fifo_wb_rd_data_dout_s(63+(i*64) downto 0+(i*64));
                    wb_ack_shift_s(i) <= fifo_wb_rd_mask_dout_s(i); -- The data are reversed
                end loop;
            else
                wb_rd_data_shift_a(c_register_shift_size-1) <= (others => '0');
                wb_ack_shift_s(c_register_shift_size-1) <= '0';
                for i in 0 to c_register_shift_size-2 loop
                    wb_rd_data_shift_a(i) <= wb_rd_data_shift_a(i+1);
                    wb_ack_shift_s(i) <= wb_ack_shift_s(i+1);
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
 
    
    
    fifo_wb_rd_data_rd_s <= '1' when wb_ack_shift_s(c_register_shift_size-1 downto 1) = "0000000" and fifo_wb_rd_mask_empty_s = '0' and fifo_wb_rd_data_empty_s = '0' else
                            '0';
    fifo_wb_rd_mask_rd_s <= fifo_wb_rd_data_rd_s;
    
    wb_dat_o <= wb_rd_data_shift_a(0);
    wb_ack_s <= wb_ack_shift_s(0);
    
    wb_rd_shift_flush_s <= '1' when wb_rd_flush_v_s /= (wb_rd_flush_v_s'range => '0') else
                           '0';
    
    fifo_wb_read_addr : fifo_29x32
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
    
    fifo_wb_read_mask : fifo_8x32
        PORT MAP (
          rst => rst_s,
          wr_clk => wb_clk_i,
          rd_clk => wb_clk_i,
          din => fifo_wb_rd_mask_din_s(7 downto 0),
          wr_en => fifo_wb_rd_mask_wr_s,
          rd_en => fifo_wb_rd_mask_rd_s,
          dout => fifo_wb_rd_mask_dout_s,
          --dout(7 downto 0) => fifo_wb_rd_mask_dout_s,
          --dout(36 downto 8) => open,--ddr_wb_rd_mask_addr_dout_do,
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
    ddr_addr_o <= fifo_wb_rd_addr_dout_s;
     
    ddr_cmd_o   <= "001";
    ddr_cmd_en_o<= fifo_wb_rd_addr_rd_s;
    
    ddr_req_o <= not fifo_wb_rd_addr_empty_s;
    
    --------------------------------------
    -- DDR Data in
    --------------------------------------
    fifo_wb_rd_addr_rd_s <= ddr_rdy_i and (not fifo_wb_rd_addr_empty_s) and (not fifo_wb_rd_data_almost_full_s) and ddr_gnt_i; -- and (not fifo_wb_rd_mask_full_s);
    fifo_wb_rd_data_wr_s <= ddr_rd_data_valid_i and ddr_rd_data_end_i;
    fifo_wb_rd_data_din_s <= ddr_rd_data_i;
       


end Behavioral;
