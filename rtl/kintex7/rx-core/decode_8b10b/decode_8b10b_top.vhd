-------------------------------------------------------------------------------
--
--  Module      : decode_8b10b_top.vhd
--
--  Version     : 1.1
--
--  Last Update : 2008-10-31
--
--  Project     : 8b/10b Decoder Reference Design
--
--  Description : Core wrapper file
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

-------------------------------------------------------------------------------
-- Entity Declaration
-------------------------------------------------------------------------------
ENTITY decode_8b10b_top IS
  GENERIC (
    C_DECODE_TYPE      : INTEGER          := 0;
    C_HAS_BPORTS       : INTEGER          := 0;
    C_HAS_CE           : INTEGER          := 0;
    C_HAS_CODE_ERR     : INTEGER          := 0;
    C_HAS_DISP_ERR     : INTEGER          := 0;
    C_HAS_DISP_IN      : INTEGER          := 0;
    C_HAS_ND           : INTEGER          := 0;
    C_HAS_RUN_DISP     : INTEGER          := 0;
    C_HAS_SINIT        : INTEGER          := 0;
    C_HAS_SYM_DISP     : INTEGER          := 0;
    C_SINIT_VAL        : STRING(1 TO 10)  := "0000000000";
    C_SINIT_VAL_B      : STRING(1 TO 10)  := "0000000000"
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
END decode_8b10b_top;


-------------------------------------------------------------------------------
-- Architecture
-------------------------------------------------------------------------------
ARCHITECTURE xilinx OF decode_8b10b_top IS

CONSTANT SINIT_DOUT   : STRING(1 TO 8)  := C_SINIT_VAL(1 TO 8);
CONSTANT SINIT_DOUT_B : STRING(1 TO 8)  := C_SINIT_VAL_B(1 TO 8);
CONSTANT SINIT_RD     : INTEGER         := calc_init_val_rd(C_SINIT_VAL);
                                     -- converts C_SINIT_VAL string to integer
CONSTANT SINIT_RD_B   : INTEGER         := calc_init_val_rd(C_SINIT_VAL_B);
                                     -- converts C_SINIT_VAL_B string to integer

-- If C_HAS_BPORTS=1, the optional B ports are configured the same way as the
-- optional A ports.  Otherwise, all the B ports are disabled.
CONSTANT C_HAS_CE_B       : INTEGER := has_bport(C_HAS_BPORTS, C_HAS_CE);
CONSTANT C_HAS_CODE_ERR_B : INTEGER := has_bport(C_HAS_BPORTS, C_HAS_CODE_ERR);
CONSTANT C_HAS_DISP_ERR_B : INTEGER := has_bport(C_HAS_BPORTS, C_HAS_DISP_ERR);
CONSTANT C_HAS_DISP_IN_B  : INTEGER := has_bport(C_HAS_BPORTS, C_HAS_DISP_IN);
CONSTANT C_HAS_ND_B       : INTEGER := has_bport(C_HAS_BPORTS, C_HAS_ND);
CONSTANT C_HAS_RUN_DISP_B : INTEGER := has_bport(C_HAS_BPORTS, C_HAS_RUN_DISP);
CONSTANT C_HAS_SINIT_B    : INTEGER := has_bport(C_HAS_BPORTS, C_HAS_SINIT);
CONSTANT C_HAS_SYM_DISP_B : INTEGER := has_bport(C_HAS_BPORTS, C_HAS_SYM_DISP);

-------------------------------------------------------------------------------
-- BEGIN ARCHITECTURE
-------------------------------------------------------------------------------
BEGIN

dec : ENTITY decode_8b10b.decode_8b10b_rtl
  GENERIC MAP (
    C_DECODE_TYPE      =>  C_DECODE_TYPE,
    C_ELABORATION_DIR  =>  "./../../src/",
    C_HAS_BPORTS       =>  C_HAS_BPORTS,
    C_HAS_CE           =>  C_HAS_CE,
    C_HAS_CE_B         =>  C_HAS_CE_B,
    C_HAS_CODE_ERR     =>  C_HAS_CODE_ERR,
    C_HAS_CODE_ERR_B   =>  C_HAS_CODE_ERR_B,
    C_HAS_DISP_ERR     =>  C_HAS_DISP_ERR,
    C_HAS_DISP_ERR_B   =>  C_HAS_DISP_ERR_B,
    C_HAS_DISP_IN      =>  C_HAS_DISP_IN,
    C_HAS_DISP_IN_B    =>  C_HAS_DISP_IN_B,
    C_HAS_ND           =>  C_HAS_ND,
    C_HAS_ND_B         =>  C_HAS_ND_B,
    C_HAS_RUN_DISP     =>  C_HAS_RUN_DISP,
    C_HAS_RUN_DISP_B   =>  C_HAS_RUN_DISP_B,
    C_HAS_SINIT        =>  C_HAS_SINIT,
    C_HAS_SINIT_B      =>  C_HAS_SINIT_B,
    C_HAS_SYM_DISP     =>  C_HAS_SYM_DISP,
    C_HAS_SYM_DISP_B   =>  C_HAS_SYM_DISP_B,
    C_SINIT_DOUT       =>  SINIT_DOUT,
    C_SINIT_DOUT_B     =>  SINIT_DOUT_B,
    C_SINIT_KOUT       =>  0,
    C_SINIT_KOUT_B     =>  0,
    C_SINIT_RUN_DISP   =>  SINIT_RD,
    C_SINIT_RUN_DISP_B =>  SINIT_RD_B
  )
  PORT MAP(
    CLK        =>  CLK,
    DIN        =>  DIN,
    DOUT       =>  DOUT,
    KOUT       =>  KOUT,
    CE         =>  CE,
    CE_B       =>  CE_B,
    CLK_B      =>  CLK_B,
    DIN_B      =>  DIN_B,
    DISP_IN    =>  DISP_IN,
    DISP_IN_B  =>  DISP_IN_B,
    SINIT      =>  SINIT,
    SINIT_B    =>  SINIT_B,
    CODE_ERR   =>  CODE_ERR,
    CODE_ERR_B =>  CODE_ERR_B,
    DISP_ERR   =>  DISP_ERR,
    DISP_ERR_B =>  DISP_ERR_B,
    DOUT_B     =>  DOUT_B,
    KOUT_B     =>  KOUT_B,
    ND         =>  ND,
    ND_B       =>  ND_B,
    RUN_DISP   =>  RUN_DISP,
    RUN_DISP_B =>  RUN_DISP_B,
    SYM_DISP   =>  SYM_DISP,
    SYM_DISP_B =>  SYM_DISP_B
  );
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


END xilinx;





