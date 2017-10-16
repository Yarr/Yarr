# Trigger Logic

## Introduction

The trigger unit has 4 external inputs (ext_trig_i) and 1 eudet input
(see section "Eudet stuff"). The external inputs are sent through a
synchronizer to deglitch and lock the inputs to clk_i, an edge
detector, and a shift register to introduce time delays between the 5
inputs. The output (ext_trig_o) can be any boolean function of the 5
inputs. The unit also reads out a trigger tag (trig_tag) which is
either a counter, clk_i timestamp, or the tag from the eudet_tlu.

![functional diagram](https://raw.githubusercontent.com/VBaratham/Yarr-fw/trigger/doc/Trigger%20logic%20core.png)

## Quick Configuration Example

All configuration is done over the Wishbone bus. The following sequence of writes shows an example configuration:

| Address   | Data   | Function                                                                    |
| --------- | ------ | --------------------------------------------------------------------------- |
| 0x3       | 0x00   | Use rising edge for all channels (default)                                  |
| 0x1       | 0x01   | Trigger tag is clk_i timestamp                                              |
| 0x8       | 0xC8   | Set deadtime to 200 clk_i cycles                                            |
| 0x0       | 0x07   | Mask-in ext[2:0], do not use ext[3] and eudet                               |
| 0x4       | 0x02   | Put input channel ext[0] on a 2 clock cycle delay                           |
| 0x2       | 0x5E   | (0x5E = 0b110100) Trigger on patterns 011, 101, 110 (see truth table below) |

With this configuration (logic: 0x5E = 0b110100, mask: 0x7 = 0b00111),
the unit outputs as follows:

| ext[2] | ext[1] | ext[0] | output | note                      |
| ------ | ------ | ------ | ------ | ------------------------- |
| 1      | 1      | 1      | 0      | bit [7] of config not set |
| 1      | 1      | 0      | 1      | bit [6] of config is set  |
| 1      | 0      | 1      | 1      | bit [5] of config is set  |
| 1      | 0      | 0      | 0      | bit [4] of config not set |
| 0      | 1      | 1      | 1      | bit [3] of config is set  |
| 0      | 1      | 0      | 0      | bit [2] of config not set |
| 0      | 0      | 1      | 0      | bit [1] of config not set |
| 0      | 0      | 0      | 0      | bit [0] of config not set |

## Configuration

Both the trigger logic and tag are configured over the wishbone bus
(wb_* signals). See http://cdn.opencores.org/downloads/wbspec_b4.pdf
for the Wishbone interface specification. The address space looks
like (see following sections for detail):

| Address   | Function                  |
| --------- | ------------------------- |
| 0x00      | Trigger mask              |
| 0x01      | Trigger tag mode          |
| 0x02      | Boolean logic config      |
| 0x03      | Edge selection            |
| 0x04      | ext[0] delay setting      |
| 0x05      | ext[1] delay setting      |
| 0x06      | ext[2] delay setting      |
| 0x07      | ext[3] delay setting      |
| 0x08      | deadtime setting          |

### Trigger logic

#### Trigger mask (optional)

In configuring the boolean logic, you may find it helpful to first
mask out inputs you aren't using. The state of these inputs is ignored
when the output is computed. (The alternative is to write logic that
accepts both high and low unused inputs).

To configure the mask, send a word to 0x00 on the Wishbone bus with
the mask in the 5 LSBs (1 = use, 0 = ignore):

| trig_mask bit   | Selects input trigger...  |
| --------------- | ------------------------- |
| trig_mask[31:5] | (ignored)                 |
| trig_mask[4]    | eudet_trig_i              |
| trig_mask[3:0]  | ext_trig_i[3:0]           |

eg. 0x00 <- 000000111 to mask out all but channels ext(0), ext(1), and ext(2)

#### Boolean logic

To configure what function of the inputs should appear at the output,
compute the configuration word and write it to 0x02. The configuration
word should have a bit set to 1 for each input pattern to accept.
Which bit to set for a given pattern can be computed by interpreting
the bit pattern as a binary number (where each bit corresponds to an
input channel as above in the section on configuring the mask).

For example, to trigger on a hit on ext[0] but no other channels,
set bit [1] (0b00001) of the configuration word. To trigger on a hit
on eudet and ext[2] but no other channels, set bit [20] (0b10100).

Any bits that are zero'd out in trig_mask will be ignored. 

### Trig tag

To configure the trig_tag output, write trig_tag_mode to 0x01 on the
Wishbone bus. trig_tag_mode must be one of the following:

| trig_tag_mode | trig_tag behavior           |
| ------------- | --------------------------- |
|             0 | trigger counter (default)   |
|             1 | clk_i timestamp             |
|             2 | eudet input                 |

### Edge detector

The edge detector can be configured to pick up
either rising or falling edges by writing to 0x03. Each bit corresponds to
an input channel as above in the section on configuring the mask. Here,
0 = positive and 1 = negative edge.

### Per-channel delay

The external inputs can be individually delayed (relative to the eudet input).
Write the delay in clk_i cycles (max of 8) to the appropriate address for a
particular input:

| Address | Channel |
| ------- | ------- |
| 0x7     | ext[3]  |
| 0x6     | ext[2]  |
| 0x5     | ext[1]  |
| 0x4     | ext[0]  |

Note that the synchronizer/edge detector introduce a 6 clk_i cycle
delay for all external inputs even when the channel delay is set to 0

### Deadtime

The unit does not respond to inputs for a configurable amount of time
after a trigger. To configure this, write the number of clk_i cycles
to 0x8.

## Eudet stuff

(to be written by Timon)
