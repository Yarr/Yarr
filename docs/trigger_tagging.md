# Trigger Tagging Guide

"Trigger tagging" is the association of sequential tags for every trigger sent to the chip, at the firmware level. This has been implemented in both the firmware and software, but it will require re-flashing a trigger-tagging specific bitfile until it is fully integrated during the next firmware release.

## When to use trigger tagging

Some types of scans require that trigger tagging be disabled, and others benefit from it. Including trigger tagging can greatly improve the maximum rate, L0 timing distribution, and frequency of dropped events at high rate. 

### Correct Usage
- Noisescans
- Externally triggered scans

### Incorrect Usage
- Digital/analog scans
- Selftrigger scans
- pToT scans
- ToT scans
- threshold scans
- **Anything** not listed in the first category above!

## Usage guide

### 1. Firmware

To flash the firmware, refer to the [PCIe Firmware and Hardware Guide](fw_guide.md). Remember to restart the PC.

### 2. Software

To enable trigger tagging, navigate to the `Spec` card controller config you are using. It is typically advisable to make a copy of the configuration file with the suffix `_trigtag` to indicate that it has trigger tagging enabled, as it is easy to forget to switch on/off the trigger tagging functionality. Trigger tagging is disabled by default.

Once it is enabled, you must add the key-value pair `"triggerEncoderEnable": 1` to the `trigConfig` section of your controller config. 

```json
{
    "ctrlCfg": {
        "cfg": {
            "autoZero": {
                "interval": 500,
                "word": 0
            },
            "cmdPeriod": 6.66666677417993e-9,
            "delay": [10, 26, 12, 10, 14, 16, 15, 15, 13, 14, 14, 13, 0, 0, 0, 0],
            "rxActiveLanes": 15,
            "rxPolarity": 65535,
            "specNum": 0,
            "spiConfig": 541200,
            "trigConfig": {
                "config": 65534,
                "deadtime": 1200,
                "delay": [0, 0, 0, 0],
                "edge": 0,
                "mask": 1,
                "mode": 0,
                "triggerEncoderEnable": 1,
                "triggerEncoderMultiplier": 8
            }
        },
        "type": "spec"
    }
}
```

You may also specify a `triggerEncoderMultiplier` size, which sets the trigger window size. **It is important to note that this is the only way to set the trigger multiplier while using trigger tagging** -- the scan config will not work for this.

### Troubleshooting

- **Trigger multiplier is not correct:** Remember to set trigger multiplier with the `triggerEncoderMultiplier` parameter in the controller configuration file, as descibed above.
- **No output from noisescans:** Try resetting the FPGA using `specSoftReset` as follows:
    ```bash
    ./bin/specSoftReset -r <controller_config> -o 15
    ```