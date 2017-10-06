# Trigger Logic

## Introduction

The trigger unit has 4 external inputs (ext_trig_i) and 1 eudet input
(see section "Eudet stuff"). The output (ext_trig_o) can be any
boolean function of the 5 inputs. The unit also reads out a trigger
tag (trig_tag) which is either a counter, clk_i timestamp, or the tag
from the eudet_tlu.

## Configuration

Both the trigger logic and tag are configured over the wishbone bus
(wb_* signals). See http://cdn.opencores.org/downloads/wbspec_b4.pdf
for the Wishbone interface specification. The address space looks
like (see following sections for detail):

| Address   | Function                  |
| --------- | ------------------------- |
| 0x00      | Trigger mask              |
| 0x01      | Enable trigger state      |
| 0x02      | Disable trigger state     |
| 0x03      | Trigger tag mode          |
| 0x04      | Full trigger logic config |

### Trigger logic

You can select which input patterns to trigger on one by one, or
compute a configuration word and write it in one transaction.

#### Trigger mask

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

#### Boolean logic, one by one

To trigger on a particular input pattern, write the pattern to 0x01 on
the Wishbone bus. The pattern should look like:

| pattern bit     | Describes input trigger...  |
| --------------- | --------------------------- |
| pattern[31:5]   | (ignored)                   |
| pattern[4]      | eudet_trig_i                |
| pattern[3:0]    | ext_trig_i[3:0]             |

A given pattern matches the current trigger state when they are
bitwise equivalent, ignoring bits [31:5] as well as any bits that are
zero'd out in trig_mask.

To deselect a given pattern, write it to 0x02 on the Wishbone bus.

eg. If we write 0x01 <- 000000011, then 0x01 <- 000000101, we will
trigger on 1.) concidences of ext_trig_i[0] and ext_trig_i[1] when
ext_trig[2] is not active, and 2.) concidences of ext_trig_i[0] and
ext_trig_i[2] when ext_trig[1] is not active.

If we then write 0x02 <- 000000011, we will only trigger on 2.)

#### Boolean logic, one configuration word

To configure all the trigger logic in one Wishbone transaction,
compute the configuration word and write it to 0x04 on the Wishbone
bus. To compute the configuration word, start with all zeros, write
down all the patterns you want to accept (as described in the previous
section), convert them all to numbers (interpret as binary), and set
the corresponding bits of the configuration word.

eg. In the example of the previous section, pattern 1.) corresponds to
bit [3] of the configuration word, and pattern 2.) corresponds to bit
[5], so the same trigger config could have been achieved with:
0x4 <- 000...0000101000

Then to deselect pattern 1.), we can write 0x4 <- 000...0000100000

### Trig tag

To configure the trig_tag output, write trig_tag_mode to 0x04 on the
Wishbone bus. trig_tag_mode must be one of the following:

| trig_tag_mode | trig_tag behavior |
| ------------- | ----------------- |
|             0 | trigger counter   |
|             1 | clk_i timestamp   |
|             2 | eudet input       |

### Edge detector/delayer

All external inputs are sent through a synchronizer to deglitch and
lock the inputs to clk_i, an edge detector, and a shift register to
introduce time delays between the 5 inputs. These are not currently
configurable from outside the wb_trigger_logic component, but within
wb_trigger_logic some code changes can perform the following
configuration tasks:

The edge detector can be configured to pick up
either rising or falling edges by connecting the intermediate signal
edge_ext_trig_i to the appropriate output of the edge detector
component.

The width of the delayers can be chosen (default = 8). This is the max
number of clock cycles a given channel can be delayed. The delay of
each individual channel can be hardcoded. TODO: allow delay config
over WB bus?


## Eudet stuff

(to be written by Timon)
