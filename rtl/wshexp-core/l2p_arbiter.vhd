-------------------------------------------------------------------------------
--                                                                           --
-- CERN BE-CO-HT         GN4124 core for PCIe FMC carrier                    --
--                       http://www.ohwr.org/projects/gn4124-core            --
-------------------------------------------------------------------------------
--
-- unit name: GN4124 core arbiter (arbiter.vhd)
--
-- authors: Simon Deprez (simon.deprez@cern.ch)
--          Matthieu Cattin (matthieu.cattin@cern.ch)
--
-- date: 12-08-2010
--
-- version: 0.1
--
-- description: Arbitrates PCIe accesses between Wishbone master,
--              L2P DMA master and P2L DMA master
--
-- dependencies:
--
--------------------------------------------------------------------------------
-- GNU LESSER GENERAL PUBLIC LICENSE
--------------------------------------------------------------------------------
-- This source file is free software; you can redistribute it and/or modify it
-- under the terms of the GNU Lesser General Public License as published by the
-- Free Software Foundation; either version 2.1 of the License, or (at your
-- option) any later version. This source is distributed in the hope that it
-- will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
-- of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
-- See the GNU Lesser General Public License for more details. You should have
-- received a copy of the GNU Lesser General Public License along with this
-- source; if not, download it from http://www.gnu.org/licenses/lgpl-2.1.html
-------------------------------------------------------------------------------
-- last changes: 23-09-2010 (mcattin) Add FF on data path and
--                                    change valid request logic
-- 26.02.2014 (theim) Changed priority order (swapped LDM <-> PDM)
-------------------------------------------------------------------------------

library IEEE;
use IEEE.STD_LOGIC_1164.all;
use IEEE.NUMERIC_STD.all;
--use work.gn4124_core_pkg.all;
--use work.common_pkg.all;


entity l2p_arbiter is
  generic(
    axis_data_width_c : integer := 64
  );
  port
    (
      ---------------------------------------------------------
      -- GN4124 core clock and reset
      clk_i   : in std_logic;
      rst_n_i : in std_logic;

      ---------------------------------------------------------
      -- From Wishbone master (wbm) to arbiter (arb)      
      wbm_arb_tdata_i : in std_logic_vector (axis_data_width_c - 1 downto 0);
      wbm_arb_tkeep_i : in std_logic_vector (axis_data_width_c/8 - 1 downto 0);
      wbm_arb_tlast_i : in std_logic;
      wbm_arb_tvalid_i : in std_logic;
	  wbm_arb_tready_o : out std_logic;
      wbm_arb_req_i    : in  std_logic;
      arb_wbm_gnt_o : out std_logic;

      ---------------------------------------------------------
      -- From P2L DMA master (pdm) to arbiter (arb)
      pdm_arb_tdata_i : in std_logic_vector (axis_data_width_c - 1 downto 0);
      pdm_arb_tkeep_i : in std_logic_vector (axis_data_width_c/8 - 1 downto 0);
      pdm_arb_tlast_i : in std_logic;
      pdm_arb_tvalid_i : in std_logic;
	  pdm_arb_tready_o : out std_logic;
      pdm_arb_req_i    : in  std_logic;
      arb_pdm_gnt_o : out std_logic;

      ---------------------------------------------------------
      -- From L2P DMA master (ldm) to arbiter (arb)
      ldm_arb_tdata_i : in std_logic_vector (axis_data_width_c - 1 downto 0);
      ldm_arb_tkeep_i : in std_logic_vector (axis_data_width_c/8 - 1 downto 0);
      ldm_arb_tlast_i : in std_logic;
      ldm_arb_tvalid_i : in std_logic;
	  ldm_arb_tready_o : out std_logic;
      ldm_arb_req_i    : in  std_logic;
      arb_ldm_gnt_o : out std_logic;

      ---------------------------------------------------------
      -- From arbiter (arb) to pcie_tx (tx)
      axis_tx_tdata_o : out STD_LOGIC_VECTOR (axis_data_width_c - 1 downto 0);
      axis_tx_tkeep_o : out STD_LOGIC_VECTOR (axis_data_width_c/8 - 1 downto 0);
      axis_tx_tuser_o : out STD_LOGIC_VECTOR (3 downto 0);
      axis_tx_tlast_o : out STD_LOGIC;
      axis_tx_tvalid_o : out STD_LOGIC;
      axis_tx_tready_i : in STD_LOGIC;
      
      ---------------------------------------------------------
      -- Debug
      eop_do : out std_logic
      );
end l2p_arbiter;


architecture rtl of l2p_arbiter is


  ------------------------------------------------------------------------------
  -- Signals declaration
  ------------------------------------------------------------------------------
  signal wbm_arb_req_valid : std_logic;
  signal pdm_arb_req_valid : std_logic;
  signal ldm_arb_req_valid : std_logic;
  signal arb_wbm_gnt       : std_logic;
  signal arb_pdm_gnt       : std_logic;
  signal arb_ldm_gnt       : std_logic;
  signal eop               : std_logic;  -- End of packet
  signal axis_tx_tvalid_t   : std_logic;
  signal axis_tx_tlast_t  : std_logic;
  signal axis_tx_tdata_t    : std_logic_vector(axis_data_width_c - 1 downto 0);
  signal axis_tx_tkeep_t : std_logic_vector(axis_data_width_c/8 - 1 downto 0);
  
  
  constant c_RST_ACTIVE : std_logic := '0';

begin


  -- A request is valid only if the access not already granted to another source
  wbm_arb_req_valid <= wbm_arb_req_i and (not(arb_pdm_gnt) and not(arb_ldm_gnt));
  pdm_arb_req_valid <= pdm_arb_req_i and (not(arb_wbm_gnt) and not(arb_ldm_gnt));
  ldm_arb_req_valid <= ldm_arb_req_i and (not(arb_wbm_gnt) and not(arb_pdm_gnt));
  eop_do <= eop;

  -- Detect end of packet to delimit the arbitration phase
--  eop <= ((arb_wbm_gnt and not(wbm_arb_dframe_i) and wbm_arb_valid_i) or
--          (arb_pdm_gnt and not(pdm_arb_dframe_i) and pdm_arb_valid_i) or
--          (arb_ldm_gnt and not(ldm_arb_dframe_i) and ldm_arb_valid_i));

   process (clk_i, rst_n_i)
   begin
      if (rst_n_i = c_RST_ACTIVE) then
      eop <= '0';
      elsif rising_edge(clk_i) then
         if ((arb_wbm_gnt = '1' and wbm_arb_tlast_i = '1') or
             (arb_pdm_gnt = '1' and pdm_arb_tlast_i = '1') or
             (arb_ldm_gnt = '1' and ldm_arb_tlast_i = '1')) then
            eop <= '1';
         else
            eop <= '0';
         end if;
      end if;
   end process;
   
  -----------------------------------------------------------------------------
  -- Arbitration is started when a valid request is present and ends when the
  -- EOP condition is detected
  --
  -- Strict priority arbitration scheme
  -- Highest : WBM request
  --         : LDM request
  -- Lowest  : PDM request
  -----------------------------------------------------------------------------
  process (clk_i, rst_n_i)
  begin
    if(rst_n_i = c_RST_ACTIVE) then
      arb_wbm_gnt <= '0';
      arb_pdm_gnt <= '0';
      arb_ldm_gnt <= '0';
    elsif rising_edge(clk_i) then
      --if (arb_req_valid = '1') then
      if (eop = '1') then
        arb_wbm_gnt <= '0';
        arb_pdm_gnt <= '0';
        arb_ldm_gnt <= '0';
      elsif (wbm_arb_req_valid = '1') then
        arb_wbm_gnt <= '1';
        arb_pdm_gnt <= '0';
        arb_ldm_gnt <= '0';
      elsif (ldm_arb_req_valid = '1') then
        arb_wbm_gnt <= '0';
        arb_pdm_gnt <= '0';
        arb_ldm_gnt <= '1';
      elsif (pdm_arb_req_valid = '1') then
        arb_wbm_gnt <= '0';
        arb_pdm_gnt <= '1';
        arb_ldm_gnt <= '0';
      end if;
    end if;
  end process;

  process (clk_i, rst_n_i)
  begin
    if rst_n_i = '0' then
      axis_tx_tvalid_t  <= '0';
      axis_tx_tlast_t <= '0';
      axis_tx_tdata_t   <= (others => '0');
      axis_tx_tkeep_t <= (others => '0');
    elsif rising_edge(clk_i) then
      if arb_wbm_gnt = '1' then
        axis_tx_tvalid_t  <= wbm_arb_tvalid_i;
        axis_tx_tlast_t <= wbm_arb_tlast_i;
        axis_tx_tdata_t   <= wbm_arb_tdata_i;
        axis_tx_tkeep_t <= wbm_arb_tkeep_i;
      elsif arb_pdm_gnt = '1' then
        axis_tx_tvalid_t  <= pdm_arb_tvalid_i;
        axis_tx_tlast_t <= pdm_arb_tlast_i;
        axis_tx_tdata_t   <= pdm_arb_tdata_i;
        axis_tx_tkeep_t <= pdm_arb_tkeep_i;
      elsif arb_ldm_gnt = '1' then
        axis_tx_tvalid_t  <= ldm_arb_tvalid_i;
        axis_tx_tlast_t <= ldm_arb_tlast_i;
        axis_tx_tdata_t   <= ldm_arb_tdata_i;
        axis_tx_tkeep_t <= ldm_arb_tkeep_i;
      else
        axis_tx_tvalid_t  <= '0';
        axis_tx_tlast_t <= '0';
        axis_tx_tdata_t   <= (others => '0');
        axis_tx_tkeep_t <= (others => '0');
      end if;
    end if;
  end process;

  process (clk_i, rst_n_i)
  begin
    if rst_n_i = c_RST_ACTIVE then
      axis_tx_tvalid_o  <= '0';
      axis_tx_tlast_o <= '0';
      axis_tx_tdata_o   <= (others => '0');
      axis_tx_tkeep_o <= (others => '0');
    elsif rising_edge(clk_i) then
      axis_tx_tvalid_o  <= axis_tx_tvalid_t;
      axis_tx_tlast_o <= axis_tx_tlast_t;
      axis_tx_tdata_o   <= axis_tx_tdata_t;
      axis_tx_tkeep_o <= axis_tx_tkeep_t;
    end if;
  end process;

  arb_wbm_gnt_o <= arb_wbm_gnt;
  arb_pdm_gnt_o <= arb_pdm_gnt;
  arb_ldm_gnt_o <= arb_ldm_gnt;
  
  wbm_arb_tready_o <= axis_tx_tready_i and arb_wbm_gnt;
  pdm_arb_tready_o <= axis_tx_tready_i and arb_pdm_gnt;
  ldm_arb_tready_o <= axis_tx_tready_i and arb_ldm_gnt;
  
  axis_tx_tuser_o <= "0000";

end rtl;
