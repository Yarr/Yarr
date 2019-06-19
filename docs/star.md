# Star testing

There is preliminary support for the ABC and HCC star ASICs (Front-end for
Strips). Currently only initial testing has been done.
The test program writes an HCC register, and reads back data packets
(HPR will be sent back continuously, once every 1ms).

Using default SPEC hardware:

```bash
bin/star_test
```

Or by specifying a config for communication with felix_core:

```bash
bin/star_test config/controller/netio.json
```
