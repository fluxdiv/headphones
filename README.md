# Headphones

A basic packet listener. Primarily a learning project.

## Goals

- [X] Form a basic mental model about how userspace programs can interact with the kernel
- [X] Learn about & gain experience with raw syscalls (specifically mmap)
- [X] Learn about & gain experience with unix sockets

## Credits

- Chetan Loke (2011) - lolpcap, Licensed under the GNU General Public License v2.0 (GPL-2.0)
- Daniel Borkmann
- https://www.kernel.org/doc/Documentation/networking/packet_mmap.txt
- https://github.com/jrdriscoll/packet_mmap

## Requirements

- cmake
- ncurses
- [fmt](https://github.com/fmtlib/fmt)

## Building

```shell
git clone https://github.com/fluxdiv/headphones
# create build directory
mkdir build && cd build
# run cmake to generate makefile
cmake ..
# run makefile to build
make
# run listener (internal socket creation requires elevated perms)
sudo ./listen eth0
```

## Usage

Running the listener
```shell
# running the listener
sudo ./listen eth0
# check current promiscuous mode
sudo ./listen pget
# toggle promiscuous mode on/off
sudo ./listen pon
sudo ./listen poff
```
- [promiscuous mode](https://en.wikipedia.org/wiki/Promiscuous_mode)
