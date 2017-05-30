--==============================================================================
--! @file gn4124_core_pkg_s6.vhd
--==============================================================================

--! Standard library
library IEEE;
--! Standard packages
use IEEE.STD_LOGIC_1164.all;
use IEEE.NUMERIC_STD.all;

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
-- Package for gn4124 core
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--! @brief
--! Package for components declaration and core wide constants.
--! Spartan6 FPGAs version.
--------------------------------------------------------------------------------
--! @version
--! 0.1 | mc | 01.09.2010 | File creation and Doxygen comments
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
--! Package declaration
--==============================================================================
package wshexp_core_pkg is


--==============================================================================
--! Constants declaration
--==============================================================================
  constant c_RST_ACTIVE : std_logic := '0';  -- Active low reset


--==============================================================================
--! Functions declaration
--==============================================================================
  function f_byte_swap_64 (
    constant enable    : boolean;
    signal   din       : std_logic_vector(63 downto 0);
    signal   byte_swap : std_logic_vector(2 downto 0))
    return std_logic_vector;
    
  function f_byte_swap (
    constant enable    : boolean;
    signal   din       : std_logic_vector(31 downto 0);
    signal   byte_swap : std_logic_vector(1 downto 0))
    return std_logic_vector;

  function log2_ceil(N : natural) return positive;

--==============================================================================
--! Components declaration
--==============================================================================

-----------------------------------------------------------------------------



end wshexp_core_pkg;

package body wshexp_core_pkg is

    -----------------------------------------------------------------------------
  -- Byte swap function
  --
  -- enable | byte_swap | din  | dout
  -- false  | XX        | ABCD | ABCD
  -- true   | 00        | ABCD | ABCD
  -- true   | 01        | ABCD | BADC
  -- true   | 10        | ABCD | CDAB
  -- true   | 11        | ABCD | DCBA
  -----------------------------------------------------------------------------
  function f_byte_swap (
    constant enable    : boolean;
    signal   din       : std_logic_vector(31 downto 0);
    signal   byte_swap : std_logic_vector(1 downto 0))
    return std_logic_vector is
    variable dout : std_logic_vector(31 downto 0) := din;
  begin
    if (enable = true) then
      case byte_swap is
        when "00" =>
          dout := din;
        when "01" =>
          dout := din(23 downto 16)
                  & din(31 downto 24)
                  & din(7 downto 0)
                  & din(15 downto 8);
        when "10" =>
          dout := din(15 downto 0)
                  & din(31 downto 16);
        when "11" =>
          dout := din(7 downto 0)
                  & din(15 downto 8)
                  & din(23 downto 16)
                  & din(31 downto 24);
        when others =>
          dout := din;
      end case;
    else
      dout := din;
    end if;
    return dout;
  end function f_byte_swap;
  
  -----------------------------------------------------------------------------
  -- Byte swap function
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
  function f_byte_swap_64 (
    constant enable    : boolean;
    signal   din       : std_logic_vector(63 downto 0);
    signal   byte_swap : std_logic_vector(2 downto 0))
    return std_logic_vector is
    variable dout : std_logic_vector(63 downto 0) := din;
  begin
    if (enable = true) then
      if byte_swap(2) = '0' then
        dout := f_byte_swap(true, din(63 downto 32), byte_swap(1 downto 0)) & f_byte_swap(true, din(31 downto 0), byte_swap(1 downto 0));
      else
        dout := f_byte_swap(true, din(31 downto 0), byte_swap(1 downto 0)) & f_byte_swap(true, din(63 downto 32), byte_swap(1 downto 0));
      end if;
      
    else
      dout := din;
    end if;
    return dout;
  end function f_byte_swap_64;



  -----------------------------------------------------------------------------
  -- Returns log of 2 of a natural number
  -----------------------------------------------------------------------------
  function log2_ceil(N : natural) return positive is
  begin
    if N <= 2 then
      return 1;
    elsif N mod 2 = 0 then
      return 1 + log2_ceil(N/2);
    else
      return 1 + log2_ceil((N+1)/2);
    end if;
  end;

end wshexp_core_pkg;
