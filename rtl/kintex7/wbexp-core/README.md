

# Wishbone Express Core
## Introduction
The Wishbone Express Core is based on GN4124 core and has the same the same purpose. The GN4124 core is designed to work with a chip which has the same name. This chip transfers data up to 800 MB/s. As we needed a  faster transfer speed, a new board was found, but it has no PCIe bridge chip. Therefore, this chip was substituted with the Xilinx "7 Series FPGAs Integrated Block for PCI Express" which is a bridge between an AXI-Stream and PCIe bus. 
## Functional specification
![functional diagram](https://raw.githubusercontent.com/Yarr/Yarr-fw/master/doc/Wishbone%20express%20cores.png)
### AXI-Stream interface
This bus is specifed by Xilinx as a "standard" bus made for stream packet data transfer. This core is made to be connected to "7 Series FPGAs Integrated Block for PCI Express" which is a Xilinx IP core designed to communicate from a PCI-express to AXI-Stream bus. The both connected allows to communicate from PCI-express bus to two different Wishbone buses (classic and pipelined).
[PG054]

| Signal                  | Description                                                 |
| ----------------------- | ----------------------------------------------------------- |
| m_axis_tx_tready_i      | Transmit destination ready to accept data                   |
| m_axis_tx_tdata_o       | Transmit data                                               |
| m_axis_tx_tkeep_o       | Transmit data strobe                                        |
| m_axis_tx_tlast_o       | Indicates the last data beaf of a packet                    |
| m_axis_tx_tvalid_o      | Indicates valid transmit data                               |
| m_axis_tx_tuser_o       | Indicates custom informations about the transmit destination|
| s_axis_rx_tdata_i       | Receive data                                                |
| s_axis_rx_tkeep_i       | Receive data strobe                                         |
| s_axis_rx_tlast_i       | Indicates the last data beaf of a packet                    |
| s_axis_rx_tvalid_i      | Indicates valid receive data                                |
| s_axis_rx_tready_o      | Receive source ready to accept data                         |
| s_axis_rx_tuser_i       | Indicates custom informations about the receive source      |
 [PG054]

### CSR Wishbone interface
When the core receive Memory write and read request with a double word payload, it sends the data payload to the CSR wishbone interface. Only packet with a single double word payload will be send to the CSR wishbone bus. It also allows to read double word by receiving Memory read request.

#### Wishbone signals table
| Signal      | Description            |
| ----------- | ---------------------- |
| wb_adr_o    |   Address              |
| wb_dat_o    | Data out               |
| wb_sel_o    | Byte select            |
| wb_stb_o    | Read or write cycle    |
| wb_we_o     | Write enable           |
| wb_cyc_o    | Read or write strobe   |
| wb_dat_i    | Data in                |
| wb_ack_i    | Acknowledge            |
| wb_stall_i  | For pipelined Wishbone |

### DMA wishbone registers interface
This is wishbone slave interface connected to registers which set the pipelined wishbone transfer performed by the DMA masters.

| NAME        | OFFSET | MODE | RESET      | DESCRIPTION
| ------------| ------ | ---- | ---------- | ---------------------------------------- |
| DMACTRLR    | 0x00   | R/W  | 0x00000000 | DMA engine control                       |
| DMASTATR    | 0x04   | RO   | 0x00000000 | DMA engine status                        |
| DMACSTARTR  | 0x08   | R/W  | 0x00000000 | DMA start address in the carrier         |
| DMAHSTARTLR | 0x0C   | R/W  | 0x00000000 | DMA start address (low) in the PCIe host |
| DMAHSTARTHR | 0x10   | R/W  | 0x00000000 | DMA start address (high) in the PCIe host|
| DMALENR     | 0x14   | R/W  | 0x00000000 | DMA read length in bytes                 |
| DMANEXTLR   | 0x18   | R/W  | 0x00000000 | Pointer (low) to next item in list       |
| DMANEXTHR   | 0x1C   | R/W  | 0x00000000 | Pointer (high) to next item in list      |
| DMAATTRIBR  | 0x20   | R/W  | 0x00000000 | DMA chain control                        |

### DMA pipelined wishbone interface
This interface transfer data to the PCIe bus according the settings written in DMA registers. Those transfer are performed by one of the DMA masters. The host is a slave which receive Memory write and read requests.

### Interrupt interface
To notify the end of a DMA transfer the core notify the host with an interrupt PCIe packet. The Xilinx IP has signals devoted to interrupt. The core manage to send the PCIe packet to the host.

## Hierarchy
coming soon


## Bibliography

[PG054] : https://www.xilinx.com/support/documentation/ip_documentation/pcie_7x/v3_0/pg054-7series-pcie.pdf "7 Series FPGAs Integrated Block for PCI Express v3.0 LogiCORE IP Product Guide"