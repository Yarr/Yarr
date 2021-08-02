# Star testing

## Test program

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

For comparison purposes, a barebones interface to the itsdaq FW is provided:

```bash
bin/star_test config/controller/itsdaq.json
```

The emulator version can be run as follows:

```bash
bin/test_star -r 0 configs/controller/emuCfg_star.json
```

## Loop Actions

List of available loop actions and their configuration parameters.

### StarTriggerLoop

The Star version of the trigger loop is similar to other variants.
This will repeatedly send a particular command to the front end. 

All config parameters are optional, see defaults:
    
- l0_latency ``<int>``: time in bunch crossings between injection and trigger (default 45)
- noInject ``<bool>``: if false, inject calibration charge. if true, no injection (default false)
- digital ``<bool>``: if true inject digital pulse instead of calibration pulse (default false)
- trig_count ``<int>``: number of injections, if 0 will run for specified time (trig_time) (default 50)
- trig_frequency ``<int>``: trigger(/injection)frequency in Hz (default 1000)
- trig_time ``<int>``: time in seconds, if count is set to 0 will run for this amount of time (default 10)

### StarMaskLoop

The Star version of the mask loop allows for iteration over various mask
settings. There are a few different modes depending on how the flags are set.

The standard mode loops over a set of strips and allows charge injection to
all enabled strips. The number of strips is given by the range of the scan
(min to max).

A second mode allows for cross-talk measurements. This sets the mask and
calibration mask registers differently. All strips in a group will be enabled,
but charge injection is enabled on a smaller number of strips.

Finally, in nmask mode, the bin number corresponds to the number of channels
that are enabled, this makes little sense except with the parameter flag set.

Required config parameters:
    
- min ``<int>``: start bin
- max ``<int>``: end bin
- step ``<int>``: step size through possible bins
- nMaskedStripsPerGroup ``<int>``: number of strips that are masked in each group
- nEnabledStripsPerGroup ``<int>``: number of strips with charge injection enabled in each group
- EnabledMaskedShift ``<int>``: offset of the enabled strips with the group

Optional config parameters:

- parameter ``<bool>``: if true this identifies as a parameter loop so that data from each step will be placed in a different bin (default: mask loop)
- maskOnly ``<bool>``: if true only the mask register is written, the calibration register remains unchanged (default: false)
- doNmask ``<bool>``: if true run in nmask mode (default: false)

## Configuration parameters

Both StdParameterLoop and the list of parameters in the prescan field of the
scan configuration use the same names for parameters. These are formatted as
follows.

Most of the parameters are named directly from register fields in the ASIC
documentation. A loop over a field in the ABC is named ABCs_{NAME}, and one
in the HCC is named HCC_{NAME}.

Some of the more commonly used ones:

- ABCs_STR_DEL: strobe delay
- ABCs_BVT: discriminator threshold
- ABCs_BTRANGE: trim range
- HCC_CFD_PRLP_FINEDELAY: fine delay of the PRLP output from HCC

A special virtual register is ABCs_MASKs. If this is set to 1, all masks are
enabled (resulting in 0 outpu), otherwise all masks are disabled.
