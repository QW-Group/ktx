# KTX: a QuakeWorld server modification

**[KTX](https://github.com/deurk/ktx)** (Kombat Teams eXtreme) is a popular **QuakeWorld** server modification, adding numerous features to the core features of the server.

Although it had been developed to be **Quakeworld** server agnostic, it has over the years been developed very close to **[MVDSV](https://github.com/deurk/mvdsv)** to which it has become an extent, thus compatibility with other **Quakeworld** servers might not have been maintained.

_(This README.md file is still a work in progress. bear with us while we polish it!)_

## Getting Started

The following instructions will help you get **[KTX](https://github.com/deurk/ktx)** installed on a running **[MVDSV](https://github.com/deurk/mvdsv)** server using prebuilt binaries. Details on how to compile your own **[KTX](https://github.com/deurk/ktx)** binary will also be included to match specific architectures or for development purposes.

## Supported architectures

The following architectures are fully supported by **[KTX](https://github.com/deurk/ktx)** and are available as prebuilt binaries:
* Linux i686 (Intel and AMD 32-bit processors)
* Linux amd64 (Intel and AMD 64-bits processors)
* Windows x86 (Intel and AMD 32-bit processors)
* Windows x64 (Intel and AMD 64-bits processors)
* Mac OS X (Intel 64-bit processors)
* Linux armhf (ARM 32-bit processors)

## Prerequisites

TBD

## Installing

For more detailed information we suggest [nquake/server-linux](https://github.com/nQuake/server-linux), which uses [MVDSV](https://github.com/deurk/mvdsv) ad QuakeWorld server.

## Building binaries

### Build from source with meson

Detailed commands to install packages, tools and compilation can be found in ``.travis.yml`` file.
There are extra conditionals to install desired packages based on the TARGET.

In general:

- use Ubuntu 14.04 (but should work under 16.04 as well) as virtual machine, check out source code there
- install required packages for compilation
- set up virtualenv and install python packages (required for meson and ninja builders)
- run meson build for given directory (optionally with cross compilation settings)
- run ninja to generate the binary file
- you should have ``qwprogs.so`` file in ``build_*`` directory.

Example for Linux amd64 under Ubuntu 14.04 (should be similar under 16.04)

Install required packages:

```bash
$ sudo apt-get update
$ sudo apt-get -y upgrade
$ sudo apt-get -y install build-essential python-virtualenv python3-dev python3-pip ninja-build cmake gcc-multilib
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
$ rm -rf build_${TARGET}

$ meson build_${TARGET} --cross-file tools/cross-compilation/${TARGET}.txt
The Meson build system
Version: 0.41.2
Source dir: /home/kaszpir/src/deurk/ktx
Build dir: /home/kaszpir/src/deurk/ktx/build_linux-linux-amd64
Build type: cross build
Project name: ktx
Native c compiler: cc (gcc 5.4.0)
Cross c compiler: gcc (gcc 5.4.0)
Host machine cpu family: x86_64
Host machine cpu: x86_64
Target machine cpu family: x86_64
Target machine cpu: x86_64
Build machine cpu family: x86_64
Build machine cpu: x86_64
Dependency threads found: YES
Library m found: YES
Build targets in project: 1

$ ninja -C build_${TARGET}

ninja: Entering directory `build_linux-amd64'
[46/46] Linking target qwprogs.so.

```

Check the output binary file:

```bash
$ file build_${TARGET}/qwprogs.so
qwprogs.so: ELF 64-bit LSB shared object, x86-64, version 1 (SYSV), dynamically linked, BuildID[sha1]=5bd27876114dbf4b0dcf6a190c90f5e800ef480c, not stripped

```

In ``build_*/`` there will be ``qwprogs.so`` binary, copy it to your quake server.

Known issues:

- When changing architecture builds, for example for arm, apt-get will install/remove conflicting packages. Don't be surprised that you compile ``linux-amd64``, then ``linux-armv7hl`` and then back ``linux-amd64`` and it does not work because files are missing :)


## Built With

TBD

## Versioning

We use a pretty crappy system for versioning for now. For the versions available, see the [tags on this repository](https://github.com/deurk/ktx/tags).

## Authors

(Listed by last name alphabetic order)

* **Ivan Bolshunov** - *qqshka*
* **Dominic Evans** - *oldman*
* **Anton Gavrilov** - *tonik*
* **Andrew Grondalski** - *ult*
* **Dmitry Musatov** - *disconnect*
* **Alexandre Nizoux** - *deurk*
* **Tero Parkkonen** - *Renzo*
* **Vladimir Vladimirovich** - *VVD*

## Contributing

Please read [CONTRIBUTING.md](CONTRIBUTING.md) for details about how to contribute to **[KTX](https://github.com/deurk/ktx)**.

## Code of Conduct

We try to stick to our code of conduct when it comes to interaction around this project. See the [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md) file for details.

## License

This project is licensed under the GPL-2.0 License - see the [LICENSE.md](LICENSE.md) file for details.

## Acknowledgments

* Hat tip to anyone who's code was used
* Inspiration
