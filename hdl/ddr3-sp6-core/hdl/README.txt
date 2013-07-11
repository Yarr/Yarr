
Note:

When (re)generating MCB cores, the IBUFG for the clock must be commented out and
bypassed. Otherwise it can cause translation errors (automacically connects it to
a PAD and then complains because the buffer as multi-source!).
The IBUFG instanciation is in the folling file:
<board>/ip_cores/ddr3_ctrl_<board>_bank<nbank>_<nbit0>b_<nbit1>b/user_design/rtl/memc<nbank>_infrastructure.vhd

Diff example:

       --***********************************************************************
       -- SINGLE_ENDED input clock input buffers
       --***********************************************************************
-      u_ibufg_sys_clk : IBUFG
-        port map (
-          I  => sys_clk,
-          O  => sys_clk_ibufg
-          );
+      --u_ibufg_sys_clk : IBUFG
+      --  port map (
+      --    I  => sys_clk,
+      --    O  => sys_clk_ibufg
+      --    );
+    sys_clk_ibufg <= sys_clk;
   end generate;

   --***************************************************************************
