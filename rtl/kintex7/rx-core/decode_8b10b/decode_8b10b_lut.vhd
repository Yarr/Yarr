---------------------------------------------------------------------------
--
--  Module      : decode_8b10b_lut.vhd
--
--  Version     : 1.1
--
--  Last Update : 2008-10-31
--
--  Project     : 8b/10b Decoder Reference Design
--
--  Description : LUT-based Decoder for decoding 8b/10b encoded symbols
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

-----------------------------------------------------------------------------
-- Entity Declaration
-----------------------------------------------------------------------------
ENTITY decode_8b10b_lut IS
    GENERIC (
      C_HAS_BPORTS       : INTEGER := 0;
      C_HAS_CODE_ERR     : INTEGER := 0;
      C_HAS_CODE_ERR_B   : INTEGER := 0;
      C_HAS_DISP_ERR     : INTEGER := 0;
      C_HAS_DISP_ERR_B   : INTEGER := 0;
      C_HAS_DISP_IN      : INTEGER := 0;
      C_HAS_DISP_IN_B    : INTEGER := 0;
      C_HAS_ND           : INTEGER := 0;
      C_HAS_ND_B         : INTEGER := 0;
      C_HAS_SYM_DISP     : INTEGER := 0;
      C_HAS_SYM_DISP_B   : INTEGER := 0;
      C_HAS_RUN_DISP     : INTEGER := 0;
      C_HAS_RUN_DISP_B   : INTEGER := 0;
      C_SINIT_DOUT       : STRING  := "00000000";
      C_SINIT_DOUT_B     : STRING  := "00000000";
      C_SINIT_KOUT       : INTEGER := 0;
      C_SINIT_KOUT_B     : INTEGER := 0;
      C_SINIT_RUN_DISP   : INTEGER := 0;
      C_SINIT_RUN_DISP_B : INTEGER := 0
    );
    PORT (
      CLK        : IN  STD_LOGIC                    := '0';
      DIN        : IN  STD_LOGIC_VECTOR(9 DOWNTO 0) := (OTHERS => '0');
      DOUT       : OUT STD_LOGIC_VECTOR(7 DOWNTO 0) ;
      KOUT       : OUT STD_LOGIC                    ;

      CE         : IN  STD_LOGIC                    := '0';
      CE_B       : IN  STD_LOGIC                    := '0';
      CLK_B      : IN  STD_LOGIC                    := '0';
      DIN_B      : IN  STD_LOGIC_VECTOR(9 DOWNTO 0) := (OTHERS => '0');
      DISP_IN    : IN  STD_LOGIC                    := '0';
      DISP_IN_B  : IN  STD_LOGIC                    := '0';
      SINIT      : IN  STD_LOGIC                    := '0';
      SINIT_B    : IN  STD_LOGIC                    := '0';
      CODE_ERR   : OUT STD_LOGIC                    := '0';
      CODE_ERR_B : OUT STD_LOGIC                    := '0';
      DISP_ERR   : OUT STD_LOGIC                    := '0';
      DISP_ERR_B : OUT STD_LOGIC                    := '0';
      DOUT_B     : OUT STD_LOGIC_VECTOR(7 DOWNTO 0) ;
      KOUT_B     : OUT STD_LOGIC                    ;
      ND         : OUT STD_LOGIC                    := '0';
      ND_B       : OUT STD_LOGIC                    := '0';
      RUN_DISP   : OUT STD_LOGIC                    ;
      RUN_DISP_B : OUT STD_LOGIC                    ;
      SYM_DISP   : OUT STD_LOGIC_VECTOR(1 DOWNTO 0) ;
      SYM_DISP_B : OUT STD_LOGIC_VECTOR(1 DOWNTO 0)
    );

END decode_8b10b_lut;

-------------------------------------------------------------------------------
-- Architecture
-------------------------------------------------------------------------------
ARCHITECTURE xilinx OF decode_8b10b_lut IS

-------------------------------------------------------------------------------
-- Begin Architecture
-------------------------------------------------------------------------------
BEGIN

  ----Instantiate the first decoder (A decoder)--------------------------------
  deca : ENTITY decode_8b10b.decode_8b10b_lut_base
    GENERIC MAP (
      C_HAS_CODE_ERR   => C_HAS_CODE_ERR,
      C_HAS_DISP_ERR   => C_HAS_DISP_ERR,
      C_HAS_DISP_IN    => C_HAS_DISP_IN,
      C_HAS_ND         => C_HAS_ND,
      C_HAS_SYM_DISP   => C_HAS_SYM_DISP,
      C_HAS_RUN_DISP   => C_HAS_RUN_DISP,
      C_SINIT_DOUT     => C_SINIT_DOUT,
      C_SINIT_KOUT     => C_SINIT_KOUT,
      C_SINIT_RUN_DISP => C_SINIT_RUN_DISP
      )
    PORT MAP (
      CLK              => CLK,
      DIN              => DIN,
      DOUT             => DOUT,
      KOUT             => KOUT,

      CE               => CE,
      DISP_IN          => DISP_IN,
      SINIT            => SINIT,
      CODE_ERR         => CODE_ERR,
      DISP_ERR         => DISP_ERR,
      ND               => ND,
      RUN_DISP         => RUN_DISP,
      SYM_DISP         => SYM_DISP
      );



  gdecb : IF (C_HAS_BPORTS=1) GENERATE
  ----Instantiate second decoder (B decoder, only if bports are present)------
  decb : ENTITY decode_8b10b.decode_8b10b_lut_base
    GENERIC MAP (
      C_HAS_CODE_ERR   => C_HAS_CODE_ERR_B,
      C_HAS_DISP_ERR   => C_HAS_DISP_ERR_B,
      C_HAS_DISP_IN    => C_HAS_DISP_IN_B,
      C_HAS_ND         => C_HAS_ND_B,
      C_HAS_SYM_DISP   => C_HAS_SYM_DISP_B,
      C_HAS_RUN_DISP   => C_HAS_RUN_DISP_B,
      C_SINIT_DOUT     => C_SINIT_DOUT_B,
      C_SINIT_KOUT     => C_SINIT_KOUT_B,
      C_SINIT_RUN_DISP => C_SINIT_RUN_DISP_B
      )
    PORT MAP (
      CLK              => CLK_B,
      DIN              => DIN_B,
      DOUT             => DOUT_B,
      KOUT             => KOUT_B,

      CE               => CE_B,
      DISP_IN          => DISP_IN_B,
      SINIT            => SINIT_B,
      CODE_ERR         => CODE_ERR_B,
      DISP_ERR         => DISP_ERR_B,
      ND               => ND_B,
      RUN_DISP         => RUN_DISP_B,
      SYM_DISP         => SYM_DISP_B
      );
  END GENERATE gdecb;

END xilinx ;


