files = ["ip_cores/clk_gen.vhd",
         "ip_cores/ila.ngc",
         "ip_cores/ila_icon.ngc",
         "ip_cores/fifo_64x512.ngc",
         "ip_cores/fifo_32x512.ngc",
         "ip_cores/l2p_fifo.ngc",
         "ip_cores/rx_bridge_fifo.ngc",
         "ip_cores/rx_channel_fifo.ngc",
         "ip_cores/rx_bridge_ctrl_fifo.ngc",
         "ip_cores/ila.vhd",
         "ip_cores/ila_icon.vhd",
         "ip_cores/fifo_64x512.vhd",
         "ip_cores/fifo_32x512.vhd",
         "ip_cores/l2p_fifo.vhd",
         "ip_cores/rx_bridge_fifo.vhd",
         "ip_cores/rx_channel_fifo.vhd",
         "ip_cores/rx_bridge_ctrl_fifo.vhd"]

modules = { "local" : [
            "common",
            "gn4124-core",
            "ddr3-core",
            "tx-core",
            "rx-core"
            ],
          }
