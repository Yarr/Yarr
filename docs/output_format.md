# Scan Output Format

Each scan produces a timestamped directory under `data/` and a `last_scan` symlink pointing to it.

## Directory layout

```
data/
├── last_scan -> 000042_std_digitalscan/   # symlink to most recent scan
└── 000042_std_digitalscan/
    ├── scanLog.json                        # full run record (configs + metadata)
    ├── std_digitalscan.json                # copy of the scan config used
    ├── 0x10AA6.json.before                 # chip config before the scan
    ├── 0x10AA6.json.after                  # chip config after the scan
    ├── 0x10AA6_OccupancyMap.json           # per-chip histogram output
    ├── 0x10AA6_EnMask.json
    └── ...
```

The directory name is `<run_number>_<scan_name>`, where the run number is a zero-padded 6-digit integer incremented from `$HOME/.yarr/runCounter`.

## Run counter

The global run counter is stored as a plain integer in `$HOME/.yarr/runCounter`. It is incremented automatically at the start of each scan. To reset it (e.g. for reproducible test output):

```bash
truncate $HOME/.yarr/runCounter --size=0
```

The next scan will then start at run number `000001`.

## scanLog.json

`scanLog.json` records the full configuration of the run and is suitable for reproducing it exactly:

```
{
  "runNumber": 42,
  "ctrlCfg": { ... },        # controller config as used
  "connectivity": [ ... ],   # connectivity config as used
  "scan": { ... },           # scan config as used
  "ctrlStatus": { ... }      # hardware status at run time
}
```

You can re-run a scan from the log using JSON fragment references:

```bash
bin/scanConsole \
  -r data/last_scan/scanLog.json#/ctrlCfg \
  -c data/last_scan/scanLog.json#/connectivity/0 \
  -s path/to/scan.json
```

## Histogram JSON format

Each histogram file is named `<chip_serial>_<histogram_name>.json`. The format is:

```json
{
  "Name":      "OccupancyMap",
  "Type":      "Histo2d",
  "Entries":   153600,
  "Overflow":  0.0,
  "Underflow": 0.0,
  "x": { "Bins": 400, "Low": 0.5, "High": 400.5, "AxisTitle": "Column" },
  "y": { "Bins": 384, "Low": 0.5, "High": 384.5, "AxisTitle": "Row" },
  "z": { "AxisTitle": "Hits" },
  "Data": [
    [100.0, 100.0, ...],   // row 0, one float per column
    [100.0, 100.0, ...],   // row 1
    ...
  ]
}
```

`Data` is a 2D array of **floats** indexed `[row][col]`. For `Histo1d` the `Data` key is a flat array and the `y`/`z` axis keys are absent.

## Chip config snapshots

`<chip_serial>.json.before` and `<chip_serial>.json.after` capture the full chip register state immediately before and after the scan. Comparing the two shows any register changes applied by the scan (e.g. tuned TDAC values).
