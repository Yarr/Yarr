# Kernel Driver Installation

For any of the PCIe FPGA cards the custom PCIe kernel driver is required. This driver can be found [here](https://gitlab.cern.ch/YARR/yarr-pcie/spec-driver):
```
$ git clone https://gitlab.cern.ch/YARR/yarr-pcie/spec-driver.git
```

<span style="color:red">The installation of the kernel driver is only required once and sometimes after a kernel update.</span>

# Installation

PCIe kernel driver can be installed in the following way:

```bash
$ git clone https://gitlab.cern.ch/YARR/yarr-pcie/spec-driver.git
$ cd spec-driver
$ cd src
$ make
<Lots of text>
$ sudo make install
<Copying files>
$ sudo depmod
<Builds dependencies>
$ sudo modprobe -v specDriver
insmod /lib/modules/3.10.0-229.14.1.el7.x86_64/extra/specDriver.ko
```

## Test

You can test if the driver is installed and loaded properly by looking at the `dmesg` output:

```bash
$ dmesg | grep spec
<Lots of text>
[246519.712618] specDriver: Major 247 allocated to nodename 'spec'
[246519.712637] specDriver: Found SPEC card at 0000:01:00.0
[246519.712689] specDriver: 64bits bus master DMA capable
[246519.712706] specDriver 0000:01:00.0: irq 47 for MSI/MSI-X
[246519.713400] specDriver: Device /dev/spec0 added
[246519.713452] specDriver: Mapped BAR0 at 0xF7900000 with length 1048576
[246519.713495] specDriver: 
[246519.713496] Mapped BAR2 at 0xF7800000 with length 1048576
[246519.713498] specDriver: 
[246519.713499] Mapped BAR4 at 0xF7A00000 with length 4096
[246519.713526] specDriver: 
[246519.713527] Module loaded
```

## Troubleshoot

### Dependency

If you encounter something like
```bash
$ make -C /lib/modules/3.10.0-1127.18.2.el7.x86_64/build M=/home/$USER/Yarr/src/kernel modules
make[1]: *** /lib/modules/3.10.0-1127.18.2.el7.x86_64/build: No such file or directory.  Stop.
make: *** [Makefile:10: default] Error 2
```
then the ``kernel-devel`` package is missing. Install with ``yum install kernel-devel-$(uname -r)``.

### Signing

In case you run into a problem during ``modprobe`` which looks like this: ``modprobe: ERROR: could not insert 'specDriver': Required key not available``. Do the following:
```bash
$ sudo yum install mokutil
$ sudo mokutil -disable-validation
```
This will require you to create a password with at least 8 characters. Reboot after that and the UEFI will ask you to change the security settings. Choose "yes", reenter the password or whatever it asks for.


