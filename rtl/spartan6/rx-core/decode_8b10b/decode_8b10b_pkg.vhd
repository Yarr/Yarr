---------------------------------------------------------------------------
--
--  Module      : decode_8b10b_pkg.vhd
--
--  Version     : 1.1
--
--  Last Update : 2008-10-31
--
--  Project     : 8b/10b Decoder Reference Design
--
--  Description : 8b/10b Decoder package file
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
---------------------------------------------------------------------------
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

-------------------------------------------------------------------------------
-- Package Declaration
-------------------------------------------------------------------------------
PACKAGE decode_8b10b_pkg IS

-------------------------------------------------------------------------------
-- Constant Declarations
-------------------------------------------------------------------------------
  CONSTANT TFF           : TIME            := 2 ns;
  CONSTANT CONST_NEG     : STRING (1 TO 2) := "00";
  CONSTANT CONST_POS     : STRING (1 TO 2) := "01";

-------------------------------------------------------------------------------
-- Function Declarations
-------------------------------------------------------------------------------
  FUNCTION int_to_str_1bit (
    bit : INTEGER
    ) RETURN STRING;
  FUNCTION bint_2_sl (
    X : INTEGER
    ) RETURN STD_LOGIC;
  FUNCTION concat_sinit (
    integer_run_disp : INTEGER;
    integer_kout     : INTEGER;
    string_dout      : STRING
    ) RETURN STRING;
  FUNCTION str_to_slv(
    bitsin : STRING;
    nbits  : INTEGER
    ) RETURN STD_LOGIC_VECTOR;
  FUNCTION calc_init_val_rd (
    SINIT_VAL : STRING(10 DOWNTO 1)
    ) RETURN INTEGER;
  FUNCTION has_bport (
    C_HAS_BPORTS : INTEGER;
    has_aport    : INTEGER
    ) RETURN INTEGER;

END decode_8b10b_pkg;

-------------------------------------------------------------------------------
-- Package Body
-------------------------------------------------------------------------------
PACKAGE BODY decode_8b10b_pkg IS

-------------------------------------------------------------------------------
-- Convert binary integer (0 or 1) into a single-character-string ("0" or "1")
-------------------------------------------------------------------------------
  FUNCTION int_to_str_1bit(bit : INTEGER) RETURN STRING IS
  BEGIN
    IF (bit = 1) THEN
      RETURN "1";
    ELSE
      RETURN "0";
    END IF;
  END int_to_str_1bit;

-------------------------------------------------------------------------------
-- Convert binary integer (0 or 1) into std_logic ('0' or '1')
-------------------------------------------------------------------------------
  FUNCTION bint_2_sl (X : INTEGER) RETURN STD_LOGIC IS
  BEGIN
    IF (X = 0) THEN
      RETURN '0';
    ELSE
      RETURN '1';
    END IF;
  END bint_2_sl;

-------------------------------------------------------------------------------
-- Calculate the SINIT value for the inferred BRAM using the generics:
-- C_SINIT_DOUT, C_SINIT_KOUT, and C_SINIT_RUN_DISP
-------------------------------------------------------------------------------
  FUNCTION concat_sinit(
    integer_run_disp : INTEGER;
    integer_kout     : INTEGER;
    string_dout      : STRING)
    RETURN STRING IS
    VARIABLE tmp_sym_disp                : STRING(1 TO 2);
    CONSTANT TMP_CODE_ERR                : STRING(1 TO 1) := "0";
    CONSTANT TMP_KOUT_0                  : STRING(1 TO 1) := "0";
    CONSTANT TMP_KOUT_1                  : STRING(1 TO 1) := "1";
    VARIABLE tmp_str                     : STRING(1 TO 14);
  BEGIN
    IF (integer_run_disp = 1) THEN
      tmp_sym_disp := CONST_POS;         -- D0.0+ has sym_disp of pos
    ELSE
      tmp_sym_disp := CONST_NEG;         -- D0.0- has sym_disp of neg
    END IF;
    IF (integer_kout = 1) THEN
      tmp_str := tmp_sym_disp & tmp_sym_disp & TMP_CODE_ERR &
                 TMP_KOUT_1 & string_dout;
    ELSE
      tmp_str := tmp_sym_disp & tmp_sym_disp & TMP_CODE_ERR &
                 TMP_KOUT_0 & string_dout;
    END IF;
    RETURN tmp_str;
  END concat_sinit;

-----------------------------------------------------------------------------
-- Convert a string containing 1's and 0's into a std_logic_vector of
-- width nbits
-----------------------------------------------------------------------------
  FUNCTION str_to_slv(
    bitsin : STRING;
    nbits  : INTEGER)
    RETURN STD_LOGIC_VECTOR IS
    VARIABLE ret       : STD_LOGIC_VECTOR(bitsin'range);
    VARIABLE ret0s     : STD_LOGIC_VECTOR(1 TO nbits) := (OTHERS => '0');
    VARIABLE retpadded : STD_LOGIC_VECTOR(1 TO nbits) := (OTHERS => '0');
    VARIABLE offset    : INTEGER := 0;
  BEGIN
    IF(bitsin'length = 0) THEN -- Make all '0's
      RETURN ret0s;
    END IF;
    IF(bitsin'length < nbits) THEN -- pad MSBs with '0's
      offset := nbits - bitsin'length;
      FOR i IN bitsin'range LOOP
        IF bitsin(i) = '1' THEN
          retpadded(i+offset) := '1';
        ELSIF (bitsin(i) = 'X' OR bitsin(i) = 'x') THEN
          retpadded(i+offset) := 'X';
        ELSIF (bitsin(i) = 'Z' OR bitsin(i) = 'z') THEN
          retpadded(i+offset) := 'Z';
        ELSIF (bitsin(i) = '0') THEN
          retpadded(i+offset) := '0';
        END IF;
      END LOOP;
      retpadded(1 TO offset) := (OTHERS => '0');
      RETURN retpadded;
    END IF;
    FOR i IN bitsin'range LOOP
      IF bitsin(i) = '1' THEN
        ret(i) := '1';
      ELSIF (bitsin(i) = 'X' OR bitsin(i) = 'x') THEN
        ret(i) := 'X';
      ELSIF (bitsin(i) = 'Z' OR bitsin(i) = 'z') THEN
        ret(i) := 'Z';
      ELSIF (bitsin(i) = '0') THEN
        ret(i) := '0';
      END IF;
    END LOOP;

    RETURN ret;
  END str_to_slv;

-----------------------------------------------------------------------------
-- Translate the SINIT string value from the core wrapper to
-- C_SINIT_RUN_DISP
-----------------------------------------------------------------------------
  FUNCTION calc_init_val_rd (SINIT_VAL : STRING(10 DOWNTO 1)) RETURN INTEGER IS
    VARIABLE tmp_init_val : INTEGER;
  BEGIN
    CASE SINIT_VAL IS
      WHEN "0000000001"  => tmp_init_val := 1;  --D.0.0 (pos)
      WHEN "0000000000"  => tmp_init_val := 0;  --D.0.0 (neg)
      WHEN "0100101001"  => tmp_init_val := 1;  --D.10.2 (pos)
      WHEN "0100101000"  => tmp_init_val := 0;  --D.10.2 (neg)
      WHEN "1011010101"  => tmp_init_val := 1;  --D.21.5 (pos)
      WHEN "1011010100"  => tmp_init_val := 0;  --D.21.5 (neg)
      WHEN OTHERS        => tmp_init_val := 0;  --invalid init value
    END CASE;
    RETURN tmp_init_val;
  END calc_init_val_rd;

-----------------------------------------------------------------------------
-- If C_HAS_BPORTS = 1, then the optional B ports are configured the
-- same as the optional A ports
-- If C_HAS_BPORTS = 0, then the optional B ports are disabled (= 0)
-----------------------------------------------------------------------------
  FUNCTION has_bport (
    C_HAS_BPORTS : INTEGER;
    has_aport    : INTEGER)
    RETURN INTEGER IS
    VARIABLE has_bport : INTEGER;
  BEGIN
    IF (C_HAS_BPORTS = 1) THEN
      has_bport := has_aport;
    ELSE
      has_bport := 0;
    END IF;
    RETURN has_bport;
  END has_bport;

END decode_8b10b_pkg;

