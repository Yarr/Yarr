# Plotting Tools

## Installation and compilation

If ROOT is installed Yarr will automatically compile and install [Plotting-Tools](https://gitlab.cern.ch/YARR/utilities/plotting-tools), this is the same library used to make ROOT plots as used by the local-DB.

If ROOT is not installed (you can check by calling ``$ root``), you can install it via
```bash
$ sudo yum install root
```

Once installed remove your build folder and restart the [software install](install.md) from the compilation stage.

## Usage

Please refer to [Plotting-Tools](https://gitlab.cern.ch/YARR/utilities/plotting-tools) for detailed documentation on how to run the code.

A quick usage guide is:
```bash
$ bin/plotFromDir -i data/last_data -P png
```
