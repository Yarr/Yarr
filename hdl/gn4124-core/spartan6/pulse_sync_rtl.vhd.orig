--=============================================================================
-- @file pulse_sync_rtl.vhd
--=============================================================================
--! Standard library
library IEEE;
--! Standard packages
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;
--! Specific packages
-------------------------------------------------------------------------------
-- --
-- CERN, BE-CO-HT, Synchronize a pulse between two clock domains
-- --
-------------------------------------------------------------------------------
--
-- Unit name: Pulse synchronizer (pulse_sync_rtl)
--
--! @brief Synchronize a pulse between two clock domains
--!
--
--! @author Matthieu Cattin (matthieu dot cattin at cern dot ch)
--
--! @date 17\03\2009
--
--! @version v.0.1
--
--! @details
--!
--! <b>Dependencies:</b>\n
--! None
--!
--! <b>References:</b>\n
--!
--!
--! <b>Modified by:</b>\n
--! Author:
-------------------------------------------------------------------------------
--! \n\n<b>Last changes:</b>\n
--! 19.06.2009    mcattin     add an extra FF in p_pulse_sync process
--! 23.10.2009    mcattin     modify it to a well known pulse synchronizer
-------------------------------------------------------------------------------
--! @todo
--
-------------------------------------------------------------------------------

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


--=============================================================================
--! Entity declaration for Pulse synchronizer
--=============================================================================
entity pulse_synchronizer is
   port (
     clk_in_i  : in std_logic;   --! Input pulse clock domain
     clk_out_i : in std_logic;   --! Output pulse clock domain
     pulse_i   : in std_logic;   --! One clk_in_i tick input pulse
     done_o    : out std_logic;  --! Input pulse is synchronized (1 clk_in_i tick)
     pulse_o   : out std_logic   --! One clk_out_i tick output pulse
   );
end entity pulse_synchronizer;


--=============================================================================
--! Architecture declaration Pulse synchronizer
--=============================================================================
architecture rtl of pulse_synchronizer is

  signal s_input_toggle : std_logic := '0';
  signal s_input_sync   : std_logic_vector(2 downto 0);
  signal s_gotit_toggle : std_logic := '0';
  signal s_gotit_sync   : std_logic_vector(2 downto 0);
  signal s_output_pulse : std_logic;

--=============================================================================
--! Architecture begin
--=============================================================================
begin


--*****************************************************************************
-- Begin of p_input_pulse_to_toggle
--! Process: Toggles FF output on every input pulse
--*****************************************************************************
p_input_pulse_to_toggle : process(clk_in_i)
begin
  if rising_edge(clk_in_i) then
    if pulse_i = '1' then
         s_input_toggle <= not(s_input_toggle);
    end if;
  end if;
end process p_input_pulse_to_toggle;


--*****************************************************************************
-- Begin of p_input_sync
--! Process: Synchronizes input toggle to output clock domain
--*****************************************************************************
p_input_sync: process(clk_out_i)
begin
   if rising_edge(clk_out_i) then
     s_input_sync(0) <= s_input_toggle;
     s_input_sync(1) <= s_input_sync(0);
     s_input_sync(2) <= s_input_sync(1);
   end if;
end process p_input_sync;

-- generates 1 tick pulse when s_input_toggle changes
s_output_pulse <= s_input_sync(1) xor s_input_sync(2);

-- assign pulse output port
pulse_o <= s_output_pulse;

--*****************************************************************************
-- Begin of p_output_pulse_to_toggle
--! Process: Toggles FF output on every output pulse
--*****************************************************************************
p_output_pulse_to_toggle : process(clk_out_i)
begin
  if rising_edge(clk_out_i) then
    if s_output_pulse = '1' then
         s_gotit_toggle <= not(s_gotit_toggle);
    end if;
  end if;
end process p_output_pulse_to_toggle;


--*****************************************************************************
-- Begin of p_gotit_sync
--! Process: Synchronizes gotit toggle to input clock domain
--*****************************************************************************
p_gotit_sync: process(clk_in_i)
begin
   if rising_edge(clk_in_i) then
     s_gotit_sync(0) <= s_gotit_toggle;
     s_gotit_sync(1) <= s_gotit_sync(0);
     s_gotit_sync(2) <= s_gotit_sync(1);
   end if;
end process p_gotit_sync;

-- generates 1 tick pulse when s_gotit_toggle changes
done_o <= s_gotit_sync(1) xor s_gotit_sync(2);


end architecture rtl;
--=============================================================================
--! Architecture end
--=============================================================================
