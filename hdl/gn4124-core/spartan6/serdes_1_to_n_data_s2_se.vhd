------------------------------------------------------------------------------
-- Copyright (c) 2009 Xilinx, Inc.
-- This design is confidential and proprietary of Xilinx, All Rights Reserved.
------------------------------------------------------------------------------
--   ____  ____
--  /   /\/   /
-- /___/  \  /   Vendor: Xilinx
-- \   \   \/    Version: 1.0
--  \   \        Filename: serdes_1_to_n_data_s2_se.vhd
--  /   /        Date Last Modified:  November 5 2009
-- /___/   /\    Date Created: August 1 2008
-- \   \  /  \
--  \___\/\___\
-- 
--Device:       Spartan 6
--Purpose:      D-bit generic 1:n data receiver module with se inputs
--              Takes in 1 bit of se data and deserialises this to n bits
--              data is received LSB first
--              Serial input words
--              Line0     : 0,   ...... DS-(S+1)
--              Line1     : 1,   ...... DS-(S+2)
--              Line(D-1) : .           .
--              Line0(D)  : D-1, ...... DS
--              Parallel output word
--              DS, DS-1 ..... 1, 0
--
--              Includes state machine to control CAL and the phase detector
--              Data inversion can be accomplished via the RX_RX_SWAP_MASK 
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

entity serdes_1_to_n_data_s2_se is
  generic (
    USE_PD : boolean := false;          -- Parameter to set generation of phase detector logic
    S      : integer := 2;              -- Parameter to set the serdes factor 1..8
    D      : integer := 16) ;           -- Set the number of inputs and outputs
  port (
    use_phase_detector : in  std_logic;                             -- Set generation of phase detector logic
    datain             : in  std_logic_vector(D-1 downto 0);        -- Input from se receiver pin
    rxioclk            : in  std_logic;                             -- IO Clock network
    rxserdesstrobe     : in  std_logic;                             -- Parallel data capture strobe
    reset              : in  std_logic;                             -- Reset line
    gclk               : in  std_logic;                             -- Global clock
    bitslip            : in  std_logic;                             -- Bitslip control line
    debug_in           : in  std_logic_vector(1 downto 0);          -- input debug data
    data_out           : out std_logic_vector((D*S)-1 downto 0);    -- Output data
    debug              : out std_logic_vector((2*D)+6 downto 0)) ;  -- Debug bus, 2D+6 = 2 lines per input (from mux and ce) + 7, leave nc if debug not required
end serdes_1_to_n_data_s2_se;

architecture arch_serdes_1_to_n_data_s2_se of serdes_1_to_n_data_s2_se is

  signal ddly_m          : std_logic_vector(D-1 downto 0);  -- Master output from IODELAY1
  signal ddly_s          : std_logic_vector(D-1 downto 0);  -- Slave output from IODELAY1
  signal cascade         : std_logic_vector(D-1 downto 0);
  signal busys           : std_logic_vector(D-1 downto 0);
  signal rx_data_in      : std_logic_vector(D-1 downto 0);
  signal rx_data_in_fix  : std_logic_vector(D-1 downto 0);
  signal state           : integer range 0 to 8;
  signal busyd           : std_logic_vector(D-1 downto 0);
  signal cal_data_sint   : std_logic;
  signal ce_data_inta    : std_logic;
  signal busy_data       : std_logic_vector(D-1 downto 0);
  signal busy_data_d     : std_logic;
  signal counter         : std_logic_vector(8 downto 0);
  signal enable          : std_logic;
  signal pd_edge         : std_logic_vector(D-1 downto 0);
  signal cal_data_slave  : std_logic;
  signal cal_data_master : std_logic;
  signal valid_data      : std_logic_vector(D-1 downto 0);
  signal valid_data_d    : std_logic;
  signal rst_data        : std_logic;
  signal mdataout        : std_logic_vector((8*D)-1 downto 0);
  signal pdcounter       : std_logic_vector(4 downto 0);
  signal inc_data        : std_logic;
  signal ce_data         : std_logic_vector(D-1 downto 0);
  signal inc_data_int    : std_logic;
  signal incdec_data     : std_logic_vector(D-1 downto 0);
  signal incdec_data_d   : std_logic;
  signal flag            : std_logic;
  signal mux             : std_logic_vector(D-1 downto 0);
  signal incdec_data_or  : std_logic_vector(D downto 0);
  signal valid_data_or   : std_logic_vector(D downto 0);
  signal busy_data_or    : std_logic_vector(D downto 0);
  signal incdec_data_im  : std_logic_vector(D-1 downto 0);
  signal valid_data_im   : std_logic_vector(D-1 downto 0);
  signal all_ce          : std_logic_vector(D-1 downto 0);

  constant RX_SWAP_MASK : std_logic_vector(D-1 downto 0) := (others => '0');  -- pinswap mask for input bits (0 = no swap (default), 1 = swap). Allows inputs to be connected the wrong way round to ease PCB routing.

begin

  busy_data <= busys;
  debug     <= mux & cal_data_master & rst_data & cal_data_slave & busy_data_d & inc_data & ce_data & valid_data_d & incdec_data_d;

  cal_data_slave <= cal_data_sint;

  process (gclk, reset)
  begin
    if reset = '1' then
      state           <= 0;
      cal_data_master <= '0';
      cal_data_sint   <= '0';
      counter         <= (others => '0');
      enable          <= '0';
      counter         <= (others => '0');
      mux             <= (0      => '1', others => '0');
    elsif gclk'event and gclk = '1' then
      counter <= counter + 1;
      if counter(8) = '1' then
        counter <= "000000000";
      end if;
      if counter(5) = '1' then
        enable <= '1';
      end if;
      if state = 0 and enable = '1' then  -- Wait for all IODELAYs to be available
        cal_data_master <= '0';
        cal_data_sint   <= '0';
        rst_data        <= '0';
        if busy_data_d = '0' then
          state <= 1;
        end if;
      elsif state = 1 then                -- Issue calibrate command to both master and slave
        cal_data_master <= '1';
        cal_data_sint   <= '1';
        if busy_data_d = '1' then         -- and wait for command to be accepted
          state <= 2;
        end if;
      elsif state = 2 then                -- Now RST all master and slave IODELAYs
        cal_data_master <= '0';
        cal_data_sint   <= '0';
        if busy_data_d = '0' then
          rst_data <= '1';
          state    <= 3;
        end if;
      elsif state = 3 then                -- Wait for all IODELAYs to be available
        rst_data <= '0';
        if busy_data_d = '0' then
          state <= 4;
        end if;
      elsif state = 4 then                -- Hang around
        if counter(8) = '1' then
          state <= 5;
        end if;
      elsif state = 5 then                -- Calibrate slave only
        if busy_data_d = '0' then
          cal_data_sint <= '1';
          state         <= 6;
          if D /= 1 then
            mux <= mux(D-2 downto 0) & mux(D-1);
          end if;
        end if;
      elsif state = 6 then                -- Wait for command to be accepted
        if busy_data_d = '1' then
          cal_data_sint <= '0';
          state         <= 7;
        end if;
      elsif state = 7 then                -- Wait for all IODELAYs to be available, ie CAL command finished
        cal_data_sint <= '0';
        if busy_data_d = '0' then
          state <= 4;
        end if;
      end if;
    end if;
  end process;

  process (gclk, reset)
  begin
    if reset = '1' then
      pdcounter    <= "10000";
      ce_data_inta <= '0';
      flag         <= '0';
    elsif gclk'event and gclk = '1' then
      busy_data_d <= busy_data_or(D);
      if use_phase_detector = '1' then                             -- decide whther pd is used
        incdec_data_d <= incdec_data_or(D);
        valid_data_d  <= valid_data_or(D);
        if ce_data_inta = '1' then
          ce_data <= mux;
        else
          ce_data <= (others => '0');
        end if;
        if state = 7 then
          flag <= '0';
        elsif state /= 4 or busy_data_d = '1' then                 -- Reset filter if state machine issues a cal command or unit is busy
          pdcounter    <= "10000";
          ce_data_inta <= '0';
        elsif pdcounter = "11111" and flag = '0' then              -- Filter has reached positive max - increment the tap count
          ce_data_inta <= '1';
          inc_data_int <= '1';
          pdcounter    <= "10000";
          flag         <= '0';
        elsif pdcounter = "00000" and flag = '0' then              -- Filter has reached negative max - decrement the tap count
          ce_data_inta <= '1';
          inc_data_int <= '0';
          pdcounter    <= "10000";
          flag         <= '0';
        elsif valid_data_d = '1' then                              -- increment filter
          ce_data_inta <= '0';
          if incdec_data_d = '1' and pdcounter /= "11111" then
            pdcounter <= pdcounter + 1;
          elsif incdec_data_d = '0' and pdcounter /= "00000" then  -- decrement filter
            pdcounter <= pdcounter - 1;
          end if;
        else
          ce_data_inta <= '0';
        end if;
      else
        ce_data      <= all_ce;
        inc_data_int <= debug_in(1);
      end if;
    end if;
  end process;

  inc_data <= inc_data_int;

  incdec_data_or(0) <= '0';             -- Input Mux - Initialise generate loop OR gates
  valid_data_or(0)  <= '0';
  busy_data_or(0)   <= '0';

  loop0 : for i in 0 to (D - 1) generate

    incdec_data_im(i)   <= incdec_data(i) and mux(i);               -- Input muxes
    incdec_data_or(i+1) <= incdec_data_im(i) or incdec_data_or(i);  -- AND gates to allow just one signal through at a tome
    valid_data_im(i)    <= valid_data(i) and mux(i);                -- followed by an OR
    valid_data_or(i+1)  <= valid_data_im(i) or valid_data_or(i);    -- for the three inputs from each PD
    busy_data_or(i+1)   <= busy_data(i) or busy_data_or(i);         -- The busy signals just need an OR gate

    all_ce(i) <= debug_in(0);

    rx_data_in_fix(i) <= rx_data_in(i) xor RX_SWAP_MASK(i);  -- Invert signals as required

    iob_clk_in : IBUF port map (
      I => datain(i),
      O => rx_data_in(i));

    loop2 : if (USE_PD = true) generate  --Two oserdes are needed

      iodelay_m : IODELAY2 generic map(
        DATA_RATE          => "SDR",                  -- <SDR>, DDR
        IDELAY_VALUE       => 0,                      -- {0 ... 255}
        IDELAY2_VALUE      => 0,                      -- {0 ... 255}
        IDELAY_MODE        => "NORMAL" ,              -- NORMAL, PCI
        ODELAY_VALUE       => 0,                      -- {0 ... 255}
        IDELAY_TYPE        => "DIFF_PHASE_DETECTOR",  -- "DEFAULT", "DIFF_PHASE_DETECTOR", "FIXED", "VARIABLE_FROM_HALF_MAX", "VARIABLE_FROM_ZERO"
        COUNTER_WRAPAROUND => "WRAPAROUND",           -- <STAY_AT_LIMIT>, WRAPAROUND
        DELAY_SRC          => "IDATAIN",              -- "IO", "IDATAIN", "ODATAIN"
        SERDES_MODE        => "MASTER",               -- <NONE>, MASTER, SLAVE
        SIM_TAPDELAY_VALUE => 49)                     --
        port map (
          IDATAIN  => rx_data_in_fix(i),              -- data from primary IOB
          TOUT     => open,                           -- tri-state signal to IOB
          DOUT     => open,                           -- output data to IOB
          T        => '1',                            -- tri-state control from OLOGIC/OSERDES2               
          ODATAIN  => '0',                            -- data from OLOGIC/OSERDES2
          DATAOUT  => ddly_m(i),                      -- Output data 1 to ILOGIC/ISERDES2
          DATAOUT2 => open,                           -- Output data 2 to ILOGIC/ISERDES2
          IOCLK0   => rxioclk,                        -- High speed clock for calibration
          IOCLK1   => '0',                            -- High speed clock for calibration
          CLK      => gclk,                           -- Fabric clock (GCLK) for control signals
          CAL      => cal_data_master,                -- Calibrate control signal
          INC      => inc_data,                       -- Increment counter
          CE       => ce_data(i),                     -- Clock Enable
          RST      => rst_data,                       -- Reset delay line
          BUSY     => open) ;                         -- output signal indicating sync circuit has finished / calibration has finished 

      iodelay_s : IODELAY2 generic map(
        DATA_RATE          => "SDR",                  -- <SDR>, DDR
        IDELAY_VALUE       => 0,                      -- {0 ... 255}
        IDELAY2_VALUE      => 0,                      -- {0 ... 255}
        IDELAY_MODE        => "NORMAL",               -- NORMAL, PCI
        ODELAY_VALUE       => 0,                      -- {0 ... 255}
        IDELAY_TYPE        => "DIFF_PHASE_DETECTOR",  -- "DEFAULT", "DIFF_PHASE_DETECTOR", "FIXED", "VARIABLE_FROM_HALF_MAX", "VARIABLE_FROM_ZERO"
        COUNTER_WRAPAROUND => "WRAPAROUND" ,          -- <STAY_AT_LIMIT>, WRAPAROUND
        DELAY_SRC          => "IDATAIN" ,             -- "IO", "IDATAIN", "ODATAIN"
        SERDES_MODE        => "SLAVE",                -- <NONE>, MASTER, SLAVE
        SIM_TAPDELAY_VALUE => 49)                     --
        port map (
          IDATAIN  => rx_data_in_fix(i),              -- data from primary IOB
          TOUT     => open,                           -- tri-state signal to IOB
          DOUT     => open,                           -- output data to IOB
          T        => '1',                            -- tri-state control from OLOGIC/OSERDES2
          ODATAIN  => '0',                            -- data from OLOGIC/OSERDES2
          DATAOUT  => ddly_s(i),                      -- Output data 1 to ILOGIC/ISERDES2
          DATAOUT2 => open,                           -- Output data 2 to ILOGIC/ISERDES2
          IOCLK0   => rxioclk,                        -- High speed clock for calibration
          IOCLK1   => '0',                            -- High speed clock for calibration
          CLK      => gclk,                           -- Fabric clock (GCLK) for control signals
          CAL      => cal_data_slave,                 -- Calibrate control signal
          INC      => inc_data,                       -- Increment counter
          CE       => ce_data(i) ,                    -- Clock Enable
          RST      => rst_data,                       -- Reset delay line
          BUSY     => busys(i)) ;                     -- output signal indicating sync circuit has finished / calibration has finished

      iserdes_m : ISERDES2 generic map (
        DATA_WIDTH     => S,            -- SERDES word width.  This should match the setting is BUFPLL
        DATA_RATE      => "SDR",        -- <SDR>, DDR
        BITSLIP_ENABLE => true,         -- <FALSE>, TRUE
        SERDES_MODE    => "MASTER",     -- <DEFAULT>, MASTER, SLAVE
        INTERFACE_TYPE => "RETIMED")    -- NETWORKING, NETWORKING_PIPELINED, <RETIMED>
        port map (
          D         => ddly_m(i),
          CE0       => '1',
          CLK0      => rxioclk,
          CLK1      => '0',
          IOCE      => rxserdesstrobe,
          RST       => reset,
          CLKDIV    => gclk,
          SHIFTIN   => pd_edge(i),
          BITSLIP   => bitslip,
          FABRICOUT => open,
          Q4        => mdataout((8*i)+7),
          Q3        => mdataout((8*i)+6),
          Q2        => mdataout((8*i)+5),
          Q1        => mdataout((8*i)+4),
          DFB       => open,
          CFB0      => open,
          CFB1      => open,
          VALID     => valid_data(i),
          INCDEC    => incdec_data(i),
          SHIFTOUT  => cascade(i));

      iserdes_s : ISERDES2 generic map(
        DATA_WIDTH     => S,            -- SERDES word width.  This should match the setting is BUFPLL
        DATA_RATE      => "SDR",        -- <SDR>, DDR
        BITSLIP_ENABLE => true,         -- <FALSE>, TRUE
        SERDES_MODE    => "SLAVE",      -- <DEFAULT>, MASTER, SLAVE
        INTERFACE_TYPE => "RETIMED")    -- NETWORKING, NETWORKING_PIPELINED, <RETIMED>
        port map (
          D         => ddly_s(i),
          CE0       => '1',
          CLK0      => rxioclk,
          CLK1      => '0',
          IOCE      => rxserdesstrobe,
          RST       => reset,
          CLKDIV    => gclk,
          SHIFTIN   => cascade(i),
          BITSLIP   => bitslip,
          FABRICOUT => open,
          Q4        => mdataout((8*i)+3),
          Q3        => mdataout((8*i)+2),
          Q2        => mdataout((8*i)+1),
          Q1        => mdataout((8*i)+0),
          DFB       => open,
          CFB0      => open,
          CFB1      => open,
          VALID     => open,
          INCDEC    => open,
          SHIFTOUT  => pd_edge(i));
    end generate;

    loop3 : if (USE_PD /= true) generate  -- Only one oserdes is needed

      iodelay_m : IODELAY2 generic map(
        DATA_RATE          => "SDR",                     -- <SDR>, DDR
        IDELAY_VALUE       => 0,                         -- {0 ... 255}
        IDELAY2_VALUE      => 0,                         -- {0 ... 255}
        IDELAY_MODE        => "NORMAL" ,                 -- NORMAL, PCI
        ODELAY_VALUE       => 0,                         -- {0 ... 255}
        IDELAY_TYPE        => "VARIABLE_FROM_HALF_MAX",  -- "DEFAULT", "DIFF_PHASE_DETECTOR", "FIXED", "VARIABLE_FROM_HALF_MAX", "VARIABLE_FROM_ZERO"
        COUNTER_WRAPAROUND => "WRAPAROUND",              -- <STAY_AT_LIMIT>, WRAPAROUND
        DELAY_SRC          => "IDATAIN",                 -- "IO", "IDATAIN", "ODATAIN"
--        SERDES_MODE        => "MASTER",               -- <NONE>, MASTER, SLAVE
        SIM_TAPDELAY_VALUE => 49)                        --
        port map (
          IDATAIN  => rx_data_in_fix(i),                 -- data from primary IOB
          TOUT     => open,                              -- tri-state signal to IOB
          DOUT     => open,                              -- output data to IOB
          T        => '1',                               -- tri-state control from OLOGIC/OSERDES2               
          ODATAIN  => '0',                               -- data from OLOGIC/OSERDES2
          DATAOUT  => ddly_m(i),                         -- Output data 1 to ILOGIC/ISERDES2
          DATAOUT2 => open,                              -- Output data 2 to ILOGIC/ISERDES2
          IOCLK0   => rxioclk,                           -- High speed clock for calibration
          IOCLK1   => '0',                               -- High speed clock for calibration
          CLK      => gclk,                              -- Fabric clock (GCLK) for control signals
          CAL      => cal_data_master,                   -- Calibrate control signal
          INC      => inc_data,                          -- Increment counter
          CE       => ce_data(i),                        -- Clock Enable
          RST      => rst_data,                          -- Reset delay line
          BUSY     => open) ;                            -- output signal indicating sync circuit has finished / calibration has finished 

      iserdes_m : ISERDES2 generic map (
        DATA_WIDTH     => S,            -- SERDES word width.  This should match the setting is BUFPLL
        DATA_RATE      => "SDR",        -- <SDR>, DDR
        BITSLIP_ENABLE => true,         -- <FALSE>, TRUE
--        SERDES_MODE    => "MASTER",     -- <DEFAULT>, MASTER, SLAVE
        INTERFACE_TYPE => "RETIMED")    -- NETWORKING, NETWORKING_PIPELINED, <RETIMED>
        port map (
          D         => rx_data_in_fix(i),--ddly_m(i),
          CE0       => '1',
          CLK0      => rxioclk,
          CLK1      => '0',
          IOCE      => rxserdesstrobe,
          RST       => reset,
          CLKDIV    => gclk,
          SHIFTIN   => '0',
          BITSLIP   => bitslip,
          FABRICOUT => open,
          Q4        => mdataout((8*i)+7),
          Q3        => mdataout((8*i)+6),
          Q2        => mdataout((8*i)+5),
          Q1        => mdataout((8*i)+4),
          DFB       => open,
          CFB0      => open,
          CFB1      => open,
          VALID     => open,
          INCDEC    => open,
          SHIFTOUT  => open);
    end generate;

    loop1 : for j in 7 downto (8-S) generate
      data_out(((D*(j+S-8))+i)) <= mdataout((8*i)+j);
    end generate;
  end generate;

end arch_serdes_1_to_n_data_s2_se;
