# Installation

```
git clone https://github.com/Lurkki14/tuxclocker-cli
cd tuxclocker-cli
meson build --prefix=/usr
cd build
ninja
ninja install
```

This is assuming your prefix is `/usr`, consult your distribution's documentation to find out if that's correct for your distribution.

# Running

```
tuxclocker_cli <option>
```
Run with --help for help.
