----------------------------------------------------------------------------------
-- Company: LBNL
-- Engineer: Arnaud Sautaux
-- 
-- Create Date: 07/27/2017 10:50:41 AM
-- Design Name: DDR3 Wishbone control core
-- Module Name: ddr3_ctrl_wb - Behavioral
-- Project Name: YARR
-- Target Devices: 
-- Tool Versions: Vivado v2016.2 (64 bit)
-- Description: 
-- Wishbone to Xilinx MiG interface
-- Dependencies:
-- ddr3_read_core
-- ddr3_write_core
-- Revision:
-- Revision 0.01 - File Created
-- Additional Comments:
-- 
----------------------------------------------------------------------------------


library IEEE;
use IEEE.STD_LOGIC_1164.all;
use IEEE.NUMERIC_STD.all;

entity ddr3_ctrl_wb is
    generic (
           g_BYTE_ADDR_WIDTH : integer := 29;
           g_MASK_SIZE       : integer := 8;
           g_DATA_PORT_SIZE  : integer := 64;
           g_NOT_CONSECUTIVE_DETECTION : boolean := false
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
        -- Wishbone bus port
        ----------------------------------------------------------------------------
        wb1_sel_i   : in  std_logic_vector(g_MASK_SIZE - 1 downto 0);
        wb1_cyc_i   : in  std_logic;
        wb1_stb_i   : in  std_logic;
        wb1_we_i    : in  std_logic;
        wb1_addr_i  : in  std_logic_vector(32 - 1 downto 0);
        wb1_data_i  : in  std_logic_vector(g_DATA_PORT_SIZE - 1 downto 0);
        wb1_data_o  : out std_logic_vector(g_DATA_PORT_SIZE - 1 downto 0);
        wb1_ack_o   : out std_logic;
        wb1_stall_o : out std_logic;        
        
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
    
    
    component ddr3_write_core is
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
    end component;    

    component ddr3_read_core is
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
    end component;
    
    component rr_arbiter is
        generic (
            g_CHANNELS : integer := 16
        );
        port (
            -- sys connect
            clk_i : in std_logic;
            rst_i : in std_logic;
            
            -- requests
            req_i : in std_logic_vector(g_CHANNELS-1 downto 0);
            -- grant
            gnt_o : out std_logic_vector(g_CHANNELS-1 downto 0)
        );
            
    end component;
    
    --------------------------------------
    -- Constants
    --------------------------------------   
    constant c_register_shift_size : integer := 8;
    constant c_wb_wr0_nb : integer := 0;
    constant c_wb_wr1_nb : integer := 1;
    constant c_wb_rd0_nb : integer := 2;
    type data_array is array (0 to c_register_shift_size-1) of std_logic_vector(g_DATA_PORT_SIZE - 1 downto 0);
    type mask_array is array (0 to c_register_shift_size-1) of std_logic_vector(g_MASK_SIZE - 1 downto 0);
    type addr_array is array (0 to c_register_shift_size-1) of std_logic_vector(g_BYTE_ADDR_WIDTH - 1 downto 0);
    type row_array is array (0 to c_register_shift_size-1) of std_logic_vector(c_register_shift_size-1 downto 0);

    --------------------------------------
    -- Signals
    --------------------------------------
    signal rst_s : std_logic;
    signal rr_rst_s : std_logic;

    signal wb_sel_s   : std_logic_vector(g_MASK_SIZE - 1 downto 0);
    signal wb_cyc_s   : std_logic;
    signal wb_stb_s   : std_logic;
    signal wb_we_s    : std_logic;
    signal wb_addr_s  : std_logic_vector(32 - 1 downto 0);
    signal wb_data_s  : std_logic_vector(g_DATA_PORT_SIZE - 1 downto 0);      
    
    signal ddr_wr_addr_s  : std_logic_vector(g_BYTE_ADDR_WIDTH-1 downto 0);
    signal ddr_wr_cmd_s   : std_logic_vector(2 downto 0);
    signal ddr_wr_cmd_en_s: std_logic;   
    signal ddr_wdf_data_s              : std_logic_vector(511 downto 0); 
    signal ddr_wdf_end_s               : std_logic;
    signal ddr_wdf_mask_s              : std_logic_vector(63 downto 0);
    signal ddr_wdf_wren_s              : std_logic;

    signal ddr1_wr_addr_s  : std_logic_vector(g_BYTE_ADDR_WIDTH-1 downto 0);
    signal ddr1_wr_cmd_s   : std_logic_vector(2 downto 0);
    signal ddr1_wr_cmd_en_s: std_logic;   
    signal ddr1_wdf_data_s              : std_logic_vector(511 downto 0); 
    signal ddr1_wdf_end_s               : std_logic;
    signal ddr1_wdf_mask_s              : std_logic_vector(63 downto 0);
    signal ddr1_wdf_wren_s              : std_logic;
    
    signal ddr_rd_addr_s : std_logic_vector(g_BYTE_ADDR_WIDTH-1 downto 0);
    signal ddr_rd_cmd_s : std_logic_vector(2 downto 0);
    signal ddr_rd_cmd_en_s : std_logic;
    signal ddr_rd_data_s               : std_logic_vector(511 downto 0);
    signal ddr_rd_data_end_s           : std_logic;
    signal ddr_rd_data_valid_s         : std_logic;


    signal arb_req_s : std_logic_vector(2 downto 0);
    signal arb_gnt_s : std_logic_vector(2 downto 0);
    
    signal wb_wr_stall_s   : std_logic;
    signal wb1_wr_stall_s   : std_logic;
    signal wb_rd_stall_s : std_logic;
    signal wb_wr_ack_s : std_logic;
    signal wb1_wr_ack_s : std_logic;
    signal wb_rd_ack_s : std_logic;    
    
    --------------------------------------
    -- Counter
    --------------------------------------
    signal wb_write_wait_cnt : unsigned(7 downto 0);    
    signal wb_read_wait_cnt : unsigned(7 downto 0);

    
    
    
begin
    rst_s <= not rst_n_i;
    rr_rst_s <= rst_s or (not ddr_rdy_i) or (not ddr_wdf_rdy_i);
    
    ddr_sr_req_o                <= '0';
    ddr_ref_req_o               <= '0';
    ddr_zq_req_o                <= '0';
    

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
            wb_addr_s  <= (others =>'0');
            wb_data_s  <= (others =>'0');  
        elsif rising_edge(wb_clk_i) then
             wb_sel_s   <= wb_sel_i;
             wb_cyc_s   <= wb_cyc_i;
             wb_stb_s   <= wb_stb_i;
             wb_we_s    <= wb_we_i;
             wb_addr_s  <= wb_addr_i;
             wb_data_s  <= wb_data_i;       
        end if;
    end process p_wb_in;
    
    --------------------------------------
    -- Wishbone ack and stall
    --------------------------------------    
    
    wb_ack_o <= wb_wr_ack_s or wb_rd_ack_s;
    wb_stall_o <= wb_wr_stall_s or wb_rd_stall_s;

    wb1_ack_o <= wb1_wr_ack_s;
    wb1_stall_o <= wb1_wr_stall_s;

    --------------------------------------
    -- Wishbone write
    --------------------------------------    


    ddr3_write_core_cmp0:ddr3_write_core
        generic map (
           g_BYTE_ADDR_WIDTH => g_BYTE_ADDR_WIDTH,
           g_MASK_SIZE       => g_MASK_SIZE,
           g_DATA_PORT_SIZE  => g_DATA_PORT_SIZE,
           g_NOT_CONSECUTIVE_DETECTION => g_NOT_CONSECUTIVE_DETECTION
        )
        Port map ( 
        
               ----------------------------------------------------------------------------
               -- Reset input (active low)
               ----------------------------------------------------------------------------
               rst_n_i  => rst_n_i,
               wb_clk_i => wb_clk_i,
               wb_sel_i => wb_sel_i,
               wb_stb_i => wb_stb_i,
               wb_cyc_i => wb_cyc_i,
               wb_we_i => wb_we_i,
               wb_adr_i => wb_addr_i,
               wb_dat_i => wb_data_i,
               wb_dat_o => open,
               wb_ack_o => wb_wr_ack_s,
               wb_stall_o => wb_wr_stall_s,
               
               ddr_addr_o  => ddr_wr_addr_s,                
               ddr_cmd_o   => ddr_wr_cmd_s,
               ddr_cmd_en_o  => ddr_wr_cmd_en_s,
               ddr_wdf_data_o => ddr_wdf_data_s,
               ddr_wdf_end_o => ddr_wdf_end_s,
               ddr_wdf_mask_o => ddr_wdf_mask_s,
               ddr_wdf_wren_o => ddr_wdf_wren_s,
               ddr_rdy_i      => ddr_rdy_i,
               ddr_wdf_rdy_i  => ddr_wdf_rdy_i,
               ddr_ui_clk_i   => ddr_ui_clk_i,
              
               ddr_req_o      => arb_req_s(c_wb_wr0_nb),         
               ddr_gnt_i      => arb_gnt_s(c_wb_wr0_nb)
     );    

    ddr3_write_core_cmp1:ddr3_write_core
        generic map (
           g_BYTE_ADDR_WIDTH => g_BYTE_ADDR_WIDTH,
           g_MASK_SIZE       => g_MASK_SIZE,
           g_DATA_PORT_SIZE  => g_DATA_PORT_SIZE,
           g_NOT_CONSECUTIVE_DETECTION => g_NOT_CONSECUTIVE_DETECTION
        )
        Port map ( 
        
               ----------------------------------------------------------------------------
               -- Reset input (active low)
               ----------------------------------------------------------------------------
               rst_n_i  => rst_n_i,
               wb_clk_i => wb_clk_i,
               wb_sel_i => wb1_sel_i,
               wb_stb_i => wb1_stb_i,
               wb_cyc_i => wb1_cyc_i,
               wb_we_i => wb1_we_i,
               wb_adr_i => wb1_addr_i,
               wb_dat_i => wb1_data_i,
               wb_dat_o => wb1_data_o,
               wb_ack_o => wb1_wr_ack_s,
               wb_stall_o => wb1_wr_stall_s,
               
               ddr_addr_o  => ddr1_wr_addr_s,                
               ddr_cmd_o   => ddr1_wr_cmd_s,
               ddr_cmd_en_o  => ddr1_wr_cmd_en_s,
               ddr_wdf_data_o => ddr1_wdf_data_s,
               ddr_wdf_end_o => ddr1_wdf_end_s,
               ddr_wdf_mask_o => ddr1_wdf_mask_s,
               ddr_wdf_wren_o => ddr1_wdf_wren_s,
               ddr_rdy_i      => ddr_rdy_i,
               ddr_wdf_rdy_i  => ddr_wdf_rdy_i,
               ddr_ui_clk_i   => ddr_ui_clk_i,
              
               ddr_req_o      => arb_req_s(c_wb_wr1_nb),         
               ddr_gnt_i      => arb_gnt_s(c_wb_wr1_nb)
     ); 
    
    
    

    --------------------------------------
    -- Wishbone read
    --------------------------------------

    ddr3_read_core_cmp:ddr3_read_core
        generic map (
           g_BYTE_ADDR_WIDTH => g_BYTE_ADDR_WIDTH,
           g_MASK_SIZE       => g_MASK_SIZE,
           g_DATA_PORT_SIZE  => g_DATA_PORT_SIZE,
           g_NOT_CONSECUTIVE_DETECTION => g_NOT_CONSECUTIVE_DETECTION
        )
        Port map ( 
        
               ----------------------------------------------------------------------------
               -- Reset input (active low)
               ----------------------------------------------------------------------------
               rst_n_i  => rst_n_i,
               wb_clk_i => wb_clk_i,
               wb_sel_i => wb_sel_i,
               wb_stb_i => wb_stb_i,
               wb_cyc_i => wb_cyc_i,
               wb_we_i => wb_we_i,
               wb_adr_i => wb_addr_i,
               wb_dat_i => wb_data_i,
               wb_dat_o => wb_data_o,
               wb_ack_o => wb_rd_ack_s,
               wb_stall_o => wb_rd_stall_s,
               ddr_rd_data_i      => ddr_rd_data_s,
               ddr_rd_data_end_i  => ddr_rd_data_end_s,
               ddr_rd_data_valid_i => ddr_rd_data_valid_s,
               ddr_addr_o  => ddr_rd_addr_s,                
               ddr_cmd_o   => ddr_rd_cmd_s,
               ddr_cmd_en_o  => ddr_rd_cmd_en_s,

               ddr_rdy_i      => ddr_rdy_i,
               ddr_ui_clk_i   => ddr_ui_clk_i,
              
               ddr_req_o      => arb_req_s(c_wb_rd0_nb),         
               ddr_gnt_i      => arb_gnt_s(c_wb_rd0_nb)
               );  
    
    --------------------------------------
    -- DDR CMD
    --------------------------------------
    
    
    ddr_addr_o <= ddr_wr_addr_s when arb_gnt_s(c_wb_wr0_nb) = '1' else
                  ddr_rd_addr_s when arb_gnt_s(c_wb_rd0_nb) = '1' else
                  ddr1_wr_addr_s when arb_gnt_s(c_wb_wr1_nb) = '1' else
                  (others => '0');
     
    ddr_cmd_o   <= ddr_wr_cmd_s when arb_gnt_s(c_wb_wr0_nb) = '1' else
                   ddr_rd_cmd_s when arb_gnt_s(c_wb_rd0_nb) = '1' else
                   ddr1_wr_cmd_s when arb_gnt_s(c_wb_wr1_nb) = '1' else
                   (others => '0');
    ddr_cmd_en_o<= ddr_wr_cmd_en_s or ddr1_wr_cmd_en_s or ddr_rd_cmd_en_s;

    cmp_rr_arbiter:rr_arbiter
        generic map (
            g_CHANNELS => 3
        )
        port map (
            -- sys connect
            clk_i => ddr_ui_clk_i,
            rst_i => rr_rst_s,
            
            -- requests
            req_i => arb_req_s,
            -- grant
            gnt_o => arb_gnt_s
        );
            
        
    --------------------------------------
    -- DDR Data out
    --------------------------------------
    ddr_wdf_data_o <= ddr_wdf_data_s when arb_gnt_s(c_wb_wr0_nb) = '1' else
                      ddr1_wdf_data_s;
    ddr_wdf_end_o <= ddr_wdf_end_s when arb_gnt_s(c_wb_wr0_nb) = '1' else
                     ddr1_wdf_end_s;
    ddr_wdf_mask_o <= ddr_wdf_mask_s when arb_gnt_s(c_wb_wr0_nb) = '1' else
                      ddr1_wdf_mask_s;
    ddr_wdf_wren_o <= ddr_wdf_wren_s when arb_gnt_s(c_wb_wr0_nb) = '1' else
                      ddr1_wdf_wren_s;
    
    --------------------------------------
    -- DDR Data in
    --------------------------------------
    
    ddr_rd_data_s       <= ddr_rd_data_i;
    ddr_rd_data_end_s   <= ddr_rd_data_end_i;
    ddr_rd_data_valid_s <= ddr_rd_data_valid_i;
	


end architecture behavioral;

