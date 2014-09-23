---------------------------------------------------------------------------
--
--  Module      : decode_8b10b_rtl.vhd
--
--  Version     : 1.1
--
--  Last Update : 2008-10-31
--
--  Project     : 8b/10b Decoder Reference Design
--
--  Description : Top-level, synthesizable 8b/10b decoder core file
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

LIBRARY decode_8b10b;
USE decode_8b10b.decode_8b10b_pkg.ALL;

-----------------------------------------------------------------------------
-- Entity Declaration
-----------------------------------------------------------------------------
ENTITY decode_8b10b_rtl IS
  GENERIC (
    C_DECODE_TYPE      : INTEGER := 0;
    C_ELABORATION_DIR  : STRING  := "./../../src/";
    C_HAS_BPORTS       : INTEGER := 0;
    C_HAS_CE           : INTEGER := 0;
    C_HAS_CE_B         : INTEGER := 0;
    C_HAS_CODE_ERR     : INTEGER := 0;
    C_HAS_CODE_ERR_B   : INTEGER := 0;
    C_HAS_DISP_ERR     : INTEGER := 0;
    C_HAS_DISP_ERR_B   : INTEGER := 0;
    C_HAS_DISP_IN      : INTEGER := 0;
    C_HAS_DISP_IN_B    : INTEGER := 0;
    C_HAS_ND           : INTEGER := 0;
    C_HAS_ND_B         : INTEGER := 0;
    C_HAS_RUN_DISP     : INTEGER := 0;
    C_HAS_RUN_DISP_B   : INTEGER := 0;
    C_HAS_SINIT        : INTEGER := 0;
    C_HAS_SINIT_B      : INTEGER := 0;
    C_HAS_SYM_DISP     : INTEGER := 0;
    C_HAS_SYM_DISP_B   : INTEGER := 0;
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
END decode_8b10b_rtl;

--------------------------------------------------------------------------------
-- Generic Definitions:
--------------------------------------------------------------------------------
    --  C_DECODE_TYPE      : Implementation: 0=Slice based, 1=BlockRam
    --  C_ELABORATION_DIR  : Directory path for mif file
    --  C_HAS_BPORTS       : 1 indicates second decoder should be generated
    --  C_HAS_CE           : 1 indicates ce port is present
    --  C_HAS_CE_B         : 1 indicates ce_b port is present (if c_has_bports=1)
    --  C_HAS_CODE_ERR     : 1 indicates code_err port is present
    --  C_HAS_CODE_ERR_B   : 1 indicates code_err_b port is present
    --                        (if c_has_bports=1)
    --  C_HAS_DISP_ERR     : 1 indicates disp_err port is present
    --  C_HAS_DISP_ERR_B   : 1 indicates disp_err_b port is present
    --                        (if c_has_bports=1)
    --  C_HAS_DISP_IN      : 1 indicates disp_in port is present
    --  C_HAS_DISP_IN_B    : 1 indicates disp_in_b port is present
    --                        (if c_has_bports=1)
    --  C_HAS_ND           : 1 indicates nd port is present
    --  C_HAS_ND_B         : 1 indicates nd_b port is present (if c_has_bports=1)
    --  C_HAS_RUN_DISP     : 1 indicates run_disp port is present
    --  C_HAS_RUN_DISP_B   : 1 indicates run_disp_b port is present
    --                        (if c_has_bports=1)
    --  C_HAS_SINIT        : 1 indicates sinit port is present
    --  C_HAS_SINIT_B      : 1 indicates sinit_b port is present
    --                        (if c_has_bports=1)
    --  C_HAS_SYM_DISP     : 1 indicates sym_disp port is present
    --  C_HAS_SYM_DISP_B   : 1 indicates sym_disp_b port is present
    --                        (if c_has_bports=1)
    --  C_SINIT_DOUT       : 8-bit binary string, dout value when sinit is active
    --  C_SINIT_DOUT_B     : 8-bit binary string, dout_b value when sinit_b is
    --                        active
    --  C_SINIT_KOUT       : controls kout output when sinit is active
    --  C_SINIT_KOUT_B     : controls kout_b output when sinit_b is active
    --  C_SINIT_RUN_DISP   : Initializes run_disp (and disp_in) value to
    --                        positive(1) or negative(0)
    --  C_SINIT_RUN_DISP_B : Initializes run_disp_b (and disp_in_b) value to
    --                        positive(1) or negative(0)
--------------------------------------------------------------------------------
-- Port Definitions:
--------------------------------------------------------------------------------
    -- Mandatory Pins
    --  CLK  : Clock Input
    --  DIN  : Encoded Symbol Input
    --  DOUT : Data Output, decoded data byte
    --  KOUT : Command Output
    -------------------------------------------------------------------------
    -- Optional Pins
    --  CE         : Clock Enable
    --  CE_B       : Clock Enable (B port)
    --  CLK_B      : Clock Input (B port)
    --  DIN_B      : Encoded Symbol Input (B port)
    --  DISP_IN    : Disparity Input (running disparity in)
    --  DISP_IN_B  : Disparity Input (running disparity in) (B port)
    --  SINIT      : Synchronous Initialization. Resets core to known state.
    --  SINIT_B    : Synchronous Initialization. Resets core to known state.
    --                (B port)
    --  CODE_ERR   : Code Error, indicates that input symbol did not correspond
    --                to a valid member of the code set.
    --  CODE_ERR_B : Code Error, indicates that input symbol did not correspond
    --                to a valid member of the code set. (B port)
    --  DISP_ERR   : Disparity Error
    --  DISP_ERR_B : Disparity Error (B port)
    --  DOUT_B     : Data Output, decoded data byte (B port)
    --  KOUT_B     : Command Output (B port)
    --  ND         : New Data
    --  ND_B       : New Data (B port)
    --  RUN_DISP   : Running Disparity
    --  RUN_DISP_B : Running Disparity (B port)
    --  SYM_DISP   : Symbol Disparity
    --  SYM_DISP_B : Symbol Disparity (B port)
    -------------------------------------------------------------------------


-------------------------------------------------------------------------------
-- Architecture
-------------------------------------------------------------------------------
ARCHITECTURE xilinx OF decode_8b10b_rtl IS

-------------------------------------------------------------------------------
-- Signal Declarations
-------------------------------------------------------------------------------
  SIGNAL dout_i       : STD_LOGIC_VECTOR(7 DOWNTO 0) :=
                        str_to_slv(C_SINIT_DOUT,8);
                        --convert C_SINIT_DOUT string to 8bit std_logic_vector
  SIGNAL kout_i       : STD_LOGIC                    :=
                        bint_2_sl(C_SINIT_KOUT);
                        --convert C_SINIT_KOUT integer to std_logic
  SIGNAL clk_b_i      : STD_LOGIC                    := '0';
  SIGNAL din_b_i      : STD_LOGIC_VECTOR(9 DOWNTO 0) := (OTHERS => '0');
  SIGNAL ce_i         : STD_LOGIC                    := '0';
  SIGNAL ce_b_i       : STD_LOGIC                    := '0';
  SIGNAL disp_in_i    : STD_LOGIC                    := '0';
  SIGNAL disp_in_b_i  : STD_LOGIC                    := '0';
  SIGNAL sinit_i      : STD_LOGIC                    := '0';
  SIGNAL sinit_b_i    : STD_LOGIC                    := '0';
  SIGNAL code_err_i   : STD_LOGIC                    := '0';
  SIGNAL code_err_b_i : STD_LOGIC                    := '0';
  SIGNAL disp_err_i   : STD_LOGIC                    := '0';
  SIGNAL disp_err_b_i : STD_LOGIC                    := '0';
  SIGNAL dout_b_i     : STD_LOGIC_VECTOR(7 DOWNTO 0) :=
                        str_to_slv(C_SINIT_DOUT_B,8);
                        --convert C_SINIT_DOUT_B string to 8bit std_logic_vector
  SIGNAL kout_b_i     : STD_LOGIC                    :=
                        bint_2_sl(C_SINIT_KOUT_B);
                        --convert C_SINIT_KOUT_B integer to std_logic
  SIGNAL nd_i         : STD_LOGIC                    := '0';
  SIGNAL nd_b_i       : STD_LOGIC                    := '0';
  SIGNAL run_disp_i   : STD_LOGIC                    :=
                        bint_2_sl(C_SINIT_RUN_DISP);
                        --convert C_SINIT_RUN_DISP integer to std logic
  SIGNAL run_disp_b_i : STD_LOGIC                    :=
                        bint_2_sl(C_SINIT_RUN_DISP_B);
                        --convert C_SINIT_RUN_DISP_B integer to std logic
  SIGNAL sym_disp_i   : STD_LOGIC_VECTOR(1 DOWNTO 0) :=
                        conv_std_logic_vector(C_SINIT_RUN_DISP,2);
                        --convert C_SINIT_RUN_DISP integer to slv
  SIGNAL sym_disp_b_i : STD_LOGIC_VECTOR(1 DOWNTO 0) :=
                        conv_std_logic_vector(C_SINIT_RUN_DISP_B,2);
                        --convert C_SINIT_RUN_DISP_B integer to slv

-------------------------------------------------------------------------------
-- Begin Architecture
-------------------------------------------------------------------------------
BEGIN

  -----------------------------------------------------------------------------
  -- LUT-based decoder
  -----------------------------------------------------------------------------
  glut : IF (C_DECODE_TYPE = 0) GENERATE
    ldec : ENTITY decode_8b10b.decode_8b10b_lut
      GENERIC MAP (
        C_HAS_BPORTS       =>  C_HAS_BPORTS,
        C_HAS_CODE_ERR     =>  C_HAS_CODE_ERR,
        C_HAS_CODE_ERR_B   =>  C_HAS_CODE_ERR_B,
        C_HAS_DISP_ERR     =>  C_HAS_DISP_ERR,
        C_HAS_DISP_ERR_B   =>  C_HAS_DISP_ERR_B,
        C_HAS_DISP_IN      =>  C_HAS_DISP_IN,
        C_HAS_DISP_IN_B    =>  C_HAS_DISP_IN_B,
        C_HAS_ND           =>  C_HAS_ND,
        C_HAS_ND_B         =>  C_HAS_ND_B,
        C_HAS_SYM_DISP     =>  C_HAS_SYM_DISP,
        C_HAS_SYM_DISP_B   =>  C_HAS_SYM_DISP_B,
        C_HAS_RUN_DISP     =>  C_HAS_RUN_DISP,
        C_HAS_RUN_DISP_B   =>  C_HAS_RUN_DISP_B,
        C_SINIT_DOUT       =>  C_SINIT_DOUT,
        C_SINIT_DOUT_B     =>  C_SINIT_DOUT_B,
        C_SINIT_KOUT       =>  C_SINIT_KOUT,
        C_SINIT_KOUT_B     =>  C_SINIT_KOUT_B,
        C_SINIT_RUN_DISP   =>  C_SINIT_RUN_DISP,
        C_SINIT_RUN_DISP_B =>  C_SINIT_RUN_DISP_B
        )
      PORT MAP(
        CLK        =>  CLK,
        DIN        =>  DIN,
        DOUT       =>  dout_i,
        KOUT       =>  kout_i,

        CE         =>  ce_i,
        DISP_IN    =>  disp_in_i,
        SINIT      =>  sinit_i,
        CODE_ERR   =>  code_err_i,
        DISP_ERR   =>  disp_err_i,
        ND         =>  nd_i,
        RUN_DISP   =>  run_disp_i,
        SYM_DISP   =>  sym_disp_i,

        CLK_B      =>  clk_b_i,
        DIN_B      =>  din_b_i,
        DOUT_B     =>  dout_b_i,
        KOUT_B     =>  kout_b_i,

        CE_B       =>  ce_b_i,
        DISP_IN_B  =>  disp_in_b_i,
        SINIT_B    =>  sinit_b_i,
        CODE_ERR_B =>  code_err_b_i,
        DISP_ERR_B =>  disp_err_b_i,
        ND_B       =>  nd_b_i,
        RUN_DISP_B =>  run_disp_b_i,
        SYM_DISP_B =>  sym_disp_b_i
        );
  END GENERATE glut;

  -----------------------------------------------------------------------------
  -- BRAM-based decoder
  -----------------------------------------------------------------------------
  gbram : IF (C_DECODE_TYPE /= 0) GENERATE
    bdec : ENTITY decode_8b10b.decode_8b10b_bram
      GENERIC MAP (
        C_ELABORATION_DIR  =>  C_ELABORATION_DIR,
        C_HAS_BPORTS       =>  C_HAS_BPORTS,
        C_HAS_DISP_IN      =>  C_HAS_DISP_IN,
        C_HAS_DISP_IN_B    =>  C_HAS_DISP_IN_B,
        C_HAS_DISP_ERR     =>  C_HAS_DISP_ERR,
        C_HAS_DISP_ERR_B   =>  C_HAS_DISP_ERR_B,
        C_HAS_RUN_DISP     =>  C_HAS_RUN_DISP,
        C_HAS_RUN_DISP_B   =>  C_HAS_RUN_DISP_B,
        C_HAS_SYM_DISP     =>  C_HAS_SYM_DISP,
        C_HAS_SYM_DISP_B   =>  C_HAS_SYM_DISP_B,
        C_HAS_ND           =>  C_HAS_ND,
        C_HAS_ND_B         =>  C_HAS_ND_B,
        C_SINIT_DOUT       =>  C_SINIT_DOUT,
        C_SINIT_DOUT_B     =>  C_SINIT_DOUT_B,
        C_SINIT_KOUT       =>  C_SINIT_KOUT,
        C_SINIT_KOUT_B     =>  C_SINIT_KOUT_B,
        C_SINIT_RUN_DISP   =>  C_SINIT_RUN_DISP,
        C_SINIT_RUN_DISP_B =>  C_SINIT_RUN_DISP_B
        )

      PORT MAP(
        CLK        =>  CLK,
        DIN        =>  DIN,
        DOUT       =>  dout_i,
        KOUT       =>  kout_i,

        CE         =>  ce_i,
        DISP_IN    =>  disp_in_i,
        SINIT      =>  sinit_i,
        CODE_ERR   =>  code_err_i,
        DISP_ERR   =>  disp_err_i,
        ND         =>  nd_i,
        RUN_DISP   =>  run_disp_i,
        SYM_DISP   =>  sym_disp_i,

        CLK_B      =>  clk_b_i,
        DIN_B      =>  din_b_i,
        DOUT_B     =>  dout_b_i,
        KOUT_B     =>  kout_b_i,

        CE_B       =>  ce_b_i,
        DISP_IN_B  =>  disp_in_b_i,
        SINIT_B    =>  sinit_b_i,
        CODE_ERR_B =>  code_err_b_i,
        DISP_ERR_B =>  disp_err_b_i,
        ND_B       =>  nd_b_i,
        RUN_DISP_B =>  run_disp_b_i,
        SYM_DISP_B =>  sym_disp_b_i
        );
  END GENERATE gbram;


  ---------------------------------------------------------------------------
  -- Mandatory A Ports
  ---------------------------------------------------------------------------
  DOUT <= dout_i;
  KOUT <= kout_i;

  ---------------------------------------------------------------------------
  -- Optional A Ports                  --tying off unused ports
  ---------------------------------------------------------------------------
  --Inputs
  --ce
  gen : IF (C_HAS_CE/=0) GENERATE
    ce_i <= CE;
  END GENERATE gen;
  ngen : IF (C_HAS_CE = 0) GENERATE
    ce_i <= '1';
  END GENERATE ngen;

  --disp_in
  gdi : IF (C_HAS_DISP_IN /= 0) GENERATE
    disp_in_i <= DISP_IN;
  END GENERATE gdi;
  ngdi : IF (C_HAS_DISP_IN = 0) GENERATE
    disp_in_i <= '0';
  END GENERATE ngdi;

  --sinit
  gs : IF (C_HAS_SINIT /= 0) GENERATE
    sinit_i <= SINIT;
  END GENERATE gs;
  ngs : IF (C_HAS_SINIT = 0) GENERATE
    sinit_i <= '0';
  END GENERATE ngs;


  --Outputs
  --nd
  gnd : IF (C_HAS_ND /= 0) GENERATE
    ASSERT (C_HAS_CE /= 0)
      REPORT "Invalid configuration: ND port requires CE port"
      SEVERITY WARNING;
    ND  <= nd_i;
  END GENERATE gnd;
  ngnd : IF (C_HAS_ND = 0) GENERATE
    ND  <= '0';
  END GENERATE ngnd;

  --code_err
  gce : IF (C_HAS_CODE_ERR /= 0) GENERATE
    CODE_ERR  <= code_err_i;
  END GENERATE gce;
  ngce : IF (C_HAS_CODE_ERR = 0) GENERATE
    CODE_ERR  <= '0';
  END GENERATE ngce;

  --disp_err
  gder : IF (C_HAS_DISP_ERR /= 0) GENERATE
    DISP_ERR  <= disp_err_i;
  END GENERATE gder;
  ngder : IF (C_HAS_DISP_ERR = 0) GENERATE
    DISP_ERR  <= '0';
  END GENERATE ngder;

  --run_disp
  grd : IF (C_HAS_RUN_DISP /= 0) GENERATE
    RUN_DISP  <= run_disp_i;
  END GENERATE grd;
  ngrd : IF (C_HAS_RUN_DISP = 0) GENERATE
    RUN_DISP  <= '0';
  END GENERATE ngrd;

  --sym_disp
  gsd : IF (C_HAS_SYM_DISP /= 0) GENERATE
    SYM_DISP  <= sym_disp_i;
  END GENERATE gsd;
  ngsd : IF (C_HAS_SYM_DISP = 0) GENERATE
    SYM_DISP  <= "00";
  END GENERATE ngsd;

  ----------------------------------------------------------------------------
  -- Optional B Ports                  -- tying off unused ports
  ----------------------------------------------------------------------------
  --Mandatory B ports (if B ports are selected)
  gbpt : IF (C_HAS_BPORTS /= 0) GENERATE
    din_b_i  <= DIN_B;
    clk_b_i  <= CLK_B;
    DOUT_B   <= dout_b_i;
    KOUT_B   <= kout_b_i;
  END GENERATE gbpt;
  ngbpt : IF (C_HAS_BPORTS = 0) GENERATE
    din_b_i  <= (OTHERS => '0');
    clk_b_i  <= '0';
    DOUT_B   <= (OTHERS => '0');
    KOUT_B   <= '0';
  END GENERATE ngbpt;

  --Inputs
  --ce_b
  genb : IF (C_HAS_CE_B /= 0 AND C_HAS_BPORTS /= 0) GENERATE
    ce_b_i <= CE_B;
  END GENERATE genb;
  ngenb : IF (C_HAS_CE_B = 0 OR C_HAS_BPORTS = 0) GENERATE
    ce_b_i <= '1';
  END GENERATE ngenb;
  ASSERT (NOT(C_HAS_CE_B /= 0 AND C_HAS_BPORTS = 0))
    REPORT "Invalid configuration: Will not generate CE_B when C_HAS_BPORTS=0"
    SEVERITY WARNING;

  --disp_in_b
  gdib : IF (C_HAS_DISP_IN_B /= 0 AND C_HAS_BPORTS /= 0) GENERATE
    disp_in_b_i <= DISP_IN_B;
  END GENERATE gdib;
  ngdib : IF (C_HAS_DISP_IN_B = 0 OR C_HAS_BPORTS = 0) GENERATE
    disp_in_b_i <= '0';
  END GENERATE ngdib;
  ASSERT (NOT(C_HAS_DISP_IN_B /= 0 AND C_HAS_BPORTS = 0))
    REPORT "Invalid configuration: Will not generate DISP_IN_B when " &
    "C_HAS_BPORTS=0"
    SEVERITY WARNING;

  --sinit_b
  gsb : IF (C_HAS_SINIT_B /= 0 AND C_HAS_BPORTS /= 0) GENERATE
    sinit_b_i <= SINIT_B;
  END GENERATE gsb;
  ngsb : IF (C_HAS_SINIT_B = 0 OR C_HAS_BPORTS = 0) GENERATE
    sinit_b_i <= '0';
  END GENERATE ngsb;
  ASSERT (NOT(C_HAS_SINIT_B /= 0 AND C_HAS_BPORTS = 0))
    REPORT "Invalid configuration: Will not generate SINIT_B when C_HAS_BPORTS=0"
    SEVERITY WARNING;

  --Outputs
  --code_err_b
  gceb : IF (C_HAS_CODE_ERR_B /= 0 AND C_HAS_BPORTS /= 0) GENERATE
    CODE_ERR_B <= code_err_b_i;
  END GENERATE gceb;
  ngceb : IF (C_HAS_CODE_ERR_B = 0 OR C_HAS_BPORTS = 0) GENERATE
    CODE_ERR_B <= '0';
  END GENERATE ngceb;
  ASSERT (NOT(C_HAS_CODE_ERR_B /= 0 AND C_HAS_BPORTS = 0))
    REPORT "Invalid configuration: Will not generate CODE_ERR_B when " &
    "C_HAS_BPORTS=0"
    SEVERITY WARNING;

  --disp_err_b
  gdeb : IF (C_HAS_DISP_ERR_B /= 0 AND C_HAS_BPORTS /= 0) GENERATE
    DISP_ERR_B <= disp_err_b_i;
  END GENERATE gdeb;
  ngdeb : IF (C_HAS_DISP_ERR_B = 0 OR C_HAS_BPORTS = 0) GENERATE
    DISP_ERR_B <= '0';
  END GENERATE ngdeb;
  ASSERT (NOT(C_HAS_DISP_ERR_B /= 0 AND C_HAS_BPORTS = 0))
    REPORT "Invalid configuration: Will not generate DISP_ERR_B when " &
    "C_HAS_BPORTS=0"
    SEVERITY WARNING;

  --nd_b
  gndb : IF (C_HAS_ND_B /= 0 AND C_HAS_BPORTS /= 0) GENERATE
    ASSERT (C_HAS_CE_B /= 0)
      REPORT "Invalid configuration: ND_B port requires CE_B port"
      SEVERITY WARNING;
    ND_B <= nd_b_i;
  END GENERATE gndb;
  ngndb : IF (C_HAS_ND_B = 0 OR C_HAS_BPORTS = 0) GENERATE
    ND_B <= '0';
  END GENERATE ngndb;
  ASSERT (NOT(C_HAS_ND_B /= 0 AND C_HAS_BPORTS = 0))
    REPORT "Invalid configuration: Will not generate ND_B when C_HAS_BPORTS=0"
    SEVERITY WARNING;

  --run_disp_b
  grdb : IF (C_HAS_RUN_DISP_B /= 0 AND C_HAS_BPORTS /= 0) GENERATE
    RUN_DISP_B <= run_disp_b_i;
  END GENERATE grdb;
  ngrdb : IF (C_HAS_RUN_DISP_B = 0 OR C_HAS_BPORTS = 0) GENERATE
    RUN_DISP_B <= '0';
  END GENERATE ngrdb;
  ASSERT (NOT(C_HAS_RUN_DISP_B /= 0 AND C_HAS_BPORTS = 0))
    REPORT "Invalid configuration: Will not generate RUN_DISP_B when " &
    "C_HAS_BPORTS=0"
    SEVERITY WARNING;

  --sym_disp_b
  gsdb : IF (C_HAS_SYM_DISP_B /= 0 AND C_HAS_BPORTS /= 0) GENERATE
    SYM_DISP_B <= sym_disp_b_i;
  END GENERATE gsdb;
  ngsdb : IF (C_HAS_SYM_DISP_B = 0 OR C_HAS_BPORTS = 0) GENERATE
    SYM_DISP_B <= "00";
  END GENERATE ngsdb;
  ASSERT (NOT(C_HAS_SYM_DISP_B /= 0 AND C_HAS_BPORTS = 0))
    REPORT "Invalid configuration: Will not generate SYM_DISP_B when " &
    "C_HAS_BPORTS=0"
    SEVERITY WARNING;

END xilinx;

