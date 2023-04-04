# Plotting Tools

## Installation and compilation

If ROOT is installed Yarr will automatically compile and install [Plotting-Tools](https://gitlab.cern.ch/YARR/utilities/plotting-tools), this is the same library used to make ROOT plots as used by the local-DB.
To skip the installation of the plotting tools, see [software install](install.md).

If ROOT is not installed (you can check by calling ``$ root``), you can install it via
```bash
$ sudo yum install root
```

Once installed remove your build folder and restart the [software install](install.md) from the compilation stage.

The minimum version requirement for ROOT is v6.0.8. We recommend v6.24 or higher.

## Usage

Please refer to [Plotting-Tools](https://gitlab.cern.ch/YARR/utilities/plotting-tools) for detailed documentation on how to run the code.

A quick usage guide is:
```bash
$ bin/plotFromDir -i data/last_data -P png
```
