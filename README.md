# Installation

```
git clone https://github.com/Lurkki14/tuxclocker-cli
cd tuxclocker-cli
./autogen.sh
mkdir build
cd build
../configure
make
sudo make install
```

Libraries are installed into /usr/local/lib and you must specify this path as LD_LIBRARY_PATH=/usr/local/lib when running

# Running

```
tuxclocker_cli <option>
```
Run with --help for help.
