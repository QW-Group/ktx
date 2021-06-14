# KTX: a QuakeWorld server modification
![KTX Logo](https://raw.githubusercontent.com/deurk/ktx/master/resources/logo/ktx.png)

[![Build Status](https://travis-ci.org/deurk/ktx.svg?branch=master)](https://travis-ci.org/deurk/ktx)
[![Contributor Covenant](https://img.shields.io/badge/Contributor%20Covenant-v2.0%20adopted-ff69b4.svg)](code_of_conduct.md)
[![CII Best Practices](https://bestpractices.coreinfrastructure.org/projects/1256/badge)](https://bestpractices.coreinfrastructure.org/projects/1256)

**[KTX][ktx]** (Kombat Teams eXtreme) is a popular **QuakeWorld** server modification, adding numerous features to the core features of the server.

Although it had been developed to be **Quakeworld** server agnostic, it has over the years been developed very close to **[MVDSV][mvdsv]** to which it has become an extent, thus compatibility with other **Quakeworld** servers might not have been maintained.

## Getting Started

The following instructions will help you get **[KTX][ktx]** installed on a running **[MVDSV][mvdsv]** server using prebuilt binaries. Details on how to compile your own **[KTX][ktx]** binary will also be included to match specific architectures or for development purposes.

## Supported architectures

The following architectures are fully supported by **[KTX][ktx]** and are available as prebuilt binaries:
* Linux amd64 (Intel and AMD 64-bits processors)
* Linux i686 (Intel and AMD 32-bit processors)
* Linux armhf (ARM 32-bit processors)
* Windows x64 (Intel and AMD 64-bits processors)
* Windows x86 (Intel and AMD 32-bit processors)

## Prebuilt binaries

You can find the prebuilt binaries on [this download page][ktx-builds].

## Prerequisites

**[KTX][ktx]** is a server mod and won't run without a proper **Quakeworld** server set up. **[MVDSV][mvdsv]** is the recommended one, but **[FTE][fte]** might work as well (unconfirmed with current code).

## Installing

For more detailed information we suggest looking at the [nQuake server][nquake-linux], which uses **[MVDSV][mvdsv]** and **[KTX][ktx]** as **QuakeWorld** server.

## Building binaries

### Build from source with meson

Detailed commands to install packages, tools and compilation can be found in ``.travis.yml`` file.
There are extra conditionals to install desired packages based on the TARGET.

In general:

- use Ubuntu 18.04 as virtual machine, check out details about it on code on [Travis CI website][travis-build-env]
- install required packages for compilation
- set up virtualenv and install python packages (required for meson and ninja builders)
- run meson build for given directory (optionally with cross compilation settings)
- run ninja to generate the binary file
- you should have ``qwprogs.so``, ``qwprogs.dll`` or ``qwprogs.dylib`` file (depending on the TARGET architecture) in ``build`` directory.

#### Example for Linux amd64

Install required packages:

```bash
$ sudo apt-get update
$ sudo apt-get -y upgrade
$ sudo apt-get -y install build-essential python3-virtualenv python3-dev python3-pip ninja-build cmake gcc-multilib
```

Check out the code to the current directory:

```bash
git clone https://github.com/deurk/ktx.git .
```

Create virtualenv + install python packages:

```bash
$ virtualenv .venv --python=python3
$ . .venv/bin/activate
$ pip3 install --upgrade pip
$ pip3 install -r requirements.txt
```

For more detailed TARGET see ``.travis.yml`` - ``matrix`` section.
Export env var to define what target to compile, run the build commands.

```bash
$ export TARGET=linux-amd64
$ rm -rf build

$ meson build --cross-file tools/cross-compilation/${TARGET}.txt
The Meson build system
Version: 0.58.0
Source dir: /home/taps/workspace/ktx
Build dir: /home/taps/workspace/ktx/build
Build type: cross build
Project name: ktx
Project version: undefined
C compiler for the host machine: gcc (gcc 9.3.0 "gcc (Ubuntu 9.3.0-17ubuntu1~20.04) 9.3.0")
C linker for the host machine: gcc ld.bfd 2.34
C compiler for the build machine: cc (gcc 9.3.0 "cc (Ubuntu 9.3.0-17ubuntu1~20.04) 9.3.0")
C linker for the build machine: cc ld.bfd 2.34
Build machine cpu family: x86_64
Build machine cpu: x86_64
Host machine cpu family: x86_64
Host machine cpu: x86_64
Target machine cpu family: x86_64
Target machine cpu: x86_64
Library m found: YES
Build targets in project: 1

$ ninja -C build

ninja: Entering directory `build'
[100/100] Linking target qwprogs.so.

```

Check the output binary file:

```bash
$ file build/qwprogs.so
qwprogs.so: ELF 64-bit LSB shared object, x86-64, version 1 (SYSV), dynamically linked, BuildID[sha1]=5bd27876114dbf4b0dcf6a190c90f5e800ef480c, not stripped

```

In ``build/`` there will be ``qwprogs.so`` binary, copy it to your quake server.

Known issues:

- When changing architecture builds, for example for arm, apt-get will install/remove conflicting packages. Don't be surprised that you compile ``linux-amd64``, then ``linux-armv7hl`` and then back ``linux-amd64`` and it does not work because files are missing :)

## Versioning

For the versions available, see the [tags on this repository][ktx-tags].

## Authors

(Listed by last name alphabetic order)

* **Ivan** *"qqshka"* **Bolsunov**
* **Dominic** *"oldman"* **Evans**
* **Anton** *"tonik"* **Gavrilov**
* **Andrew** *"ult"* **Grondalski**
* **Paul Klumpp**
* **Niclas** *"empezar"* **Lindström**
* **Dmitry** *"disconnect"* **Musatov**
* **Peter** *"meag"* **Nicol**
* **Andreas** *"molgrum"* **Nilsson**
* **Alexandre** *"deurk"* **Nizoux**
* **Tero** *"Renzo"* **Parkkonen**
* **Joseph** *"bogojoker"* **Pecoraro**
* **Michał** *"\_KaszpiR\_"* **Sochoń**
* **Jonny** *"dimman"* **Svärd**
* **Vladimir** *"VVD"* **Vladimirovich**
* **Florian** *"Tuna"* **Zwoch**

## Contributing

Please read [CONTRIBUTING.md](CONTRIBUTING.md) for details about how to contribute to **[KTX][ktx]**.

## Code of Conduct

We try to stick to our code of conduct when it comes to interaction around this project. See the [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md) file for details.

## License

This project is licensed under the GPL-2.0 License - see the [LICENSE.md](LICENSE.md) file for details.

## Acknowledgments

* Thanks to kemiKal, Cenobite, Sturm and Fang for Kombat teams 2.21 which has served as a base for **[KTX][ktx]**.
* Thanks to **Jon "bps" Cednert** for the **[KTX][ktx]** logo.
* Thanks to the fine folks on [Quakeworld Discord][discord-qw] for their support and ideas.

[ktx]: https://github.com/deurk/ktx
[ktx-tags]: https://github.com/deurk/ktx/tags
[ktx-builds]: https://builds.quakeworld.nu/ktx
[mvdsv]: https://github.com/deurk/mvdsv
[nquake-linux]: https://github.com/nQuake/server-linux
[travis-build-env]: https://docs.travis-ci.com/user/reference/bionic/
[discord-qw]: http://discord.quake.world/
