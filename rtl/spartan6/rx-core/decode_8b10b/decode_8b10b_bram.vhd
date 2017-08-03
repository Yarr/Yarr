---------------------------------------------------------------------------
--
--  Module      : decode_8b10b_bram.vhd
--
--  Version     : 1.1
--
--  Last Update : 2008-10-31
--
--  Project     : 8b/10b Decoder Reference Design
--
--  Description : Block memory-based Decoder for decoding 8b/10b encoded symbols
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
USE IEEE.std_logic_arith.ALL;
USE IEEE.std_logic_unsigned.ALL;
USE STD.textio.ALL;                 -- required to initialize bram from .mif


LIBRARY decode_8b10b;
USE decode_8b10b.decode_8b10b_pkg.ALL;

-----------------------------------------------------------------------------
-- Entity Declaration
-----------------------------------------------------------------------------
ENTITY decode_8b10b_bram IS
  GENERIC (
    C_ELABORATION_DIR  : STRING  := "./../../src/";
    C_HAS_BPORTS       : INTEGER := 0;
    C_HAS_DISP_IN      : INTEGER := 0;
    C_HAS_DISP_IN_B    : INTEGER := 0;
    C_HAS_DISP_ERR     : INTEGER := 0;
    C_HAS_DISP_ERR_B   : INTEGER := 0;
    C_HAS_RUN_DISP     : INTEGER := 0;
    C_HAS_RUN_DISP_B   : INTEGER := 0;
    C_HAS_SYM_DISP     : INTEGER := 0;
    C_HAS_SYM_DISP_B   : INTEGER := 0;
    C_HAS_ND           : INTEGER := 0;
    C_HAS_ND_B         : INTEGER := 0;
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
    DIN_B      : IN  STD_LOGIC_VECTOR(9 DOWNTO 0) := "0000000000";
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

END decode_8b10b_bram;


-----------------------------------------------------------------------------
-- Architecture
-----------------------------------------------------------------------------
ARCHITECTURE xilinx OF decode_8b10b_bram IS

-----------------------------------------------------------------------------
-- .MIF file support
-----------------------------------------------------------------------------
  -- Specify relative path for .mif file
  CONSTANT mif_file_name : STRING := "dec.mif";

  -- Initialize inferred ROM from mif file
  TYPE RomType IS ARRAY(0 TO 1023) OF BIT_VECTOR(13 DOWNTO 0);
  IMPURE FUNCTION InitRomFromFile (RomFileName : IN STRING) RETURN RomType IS
    FILE RomFile : TEXT OPEN READ_MODE IS RomFileName;
    VARIABLE RomFileLine : LINE;
    VARIABLE ROM : RomType;
  BEGIN
    FOR I IN RomType'range LOOP
      READLINE (RomFile, RomFileLine);
      READ (RomFileLine, ROM(I));
    END LOOP;
    RETURN ROM;
  END FUNCTION;
  SIGNAL ROM : RomType := InitRomFromFile(mif_file_name);

-----------------------------------------------------------------------------
-- Constant initialization values for internal signals ROM_data(_b)
-----------------------------------------------------------------------------
  CONSTANT INIT_DATA   : STRING :=
    concat_sinit(C_SINIT_RUN_DISP,C_SINIT_KOUT, C_SINIT_DOUT);
  CONSTANT INIT_DATA_B : STRING :=
    concat_sinit(C_SINIT_RUN_DISP_B,C_SINIT_KOUT_B, C_SINIT_DOUT_B);

-----------------------------------------------------------------------------
-- Signal Declarations
-----------------------------------------------------------------------------
  SIGNAL dout_i       : STD_LOGIC_VECTOR(7 DOWNTO 0) :=
                        str_to_slv(C_SINIT_DOUT,8);
  SIGNAL kout_i       : STD_LOGIC                    :=
                        bint_2_sl(C_SINIT_KOUT);
  SIGNAL dout_b_i     : STD_LOGIC_VECTOR(7 DOWNTO 0) :=
                        str_to_slv(C_SINIT_DOUT_B,8);
  SIGNAL kout_b_i     : STD_LOGIC                    :=
                        bint_2_sl(C_SINIT_KOUT_B);
  SIGNAL run_disp_i   : STD_LOGIC                    :=
                        bint_2_sl(C_SINIT_RUN_DISP);
  SIGNAL run_disp_b_i : STD_LOGIC                    :=
                        bint_2_sl(C_SINIT_RUN_DISP_B);
  SIGNAL sym_disp_i   : STD_LOGIC_VECTOR(1 DOWNTO 0) :=
                        conv_std_logic_vector(C_SINIT_RUN_DISP,2);
  SIGNAL sym_disp_b_i : STD_LOGIC_VECTOR(1 DOWNTO 0) :=
                        conv_std_logic_vector(C_SINIT_RUN_DISP_B,2);

--Internal signals tied to the 14x1k block memory----------------------------
  SIGNAL ROM_address : STD_LOGIC_VECTOR(9 DOWNTO 0)       := (OTHERS => '0');
  SIGNAL ROM_data    : STD_LOGIC_VECTOR(13 DOWNTO 0)      :=
    str_to_slv(INIT_DATA, 14);

-----------------------------------------------------------------------------
-- BEGIN ARCHITECTURE
-----------------------------------------------------------------------------
BEGIN

  -- Map internal signals to outputs
  DOUT       <= dout_i;
  KOUT       <= kout_i;
  DOUT_B     <= dout_b_i;
  KOUT_B     <= kout_b_i;
  RUN_DISP   <= run_disp_i;
  RUN_DISP_B <= run_disp_b_i;
  SYM_DISP   <= sym_disp_i;
  SYM_DISP_B <= sym_disp_b_i;

-----------------------------------------------------------------------------
-- Decoder A
-----------------------------------------------------------------------------

  ROM_address <= DIN;
  PROCESS (CLK)
  BEGIN
    IF (CLK'event AND CLK = '1') THEN
      IF (CE = '1') THEN
        IF (SINIT = '1') THEN
          ROM_data <= str_to_slv(INIT_DATA, 14) AFTER TFF;
        ELSE
          ROM_data <= to_stdlogicvector(ROM(conv_integer(ROM_address))) AFTER TFF;
        END IF;
      END IF;
    END IF;
  END PROCESS;
  -- Map ROM data into dout, kout, and code_err outputs
  dout_i     <= ROM_data(7 DOWNTO 0);
  kout_i     <= ROM_data(8);
  CODE_ERR   <= ROM_data(9);


  -----------------------------------------------------------------------------
  -- Instantiate disparity logic block for Decoder A
  -----------------------------------------------------------------------------
  dla : ENTITY decode_8b10b.decode_8b10b_disp
    GENERIC MAP(
      C_SINIT_DOUT     => C_SINIT_DOUT,
      C_SINIT_RUN_DISP => C_SINIT_RUN_DISP,
      C_HAS_DISP_IN    => C_HAS_DISP_IN,
      C_HAS_DISP_ERR   => C_HAS_DISP_ERR,
      C_HAS_RUN_DISP   => C_HAS_RUN_DISP,
      C_HAS_SYM_DISP   => C_HAS_SYM_DISP
      )
    PORT MAP(
      SINIT            => SINIT,
      CE               => CE,
      CLK              => CLK,
      SYM_DISP         => ROM_data(13 DOWNTO 10),
      DISP_IN          => DISP_IN,
      RUN_DISP         => run_disp_i,
      DISP_ERR         => DISP_ERR,
      USER_SYM_DISP    => sym_disp_i
      );

  -- create ND output
  gndr : IF (C_HAS_ND = 1) GENERATE
    PROCESS (CLK)
    BEGIN
      IF (CLK'event AND CLK = '1') THEN
        IF ((SINIT = '1')  AND (CE = '1')) THEN
          ND <= '0' AFTER TFF;
        ELSE
          ND <= CE AFTER TFF;
        END IF;
      END IF;
    END PROCESS;
  END GENERATE gndr;


-------------------------------------------------------------------------------
-- Generate Decoder B
-------------------------------------------------------------------------------
  gdp : IF (C_HAS_BPORTS=1) GENERATE
    --Internal signals tied to the 14x1k block memory (B)----------------------
    SIGNAL ROM_address_b : STD_LOGIC_VECTOR(9 DOWNTO 0)     := (OTHERS => '0');
    SIGNAL ROM_data_b    : STD_LOGIC_VECTOR(13 DOWNTO 0)    :=
      str_to_slv(INIT_DATA_B, 14);

  BEGIN
    ROM_address_b <= DIN_B;
    PROCESS (CLK_B)
    BEGIN
      IF (CLK_B'event AND CLK_B = '1') THEN
        IF (CE_B = '1') THEN
          IF (SINIT_B = '1') THEN
            ROM_data_b <= str_to_slv(INIT_DATA_B, 14) AFTER TFF;
          ELSE
            ROM_data_b <= to_stdlogicvector(ROM(conv_integer(ROM_address_b)))
                          AFTER TFF;
          END IF;
        END IF;
      END IF;
    END PROCESS;
    -- Map ROM_data_b into dout_b, kout_b, and code_err_b outputs
    dout_b_i   <= ROM_data_b(7 DOWNTO 0);
    kout_b_i   <= ROM_data_b(8);
    CODE_ERR_B <= ROM_data_b(9);

    -----------------------------------------------------------------------------
    -- Instantiate disparity logic block for Decoder B
    -----------------------------------------------------------------------------
    dlb : ENTITY decode_8b10b.decode_8b10b_disp
      GENERIC MAP(
        C_SINIT_DOUT     => C_SINIT_DOUT_B,
        C_SINIT_RUN_DISP => C_SINIT_RUN_DISP_B,
        C_HAS_DISP_IN    => C_HAS_DISP_IN_B,
        C_HAS_DISP_ERR   => C_HAS_DISP_ERR_B,
        C_HAS_RUN_DISP   => C_HAS_RUN_DISP_B,
        C_HAS_SYM_DISP   => C_HAS_SYM_DISP_B
        )
      PORT MAP(
        SINIT            => SINIT_B,
        CE               => CE_B,
        CLK              => CLK_B,
        SYM_DISP         => ROM_data_b(13 DOWNTO 10),
        DISP_IN          => DISP_IN_B,
        RUN_DISP         => run_disp_b_i,
        DISP_ERR         => DISP_ERR_B,
        USER_SYM_DISP    => sym_disp_b_i
        );

    -- create ND_B output
    gndbr : IF (C_HAS_ND_B = 1) GENERATE
      PROCESS (CLK_B)
      BEGIN
        IF (CLK_B'event AND CLK_B = '1') THEN
          IF ((SINIT_B = '1')  AND (CE_B = '1')) THEN
            ND_B <= '0' AFTER TFF;
          ELSE
            ND_B <= CE_B AFTER TFF;
          END IF;
        END IF;
      END PROCESS;
    END GENERATE gndbr;

  END GENERATE gdp;

END xilinx;



