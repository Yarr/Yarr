# Plotting Tools

If ROOT is installed Yarr will automatically install [Plotting-Tools](https://gitlab.cern.ch/YARR/utilities/plotting-tools), this is the same library used to make ROOT plots as used by the local-DB.

If ROOT is not installed (you can check by calling ``$ root``), you can install it via
```bash
$ sudo yum install root
```

Once installed remove your build folder and restart the [software install](install.md) from the compilation stage.
