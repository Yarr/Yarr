# Yarr-fw
This firmware is made for the XpressK7 and the SPEC board. This document will explain step by step how to get the firmware working and launch testing programs.

## Cores docs

* Wishbone-Express core : (https://github.com/Yarr/Yarr-fw/blob/master/rtl/kintex7/wbexp-core/README.md)
* DDR3K7 core : (https://github.com/Yarr/Yarr-fw/tree/master/rtl/kintex7/ddr3k7-core/README.md)

## SPEC
Firmware bit-files for the SPEC card can be found in `syn/spec/`

### Flahing the SPEC card
You can use the tool supplied with the YARR sw (https://github.com/Yarr/Yarr):
```bash
$ bin/program path/to/file/spec.bit
```

## XpressK7

All the documentation about the Xpress is on this page.
(https://github.com/Yarr/Yarr-fw/blob/master/syn/xpressk7/README.md)
