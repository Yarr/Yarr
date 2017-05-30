--==============================================================================
--! @file ddr3_ctrl_pkg.vhd
--==============================================================================

--! Standard library
library IEEE;
--! Standard packages
use IEEE.STD_LOGIC_1164.all;
use IEEE.NUMERIC_STD.all;
--! Specific packages

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
-- DDR3 Controller Package
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--! @brief
--! DDR3 controller package
--------------------------------------------------------------------------------
--! @details
--! Contains DDR3 controller core top level component declaration.
--------------------------------------------------------------------------------
--! @version
--! 0.1 | mc | 12.08.2011 | File creation and Doxygen comments
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
--! Entity declaration for ddr3_ctrl_pkg
--==============================================================================
package ddr3_ctrl_pkg is

  --==============================================================================
  --! Functions declaration
  --==============================================================================
  
    function f_qword_swap_256 (
      constant enable    : boolean;
      signal   din       : std_logic_vector(255 downto 0);
      signal   byte_swap : std_logic_vector(1 downto 0))
      return std_logic_vector;
  
  function f_qword_swap_512 (
      constant enable    : boolean;
      signal   din       : std_logic_vector(511 downto 0);
      signal   byte_swap : std_logic_vector(2 downto 0))
      return std_logic_vector;
      
  --==============================================================================
  --! Components declaration
  --==============================================================================

    COMPONENT fifo_315x16
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
    
    COMPONENT fifo_27x16
      PORT (
        rst : IN STD_LOGIC;
        wr_clk : IN STD_LOGIC;
        rd_clk : IN STD_LOGIC;
        din : IN STD_LOGIC_VECTOR(29-1 DOWNTO 0);
        wr_en : IN STD_LOGIC;
        rd_en : IN STD_LOGIC;
        dout : OUT STD_LOGIC_VECTOR(29-1 DOWNTO 0);
        full : OUT STD_LOGIC;
        empty : OUT STD_LOGIC
      );
    END COMPONENT;
    
    COMPONENT fifo_4x16
      PORT (
        rst : IN STD_LOGIC;
        wr_clk : IN STD_LOGIC;
        rd_clk : IN STD_LOGIC;
        din : IN STD_LOGIC_VECTOR(7 DOWNTO 0);
        wr_en : IN STD_LOGIC;
        rd_en : IN STD_LOGIC;
        dout : OUT STD_LOGIC_VECTOR(7 DOWNTO 0);
        full : OUT STD_LOGIC;
        empty : OUT STD_LOGIC;
        rd_data_count : OUT STD_LOGIC_VECTOR(3 DOWNTO 0)
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
        empty : OUT STD_LOGIC;
        rd_data_count : OUT STD_LOGIC_VECTOR(3 DOWNTO 0)
      );
    END COMPONENT;
    
    
    component qword_swap_512 is
        Port ( qword_swap : in STD_LOGIC_VECTOR (2 downto 0);
               din : in STD_LOGIC_VECTOR (511 downto 0);
               dout : out STD_LOGIC_VECTOR (511 downto 0));
    end component;

    component byte_swap_64 is
        Port ( qword_swap : in STD_LOGIC_VECTOR (2 downto 0);
               din : in STD_LOGIC_VECTOR (63 downto 0);
               dout : out STD_LOGIC_VECTOR (63 downto 0));
    end component;    

end ddr3_ctrl_pkg;

package body ddr3_ctrl_pkg is

  -----------------------------------------------------------------------------
  -- QWORD swap function
  --
  -- enable | byte_swap | din  | dout
  -- false  | XX        | ABCD | ABCD
  -- true   | 00        | ABCD | ABCD
  -- true   | 01        | ABCD | BADC
  -- true   | 10        | ABCD | CDAB
  -- true   | 11        | ABCD | DCBA
  -----------------------------------------------------------------------------
  function f_qword_swap_256 (
    constant enable    : boolean;
    signal   din       : std_logic_vector(255 downto 0);
    signal   byte_swap : std_logic_vector(1 downto 0))
    return std_logic_vector is
    variable dout : std_logic_vector(255 downto 0);
  begin
    if (enable = true) then
      case byte_swap is
        when "00" =>
          dout := din;
        when "01" =>
          dout := din(191 downto 128)
                  & din(255 downto 192)
                  & din(63 downto 0)
                  & din(127 downto 64);
        when "10" =>
          dout := din(127 downto 0)
                  & din(255 downto 128);
        when "11" =>
          dout := din(63 downto 0)
                  & din(127 downto 64)
                  & din(191 downto 128)
                  & din(255 downto 192);
        when others =>
          dout := din;
      end case;
    else
      dout := din;
    end if;
    return dout;
 end function f_qword_swap_256;

  -----------------------------------------------------------------------------
  -- QWORD swap function
  --
  -- enable | byte_swap  | din      | dout
  -- false  | XXX        | ABCDEFGH | ABCDEFGH
  -- true   | 000        | ABCDEFGH | ABCDEFGH
  -- true   | 001        | ABCDEFGH | BADCFEHG
  -- true   | 010        | ABCDEFGH | CDABGHEF
  -- true   | 011        | ABCDEFGH | DCBAHGFE
  -- true   | 100        | ABCDEFGH | EFGHABCD
  -- true   | 101        | ABCDEFGH | FEHGBADC
  -- true   | 110        | ABCDEFGH | GHEFCDAB
  -- true   | 111        | ABCDEFGH | HGFEDCBA
  -----------------------------------------------------------------------------
  function f_qword_swap_512 (
    constant enable    : boolean;
    signal   din       : std_logic_vector(511 downto 0);
    signal   byte_swap : std_logic_vector(2 downto 0))
    return std_logic_vector is
    variable dout : std_logic_vector(511 downto 0);
  begin
    if (enable = true) then
      if byte_swap(2) = '0' then
        dout := f_qword_swap_256(true, din(511 downto 256), byte_swap(1 downto 0)) & f_qword_swap_256(true, din(255 downto 0), byte_swap(1 downto 0));
      else
        dout := f_qword_swap_256(true, din(255 downto 0), byte_swap(1 downto 0)) & f_qword_swap_256(true, din(511 downto 256), byte_swap(1 downto 0));
      end if;
      
    else
      dout := din;
    end if;
    return dout;
end function f_qword_swap_512;


end ddr3_ctrl_pkg;
