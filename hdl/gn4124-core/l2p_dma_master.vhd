-- ####################################
-- # Project: Yarr
-- # Author: Timon Heim
-- # E-Mail: timon.heim at cern.ch
-- # Comments: Rewritten on basis from Matthieu Cattin, 
-- # taken from the gn4124-core on ohwr.org
-- ####################################


library IEEE;
use IEEE.STD_LOGIC_1164.all;
use IEEE.STD_LOGIC_UNSIGNED.all;
use IEEE.STD_LOGIC_ARITH.all;
--use IEEE.NUMERIC_STD.all;
use work.gn4124_core_pkg.all;
use work.common_pkg.all;

entity l2p_dma_master is
    generic (
        g_BYTE_SWAP : boolean := false
    );
    port (
        -- GN4124 core clk and reset
        clk_i   : in std_logic;
        rst_n_i : in std_logic;

        -- From the DMA controller
        dma_ctrl_target_addr_i : in  std_logic_vector(31 downto 0);
        dma_ctrl_host_addr_h_i : in  std_logic_vector(31 downto 0);
        dma_ctrl_host_addr_l_i : in  std_logic_vector(31 downto 0);
        dma_ctrl_len_i         : in  std_logic_vector(31 downto 0);
        dma_ctrl_start_l2p_i   : in  std_logic;
        dma_ctrl_done_o        : out std_logic;
        dma_ctrl_error_o       : out std_logic;
        dma_ctrl_byte_swap_i   : in  std_logic_vector(1 downto 0);
        dma_ctrl_abort_i       : in  std_logic;

        -- To the arbiter (L2P data)
        ldm_arb_valid_o  : out std_logic;
        ldm_arb_dframe_o : out std_logic;
        ldm_arb_data_o   : out std_logic_vector(31 downto 0);
        ldm_arb_req_o    : out std_logic;
        arb_ldm_gnt_i    : in  std_logic;


        -- L2P channel control
        l2p_edb_o  : out std_logic;                    -- Asserted when transfer is aborted
        l_wr_rdy_i : in  std_logic_vector(1 downto 0); -- Asserted when GN4124 is ready to receive master write
        l2p_rdy_i  : in  std_logic;                    -- De-asserted to pause transdert already in progress
        tx_error_i : in  std_logic;                    -- Asserted when unexpected or malformed paket received

        -- DMA Interface (Pipelined Wishbone)
        l2p_dma_clk_i   : in  std_logic;
        l2p_dma_adr_o   : out std_logic_vector(31 downto 0);
        l2p_dma_dat_i   : in  std_logic_vector(31 downto 0);
        l2p_dma_dat_o   : out std_logic_vector(31 downto 0);
        l2p_dma_sel_o   : out std_logic_vector(3 downto 0);
        l2p_dma_cyc_o   : out std_logic;
        l2p_dma_stb_o   : out std_logic;
        l2p_dma_we_o    : out std_logic;
        l2p_dma_ack_i   : in  std_logic;
        l2p_dma_stall_i : in  std_logic;
        p2l_dma_cyc_i   : in  std_logic -- P2L dma WB cycle for bus arbitration
    );
end l2p_dma_master;

architecture behavioral of l2p_dma_master is
    ---------------------
    -- Components
    ---------------------
    component l2p_fifo
        port (
            rst : in std_logic;
            wr_clk : in std_logic;
            rd_clk : in std_logic;
            din : in std_logic_vector(31 downto 0);
            wr_en : in std_logic;
            rd_en : in std_logic;
            prog_full_thresh_assert : in std_logic_vector(9 downto 0);
            prog_full_thresh_negate : in std_logic_vector(9 downto 0);
            dout : out std_logic_vector(31 downto 0);
            full : out std_logic;
            empty : out std_logic;
            valid : out std_logic;
            prog_full : out std_logic
        );
    end component;

    ---------------------
    -- Constants
    ---------------------
    constant c_L2P_MAX_PAYLOAD : std_logic_vector(10 downto 0) := CONV_STD_LOGIC_VECTOR(32, 11);
    constant c_ADDR_FIFO_FULL_THRES : std_logic_vector(9 downto 0) := CONV_STD_LOGIC_VECTOR(800, 10);
    constant c_DATA_FIFO_FULL_THRES : std_logic_vector(9 downto 0) := CONV_STD_LOGIC_VECTOR(800, 10);
    constant c_TIMEOUT : integer := 2000;

    ---------------------
    -- Signals
    ---------------------
    signal fifo_rst        : std_logic;
    signal fifo_rst_t      : std_logic;
    
    -- Data FIFO
    signal data_fifo_rd    : std_logic;
    signal data_fifo_wr    : std_logic;
    signal data_fifo_empty : std_logic;
    signal data_fifo_full  : std_logic;
    signal data_fifo_dout  : std_logic_vector(31 downto 0);
    signal data_fifo_din   : std_logic_vector(31 downto 0);
    
    -- Addr FIFO
    signal addr_fifo_rd    : std_logic;
    signal addr_fifo_wr    : std_logic;
    signal addr_fifo_empty : std_logic;
    signal addr_fifo_full  : std_logic;
    signal addr_fifo_dout  : std_logic_vector(31 downto 0);
    signal addr_fifo_din   : std_logic_vector(31 downto 0);

    -- L2P FSM
    type l2p_dma_state_type is (L2P_IDLE, L2P_SETUP, L2P_HEADER, 
                                L2P_ADDR_H, L2P_ADDR_L, L2P_DATA,
                                L2P_LAST_DATA, L2P_ERROR);
    signal l2p_dma_current_state : l2p_dma_state_type;

    -- L2P packets
    signal s_l2p_header    : std_logic_vector(31 downto 0);
    signal l2p_len_cnt     : std_logic_vector(29 downto 0);
    signal l2p_address_h   : std_logic_vector(31 downto 0);
    signal l2p_address_l   : std_logic_vector(31 downto 0);
    signal l2p_data_cnt    : std_logic_vector(10 downto 0);
    signal l2p_64b_address : std_logic;
    signal l2p_len_header  : std_logic_vector(9 downto 0);
    signal l2p_byte_swap   : std_logic_vector(1 downto 0);
    signal l2p_last_packet : std_logic;
    signal l2p_lbe_header  : std_logic_vector(3 downto 0);
    
    signal ldm_arb_valid   : std_logic;

    -- Counter
    signal target_addr_cnt : std_logic_vector(29 downto 0);
    signal dma_length_cnt  : std_logic_vector(29 downto 0);
    signal l2p_timeout_cnt : std_logic_vector(31 downto 0);
    signal wb_timeout_cnt  : std_logic_vector(31 downto 0);

    -- Wishbone
    signal l2p_dma_cyc_t   : std_logic;
    signal l2p_dma_stb_t   : std_logic;
    signal wb_ack_cnt      : std_logic_vector(29 downto 0);
    signal wb_read_cnt     : std_logic_vector(29 downto 0);

begin
    fifo_rst <= not rst_n_i or fifo_rst_t;

    ldm_arb_valid_o <= ldm_arb_valid;
    ---------------------
    -- L2P FSM
    ---------------------    
    p_l2p_fsm : process (clk_i, rst_n_i)
    begin
        if (rst_n_i = '0') then
            l2p_dma_current_state <= L2P_IDLE;
            ldm_arb_req_o <= '0';
            ldm_arb_data_o <= (others => '0');
            ldm_arb_valid <= '0';
            ldm_arb_dframe_o <= '0';
            data_fifo_rd <= '0';
            dma_ctrl_done_o <= '0';
            l2p_edb_o <= '0';
            l2p_timeout_cnt <= (others => '0');
            fifo_rst_t <= '0';
        elsif rising_edge(clk_i) then
            case l2p_dma_current_state is
                
                when L2P_IDLE =>
                    l2p_timeout_cnt <= (others => '0');
                    l2p_edb_o <= '0';
                    fifo_rst_t <= '0';
                    ldm_arb_req_o <= '0';
                    --ldm_arb_data_o <= (others => '0');
                    ldm_arb_valid <= '0';
                    ldm_arb_dframe_o <= '0';
                    data_fifo_rd <= '0';
                    dma_ctrl_done_o <= '0';
                    if (dma_ctrl_start_l2p_i = '1') then
                        l2p_dma_current_state <= L2P_SETUP;
                    end if;
                    

                when L2P_SETUP =>
                    l2p_timeout_cnt <= (others => '0');
                    if (l2p_rdy_i = '1') then
                        l2p_dma_current_state <= L2P_HEADER;
                        ldm_arb_req_o <= '1'; -- Request bus
                    end if;

                when L2P_HEADER =>
                    if (arb_ldm_gnt_i = '1' and l_wr_rdy_i = "11") then
                        ldm_arb_req_o <= '0'; -- Bus has been granted
                        -- Send header
                        ldm_arb_data_o <= s_l2p_header;
                        ldm_arb_valid <= '1';
                        ldm_arb_dframe_o <= '1'; -- Keep asserted to stay bus master
                        if (l2p_64b_address = '1') then
                            l2p_dma_current_state <= L2P_ADDR_H;
                        else
                            l2p_dma_current_state <= L2P_ADDR_L;
                        end if;
                    end if;

                when L2P_ADDR_H =>
                    ldm_arb_data_o <= l2p_address_h;
                    l2p_dma_current_state <= L2P_ADDR_L;

                when L2P_ADDR_L =>
                    ldm_arb_data_o <= l2p_address_l;
                    l2p_dma_current_state <= L2P_DATA;

                when L2P_DATA =>
--                    if (data_fifo_empty = '0' and l2p_rdy_i = '1' and l2p_data_cnt > 1) then
--                        data_fifo_rd <= '1';
--                    elsif (data_fifo_empty = '0' and l2p_data_cnt = 2) then
--                        data_fifo_rd <= '1';
--                    else
--                        data_fifo_rd <= '0';
--                    end if;
                    
                    if (data_fifo_empty = '0' and l2p_data_cnt > 1 and l2p_rdy_i = '1' and data_fifo_rd = '1') then
                        ldm_arb_dframe_o <= '1';
                        ldm_arb_valid <= '1';
                        ldm_arb_data_o <= f_byte_swap(g_BYTE_SWAP, data_fifo_dout, l2p_byte_swap);
                        data_fifo_rd <= '1';
                     elsif (data_fifo_empty = '0' and l2p_data_cnt > 1 and l2p_rdy_i = '0' and data_fifo_rd = '1') then
                        ldm_arb_dframe_o <= '1';
                        ldm_arb_valid <= '1';
                        ldm_arb_data_o <= f_byte_swap(g_BYTE_SWAP, data_fifo_dout, l2p_byte_swap);
                        data_fifo_rd <= '0';
                    elsif (data_fifo_empty = '0' and l2p_data_cnt > 1 and l2p_rdy_i = '1' and data_fifo_rd = '0') then
                        ldm_arb_dframe_o <= '1';
                        ldm_arb_valid <= '0';
                        ldm_arb_data_o <= f_byte_swap(g_BYTE_SWAP, data_fifo_dout, l2p_byte_swap);
                        data_fifo_rd <= '1';  
                    elsif (l2p_data_cnt = 1 and data_fifo_empty = '0' and data_fifo_rd = '1') then
                        ldm_arb_dframe_o <= '0';
                        ldm_arb_valid <= '1';
                        ldm_arb_data_o <= f_byte_swap(g_BYTE_SWAP, data_fifo_dout, l2p_byte_swap);
                        data_fifo_rd <= '0';
                        l2p_dma_current_state <= L2P_LAST_DATA;
                    elsif (l2p_data_cnt = 1 and data_fifo_empty = '0' and data_fifo_rd = '0') then
                        ldm_arb_dframe_o <= '0';
                        ldm_arb_valid <= '1';
                        ldm_arb_data_o <= f_byte_swap(g_BYTE_SWAP, data_fifo_dout, l2p_byte_swap);
                        data_fifo_rd <= '1';
                        l2p_dma_current_state <= L2P_LAST_DATA;
                    elsif (l2p_data_cnt = 0 and data_fifo_empty = '0' and data_fifo_rd = '0') then
                        ldm_arb_dframe_o <= '0';
                        ldm_arb_valid <= '1';
                        ldm_arb_data_o <= f_byte_swap(g_BYTE_SWAP, data_fifo_dout, l2p_byte_swap);
                        data_fifo_rd <= '1';
                        l2p_dma_current_state <= L2P_LAST_DATA;                            
                    else
                        ldm_arb_dframe_o <= '1';
                        ldm_arb_valid <= '0';
                        data_fifo_rd <= '0';
                    end if;
                    -- Error condition, aboirt transfer
                    if (tx_error_i = '1' or l2p_timeout_cnt > c_TIMEOUT or dma_ctrl_abort_i = '1') then
                        l2p_dma_current_state <= L2P_ERROR;
                    end if;
                     
                    -- Timeout counter
                    if (data_fifo_empty = '1' or l2p_rdy_i = '1') then
                        l2p_timeout_cnt <= l2p_timeout_cnt + 1;
                    else
                        l2p_timeout_cnt <= (others => '0');
                    end if;

                when L2P_LAST_DATA =>
                    ldm_arb_dframe_o <= '0';
                    ldm_arb_valid <= '0';
                    data_fifo_rd <= '0';
                    if (dma_ctrl_abort_i = '1' or tx_error_i = '1') then
                        l2p_dma_current_state <= L2P_IDLE;
                        dma_ctrl_done_o <= '1';
                    elsif (l2p_last_packet = '0') then
                        l2p_dma_current_state <= L2P_SETUP;
                    else
                        l2p_dma_current_state <= L2P_IDLE;
                        dma_ctrl_done_o <= '1';
                    end if;
                
                when L2P_ERROR =>
                    ldm_arb_dframe_o <= '0';
                    ldm_arb_valid <='1';
                    l2p_edb_o <= '1';
                    fifo_rst_t <= '1';
                    l2p_dma_current_state <= L2P_IDLE;
                
                when others =>
                    l2p_dma_current_state <= L2P_IDLE;

            end case;
        end if;
    end process p_l2p_fsm;


    ---------------------
    --- Paket Generator
    ---------------------
    -- Last Byte Enable must be "0000" when length = 1
    l2p_lbe_header <= "0000" when l2p_len_header = 1 else "1111";
    -- 64bit address flag
    l2p_64b_address <= '0' when l2p_address_h = 0 else '1';

    -- Packet header
    s_l2p_header <= "000"                                -->  Traffic Class
                    & '0'                                -->  Snoop
                    & "001" & l2p_64b_address            -->  Header type,
                                                         --   memory write 32-bit or
                                                         --   memory write 64-bit
                    & l2p_lbe_header                     -->  LBE (Last Byte Enable)
                    & "1111"                             -->  FBE (First Byte Enable)
                    & "000"                              -->  Reserved
                    & '0'                                -->  VC (Virtual Channel)
                    & "00"                               -->  Reserved
                    & std_logic_vector(l2p_len_header);  -->  Length (in 32-bit words)
                                                         --   0x000 => 1024 words (4096 bytes)
    p_pkt_gen : process (clk_i, rst_n_i)
    begin
        if (rst_n_i = '0') then
            l2p_len_cnt <= (others => '0');
            l2p_data_cnt <= (others => '0');
            l2p_address_h <= (others => '0');
            l2p_address_l <= (others => '0');
            l2p_len_header <= (others => '0');
            l2p_byte_swap <= (others => '0');
            l2p_last_packet <= '0';
        elsif rising_edge(clk_i) then
            if (l2p_dma_current_state = L2P_IDLE) then
                l2p_len_cnt <= dma_ctrl_len_i(31 downto 2);
                l2p_address_h <= dma_ctrl_host_addr_h_i;
                l2p_address_l <= dma_ctrl_host_addr_l_i;
                l2p_byte_swap <= dma_ctrl_byte_swap_i;
            elsif (l2p_dma_current_state = L2P_SETUP) then
                if (l2p_len_cnt > c_L2P_MAX_PAYLOAD(9 downto 0)) then
                    l2p_data_cnt <= c_L2P_MAX_PAYLOAD;
                    l2p_len_header <= c_L2P_MAX_PAYLOAD(9 downto 0);
                    l2p_last_packet <= '0';
                elsif (l2p_len_cnt = c_L2P_MAX_PAYLOAD(9 downto 0)) then
                    l2p_data_cnt <= c_L2P_MAX_PAYLOAD;
                    l2p_len_header <= c_L2P_MAX_PAYLOAD(9 downto 0);
                    l2p_last_packet <= '1';
                else
                    l2p_data_cnt <= l2p_len_cnt(10 downto 0);
                    l2p_len_header <= l2p_len_cnt(9 downto 0);
                    l2p_last_packet <= '1';
                end if;
            --elsif (l2p_dma_current_state = L2P_HEADER) then
            --elsif (l2p_dma_current_state = L2P_ADDR_H) then
            elsif (l2p_dma_current_state = L2P_ADDR_L) then
                --l2p_data_cnt <= l2p_data_cnt -1;
            elsif (l2p_dma_current_state = L2P_DATA) then
               --if (data_fifo_empty = '0' and l2p_data_cnt > 1 and data_fifo_rd = '1') then
                if (ldm_arb_valid = '1') then
                    l2p_data_cnt <= l2p_data_cnt -1;
                end if;
            elsif (l2p_dma_current_state = L2P_LAST_DATA) then
                if (l2p_last_packet = '0') then
                    -- Increase Address
                    -- TODO Not overflow safe !
                    l2p_address_l <= l2p_address_l + (CONV_INTEGER(c_L2P_MAX_PAYLOAD) * 4);
                    l2p_len_cnt <= l2p_len_cnt - c_L2P_MAX_PAYLOAD;
                else
                    l2p_len_cnt <= (others => '0');
                end if;
            end if;
        end if;
    end process p_pkt_gen;

    ---------------------
    -- Address Counter
    ---------------------
    p_target_cnt : process (clk_i, rst_n_i)
    begin
        if (rst_n_i = '0') then
            target_addr_cnt <= (others => '0');
            dma_length_cnt <= (others => '0');
            dma_ctrl_error_o <= '0';
            addr_fifo_wr <= '0';
            addr_fifo_din <= (others => '0');
        elsif rising_edge(clk_i) then
            -- New Transfer started
            if (l2p_dma_current_state = L2P_ERROR) then
                target_addr_cnt <= (others => '0');
                dma_ctrl_error_o <= '1';
                addr_fifo_wr <= '0';
                dma_length_cnt <= (others => '0');
            elsif (dma_ctrl_start_l2p_i = '1') then
                if (l2p_dma_current_state = L2P_IDLE) then
                    -- dma target adrr is byte address, need 32bit address
                    target_addr_cnt <= dma_ctrl_target_addr_i(31 downto 2);
                    -- dma target length is in byte, need 32bit
                    dma_length_cnt <= dma_ctrl_len_i(31 downto 2);
                    dma_ctrl_error_o <= '0';
                else
                    target_addr_cnt <= (others => '0');
                    dma_length_cnt <= (others => '0');
                    dma_ctrl_error_o <= '1';
                end if;
                addr_fifo_wr <= '0';
           elsif (dma_length_cnt /= 0 and addr_fifo_full = '0') then
                addr_fifo_wr <= '1';
                target_addr_cnt <= target_addr_cnt + 1;
                dma_length_cnt <= dma_length_cnt - 1;
                addr_fifo_din <= "00" & std_logic_vector(target_addr_cnt);
            else
                addr_fifo_wr <= '0';
                dma_ctrl_error_o <= '0';
            end if;
        end if;
    end process p_target_cnt;

    ---------------------
    -- Wishbone Master
    ---------------------
    -- Tie offs
    l2p_dma_cyc_o <= l2p_dma_cyc_t;
    l2p_dma_stb_o <= l2p_dma_stb_t;
    l2p_dma_adr_o <= addr_fifo_dout;
    l2p_dma_dat_o <= (others => '0');
    l2p_dma_we_o <= '0';

    p_wb_master : process (l2p_dma_clk_i, rst_n_i)
    begin
        if (rst_n_i = '0') then
            l2p_dma_stb_t <= '0';
            l2p_dma_cyc_t <= '0';
            l2p_dma_sel_o <= (others => '0');
            addr_fifo_rd <= '0';
            wb_read_cnt <= (others => '0');
            wb_ack_cnt <= (others => '0');
            --l2p_dma_adr_o <= (others => '0');
            
        elsif rising_edge(l2p_dma_clk_i) then
            -- Read logic
            if (l2p_dma_current_state = L2P_ERROR) then
                wb_read_cnt <= (others => '0');
                l2p_dma_stb_t <= '0';
                l2p_dma_sel_o <= (others => '1');
                addr_fifo_rd <= '0';               
            elsif (l2p_dma_current_state = L2P_IDLE and dma_ctrl_start_l2p_i = '1') then
                wb_read_cnt <= dma_ctrl_len_i(31 downto 2);
                l2p_dma_stb_t <= '0';
                l2p_dma_sel_o <= (others => '1');
                addr_fifo_rd <= '0';
            elsif (addr_fifo_empty = '0' and l2p_dma_stall_i = '0' 
                   and data_fifo_full = '0' and p2l_dma_cyc_i = '0'
                   and wb_read_cnt /= 0) then
                wb_read_cnt <= wb_read_cnt - 1;
                l2p_dma_stb_t <= '1';
                --l2p_dma_adr_o <= addr_fifo_dout;
                l2p_dma_sel_o <= (others => '1');
                addr_fifo_rd <= '1';
            else
                l2p_dma_stb_t <= '0';
                l2p_dma_sel_o <= (others => '1');
                addr_fifo_rd <= '0';
            end if;

--            if (addr_fifo_rd = '1') then
--               l2p_dma_adr_o <= addr_fifo_dout;
--               l2p_dma_sel_o <= (others => '1');
--               l2p_dma_stb_t <= '1';
--            else
--               l2p_dma_adr_o <= (others => '0');
--               l2p_dma_sel_o <= (others => '0');
--               l2p_dma_stb_t <= '0';
--            end if;
            
            -- Cycle logic
            if (wb_read_cnt /= 0 or wb_ack_cnt /= 0) then
                l2p_dma_cyc_t <= '1';
            else
                l2p_dma_cyc_t <= '0';
            end if;

            -- Acknowledge counter
            if (l2p_dma_ack_i = '1' and wb_ack_cnt /= 0) then
                wb_ack_cnt <= wb_ack_cnt - 1;
            elsif (l2p_dma_current_state = L2P_ERROR) then
               wb_ack_cnt <= (others => '0');
            elsif (l2p_dma_current_state = L2P_IDLE and dma_ctrl_start_l2p_i = '1') then
                wb_ack_cnt <= dma_ctrl_len_i(31 downto 2);

            end if;
            
            -- Timeout counter
            if (l2p_dma_ack_i = '0') then
               wb_timeout_cnt <= wb_timeout_cnt + 1;
            else
               wb_timeout_cnt <= (others => '0');
            end if;
        end if;
    end process p_wb_master;

    -- Receive data
    data_fifo_din <= l2p_dma_dat_i;
    data_fifo_wr <= l2p_dma_ack_i and l2p_dma_cyc_t;
    
    ---------------------
    -- FIFOs
    ---------------------
    cmp_addr_fifo : l2p_fifo
    port map (
       rst => fifo_rst,
        wr_clk => clk_i,
        rd_clk => l2p_dma_clk_i,
        din => addr_fifo_din,
        wr_en => addr_fifo_wr,
        rd_en => addr_fifo_rd,
        prog_full_thresh_assert => c_ADDR_FIFO_FULL_THRES,
        prog_full_thresh_negate => c_ADDR_FIFO_FULL_THRES-50,
        dout => addr_fifo_dout,
        full => open,
        empty => addr_fifo_empty,
        valid => open,
        prog_full => addr_fifo_full
    );     
    
    cmp_data_fifo : l2p_fifo
    port map (
       rst => fifo_rst,
        wr_clk => l2p_dma_clk_i,
        rd_clk => clk_i,
        din => data_fifo_din,
        wr_en => data_fifo_wr,
        rd_en => data_fifo_rd,
        prog_full_thresh_assert => c_DATA_FIFO_FULL_THRES,
        prog_full_thresh_negate => c_DATA_FIFO_FULL_THRES-50,
        dout => data_fifo_dout,
        full => open,
        empty => data_fifo_empty,
        valid => open,
        prog_full => data_fifo_full
    );

end behavioral;
