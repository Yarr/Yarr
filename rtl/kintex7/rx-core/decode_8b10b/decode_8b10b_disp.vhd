---------------------------------------------------------------------------
--
--  Module      : decode_8b10b_disp.vhd
--
--  Version     : 1.1
--
--  Last Update : 2008-10-31
--
--  Project     : 8b/10b Decoder Reference Design
--
--  Description : Block memory-based Decoder disparity logic
--
--  Company     : Xilinx, Inc.
--
--  DISCLAIMER OF LIABILITY
--
--                This file contains proprietary and confidential information of
--                Xilinx, Inc. ("Xilinx"), that is distributed under a license
--                from Xilinx, and may be used, copied and/or disclosed only
--                pursuant to the terms of a valid license agreement with Xilinx.
--
--                XILINX IS PROVIDING THIS DESIGN, CODE, OR INFORMATION
--                ("MATERIALS") "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER
--                EXPRESSED, IMPLIED, OR STATUTORY, INCLUDING WITHOUT
--                LIMITATION, ANY WARRANTY WITH RESPECT TO NONINFRINGEMENT,
--                MERCHANTABILITY OR FITNESS FOR ANY PARTICULAR PURPOSE. Xilinx
--                does not warrant that functions included in the Materials will
--                meet the requirements of Licensee, or that the operation of the
--                Materials will be uninterrupted or error-free, or that defects
--                in the Materials will be corrected.  Furthermore, Xilinx does
--                not warrant or make any representations regarding use, or the
--                results of the use, of the Materials in terms of correctness,
--                accuracy, reliability or otherwise.
--
--                Xilinx products are not designed or intended to be fail-safe,
--                or for use in any application requiring fail-safe performance,
--                such as life-support or safety devices or systems, Class III
--                medical devices, nuclear facilities, applications related to
--                the deployment of airbags, or any other applications that could
--                lead to death, personal injury or severe property or
--                environmental damage (individually and collectively, "critical
--                applications").  Customer assumes the sole risk and liability
--                of any use of Xilinx products in critical applications,
--                subject only to applicable laws and regulations governing
--                limitations on product liability.
--
--                Copyright 2000, 2001, 2002, 2003, 2004, 2005, 2008 Xilinx, Inc.
--                All rights reserved.
--
--                This disclaimer and copyright notice must be retained as part
--                of this file at all times.
--
-------------------------------------------------------------------------------
--
--  History
--
--  Date        Version   Description
--
--  10/31/2008  1.1       Initial release
--
-------------------------------------------------------------------------------

LIBRARY IEEE;
USE IEEE.std_logic_1164.ALL;

LIBRARY decode_8b10b;
USE decode_8b10b.decode_8b10b_pkg.ALL;

------------------------------------------------------------------------------
--Entity Declaration
------------------------------------------------------------------------------
ENTITY decode_8b10b_disp IS
  GENERIC(
    C_SINIT_DOUT     : STRING  := "00000000";
    C_SINIT_RUN_DISP : INTEGER := 0;
    C_HAS_DISP_IN    : INTEGER := 0;
    C_HAS_DISP_ERR   : INTEGER := 0;
    C_HAS_RUN_DISP   : INTEGER := 0;
    C_HAS_SYM_DISP   : INTEGER := 0
    );
  PORT(
    CE               : IN  STD_LOGIC                    := '0';
    CLK              : IN  STD_LOGIC                    := '0';
    SINIT            : IN  STD_LOGIC                    := '0';
    SYM_DISP         : IN  STD_LOGIC_VECTOR(3 DOWNTO 0) := (OTHERS => '0');
    DISP_IN          : IN  STD_LOGIC                    := '0';
    RUN_DISP         : OUT STD_LOGIC                    := '0';
    DISP_ERR         : OUT STD_LOGIC                    := '0';
    USER_SYM_DISP    : OUT STD_LOGIC_VECTOR(1 DOWNTO 0) := (OTHERS => '0')
    );
END decode_8b10b_disp;


------------------------------------------------------------------------------
-- Architecture
------------------------------------------------------------------------------
ARCHITECTURE xilinx OF decode_8b10b_disp IS

----------------------------------------------------------------------------
-- Signal Declarations
----------------------------------------------------------------------------
  SIGNAL run_disp_q : STD_LOGIC := '0';
  SIGNAL run_disp_d : STD_LOGIC := '0';
  SIGNAL disp_in_q  : STD_LOGIC := '0';
  SIGNAL disp_err_i : STD_LOGIC := '0';

-------------------------------------------------------------------------------
-- Begin Architecture
-------------------------------------------------------------------------------
BEGIN

  gmndi : IF (C_HAS_DISP_IN/=1 AND (C_HAS_RUN_DISP = 1 OR
                                    C_HAS_SYM_DISP = 1 OR
                                    C_HAS_DISP_ERR = 1)) GENERATE
    -- store the current running disparity in run_disp_q as a mux selector for
    -- the next code's run_disp and disp_err
    PROCESS (CLK)
    BEGIN
      IF (CLK'event AND CLK='1') THEN
        IF (CE = '1') THEN
          IF (SINIT = '1') THEN
            run_disp_q <= bint_2_sl(C_SINIT_RUN_DISP) AFTER TFF;
          ELSE
            run_disp_q <= run_disp_d AFTER TFF;
          END IF;
        END IF;
      END IF;
    END PROCESS;

    -- mux the sym_disp bus and decode it into disp_err and run_disp
    gde1 : IF (C_HAS_DISP_ERR = 1 OR C_HAS_SYM_DISP = 1) GENERATE
      PROCESS (run_disp_q, SYM_DISP)
      BEGIN
        IF (run_disp_q = '1') THEN
          disp_err_i <= SYM_DISP(3);
        ELSE
          disp_err_i <= SYM_DISP(1);
        END IF;
      END PROCESS;
    END GENERATE gde1;

    grd1 : IF (C_HAS_RUN_DISP = 1 OR C_HAS_SYM_DISP = 1 OR
               C_HAS_DISP_ERR = 1) GENERATE
      PROCESS (run_disp_q, SYM_DISP)
      BEGIN
        IF (run_disp_q = '1') THEN
          run_disp_d <= SYM_DISP(2);
        ELSE
          run_disp_d <= SYM_DISP(0);
        END IF;
      END PROCESS;
    END GENERATE grd1;
  END GENERATE gmndi;

  gmdi:  IF (C_HAS_DISP_IN = 1 AND (C_HAS_RUN_DISP = 1 OR
                                    C_HAS_SYM_DISP = 1 OR
                                    C_HAS_DISP_ERR = 1)) GENERATE
    -- use the current disp_in as a mux selector for the next code's run_disp
    -- and disp_err
    PROCESS (CLK)
    BEGIN
      IF (CLK'event AND CLK='1') THEN
        IF (CE = '1') THEN
          IF (SINIT = '1') THEN
            disp_in_q <= bint_2_sl(C_SINIT_RUN_DISP) AFTER TFF;
          ELSE
            disp_in_q <= DISP_IN AFTER TFF;
          END IF;
        END IF;
      END IF;
    END PROCESS;

    -- mux the sym_disp bus and decode it into disp_err and run_disp
    gde2 : IF (C_HAS_DISP_ERR = 1 OR C_HAS_SYM_DISP = 1) GENERATE
      PROCESS (disp_in_q, SYM_DISP)
      BEGIN
        IF (disp_in_q = '1') THEN
          disp_err_i <= SYM_DISP(3);
        ELSE
          disp_err_i <= SYM_DISP(1);
        END IF;
      END PROCESS;
    END GENERATE gde2;

    grd2 : IF (C_HAS_RUN_DISP = 1 OR C_HAS_SYM_DISP = 1) GENERATE
      PROCESS (disp_in_q, SYM_DISP)
      BEGIN
        IF (disp_in_q = '1') THEN
          run_disp_d <= SYM_DISP(2);
        ELSE
          run_disp_d <= SYM_DISP(0);
        END IF;
      END PROCESS;
    END GENERATE grd2;
  END GENERATE gmdi;

-- map internal signals to outputs
  DISP_ERR         <= disp_err_i;
  RUN_DISP         <= run_disp_d;
  USER_SYM_DISP(1) <= disp_err_i;
  USER_SYM_DISP(0) <= run_disp_d;

END xilinx;




