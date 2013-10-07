--==============================================================================
--! @file ddr3_ctrl_wrapper_pkg.vhd
--==============================================================================

--! Standard library
library IEEE;
--! Standard packages
use IEEE.STD_LOGIC_1164.all;
use IEEE.NUMERIC_STD.all;
--! Specific packages

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
-- DDR3 Controller Wrapper Package
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--! @brief
--! DDR3 controller wrapper package
--------------------------------------------------------------------------------
--! @details
--! Contains DDR3 controller wrapper component declaration.
--! Component used in the wrapper are generated using Xilinx CoreGen.
--------------------------------------------------------------------------------
--! @version
--! 0.1 | mc | 06.07.2012 | File creation and Doxygen comments
--!
--! @author
--! mc : Matthieu Cattin, CERN (BE-CO-HT)
--------------------------------------------------------------------------------

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
--------------------------------------------------------------------------------

--==============================================================================
--! Entity declaration for ddr3_ctrl_wrapper_pkg
--==============================================================================
package ddr3_ctrl_wrapper_pkg is

  --============================================================================
  --! Components declaration
  --============================================================================

  ----------------------------------------------------------------------------
  -- SPEC
  ----------------------------------------------------------------------------
  component ddr3_ctrl_spec_bank3_32b_32b
    generic
      (C3_P0_MASK_SIZE       : integer := 4;
       C3_P0_DATA_PORT_SIZE  : integer := 32;
       C3_P1_MASK_SIZE       : integer := 4;
       C3_P1_DATA_PORT_SIZE  : integer := 32;
       C3_MEMCLK_PERIOD      : integer := 3000;
       C3_RST_ACT_LOW        : integer := 1;
       C3_INPUT_CLK_TYPE     : string  := "SINGLE_ENDED";
       C3_CALIB_SOFT_IP      : string  := "TRUE";
       C3_SIMULATION         : string  := "FALSE";
       DEBUG_EN              : integer := 0;
       C3_MEM_ADDR_ORDER     : string  := "ROW_BANK_COLUMN";
       C3_NUM_DQ_PINS        : integer := 16;
       C3_MEM_ADDR_WIDTH     : integer := 14;
       C3_MEM_BANKADDR_WIDTH : integer := 3
       );
    port
      (mcb3_dram_dq        : inout std_logic_vector(C3_NUM_DQ_PINS-1 downto 0);
       mcb3_dram_a         : out   std_logic_vector(C3_MEM_ADDR_WIDTH-1 downto 0);
       mcb3_dram_ba        : out   std_logic_vector(C3_MEM_BANKADDR_WIDTH-1 downto 0);
       mcb3_dram_ras_n     : out   std_logic;
       mcb3_dram_cas_n     : out   std_logic;
       mcb3_dram_we_n      : out   std_logic;
       mcb3_dram_odt       : out   std_logic;
       mcb3_dram_reset_n   : out   std_logic;
       mcb3_dram_cke       : out   std_logic;
       mcb3_dram_dm        : out   std_logic;
       mcb3_dram_udqs      : inout std_logic;
       mcb3_dram_udqs_n    : inout std_logic;
       mcb3_rzq            : inout std_logic;
       mcb3_dram_udm       : out   std_logic;
       c3_sys_clk          : in    std_logic;
       c3_sys_rst_i        : in    std_logic;
       c3_calib_done       : out   std_logic;
       c3_clk0             : out   std_logic;
       c3_rst0             : out   std_logic;
       mcb3_dram_dqs       : inout std_logic;
       mcb3_dram_dqs_n     : inout std_logic;
       mcb3_dram_ck        : out   std_logic;
       mcb3_dram_ck_n      : out   std_logic;
       c3_p0_cmd_clk       : in    std_logic;
       c3_p0_cmd_en        : in    std_logic;
       c3_p0_cmd_instr     : in    std_logic_vector(2 downto 0);
       c3_p0_cmd_bl        : in    std_logic_vector(5 downto 0);
       c3_p0_cmd_byte_addr : in    std_logic_vector(29 downto 0);
       c3_p0_cmd_empty     : out   std_logic;
       c3_p0_cmd_full      : out   std_logic;
       c3_p0_wr_clk        : in    std_logic;
       c3_p0_wr_en         : in    std_logic;
       c3_p0_wr_mask       : in    std_logic_vector(C3_P0_MASK_SIZE - 1 downto 0);
       c3_p0_wr_data       : in    std_logic_vector(C3_P0_DATA_PORT_SIZE - 1 downto 0);
       c3_p0_wr_full       : out   std_logic;
       c3_p0_wr_empty      : out   std_logic;
       c3_p0_wr_count      : out   std_logic_vector(6 downto 0);
       c3_p0_wr_underrun   : out   std_logic;
       c3_p0_wr_error      : out   std_logic;
       c3_p0_rd_clk        : in    std_logic;
       c3_p0_rd_en         : in    std_logic;
       c3_p0_rd_data       : out   std_logic_vector(C3_P0_DATA_PORT_SIZE - 1 downto 0);
       c3_p0_rd_full       : out   std_logic;
       c3_p0_rd_empty      : out   std_logic;
       c3_p0_rd_count      : out   std_logic_vector(6 downto 0);
       c3_p0_rd_overflow   : out   std_logic;
       c3_p0_rd_error      : out   std_logic;
       c3_p1_cmd_clk       : in    std_logic;
       c3_p1_cmd_en        : in    std_logic;
       c3_p1_cmd_instr     : in    std_logic_vector(2 downto 0);
       c3_p1_cmd_bl        : in    std_logic_vector(5 downto 0);
       c3_p1_cmd_byte_addr : in    std_logic_vector(29 downto 0);
       c3_p1_cmd_empty     : out   std_logic;
       c3_p1_cmd_full      : out   std_logic;
       c3_p1_wr_clk        : in    std_logic;
       c3_p1_wr_en         : in    std_logic;
       c3_p1_wr_mask       : in    std_logic_vector(C3_P1_MASK_SIZE - 1 downto 0);
       c3_p1_wr_data       : in    std_logic_vector(C3_P1_DATA_PORT_SIZE - 1 downto 0);
       c3_p1_wr_full       : out   std_logic;
       c3_p1_wr_empty      : out   std_logic;
       c3_p1_wr_count      : out   std_logic_vector(6 downto 0);
       c3_p1_wr_underrun   : out   std_logic;
       c3_p1_wr_error      : out   std_logic;
       c3_p1_rd_clk        : in    std_logic;
       c3_p1_rd_en         : in    std_logic;
       c3_p1_rd_data       : out   std_logic_vector(C3_P1_DATA_PORT_SIZE - 1 downto 0);
       c3_p1_rd_full       : out   std_logic;
       c3_p1_rd_empty      : out   std_logic;
       c3_p1_rd_count      : out   std_logic_vector(6 downto 0);
       c3_p1_rd_overflow   : out   std_logic;
       c3_p1_rd_error      : out   std_logic
       );
  end component ddr3_ctrl_spec_bank3_32b_32b;

  component ddr3_ctrl_spec_bank3_64b_32b
    generic
      (C3_P0_MASK_SIZE       : integer := 8;
       C3_P0_DATA_PORT_SIZE  : integer := 64;
       C3_P1_MASK_SIZE       : integer := 4;
       C3_P1_DATA_PORT_SIZE  : integer := 32;
       C3_MEMCLK_PERIOD      : integer := 3000;
       C3_RST_ACT_LOW        : integer := 0;
       C3_INPUT_CLK_TYPE     : string  := "SINGLE_ENDED";
       C3_CALIB_SOFT_IP      : string  := "TRUE";
       C3_SIMULATION         : string  := "FALSE";
       DEBUG_EN              : integer := 0;
       C3_MEM_ADDR_ORDER     : string  := "ROW_BANK_COLUMN";
       C3_NUM_DQ_PINS        : integer := 16;
       C3_MEM_ADDR_WIDTH     : integer := 14;
       C3_MEM_BANKADDR_WIDTH : integer := 3
       );
    port
      (mcb3_dram_dq        : inout std_logic_vector(C3_NUM_DQ_PINS-1 downto 0);
       mcb3_dram_a         : out   std_logic_vector(C3_MEM_ADDR_WIDTH-1 downto 0);
       mcb3_dram_ba        : out   std_logic_vector(C3_MEM_BANKADDR_WIDTH-1 downto 0);
       mcb3_dram_ras_n     : out   std_logic;
       mcb3_dram_cas_n     : out   std_logic;
       mcb3_dram_we_n      : out   std_logic;
       mcb3_dram_odt       : out   std_logic;
       mcb3_dram_reset_n   : out   std_logic;
       mcb3_dram_cke       : out   std_logic;
       mcb3_dram_dm        : out   std_logic;
       mcb3_dram_udqs      : inout std_logic;
       mcb3_dram_udqs_n    : inout std_logic;
       mcb3_rzq            : inout std_logic;
       mcb3_dram_udm       : out   std_logic;
       c3_sys_clk          : in    std_logic;
       c3_sys_rst_i        : in    std_logic;
       c3_calib_done       : out   std_logic;
       c3_clk0             : out   std_logic;
       c3_rst0             : out   std_logic;
       mcb3_dram_dqs       : inout std_logic;
       mcb3_dram_dqs_n     : inout std_logic;
       mcb3_dram_ck        : out   std_logic;
       mcb3_dram_ck_n      : out   std_logic;
       c3_p0_cmd_clk       : in    std_logic;
       c3_p0_cmd_en        : in    std_logic;
       c3_p0_cmd_instr     : in    std_logic_vector(2 downto 0);
       c3_p0_cmd_bl        : in    std_logic_vector(5 downto 0);
       c3_p0_cmd_byte_addr : in    std_logic_vector(29 downto 0);
       c3_p0_cmd_empty     : out   std_logic;
       c3_p0_cmd_full      : out   std_logic;
       c3_p0_wr_clk        : in    std_logic;
       c3_p0_wr_en         : in    std_logic;
       c3_p0_wr_mask       : in    std_logic_vector(C3_P0_MASK_SIZE - 1 downto 0);
       c3_p0_wr_data       : in    std_logic_vector(C3_P0_DATA_PORT_SIZE - 1 downto 0);
       c3_p0_wr_full       : out   std_logic;
       c3_p0_wr_empty      : out   std_logic;
       c3_p0_wr_count      : out   std_logic_vector(6 downto 0);
       c3_p0_wr_underrun   : out   std_logic;
       c3_p0_wr_error      : out   std_logic;
       c3_p0_rd_clk        : in    std_logic;
       c3_p0_rd_en         : in    std_logic;
       c3_p0_rd_data       : out   std_logic_vector(C3_P0_DATA_PORT_SIZE - 1 downto 0);
       c3_p0_rd_full       : out   std_logic;
       c3_p0_rd_empty      : out   std_logic;
       c3_p0_rd_count      : out   std_logic_vector(6 downto 0);
       c3_p0_rd_overflow   : out   std_logic;
       c3_p0_rd_error      : out   std_logic;
       c3_p1_cmd_clk       : in    std_logic;
       c3_p1_cmd_en        : in    std_logic;
       c3_p1_cmd_instr     : in    std_logic_vector(2 downto 0);
       c3_p1_cmd_bl        : in    std_logic_vector(5 downto 0);
       c3_p1_cmd_byte_addr : in    std_logic_vector(29 downto 0);
       c3_p1_cmd_empty     : out   std_logic;
       c3_p1_cmd_full      : out   std_logic;
       c3_p1_wr_clk        : in    std_logic;
       c3_p1_wr_en         : in    std_logic;
       c3_p1_wr_mask       : in    std_logic_vector(C3_P1_MASK_SIZE - 1 downto 0);
       c3_p1_wr_data       : in    std_logic_vector(C3_P1_DATA_PORT_SIZE - 1 downto 0);
       c3_p1_wr_full       : out   std_logic;
       c3_p1_wr_empty      : out   std_logic;
       c3_p1_wr_count      : out   std_logic_vector(6 downto 0);
       c3_p1_wr_underrun   : out   std_logic;
       c3_p1_wr_error      : out   std_logic;
       c3_p1_rd_clk        : in    std_logic;
       c3_p1_rd_en         : in    std_logic;
       c3_p1_rd_data       : out   std_logic_vector(C3_P1_DATA_PORT_SIZE - 1 downto 0);
       c3_p1_rd_full       : out   std_logic;
       c3_p1_rd_empty      : out   std_logic;
       c3_p1_rd_count      : out   std_logic_vector(6 downto 0);
       c3_p1_rd_overflow   : out   std_logic;
       c3_p1_rd_error      : out   std_logic
       );
  end component ddr3_ctrl_spec_bank3_64b_32b;


  ----------------------------------------------------------------------------
  -- SVEC
  ----------------------------------------------------------------------------
  component ddr3_ctrl_svec_bank4_32b_32b
    generic
      (C4_P0_MASK_SIZE       : integer := 4;
       C4_P0_DATA_PORT_SIZE  : integer := 32;
       C4_P1_MASK_SIZE       : integer := 4;
       C4_P1_DATA_PORT_SIZE  : integer := 32;
       C4_MEMCLK_PERIOD      : integer := 3000;
       C4_RST_ACT_LOW        : integer := 1;
       C4_INPUT_CLK_TYPE     : string  := "SINGLE_ENDED";
       C4_CALIB_SOFT_IP      : string  := "TRUE";
       C4_SIMULATION         : string  := "FALSE";
       DEBUG_EN              : integer := 0;
       C4_MEM_ADDR_ORDER     : string  := "ROW_BANK_COLUMN";
       C4_NUM_DQ_PINS        : integer := 16;
       C4_MEM_ADDR_WIDTH     : integer := 14;
       C4_MEM_BANKADDR_WIDTH : integer := 3
       );
    port
      (mcb4_dram_dq        : inout std_logic_vector(C4_NUM_DQ_PINS-1 downto 0);
       mcb4_dram_a         : out   std_logic_vector(C4_MEM_ADDR_WIDTH-1 downto 0);
       mcb4_dram_ba        : out   std_logic_vector(C4_MEM_BANKADDR_WIDTH-1 downto 0);
       mcb4_dram_ras_n     : out   std_logic;
       mcb4_dram_cas_n     : out   std_logic;
       mcb4_dram_we_n      : out   std_logic;
       mcb4_dram_odt       : out   std_logic;
       mcb4_dram_reset_n   : out   std_logic;
       mcb4_dram_cke       : out   std_logic;
       mcb4_dram_dm        : out   std_logic;
       mcb4_dram_udqs      : inout std_logic;
       mcb4_dram_udqs_n    : inout std_logic;
       mcb4_rzq            : inout std_logic;
       mcb4_dram_udm       : out   std_logic;
       c4_sys_clk          : in    std_logic;
       c4_sys_rst_i        : in    std_logic;
       c4_calib_done       : out   std_logic;
       c4_clk0             : out   std_logic;
       c4_rst0             : out   std_logic;
       mcb4_dram_dqs       : inout std_logic;
       mcb4_dram_dqs_n     : inout std_logic;
       mcb4_dram_ck        : out   std_logic;
       mcb4_dram_ck_n      : out   std_logic;
       c4_p0_cmd_clk       : in    std_logic;
       c4_p0_cmd_en        : in    std_logic;
       c4_p0_cmd_instr     : in    std_logic_vector(2 downto 0);
       c4_p0_cmd_bl        : in    std_logic_vector(5 downto 0);
       c4_p0_cmd_byte_addr : in    std_logic_vector(29 downto 0);
       c4_p0_cmd_empty     : out   std_logic;
       c4_p0_cmd_full      : out   std_logic;
       c4_p0_wr_clk        : in    std_logic;
       c4_p0_wr_en         : in    std_logic;
       c4_p0_wr_mask       : in    std_logic_vector(C4_P0_MASK_SIZE - 1 downto 0);
       c4_p0_wr_data       : in    std_logic_vector(C4_P0_DATA_PORT_SIZE - 1 downto 0);
       c4_p0_wr_full       : out   std_logic;
       c4_p0_wr_empty      : out   std_logic;
       c4_p0_wr_count      : out   std_logic_vector(6 downto 0);
       c4_p0_wr_underrun   : out   std_logic;
       c4_p0_wr_error      : out   std_logic;
       c4_p0_rd_clk        : in    std_logic;
       c4_p0_rd_en         : in    std_logic;
       c4_p0_rd_data       : out   std_logic_vector(C4_P0_DATA_PORT_SIZE - 1 downto 0);
       c4_p0_rd_full       : out   std_logic;
       c4_p0_rd_empty      : out   std_logic;
       c4_p0_rd_count      : out   std_logic_vector(6 downto 0);
       c4_p0_rd_overflow   : out   std_logic;
       c4_p0_rd_error      : out   std_logic;
       c4_p1_cmd_clk       : in    std_logic;
       c4_p1_cmd_en        : in    std_logic;
       c4_p1_cmd_instr     : in    std_logic_vector(2 downto 0);
       c4_p1_cmd_bl        : in    std_logic_vector(5 downto 0);
       c4_p1_cmd_byte_addr : in    std_logic_vector(29 downto 0);
       c4_p1_cmd_empty     : out   std_logic;
       c4_p1_cmd_full      : out   std_logic;
       c4_p1_wr_clk        : in    std_logic;
       c4_p1_wr_en         : in    std_logic;
       c4_p1_wr_mask       : in    std_logic_vector(C4_P1_MASK_SIZE - 1 downto 0);
       c4_p1_wr_data       : in    std_logic_vector(C4_P1_DATA_PORT_SIZE - 1 downto 0);
       c4_p1_wr_full       : out   std_logic;
       c4_p1_wr_empty      : out   std_logic;
       c4_p1_wr_count      : out   std_logic_vector(6 downto 0);
       c4_p1_wr_underrun   : out   std_logic;
       c4_p1_wr_error      : out   std_logic;
       c4_p1_rd_clk        : in    std_logic;
       c4_p1_rd_en         : in    std_logic;
       c4_p1_rd_data       : out   std_logic_vector(C4_P1_DATA_PORT_SIZE - 1 downto 0);
       c4_p1_rd_full       : out   std_logic;
       c4_p1_rd_empty      : out   std_logic;
       c4_p1_rd_count      : out   std_logic_vector(6 downto 0);
       c4_p1_rd_overflow   : out   std_logic;
       c4_p1_rd_error      : out   std_logic
       );
  end component ddr3_ctrl_svec_bank4_32b_32b;

  component ddr3_ctrl_svec_bank4_64b_32b
    generic
      (C4_P0_MASK_SIZE       : integer := 8;
       C4_P0_DATA_PORT_SIZE  : integer := 64;
       C4_P1_MASK_SIZE       : integer := 4;
       C4_P1_DATA_PORT_SIZE  : integer := 32;
       C4_MEMCLK_PERIOD      : integer := 3000;
       C4_RST_ACT_LOW        : integer := 1;
       C4_INPUT_CLK_TYPE     : string  := "SINGLE_ENDED";
       C4_CALIB_SOFT_IP      : string  := "TRUE";
       C4_SIMULATION         : string  := "FALSE";
       DEBUG_EN              : integer := 0;
       C4_MEM_ADDR_ORDER     : string  := "ROW_BANK_COLUMN";
       C4_NUM_DQ_PINS        : integer := 16;
       C4_MEM_ADDR_WIDTH     : integer := 14;
       C4_MEM_BANKADDR_WIDTH : integer := 3
       );
    port
      (mcb4_dram_dq        : inout std_logic_vector(C4_NUM_DQ_PINS-1 downto 0);
       mcb4_dram_a         : out   std_logic_vector(C4_MEM_ADDR_WIDTH-1 downto 0);
       mcb4_dram_ba        : out   std_logic_vector(C4_MEM_BANKADDR_WIDTH-1 downto 0);
       mcb4_dram_ras_n     : out   std_logic;
       mcb4_dram_cas_n     : out   std_logic;
       mcb4_dram_we_n      : out   std_logic;
       mcb4_dram_odt       : out   std_logic;
       mcb4_dram_reset_n   : out   std_logic;
       mcb4_dram_cke       : out   std_logic;
       mcb4_dram_dm        : out   std_logic;
       mcb4_dram_udqs      : inout std_logic;
       mcb4_dram_udqs_n    : inout std_logic;
       mcb4_rzq            : inout std_logic;
       mcb4_dram_udm       : out   std_logic;
       c4_sys_clk          : in    std_logic;
       c4_sys_rst_i        : in    std_logic;
       c4_calib_done       : out   std_logic;
       c4_clk0             : out   std_logic;
       c4_rst0             : out   std_logic;
       mcb4_dram_dqs       : inout std_logic;
       mcb4_dram_dqs_n     : inout std_logic;
       mcb4_dram_ck        : out   std_logic;
       mcb4_dram_ck_n      : out   std_logic;
       c4_p0_cmd_clk       : in    std_logic;
       c4_p0_cmd_en        : in    std_logic;
       c4_p0_cmd_instr     : in    std_logic_vector(2 downto 0);
       c4_p0_cmd_bl        : in    std_logic_vector(5 downto 0);
       c4_p0_cmd_byte_addr : in    std_logic_vector(29 downto 0);
       c4_p0_cmd_empty     : out   std_logic;
       c4_p0_cmd_full      : out   std_logic;
       c4_p0_wr_clk        : in    std_logic;
       c4_p0_wr_en         : in    std_logic;
       c4_p0_wr_mask       : in    std_logic_vector(C4_P0_MASK_SIZE - 1 downto 0);
       c4_p0_wr_data       : in    std_logic_vector(C4_P0_DATA_PORT_SIZE - 1 downto 0);
       c4_p0_wr_full       : out   std_logic;
       c4_p0_wr_empty      : out   std_logic;
       c4_p0_wr_count      : out   std_logic_vector(6 downto 0);
       c4_p0_wr_underrun   : out   std_logic;
       c4_p0_wr_error      : out   std_logic;
       c4_p0_rd_clk        : in    std_logic;
       c4_p0_rd_en         : in    std_logic;
       c4_p0_rd_data       : out   std_logic_vector(C4_P0_DATA_PORT_SIZE - 1 downto 0);
       c4_p0_rd_full       : out   std_logic;
       c4_p0_rd_empty      : out   std_logic;
       c4_p0_rd_count      : out   std_logic_vector(6 downto 0);
       c4_p0_rd_overflow   : out   std_logic;
       c4_p0_rd_error      : out   std_logic;
       c4_p1_cmd_clk       : in    std_logic;
       c4_p1_cmd_en        : in    std_logic;
       c4_p1_cmd_instr     : in    std_logic_vector(2 downto 0);
       c4_p1_cmd_bl        : in    std_logic_vector(5 downto 0);
       c4_p1_cmd_byte_addr : in    std_logic_vector(29 downto 0);
       c4_p1_cmd_empty     : out   std_logic;
       c4_p1_cmd_full      : out   std_logic;
       c4_p1_wr_clk        : in    std_logic;
       c4_p1_wr_en         : in    std_logic;
       c4_p1_wr_mask       : in    std_logic_vector(C4_P1_MASK_SIZE - 1 downto 0);
       c4_p1_wr_data       : in    std_logic_vector(C4_P1_DATA_PORT_SIZE - 1 downto 0);
       c4_p1_wr_full       : out   std_logic;
       c4_p1_wr_empty      : out   std_logic;
       c4_p1_wr_count      : out   std_logic_vector(6 downto 0);
       c4_p1_wr_underrun   : out   std_logic;
       c4_p1_wr_error      : out   std_logic;
       c4_p1_rd_clk        : in    std_logic;
       c4_p1_rd_en         : in    std_logic;
       c4_p1_rd_data       : out   std_logic_vector(C4_P1_DATA_PORT_SIZE - 1 downto 0);
       c4_p1_rd_full       : out   std_logic;
       c4_p1_rd_empty      : out   std_logic;
       c4_p1_rd_count      : out   std_logic_vector(6 downto 0);
       c4_p1_rd_overflow   : out   std_logic;
       c4_p1_rd_error      : out   std_logic
       );
  end component ddr3_ctrl_svec_bank4_64b_32b;

  component ddr3_ctrl_svec_bank5_32b_32b
    generic
      (C5_P0_MASK_SIZE       : integer := 4;
       C5_P0_DATA_PORT_SIZE  : integer := 32;
       C5_P1_MASK_SIZE       : integer := 4;
       C5_P1_DATA_PORT_SIZE  : integer := 32;
       C5_MEMCLK_PERIOD      : integer := 3000;
       C5_RST_ACT_LOW        : integer := 1;
       C5_INPUT_CLK_TYPE     : string  := "SINGLE_ENDED";
       C5_CALIB_SOFT_IP      : string  := "TRUE";
       C5_SIMULATION         : string  := "FALSE";
       DEBUG_EN              : integer := 0;
       C5_MEM_ADDR_ORDER     : string  := "ROW_BANK_COLUMN";
       C5_NUM_DQ_PINS        : integer := 16;
       C5_MEM_ADDR_WIDTH     : integer := 14;
       C5_MEM_BANKADDR_WIDTH : integer := 3
       );
    port
      (mcb5_dram_dq        : inout std_logic_vector(C5_NUM_DQ_PINS-1 downto 0);
       mcb5_dram_a         : out   std_logic_vector(C5_MEM_ADDR_WIDTH-1 downto 0);
       mcb5_dram_ba        : out   std_logic_vector(C5_MEM_BANKADDR_WIDTH-1 downto 0);
       mcb5_dram_ras_n     : out   std_logic;
       mcb5_dram_cas_n     : out   std_logic;
       mcb5_dram_we_n      : out   std_logic;
       mcb5_dram_odt       : out   std_logic;
       mcb5_dram_reset_n   : out   std_logic;
       mcb5_dram_cke       : out   std_logic;
       mcb5_dram_dm        : out   std_logic;
       mcb5_dram_udqs      : inout std_logic;
       mcb5_dram_udqs_n    : inout std_logic;
       mcb5_rzq            : inout std_logic;
       mcb5_dram_udm       : out   std_logic;
       c5_sys_clk          : in    std_logic;
       c5_sys_rst_i        : in    std_logic;
       c5_calib_done       : out   std_logic;
       c5_clk0             : out   std_logic;
       c5_rst0             : out   std_logic;
       mcb5_dram_dqs       : inout std_logic;
       mcb5_dram_dqs_n     : inout std_logic;
       mcb5_dram_ck        : out   std_logic;
       mcb5_dram_ck_n      : out   std_logic;
       c5_p0_cmd_clk       : in    std_logic;
       c5_p0_cmd_en        : in    std_logic;
       c5_p0_cmd_instr     : in    std_logic_vector(2 downto 0);
       c5_p0_cmd_bl        : in    std_logic_vector(5 downto 0);
       c5_p0_cmd_byte_addr : in    std_logic_vector(29 downto 0);
       c5_p0_cmd_empty     : out   std_logic;
       c5_p0_cmd_full      : out   std_logic;
       c5_p0_wr_clk        : in    std_logic;
       c5_p0_wr_en         : in    std_logic;
       c5_p0_wr_mask       : in    std_logic_vector(C5_P0_MASK_SIZE - 1 downto 0);
       c5_p0_wr_data       : in    std_logic_vector(C5_P0_DATA_PORT_SIZE - 1 downto 0);
       c5_p0_wr_full       : out   std_logic;
       c5_p0_wr_empty      : out   std_logic;
       c5_p0_wr_count      : out   std_logic_vector(6 downto 0);
       c5_p0_wr_underrun   : out   std_logic;
       c5_p0_wr_error      : out   std_logic;
       c5_p0_rd_clk        : in    std_logic;
       c5_p0_rd_en         : in    std_logic;
       c5_p0_rd_data       : out   std_logic_vector(C5_P0_DATA_PORT_SIZE - 1 downto 0);
       c5_p0_rd_full       : out   std_logic;
       c5_p0_rd_empty      : out   std_logic;
       c5_p0_rd_count      : out   std_logic_vector(6 downto 0);
       c5_p0_rd_overflow   : out   std_logic;
       c5_p0_rd_error      : out   std_logic;
       c5_p1_cmd_clk       : in    std_logic;
       c5_p1_cmd_en        : in    std_logic;
       c5_p1_cmd_instr     : in    std_logic_vector(2 downto 0);
       c5_p1_cmd_bl        : in    std_logic_vector(5 downto 0);
       c5_p1_cmd_byte_addr : in    std_logic_vector(29 downto 0);
       c5_p1_cmd_empty     : out   std_logic;
       c5_p1_cmd_full      : out   std_logic;
       c5_p1_wr_clk        : in    std_logic;
       c5_p1_wr_en         : in    std_logic;
       c5_p1_wr_mask       : in    std_logic_vector(C5_P1_MASK_SIZE - 1 downto 0);
       c5_p1_wr_data       : in    std_logic_vector(C5_P1_DATA_PORT_SIZE - 1 downto 0);
       c5_p1_wr_full       : out   std_logic;
       c5_p1_wr_empty      : out   std_logic;
       c5_p1_wr_count      : out   std_logic_vector(6 downto 0);
       c5_p1_wr_underrun   : out   std_logic;
       c5_p1_wr_error      : out   std_logic;
       c5_p1_rd_clk        : in    std_logic;
       c5_p1_rd_en         : in    std_logic;
       c5_p1_rd_data       : out   std_logic_vector(C5_P1_DATA_PORT_SIZE - 1 downto 0);
       c5_p1_rd_full       : out   std_logic;
       c5_p1_rd_empty      : out   std_logic;
       c5_p1_rd_count      : out   std_logic_vector(6 downto 0);
       c5_p1_rd_overflow   : out   std_logic;
       c5_p1_rd_error      : out   std_logic
       );
  end component ddr3_ctrl_svec_bank5_32b_32b;

  component ddr3_ctrl_svec_bank5_64b_32b
    generic
      (C5_P0_MASK_SIZE       : integer := 8;
       C5_P0_DATA_PORT_SIZE  : integer := 64;
       C5_P1_MASK_SIZE       : integer := 4;
       C5_P1_DATA_PORT_SIZE  : integer := 32;
       C5_MEMCLK_PERIOD      : integer := 3000;
       C5_RST_ACT_LOW        : integer := 1;
       C5_INPUT_CLK_TYPE     : string  := "SINGLE_ENDED";
       C5_CALIB_SOFT_IP      : string  := "TRUE";
       C5_SIMULATION         : string  := "FALSE";
       DEBUG_EN              : integer := 0;
       C5_MEM_ADDR_ORDER     : string  := "ROW_BANK_COLUMN";
       C5_NUM_DQ_PINS        : integer := 16;
       C5_MEM_ADDR_WIDTH     : integer := 14;
       C5_MEM_BANKADDR_WIDTH : integer := 3
       );
    port
      (mcb5_dram_dq        : inout std_logic_vector(C5_NUM_DQ_PINS-1 downto 0);
       mcb5_dram_a         : out   std_logic_vector(C5_MEM_ADDR_WIDTH-1 downto 0);
       mcb5_dram_ba        : out   std_logic_vector(C5_MEM_BANKADDR_WIDTH-1 downto 0);
       mcb5_dram_ras_n     : out   std_logic;
       mcb5_dram_cas_n     : out   std_logic;
       mcb5_dram_we_n      : out   std_logic;
       mcb5_dram_odt       : out   std_logic;
       mcb5_dram_reset_n   : out   std_logic;
       mcb5_dram_cke       : out   std_logic;
       mcb5_dram_dm        : out   std_logic;
       mcb5_dram_udqs      : inout std_logic;
       mcb5_dram_udqs_n    : inout std_logic;
       mcb5_rzq            : inout std_logic;
       mcb5_dram_udm       : out   std_logic;
       c5_sys_clk          : in    std_logic;
       c5_sys_rst_i        : in    std_logic;
       c5_calib_done       : out   std_logic;
       c5_clk0             : out   std_logic;
       c5_rst0             : out   std_logic;
       mcb5_dram_dqs       : inout std_logic;
       mcb5_dram_dqs_n     : inout std_logic;
       mcb5_dram_ck        : out   std_logic;
       mcb5_dram_ck_n      : out   std_logic;
       c5_p0_cmd_clk       : in    std_logic;
       c5_p0_cmd_en        : in    std_logic;
       c5_p0_cmd_instr     : in    std_logic_vector(2 downto 0);
       c5_p0_cmd_bl        : in    std_logic_vector(5 downto 0);
       c5_p0_cmd_byte_addr : in    std_logic_vector(29 downto 0);
       c5_p0_cmd_empty     : out   std_logic;
       c5_p0_cmd_full      : out   std_logic;
       c5_p0_wr_clk        : in    std_logic;
       c5_p0_wr_en         : in    std_logic;
       c5_p0_wr_mask       : in    std_logic_vector(C5_P0_MASK_SIZE - 1 downto 0);
       c5_p0_wr_data       : in    std_logic_vector(C5_P0_DATA_PORT_SIZE - 1 downto 0);
       c5_p0_wr_full       : out   std_logic;
       c5_p0_wr_empty      : out   std_logic;
       c5_p0_wr_count      : out   std_logic_vector(6 downto 0);
       c5_p0_wr_underrun   : out   std_logic;
       c5_p0_wr_error      : out   std_logic;
       c5_p0_rd_clk        : in    std_logic;
       c5_p0_rd_en         : in    std_logic;
       c5_p0_rd_data       : out   std_logic_vector(C5_P0_DATA_PORT_SIZE - 1 downto 0);
       c5_p0_rd_full       : out   std_logic;
       c5_p0_rd_empty      : out   std_logic;
       c5_p0_rd_count      : out   std_logic_vector(6 downto 0);
       c5_p0_rd_overflow   : out   std_logic;
       c5_p0_rd_error      : out   std_logic;
       c5_p1_cmd_clk       : in    std_logic;
       c5_p1_cmd_en        : in    std_logic;
       c5_p1_cmd_instr     : in    std_logic_vector(2 downto 0);
       c5_p1_cmd_bl        : in    std_logic_vector(5 downto 0);
       c5_p1_cmd_byte_addr : in    std_logic_vector(29 downto 0);
       c5_p1_cmd_empty     : out   std_logic;
       c5_p1_cmd_full      : out   std_logic;
       c5_p1_wr_clk        : in    std_logic;
       c5_p1_wr_en         : in    std_logic;
       c5_p1_wr_mask       : in    std_logic_vector(C5_P1_MASK_SIZE - 1 downto 0);
       c5_p1_wr_data       : in    std_logic_vector(C5_P1_DATA_PORT_SIZE - 1 downto 0);
       c5_p1_wr_full       : out   std_logic;
       c5_p1_wr_empty      : out   std_logic;
       c5_p1_wr_count      : out   std_logic_vector(6 downto 0);
       c5_p1_wr_underrun   : out   std_logic;
       c5_p1_wr_error      : out   std_logic;
       c5_p1_rd_clk        : in    std_logic;
       c5_p1_rd_en         : in    std_logic;
       c5_p1_rd_data       : out   std_logic_vector(C5_P1_DATA_PORT_SIZE - 1 downto 0);
       c5_p1_rd_full       : out   std_logic;
       c5_p1_rd_empty      : out   std_logic;
       c5_p1_rd_count      : out   std_logic_vector(6 downto 0);
       c5_p1_rd_overflow   : out   std_logic;
       c5_p1_rd_error      : out   std_logic
       );
  end component ddr3_ctrl_svec_bank5_64b_32b;


  ----------------------------------------------------------------------------
  -- VFC
  ----------------------------------------------------------------------------
  component ddr3_ctrl_vfc_bank1_32b_32b
    generic
      (
        C1_P0_MASK_SIZE       : integer := 4;
        C1_P0_DATA_PORT_SIZE  : integer := 32;
        C1_P1_MASK_SIZE       : integer := 4;
        C1_P1_DATA_PORT_SIZE  : integer := 32;
        C1_MEMCLK_PERIOD      : integer := 3000;
        C1_RST_ACT_LOW        : integer := 0;
        C1_INPUT_CLK_TYPE     : string  := "SINGLE_ENDED";
        C1_CALIB_SOFT_IP      : string  := "TRUE";
        C1_SIMULATION         : string  := "FALSE";
        DEBUG_EN              : integer := 0;
        C1_MEM_ADDR_ORDER     : string  := "ROW_BANK_COLUMN";
        C1_NUM_DQ_PINS        : integer := 16;
        C1_MEM_ADDR_WIDTH     : integer := 14;
        C1_MEM_BANKADDR_WIDTH : integer := 3
        );
    port
      (
        mcb1_dram_dq        : inout std_logic_vector(C1_NUM_DQ_PINS-1 downto 0);
        mcb1_dram_a         : out   std_logic_vector(C1_MEM_ADDR_WIDTH-1 downto 0);
        mcb1_dram_ba        : out   std_logic_vector(C1_MEM_BANKADDR_WIDTH-1 downto 0);
        mcb1_dram_ras_n     : out   std_logic;
        mcb1_dram_cas_n     : out   std_logic;
        mcb1_dram_we_n      : out   std_logic;
        mcb1_dram_odt       : out   std_logic;
        mcb1_dram_reset_n   : out   std_logic;
        mcb1_dram_cke       : out   std_logic;
        mcb1_dram_dm        : out   std_logic;
        mcb1_dram_udqs      : inout std_logic;
        mcb1_dram_udqs_n    : inout std_logic;
        mcb1_rzq            : inout std_logic;
        mcb1_dram_udm       : out   std_logic;
        c1_sys_clk          : in    std_logic;
        c1_sys_rst_i        : in    std_logic;
        c1_calib_done       : out   std_logic;
        c1_clk0             : out   std_logic;
        c1_rst0             : out   std_logic;
        mcb1_dram_dqs       : inout std_logic;
        mcb1_dram_dqs_n     : inout std_logic;
        mcb1_dram_ck        : out   std_logic;
        mcb1_dram_ck_n      : out   std_logic;
        c1_p0_cmd_clk       : in    std_logic;
        c1_p0_cmd_en        : in    std_logic;
        c1_p0_cmd_instr     : in    std_logic_vector(2 downto 0);
        c1_p0_cmd_bl        : in    std_logic_vector(5 downto 0);
        c1_p0_cmd_byte_addr : in    std_logic_vector(29 downto 0);
        c1_p0_cmd_empty     : out   std_logic;
        c1_p0_cmd_full      : out   std_logic;
        c1_p0_wr_clk        : in    std_logic;
        c1_p0_wr_en         : in    std_logic;
        c1_p0_wr_mask       : in    std_logic_vector(C1_P0_MASK_SIZE - 1 downto 0);
        c1_p0_wr_data       : in    std_logic_vector(C1_P0_DATA_PORT_SIZE - 1 downto 0);
        c1_p0_wr_full       : out   std_logic;
        c1_p0_wr_empty      : out   std_logic;
        c1_p0_wr_count      : out   std_logic_vector(6 downto 0);
        c1_p0_wr_underrun   : out   std_logic;
        c1_p0_wr_error      : out   std_logic;
        c1_p0_rd_clk        : in    std_logic;
        c1_p0_rd_en         : in    std_logic;
        c1_p0_rd_data       : out   std_logic_vector(C1_P0_DATA_PORT_SIZE - 1 downto 0);
        c1_p0_rd_full       : out   std_logic;
        c1_p0_rd_empty      : out   std_logic;
        c1_p0_rd_count      : out   std_logic_vector(6 downto 0);
        c1_p0_rd_overflow   : out   std_logic;
        c1_p0_rd_error      : out   std_logic;
        c1_p1_cmd_clk       : in    std_logic;
        c1_p1_cmd_en        : in    std_logic;
        c1_p1_cmd_instr     : in    std_logic_vector(2 downto 0);
        c1_p1_cmd_bl        : in    std_logic_vector(5 downto 0);
        c1_p1_cmd_byte_addr : in    std_logic_vector(29 downto 0);
        c1_p1_cmd_empty     : out   std_logic;
        c1_p1_cmd_full      : out   std_logic;
        c1_p1_wr_clk        : in    std_logic;
        c1_p1_wr_en         : in    std_logic;
        c1_p1_wr_mask       : in    std_logic_vector(C1_P1_MASK_SIZE - 1 downto 0);
        c1_p1_wr_data       : in    std_logic_vector(C1_P1_DATA_PORT_SIZE - 1 downto 0);
        c1_p1_wr_full       : out   std_logic;
        c1_p1_wr_empty      : out   std_logic;
        c1_p1_wr_count      : out   std_logic_vector(6 downto 0);
        c1_p1_wr_underrun   : out   std_logic;
        c1_p1_wr_error      : out   std_logic;
        c1_p1_rd_clk        : in    std_logic;
        c1_p1_rd_en         : in    std_logic;
        c1_p1_rd_data       : out   std_logic_vector(C1_P1_DATA_PORT_SIZE - 1 downto 0);
        c1_p1_rd_full       : out   std_logic;
        c1_p1_rd_empty      : out   std_logic;
        c1_p1_rd_count      : out   std_logic_vector(6 downto 0);
        c1_p1_rd_overflow   : out   std_logic;
        c1_p1_rd_error      : out   std_logic
        );
  end component ddr3_ctrl_vfc_bank1_32b_32b;

  component ddr3_ctrl_vfc_bank1_64b_32b
    generic
      (
        C1_P0_MASK_SIZE       : integer := 8;
        C1_P0_DATA_PORT_SIZE  : integer := 64;
        C1_P1_MASK_SIZE       : integer := 4;
        C1_P1_DATA_PORT_SIZE  : integer := 32;
        C1_MEMCLK_PERIOD      : integer := 3000;
        C1_RST_ACT_LOW        : integer := 0;
        C1_INPUT_CLK_TYPE     : string  := "SINGLE_ENDED";
        C1_CALIB_SOFT_IP      : string  := "TRUE";
        C1_SIMULATION         : string  := "FALSE";
        DEBUG_EN              : integer := 0;
        C1_MEM_ADDR_ORDER     : string  := "ROW_BANK_COLUMN";
        C1_NUM_DQ_PINS        : integer := 16;
        C1_MEM_ADDR_WIDTH     : integer := 14;
        C1_MEM_BANKADDR_WIDTH : integer := 3
        );
    port
      (
        mcb1_dram_dq        : inout std_logic_vector(C1_NUM_DQ_PINS-1 downto 0);
        mcb1_dram_a         : out   std_logic_vector(C1_MEM_ADDR_WIDTH-1 downto 0);
        mcb1_dram_ba        : out   std_logic_vector(C1_MEM_BANKADDR_WIDTH-1 downto 0);
        mcb1_dram_ras_n     : out   std_logic;
        mcb1_dram_cas_n     : out   std_logic;
        mcb1_dram_we_n      : out   std_logic;
        mcb1_dram_odt       : out   std_logic;
        mcb1_dram_reset_n   : out   std_logic;
        mcb1_dram_cke       : out   std_logic;
        mcb1_dram_dm        : out   std_logic;
        mcb1_dram_udqs      : inout std_logic;
        mcb1_dram_udqs_n    : inout std_logic;
        mcb1_rzq            : inout std_logic;
        mcb1_dram_udm       : out   std_logic;
        c1_sys_clk          : in    std_logic;
        c1_sys_rst_i        : in    std_logic;
        c1_calib_done       : out   std_logic;
        c1_clk0             : out   std_logic;
        c1_rst0             : out   std_logic;
        mcb1_dram_dqs       : inout std_logic;
        mcb1_dram_dqs_n     : inout std_logic;
        mcb1_dram_ck        : out   std_logic;
        mcb1_dram_ck_n      : out   std_logic;
        c1_p0_cmd_clk       : in    std_logic;
        c1_p0_cmd_en        : in    std_logic;
        c1_p0_cmd_instr     : in    std_logic_vector(2 downto 0);
        c1_p0_cmd_bl        : in    std_logic_vector(5 downto 0);
        c1_p0_cmd_byte_addr : in    std_logic_vector(29 downto 0);
        c1_p0_cmd_empty     : out   std_logic;
        c1_p0_cmd_full      : out   std_logic;
        c1_p0_wr_clk        : in    std_logic;
        c1_p0_wr_en         : in    std_logic;
        c1_p0_wr_mask       : in    std_logic_vector(C1_P0_MASK_SIZE - 1 downto 0);
        c1_p0_wr_data       : in    std_logic_vector(C1_P0_DATA_PORT_SIZE - 1 downto 0);
        c1_p0_wr_full       : out   std_logic;
        c1_p0_wr_empty      : out   std_logic;
        c1_p0_wr_count      : out   std_logic_vector(6 downto 0);
        c1_p0_wr_underrun   : out   std_logic;
        c1_p0_wr_error      : out   std_logic;
        c1_p0_rd_clk        : in    std_logic;
        c1_p0_rd_en         : in    std_logic;
        c1_p0_rd_data       : out   std_logic_vector(C1_P0_DATA_PORT_SIZE - 1 downto 0);
        c1_p0_rd_full       : out   std_logic;
        c1_p0_rd_empty      : out   std_logic;
        c1_p0_rd_count      : out   std_logic_vector(6 downto 0);
        c1_p0_rd_overflow   : out   std_logic;
        c1_p0_rd_error      : out   std_logic;
        c1_p1_cmd_clk       : in    std_logic;
        c1_p1_cmd_en        : in    std_logic;
        c1_p1_cmd_instr     : in    std_logic_vector(2 downto 0);
        c1_p1_cmd_bl        : in    std_logic_vector(5 downto 0);
        c1_p1_cmd_byte_addr : in    std_logic_vector(29 downto 0);
        c1_p1_cmd_empty     : out   std_logic;
        c1_p1_cmd_full      : out   std_logic;
        c1_p1_wr_clk        : in    std_logic;
        c1_p1_wr_en         : in    std_logic;
        c1_p1_wr_mask       : in    std_logic_vector(C1_P1_MASK_SIZE - 1 downto 0);
        c1_p1_wr_data       : in    std_logic_vector(C1_P1_DATA_PORT_SIZE - 1 downto 0);
        c1_p1_wr_full       : out   std_logic;
        c1_p1_wr_empty      : out   std_logic;
        c1_p1_wr_count      : out   std_logic_vector(6 downto 0);
        c1_p1_wr_underrun   : out   std_logic;
        c1_p1_wr_error      : out   std_logic;
        c1_p1_rd_clk        : in    std_logic;
        c1_p1_rd_en         : in    std_logic;
        c1_p1_rd_data       : out   std_logic_vector(C1_P1_DATA_PORT_SIZE - 1 downto 0);
        c1_p1_rd_full       : out   std_logic;
        c1_p1_rd_empty      : out   std_logic;
        c1_p1_rd_count      : out   std_logic_vector(6 downto 0);
        c1_p1_rd_overflow   : out   std_logic;
        c1_p1_rd_error      : out   std_logic
        );
  end component ddr3_ctrl_vfc_bank1_64b_32b;


end ddr3_ctrl_wrapper_pkg;

package body ddr3_ctrl_wrapper_pkg is


end ddr3_ctrl_wrapper_pkg;
