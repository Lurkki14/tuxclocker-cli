# Installation

## Build options

Build options for meson:

```
-Dwith_amd=true
```

Build the AMD library.

```
-Dwith_nvidia=true
```

Build the Nvidia library.

Example:

```
meson build --prefix=/usr -Dwith_amd=true
```

## Compilation and installation

```
git clone https://github.com/Lurkki14/tuxclocker-cli
cd tuxclocker-cli
meson build --prefix=/usr [options]
cd build
ninja
ninja install
```

This is assuming your prefix is `/usr`, consult your distribution's documentation to find out if that's correct for your distribution.

# Running

```
tuxclocker-cli <options>
```
Run with 'help' for help.
