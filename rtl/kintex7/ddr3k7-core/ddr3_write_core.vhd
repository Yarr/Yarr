----------------------------------------------------------------------------------
-- Company: LBNL
-- Engineer: Arnaud Sautaux
-- 
-- Create Date: 07/27/2017 10:50:41 AM
-- Design Name: ddr3k7-core
-- Module Name: ddr3_write_core - Behavioral
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

entity ddr3_write_core is
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
           ddr_wdf_data_o              : out    std_logic_vector(511 downto 0);
           ddr_wdf_end_o               : out    std_logic;
           ddr_wdf_mask_o              : out    std_logic_vector(63 downto 0);
           ddr_wdf_wren_o              : out    std_logic;
           ddr_rdy_i                   : in   std_logic;
           ddr_wdf_rdy_i               : in   std_logic;
           ddr_ui_clk_i                  : in   std_logic;
           
           ddr_req_o                    : out std_logic;
           ddr_gnt_i                    : in std_logic
           );
end ddr3_write_core;

architecture Behavioral of ddr3_write_core is
    --------------------------------------
    -- Components
    --------------------------------------

    COMPONENT fifo_605x32
      PORT (
        rst : IN STD_LOGIC;
        wr_clk : IN STD_LOGIC;
        rd_clk : IN STD_LOGIC;
        din : IN STD_LOGIC_VECTOR(604 DOWNTO 0);
        wr_en : IN STD_LOGIC;
        rd_en : IN STD_LOGIC;
        dout : OUT STD_LOGIC_VECTOR(604 DOWNTO 0);
        full : OUT STD_LOGIC;
        empty : OUT STD_LOGIC
      );
    END COMPONENT;

    --------------------------------------
    -- Constants
    --------------------------------------
    constant c_write_wait_time : unsigned(7 downto 0) := TO_UNSIGNED(15, 8);
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
    signal wb_wr_aligned : std_logic_vector(c_register_shift_size-1 downto 0);
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
    
    --------------------------------------
    -- Counter
    --------------------------------------
    signal wb_write_wait_cnt : unsigned(7 downto 0);  

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
    wb_dat_o <= (others => '0');
    detection_gen : if (g_NOT_CONSECUTIVE_DETECTION = true) generate
        wb_stall_s <= fifo_wb_wr_full_s or  wb_wr_several_row_s;
    end generate;
    no_dectection_gen : if (g_NOT_CONSECUTIVE_DETECTION = false) generate
        wb_stall_s <= fifo_wb_wr_full_s;
    end generate;
        
    wb_stall_o <= wb_stall_s;

    --------------------------------------
    -- Wishbone write process
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

            wb_ack_s <= '0';
        elsif rising_edge(wb_clk_i) then
            wb_wr_shift_flush_1_s <= wb_wr_shift_flush_s;

            if (wb_cyc_s = '1' and wb_stb_s = '1' and wb_we_s = '1') then
                wb_ack_s <= '1';
                wb_write_wait_cnt <= c_write_wait_time;
            else
    
                 wb_ack_s <= '0';
                 if(wb_wr_valid_shift_s /= (wb_wr_valid_shift_s'range => '0')) then
                     
                     if (wb_write_wait_cnt /= 0) then
                        wb_write_wait_cnt <= wb_write_wait_cnt - 1; 
                         
                         
                     end if;
                 end if;         
            end if;
            
            if(wb_wr_shift_flush_s = '1') then
                wb_write_wait_cnt <= c_write_wait_time;
            end if;
            
            wb_wr_addr_shift_a <= wb_wr_addr_shift_next_a;
            wb_wr_data_shift_a <= wb_wr_data_shift_next_a;
            wb_wr_mask_shift_a <= wb_wr_mask_shift_next_a;
            wb_wr_valid_shift_s <= wb_wr_valid_shift_next_s;    
        end if;
        
        
    end process p_wb_write;

    p_wb_write_rtl : process (wb_write_wait_cnt,wb_wr_addr_shift_a,wb_wr_valid_shift_s,wb_wr_shift_flush_s,wb_wr_first_row_s,wb_wr_row_a,wb_wr_aligned,wb_wr_global_row_s)
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

    
    p_wb_write_shift: process (wb_wr_shifting_s,wb_wr_addr_shift_a,wb_wr_data_shift_a,wb_wr_mask_shift_a,wb_wr_valid_shift_s,wb_adr_s,wb_dat_s,wb_sel_s,wb_wr_flush_v_s)
    
    begin
        if(wb_wr_shifting_s = '1') then
            wb_wr_addr_shift_next_a(c_register_shift_size-1) <= wb_adr_s(g_BYTE_ADDR_WIDTH-1 downto 0);
            wb_wr_data_shift_next_a(c_register_shift_size-1) <= wb_dat_s;
            wb_wr_mask_shift_next_a(c_register_shift_size-1) <= wb_sel_s;
            wb_wr_valid_shift_next_s(c_register_shift_size-1) <= wb_cyc_s and wb_stb_s and wb_we_s;
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
                        '1' when wb_cyc_s = '1' and wb_stb_s = '1' and wb_we_s = '1' else --and wb_stall_s = '0' else
                        '1' when wb_write_wait_cnt = 0 else
                        '0';
    
    wb_wr_global_row_s <= wb_wr_aligned and wb_wr_valid_shift_s;
    wb_wr_flush_v_s <= wb_wr_first_row_s;
    
    wb_wr_shift_flush_s <= '1' when wb_wr_flush_v_s /= (wb_wr_flush_v_s'range => '0') else
                           '0'; 

    
    wr_mask_match_g:for i in 0 to c_register_shift_size-1 generate
        wb_wr_aligned(i) <= '1' when wb_wr_addr_shift_a(i)(2 downto 0) = std_logic_vector(to_unsigned(i,3)) else
                            '0';        
        wr_row_g:for j in 0 to c_register_shift_size-1 generate
            wb_wr_row_a(i)(j) <= '1' when wb_wr_addr_shift_a(i)(g_BYTE_ADDR_WIDTH-1 downto 3) = wb_wr_addr_shift_a(j)(g_BYTE_ADDR_WIDTH-1 downto 3) and wb_wr_aligned(i) = '1' and wb_wr_aligned(j) = '1' and wb_wr_valid_shift_s(i) = '1' and wb_wr_valid_shift_s(j) = '1' else
                                 '0';
        end generate;
        fifo_wb_wr_mask_s((i)*8+7 downto (i)*8)   <= wb_wr_mask_shift_a(i) when wb_wr_flush_v_s(i) = '1' else (others=>'0');
    end generate;
    
    
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

  
    
    fifo_wb_write : fifo_605x32
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
    -- DDR CMD
    --------------------------------------
    ddr_cmd_en_o <= fifo_wb_wr_rd_s;
    ddr_addr_o <= fifo_wb_wr_dout_s(604 downto 576);
    ddr_cmd_o <= "000";
    
    --------------------------------------
    -- DDR Data out
    --------------------------------------
    ddr_wdf_wren_o <= fifo_wb_wr_rd_s;
    ddr_wdf_end_o <= fifo_wb_wr_rd_s;
    ddr_wdf_data_o <= fifo_wb_wr_dout_s(511 downto 0);
    ddr_wdf_mask_o <= not fifo_wb_wr_dout_s(575 downto 512);
        
    ddr_req_o <= not fifo_wb_wr_empty_s;
    --------------------------------------
    -- DDR Data in
    --------------------------------------    
    fifo_wb_wr_rd_s <= ddr_wdf_rdy_i and ddr_rdy_i and ddr_gnt_i and (not fifo_wb_wr_empty_s);

end Behavioral;
