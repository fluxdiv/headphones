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

- `git clone`
- Generate build directory
  - `mkdir build && cd build`
- Run cmake to generate makefile
  - `cmake ..`
- Run makefile to build
  - `make`
- Run listener (socket requires elevated permissions)
  - `sudo ./listen eth0`

## Usage

Running the listener
```sh
sudo ./listen eth0
```
Check current [promiscuous mode](https://en.wikipedia.org/wiki/Promiscuous_mode)
```sh
sudo ./listen pget
```
Toggling [promiscuous mode](https://en.wikipedia.org/wiki/Promiscuous_mode) on/off
```sh
sudo ./listen pon
sudo ./listen poff
```


