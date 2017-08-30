# DDR3K7 Core

## Introduction

This core is used as interface to read and write data to the DDR3 Memory. It is optimized for pipelined transfer. It has 2 Wishbone bus, one that allows read and write operations and the orthers that allows only write operations.

## Functional specification

![functional diagram](https://raw.githubusercontent.com/Yarr/Yarr-fw/master/doc/DDR3%20ctrl.png)

## Wishbone interface

When data are coming to wishbone interface, they are processed by a shift register to be apdatped for the Xilinix MiG user bus. No data should be sent while the stall_o output is set to "1". The first bus (wb) has a shift register for write request and a shift register for read request. The second has only one shift register for write request. Each shift register has a "not consecutive address detection" which can be desactivated to free FPGA space. Depending on the project complexity you might have timing constraint issues. As all the data coming from the DMA master in the YARR, this feature is desactivated by default. This feature can be easily reactivated by changing the "g_NOT_CONSECUTIVE_DETECTION" variable in the ddr3_ctrl_wb.

#### Wishbone 0 signals table
| Signal         | Description                                            |
| -------------- | ------------------------------------------------------ |
| wb_adr_o[31:0] | Address                                                |
| wb_dat_o[63:0] | Data out                                               |
| wb_sel_o[7:0]  | Byte select, should be hold to 11111111'b8 when unused |
| wb_stb_o       | Read or write cycle                                    |
| wb_we_o        | Write enable                                           |
| wb_cyc_o       | Read or write strobe                                   |
| wb_dat_i       | Data in                                                |
| wb_ack_i       | Acknowledge                                            |
| wb_stall_i     | For pipelined Wishbone                                 |

#### Wishbone 1 signals table
| Signal          | Description                                           |
| --------------- | ----------------------------------------------------- |
| wb1_adr_o[31:0] | Address                                               |
| wb1_dat_o[63:0] | Data out, set to 00000000000000000'h16                |
| wb1_sel_o       | Byte select, should be hold to "11111111" when unused |
| wb1_stb_o       | Read or write cycle, handle only write cycle          |
| wb1_we_o        | Write enable, all the read requests are just ignored  |
| wb1_cyc_o       | Read or write strobe, handle only write strobe        |
| wb1_dat_i       | Data in                                               |
| wb1_ack_i       | Acknowledge                                           |
| wb1_stall_i     | For pipelined Wishbone                                |

## MiG user interface

This bus is designed for the Xilinx Memory Interface Solution core. More informations about this in the Bibliography.

| Signal | Direction | Description |
| -------| --------- | ----------- |
| app_addr[ADDR_WIDTH – 1:0] | Output | This input indicates the address for the current request. |
| app_cmd[2:0] | Output | This input selects the command for the current request.
| app_en | Output | This is the active-High strobe for the app_addr[], app_cmd[2:0], |
| app_rdy | Input | This output indicates that the UI is ready to accept commands. |
| app_rd_data [APP_DATA_WIDTH – 1:0] | Input | This provides the output data from read commands. |
| app_rd_data_end | Input | This active-High output indicates that the current clock cycle is the last cycle of output data on app_rd_data[]. |
| app_rd_data_valid | Input | This active-High output indicates that app_rd_data[] is valid.
| app_wdf_data[APP_DATA_WIDTH – 1:0] | Output | This provides the data for write commands. |
| app_wdf_wren | Output | This is the active-High strobe for app_wdf_data[]. |
| app_wdf_end | Output | This active-High input indicates that the current clock cycle is the last cycle of input data on app_wdf_data[]. |
| app_wdf_mask [APP_MASK_WIDTH – 1:0] | Output | This provides the mask for app_wdf_data[]. |
| app_wdf_rdy | Input | This output indicates that the write data FIFO is ready to receive data. Write data is accepted when app_wdf_rdy = 1’b1 and app_wdf_wren = 1’b1. |

## Arbiter

The arbiter works with the Round Robin algorithm without any priority.

## Bibliography

[UG586](https://www.xilinx.com/support/documentation/ip_documentation/ug586_7Series_MIS.pdf)