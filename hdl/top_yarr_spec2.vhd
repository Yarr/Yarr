--------------------------------------------
-- Project: YARR
-- Author: Timon Heim (timon.heim@cern.ch)
-- Description: Top module for YARR on SPEC
-- Dependencies: -
--------------------------------------------

library IEEE;
use IEEE.STD_LOGIC_1164.all;
use IEEE.NUMERIC_STD.all;
use work.gn4124_core_pkg.all;

library UNISIM;
use UNISIM.vcomponents.all;

entity top_yarr is
    port (
        ----------------------------------
        -- Clocks
        ----------------------------------
        CLK20_VCXO_I        : IN STD_LOGIC;

        L_CLKP              : IN STD_LOGIC;
        L_CLKN              : IN STD_LOGIC;

        CLK_125M_PLLREF_P_I : IN STD_LOGIC;
        CLK_125M_PLLREF_N_I : IN STD_LOGIC;

        ----------------------------------
        -- ##### Reset
        ----------------------------------
        L_RST_N             : IN STD_LOGIC;

        ----------------------------------
        -- #### GPIOs used for IRQ
        ----------------------------------
        GPIO                : OUT STD_LOGIC_VECTOR(1 downto 0);

        ----------------------------------
        -- PCIe to Local [Inbound Data] - RX
        ----------------------------------
        P2L_RDY             : OUT std_logic;                      -- Rx Buffer Full Flag
        P2L_CLKn            : IN  std_logic;                      -- Receiver Source Synchronous Clock-
        P2L_CLKp            : IN  std_logic;                      -- Receiver Source Synchronous Clock+
        P2L_DATA            : IN  std_logic_vector(15 downto 0);  -- Parallel receive data
        P2L_DFRAME          : IN  std_logic;                      -- Receive Frame
        P2L_VALID           : IN  std_logic;                      -- Receive Data Valid
    );
end top_yarr;

architecture rtl of top_yarr is
begin
end rtl;

