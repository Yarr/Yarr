------------------------------------------------------------------------------
-- Copyright (c) 2009 Xilinx, Inc.
-- This design is confidential and proprietary of Xilinx, All Rights Reserved.
------------------------------------------------------------------------------
--   ____  ____
--  /   /\/   /
-- /___/  \  /   Vendor: Xilinx
-- \   \   \/    Version: 1.0
--  \   \        Filename: serdes_n_to_1_s2_diff.vhd
--  /   /        Date Last Modified:  November 5 2009
-- /___/   /\    Date Created: August 1 2008
-- \   \  /  \
--  \___\/\___\
-- 
--Device:       Spartan 6
--Purpose:      D-bit generic n:1 transmitter module
--              Takes in n bits of data and serialises this to 1 bit
--              data is transmitted LSB first
--              Parallel input word
--              DS, DS-1 ..... 1, 0
--              Serial output words
--              Line0     : 0,   ...... DS-(S+1)
--              Line1     : 1,   ...... DS-(S+2)
--              Line(D-1) : .           .
--              Line0(D)  : D-1, ...... DS
--              Data inversion can be accomplished via the TX_SWAP_MASK 
--              parameter if required
--
--Reference:
--    
--Revision History:
--    Rev 1.0 - First created (nicks)
------------------------------------------------------------------------------
--
--  Disclaimer: 
--
--              This disclaimer is not a license and does not grant any rights to the materials 
--              distributed herewith. Except as otherwise provided in a valid license issued to you 
--              by Xilinx, and to the maximum extent permitted by applicable law: 
--              (1) THESE MATERIALS ARE MADE AVAILABLE "AS IS" AND WITH ALL FAULTS, 
--              AND XILINX HEREBY DISCLAIMS ALL WARRANTIES AND CONDITIONS, EXPRESS, IMPLIED, OR STATUTORY, 
--              INCLUDING BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, NON-INFRINGEMENT, OR 
--              FITNESS FOR ANY PARTICULAR PURPOSE; and (2) Xilinx shall not be liable (whether in contract 
--              or tort, including negligence, or under any other theory of liability) for any loss or damage 
--              of any kind or nature related to, arising under or in connection with these materials, 
--              including for any direct, or any indirect, special, incidental, or consequential loss 
--              or damage (including loss of data, profits, goodwill, or any type of loss or damage suffered 
--              as a result of any action brought by a third party) even if such damage or loss was 
--              reasonably foreseeable or Xilinx had been advised of the possibility of the same.
--
--  Critical Applications:
--
--              Xilinx products are not designed or intended to be fail-safe, or for use in any application 
--              requiring fail-safe performance, such as life-support or safety devices or systems, 
--              Class III medical devices, nuclear facilities, applications related to the deployment of airbags,
--              or any other applications that could lead to death, personal injury, or severe property or 
--              environmental damage (individually and collectively, "Critical Applications"). Customer assumes 
--              the sole risk and liability of any use of Xilinx products in Critical Applications, subject only 
--              to applicable laws and regulations governing limitations on product liability.
--
--  THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS PART OF THIS FILE AT ALL TIMES.
--
------------------------------------------------------------------------------

library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.std_logic_unsigned.all;

library unisim;
use unisim.vcomponents.all;

entity serdes_n_to_1_s2_diff is
  generic (
    S : integer := 2;                                         -- Parameter to set the serdes factor 1..8
    D : integer := 16) ;                                      -- Set the number of inputs and outputs
  port (
    txioclk        : in  std_logic;                           -- IO Clock network
    txserdesstrobe : in  std_logic;                           -- Parallel data capture strobe
    reset          : in  std_logic;                           -- Reset
    gclk           : in  std_logic;                           -- Global clock
    datain         : in  std_logic_vector((D*S)-1 downto 0);  -- Data for output
    dataout_p      : out std_logic_vector(D-1 downto 0);      -- output
    dataout_n      : out std_logic_vector(D-1 downto 0)) ;    -- output
end serdes_n_to_1_s2_diff;

architecture arch_serdes_n_to_1_s2_diff of serdes_n_to_1_s2_diff is

  signal cascade_di  : std_logic_vector(D-1 downto 0);
  signal cascade_do  : std_logic_vector(D-1 downto 0);
  signal cascade_ti  : std_logic_vector(D-1 downto 0);
  signal cascade_to  : std_logic_vector(D-1 downto 0);
  signal mdataina    : std_logic_vector(D*8 downto 0);
  signal mdatainb    : std_logic_vector(D*4 downto 0);
  signal tx_data_out : std_logic_vector(D-1 downto 0);

  constant TX_SWAP_MASK : std_logic_vector(D-1 downto 0) := (others => '0');  -- pinswap mask for input bits (0 = no swap (default), 1 = swap). Allows inputs to be connected the wrong way round to ease PCB routing.

begin

  loop0 : for i in 0 to (D - 1) generate

    io_clk_out : obufds port map (
      O  => dataout_p(i),
      OB => dataout_n(i),
      I  => tx_data_out(i));

    loop1 : if (S > 4) generate         -- Two oserdes are needed

      loop2 : for j in 0 to (S - 1) generate

-- re-arrange data bits for transmission and invert lines as given by the mask
-- NOTE If pin inversion is required (non-zero SWAP MASK) then inverters will occur in fabric, as there are no inverters in the ISERDES2
-- This can be avoided by doing the inversion (if necessary) in the user logic

        mdataina((8*i)+j) <= datain((i)+(D*j)) xor TX_SWAP_MASK(i);
      end generate;

      oserdes_m : OSERDES2 generic map (
        DATA_WIDTH   => S,              -- SERDES word width.  This should match the setting is BUFPLL
        DATA_RATE_OQ => "SDR",          -- <SDR>, DDR
        DATA_RATE_OT => "SDR",          -- <SDR>, DDR
        SERDES_MODE  => "MASTER",       -- <DEFAULT>, MASTER, SLAVE
        OUTPUT_MODE  => "DIFFERENTIAL")
        port map (
          OQ        => tx_data_out(i),
          OCE       => '1',
          CLK0      => txioclk,
          CLK1      => '0',
          IOCE      => txserdesstrobe,
          RST       => reset,
          CLKDIV    => gclk,
          D4        => mdataina((8*i)+7),
          D3        => mdataina((8*i)+6),
          D2        => mdataina((8*i)+5),
          D1        => mdataina((8*i)+4),
          TQ        => open,
          T1        => '0',
          T2        => '0',
          T3        => '0',
          T4        => '0',
          TRAIN     => '0',
          TCE       => '1',
          SHIFTIN1  => '1',             -- Dummy input in Master
          SHIFTIN2  => '1',             -- Dummy input in Master
          SHIFTIN3  => cascade_do(i),   -- Cascade output D data from slave
          SHIFTIN4  => cascade_to(i),   -- Cascade output T data from slave
          SHIFTOUT1 => cascade_di(i),   -- Cascade input D data to slave
          SHIFTOUT2 => cascade_ti(i),   -- Cascade input T data to slave
          SHIFTOUT3 => open,            -- Dummy output in Master
          SHIFTOUT4 => open) ;          -- Dummy output in Master

      oserdes_s : OSERDES2 generic map(
        DATA_WIDTH   => S,               -- SERDES word width.  This should match the setting is BUFPLL
        DATA_RATE_OQ => "SDR",           -- <SDR>, DDR
        DATA_RATE_OT => "SDR",           -- <SDR>, DDR
        SERDES_MODE  => "SLAVE",         -- <DEFAULT>, MASTER, SLAVE
        OUTPUT_MODE  => "DIFFERENTIAL")
        port map (
          OQ        => open,
          OCE       => '1',
          CLK0      => txioclk,
          CLK1      => '0',
          IOCE      => txserdesstrobe,
          RST       => reset,
          CLKDIV    => gclk,
          D4        => mdataina((8*i)+3),
          D3        => mdataina((8*i)+2),
          D2        => mdataina((8*i)+1),
          D1        => mdataina((8*i)+0),
          TQ        => open,
          T1        => '0',
          T2        => '0',
          T3        => '0',
          T4        => '0',
          TRAIN     => '0',
          TCE       => '1',
          SHIFTIN1  => cascade_di(i),    -- Cascade input D from Master
          SHIFTIN2  => cascade_ti(i),    -- Cascade input T from Master
          SHIFTIN3  => '1',              -- Dummy input in Slave
          SHIFTIN4  => '1',              -- Dummy input in Slave
          SHIFTOUT1 => open,             -- Dummy output in Slave
          SHIFTOUT2 => open,             -- Dummy output in Slave
          SHIFTOUT3 => cascade_do(i),    -- Cascade output D data to Master
          SHIFTOUT4 => cascade_to(i)) ;  -- Cascade output T data to Master

    end generate;

    loop3 : if (S < 5) generate         -- Only one oserdes needed

      loop4 : for j in 0 to (S - 1) generate

-- re-arrange data bits for transmission and invert lines as given by the mask
-- NOTE If pin inversion is required (non-zero SWAP MASK) then inverters will occur in fabric, as there are no inverters in the ISERDES2
-- This can be avoided by doing the inversion (if necessary) in the user logic

        mdatainb((4*i)+j) <= datain((i)+(D*j)) xor TX_SWAP_MASK(i);
      end generate;

      oserdes_m : OSERDES2 generic map (
        DATA_WIDTH   => S,              -- SERDES word width.  This should match the setting is BUFPLL
        DATA_RATE_OQ => "SDR",          -- <SDR>, DDR
        DATA_RATE_OT => "SDR")          -- <SDR>, DDR
--      SERDES_MODE             => "MASTER",            -- <DEFAULT>, MASTER, SLAVE
--      OUTPUT_MODE             => "DIFFERENTIAL")
        port map (
          OQ        => tx_data_out(i),
          OCE       => '1',
          CLK0      => txioclk,
          CLK1      => '0',
          IOCE      => txserdesstrobe,
          RST       => reset,
          CLKDIV    => gclk,
          D4        => mdatainb((4*i)+3),
          D3        => mdatainb((4*i)+2),
          D2        => mdatainb((4*i)+1),
          D1        => mdatainb((4*i)+0),
          TQ        => open,
          T1        => '0',
          T2        => '0',
          T3        => '0',
          T4        => '0',
          TRAIN     => '0',
          TCE       => '1',
          SHIFTIN1  => '1',             -- No cascades needed
          SHIFTIN2  => '1',             -- No cascades needed
          SHIFTIN3  => '1',             -- No cascades needed
          SHIFTIN4  => '1',             -- No cascades needed
          SHIFTOUT1 => open,            -- No cascades needed
          SHIFTOUT2 => open,            -- No cascades needed
          SHIFTOUT3 => open,            -- No cascades needed
          SHIFTOUT4 => open) ;          -- No cascades needed

    end generate;
  end generate;
end arch_serdes_n_to_1_s2_diff;
