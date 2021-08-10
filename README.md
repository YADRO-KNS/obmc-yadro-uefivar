# UEFI variable storage
A replacement for the standard UEFI NVRAM storage.
A special support on UEFI side is required.

```
+-------------+       +---------------------------------------------+
|    UEFI     |       |                     BMC                     |
|             |       |                                             |
|             |       | +--------+         +---------+     +------+ |
|             | IPMI  | |  IPMI  |  D-Bus  | UEFIVar |     | JSON | |
| PEI/DXE/SMM |------>| |  OEM   |-------->| service |---->| File | |
|             |       | +--------+         +---------+     +------+ |
|             |       |                                             |
+-------------+       +---------------------------------------------+
```

## Build with OpenBMC SDK
OpenBMC SDK contains toolchain and all dependencies needed for building the
project. See [official documentation](https://github.com/openbmc/docs/blob/master/development/dev-environment.md#download-and-install-sdk) for details.

Build steps:
```sh
$ source /path/to/sdk/environment-setup-arm1176jzs-openbmc-linux-gnueabi
$ mkdir build_dir
$ meson build_dir
$ ninja -C build_dir
```

## Testing
Unit tests can be built and run with OpenBMC SDK.

Run tests:
```sh
$ source /path/to/sdk/environment-setup-arm1176jzs-openbmc-linux-gnueabi
$ # build the project (see above)
$ qemu-arm -L ${SDKTARGETSYSROOT} build_dir/test/uefivar_test
```
