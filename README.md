# Headphones

A basic packet listener. Primarily a learning project.

## Goals

- [ ] Form a basic mental model about how userspace programs can interact with the kernel
- [ ] Learn about & gain experience with raw syscalls (specifically mmap)
- [ ] Learn about & gain experience with unix sockets

## Credits

- https://www.kernel.org/doc/Documentation/networking/packet_mmap.txt
- https://github.com/jrdriscoll/packet_mmap
- Chetan Loke (2011) - lolpcap, Licensed under the GNU General Public License v2.0 (GPL-2.0)
- Daniel Borkmann

## Requirements

- cmake
- [fmt](https://github.com/fmtlib/fmt)

## Building

- git clone
- Generate build directory
  - `mkdir build && cd build`
- Run cmake to generate makefile
  - `cmake ..`
- Run makefile to build
  - `make`
- Run listener
  - `./listen eth0`

