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

### Build from source with CMake

Assuming you have installed essential build tools and ``CMake``
```bash
mkdir build && cmake -B build . && cmake --build build
```
Build artifacts would be inside ``build/`` directory, for unix like systems it would be ``qwprogs.so``.

You can also use ``build_cmake.sh`` script, it mostly suitable for cross compilation
and probably useless for experienced CMake user.
Some examples:
```
./build_cmake.sh linux-amd64
```
should build KTX for ``linux-amd64`` platform, release version, check [cross-cmake](tools/cross-cmake) directory for all platforms

```
B=Debug ./build_cmake.sh linux-amd64
```
should build KTX for linux-amd64 platform with debug

```
V=1 B=Debug ./build_cmake.sh linux-amd64
```
should build KTX for linux-amd64 platform with debug, verbose (useful if you need validate compiler flags)

```
V=1 B=Debug BOT_SUPPORT=OFF ./build_cmake.sh linux-amd64
```

same as above but compile without bot support

```
G="Unix Makefiles" ./build_cmake.sh linux-amd64
```

force CMake generator to be unix makefiles

```
./build_cmake.sh linux-amd64 qvm
```

build KTX for ``linux-amd64`` and ``QVM`` version, you can provide
any platform combinations.

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

[ktx]: https://github.com/QW-Group/ktx
[ktx-builds]: https://builds.quakeworld.nu/ktx
[mvdsv]: https://github.com/QW-Group/mvdsv
[nquake-linux]: https://github.com/nQuake/server-linux
[discord-qw]: http://discord.quake.world/
