-------------------------------------------------------------------------------
--
--  Module      : decode_8b10b_wrapper.vhd
--
--  Version     : 1.1
--
--  Last Update : 2008-10-31
--
--  Project     : 8b/10b Decoder
--
--  Description : Top-level core wrapper file
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
--                Copyright 2008 Xilinx, Inc.  All rights reserved.
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

-------------------------------------------------------------------------------
-- Entity Declaration
-------------------------------------------------------------------------------
ENTITY decode_8b10b_wrapper IS
  PORT (
    
    CLK        : IN  STD_LOGIC                    := '0';
    DIN        : IN  STD_LOGIC_VECTOR(9 DOWNTO 0) := (OTHERS => '0');
    DOUT       : OUT STD_LOGIC_VECTOR(7 DOWNTO 0) ;
    KOUT       : OUT STD_LOGIC                    ;

    CE         : IN  STD_LOGIC                    := '0';
    SINIT      : IN  STD_LOGIC                    := '0';
    CODE_ERR   : OUT STD_LOGIC                    := '0';
    DISP_ERR   : OUT STD_LOGIC                    := '0';
	 ND 			: OUT STD_LOGIC						  := '0'



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
    --  CLK_B      : Clock Input (B port)
    --  DIN_B      : Encoded Symbol Input (B port)
    --  DOUT_B     : Data Output, decoded data byte (B port)
    --  KOUT_B     : Command Output (B port)  
    --  CE         : Clock Enable
    --  SINIT      : Synchronous Initialization. Resets core to known state.
    --  DISP_IN    : Disparity Input (running disparity in)
    --  CODE_ERR   : Code Error, indicates that input symbol did not correspond
    --                to a valid member of the code set.
    --  DISP_ERR   : Disparity Error
    --  ND         : New Data
    --  RUN_DISP   : Running Disparity
    --  SYM_DISP   : Symbol Disparity
    --  CE_B       : Clock Enable (B port)
    --  SINIT_B    : Synchronous Initialization. Resets core to known state.
    --                (B port)
    --  DISP_IN_B  : Disparity Input (running disparity in) (B port)
    --  CODE_ERR_B : Code Error, indicates that input symbol did not correspond
    --                to a valid member of the code set. (B port)
    --  DISP_ERR_B : Disparity Error (B port)
    --  ND_B       : New Data (B port)
    --  RUN_DISP_B : Running Disparity (B port)
    --  SYM_DISP_B : Symbol Disparity (B port)
----------------------------------------------------------------------------
END decode_8b10b_wrapper;

-------------------------------------------------------------------------------
-- Architecture
-------------------------------------------------------------------------------
ARCHITECTURE xilinx OF decode_8b10b_wrapper IS

  CONSTANT LOW    : STD_LOGIC := '0';
  CONSTANT HIGH   : STD_LOGIC := '1';
  CONSTANT LOWSLV : STD_LOGIC_VECTOR(9 DOWNTO 0) := (OTHERS => '0');
    
-------------------------------------------------------------------------------
-- BEGIN ARCHITECTURE
-------------------------------------------------------------------------------
BEGIN

dec : ENTITY decode_8b10b.decode_8b10b_top
  GENERIC MAP (
   C_DECODE_TYPE     => 1, 
   C_HAS_BPORTS      => 0, 
   C_HAS_CE          => 1,
   C_HAS_CODE_ERR    => 1,
   C_HAS_DISP_ERR    => 1,
   C_HAS_DISP_IN     => 0,
   C_HAS_ND          => 1,
   C_HAS_RUN_DISP    => 0,
   C_HAS_SINIT       => 1,
   C_HAS_SYM_DISP    => 0,
   C_SINIT_VAL       => "0000000001",
   C_SINIT_VAL_B     => "0000000000"
  )
  PORT MAP(
    CLK        =>  CLK,
    DIN        =>  DIN,
    DOUT       =>  DOUT,
    KOUT       =>  KOUT,
    
    CE         =>  CE,
    SINIT      =>  SINIT,
    DISP_IN    =>  LOW, 
    CODE_ERR   =>  CODE_ERR,
    DISP_ERR   =>  DISP_ERR,
    ND         =>  ND, 
    RUN_DISP   =>  open, 
    SYM_DISP   =>  open,  
    
    CLK_B      =>  LOW,
    DIN_B      =>  LOWSLV,
    DOUT_B     =>  open,
    KOUT_B     =>  open,
    
    CE_B       =>  HIGH,
    SINIT_B    =>  LOW, 
    DISP_IN_B  =>  LOW, 
    CODE_ERR_B =>  open, 
    DISP_ERR_B =>  open,
    ND_B       =>  open, 
    RUN_DISP_B =>  open, 
    SYM_DISP_B =>  open  
    
  );
--------------------------------------------------------------------------------
-- Generic Definitions:
--------------------------------------------------------------------------------
    --  C_DECODE_TYPE      : Implementation: 0=LUT based, 1=BRAM based
    --  C_HAS_BPORTS       : 1 indicates second decoder should be generated
    --  C_HAS_CE           : 1 indicates ce(_b) port is present
    --  C_HAS_CODE_ERR     : 1 indicates code_err(_b) port is present
    --  C_HAS_DISP_ERR     : 1 indicates disp_err(_b) port is present
    --  C_HAS_DISP_IN      : 1 indicates disp_in(_b) port is present
    --  C_HAS_ND           : 1 indicates nd(_b) port is present
    --  C_HAS_RUN_DISP     : 1 indicates run_disp(_b) port is present
    --  C_HAS_SINIT        : 1 indicates sinit(_b) port is present
    --  C_HAS_SYM_DISP     : 1 indicates sym_disp(_b) port is present
    --  C_SINIT_VAL        : 10-bit binary string, dout/kout/run_disp init value
    --  C_SINIT_VAL_B      : 10-bit binary string, dout_b/kout_b/run_disp_b init
    --                       value
--------------------------------------------------------------------------------

--------------------------------------------------------
--     C_SINIT_VAL/C_SINIT_VAL_B  {DOUT,KOUT,RD}
--------------------------------------------------------
    --  D.0.0 (pos)     DOUT: 000_00000  KOUT: 0  RD: 1
    --  D.0.0 (neg)     DOUT: 000_00000  KOUT: 0  RD: 0
    --  D.10.2 (pos)    DOUT: 010_01010  KOUT: 0  RD: 1
    --  D.10.2 (neg)    DOUT: 010_01010  KOUT: 0  RD: 0
    --  D.21.5 (pos)    DOUT: 101_10101  KOUT: 0  RD: 1
    --  D.21.5 (neg)    DOUT: 101_10101  KOUT: 0  RD: 0
--------------------------------------------------------


END xilinx;



