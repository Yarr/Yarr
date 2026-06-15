# Python Bindings

YARR provides Python bindings to the `ScanConsole` interface via [pybind11](https://pybind11.readthedocs.io). This allows running scans and accessing results from Python scripts.

## Building

Python bindings are enabled by default. They require Python 3 development headers. To build:

```bash
mkdir build && cd build
cmake .. -DYARR_ENABLE_PYTHON=on
make -j$(nproc) install
cd ..
```

The shared library `_pyyarr.so` is installed alongside the `pyYARR` package wrapper in the `lib/` directory.

## Setting up the Python path

Add the installed library directory to `PYTHONPATH` so Python can find the module:

```bash
export PYTHONPATH=$PWD/lib:$PYTHONPATH
```

Verify the import works:

```bash
python3 -c "import pyYARR; print(pyYARR.version())"
```

## Running a scan

The Python API mirrors the `scanConsole` command-line steps. The `python/scan_cli.py` script is a minimal working example:

```python
import sys
import pyYARR

con = pyYARR.ScanConsole()

# Pass the same arguments as you would to bin/scanConsole
con.init([
    "scan_cli",
    "-r", "configs/controller/emuCfg_itkpixv2.json",
    "-c", "configs/connectivity/example_itkpixv2_setup.json",
    "-s", "configs/scans/itkpixv2/std_digitalscan.json",
])

con.loadConfig()
con.initHardware()
con.configure()
con.setupScan()
con.run()
con.cleanup()
con.plot()
```

Or equivalently, from the command line:

```bash
python3 python/scan_cli.py \
  -r configs/controller/emuCfg_itkpixv2.json \
  -c configs/connectivity/example_itkpixv2_setup.json \
  -s configs/scans/itkpixv2/std_digitalscan.json
```

## Available API

| Symbol | Description |
|---|---|
| `pyYARR.ScanConsole()` | Create a scan controller instance |
| `con.init(argv)` | Parse command-line arguments (list of strings) |
| `con.loadConfig()` | Load controller, connectivity, and scan configs |
| `con.initHardware()` | Open and initialise the hardware controller |
| `con.configure()` | Write the chip configuration |
| `con.setupScan()` | Set up the scan loop stack |
| `con.run()` | Execute the scan |
| `con.cleanup()` | Release hardware resources |
| `con.plot()` | Generate output plots (requires `-p` passed to `init`) |
| `con.getResults()` | Return scan results as a JSON-serialised string |
| `con.getConfig()` | Return the merged config as a JSON-serialised string |
| `con.dump()` | Print config summary to stdout |
| `pyYARR.parseConfig(path)` | Parse a JSON config file and return it as a string |
| `pyYARR.setupLogger(path)` | Configure logging from a logger config file |
| `pyYARR.getLog()` | Return the current log output as a string |
| `pyYARR.version()` | Return the YARR version string (JSON) |

## Notes

- All `ScanConsole` methods release the Python GIL during execution, so they can be called from a multi-threaded Python program without blocking the interpreter.
- The scan output files (histograms, `scanLog.json`, chip configs) are written to disk exactly as with the command-line tool. See [Output Format](output_format.md) for details.
