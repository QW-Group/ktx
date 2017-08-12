KTX: a Quakeworld&trade; server mod
===================================

[KTX] is a popular **Quakeworld&trade;** server modification, adding numerous features to the core features of the server.

Although it had been developed to be **Quakeworld&trade;** server agnostic, it has over the years been developed very close to [MVDSV] to which it has become an extent, thus compatibility with other **Quakeworld™** servers might not have been maintained.

Current code status: [![Build Status](https://drone.io/github.com/qwassoc/ktx/status.png)](https://drone.io/github.com/qwassoc/ktx/latest)

Features
--------
*To be detailled*


Build from source (old way)
---------------------------

Linux:

```
./configure
make build-dl
```

Now, you should have the qwprogs.so. Copy it to your ktx game directory of your quakeworld server.


Build from source with meson
----------------------------

Detailed commands to install packages, tools and compilation can be found in ``.travis.yml`` file.
There are extra conditionals to install desired packages based on the TARGET.

In general:

- use Ubuntu 14.04 (but should work under 16.04 as well) as virtual machine, check out source code there
- install required packages for compilation
- set up virtualenv and install python packages (required for meson and ninja builders)
- run meson build for given directory (optionally with cross compilation settings)
- run ninja to generate .so file
- you should have ``qwprogs.so`` file in ``build_*`` directory, put it in your quake server/ktx/ directory.

You should be able to compile binaries for most popular platforms, such as:

- Linux 32-bit and 64-bit
- Windows 32-bit and 64-bit (WoW64)
- Arm 7 - for RaspBerry Pi 3 with Raspbian

Example builiding under Ubuntu 14.04 binaries for Raspberry Pi 3 (Raspbian):

Install required packages:

```bash
$ sudo apt-get update
$ sudo apt-get -y upgrade
$ sudo apt-get -y install build-essential python-virtualenv python3-dev python3-pip ninja-build cmake gcc-multilib
```

Install required packaes specifically for arm architecture for Raspberry Pi 3 (Raspbian):

```bash
$ sudo apt-get -y install gcc-arm-linux-gnueabihf pkg-config-arm-linux-gnueabihf
```

Check out the code to the current directory:

```bash
git clone https://github.com/deurk/mvdsv.git .
```

Create virtualenv + install python packages:

```bash
$ virtualenv .venv --python=python3
$ . .venv/bin/activate
$ pip3 install --upgrade pip
$ pip3 install -r requirements.txt
```

Export env var to define what target to compile, run the build commands.


```bash
$ export TARGET=linux-armv7hl
$ rm -rf build_${TARGET}

$ meson build_${TARGET} --cross-file cross-compilation_${TARGET}.txt
The Meson build system
Version: 0.41.2
Source dir: /home/kaszpir/src/deurk/ktx
Build dir: /home/kaszpir/src/deurk/ktx/build_linux-armv7hl
Build type: cross build
Project name: ktx
Native c compiler: cc (gcc 5.4.0)
Cross c compiler: arm-linux-gnueabihf-gcc (gcc 5.4.0)
Host machine cpu family: arm
Host machine cpu: armv7hl
Target machine cpu family: arm
Target machine cpu: armv7hl
Build machine cpu family: x86_64
Build machine cpu: x86_64
Library m found: YES
Build targets in project: 1

$ ninja -C build_${TARGET}

ninja: Entering directory `build_linux-armv7hl'
[25/99] Compiling C object 'qwprogs@sha/src_bot_movement.c.o'.
../src/bot_movement.c: In function ‘ApplyPhysics’:
../src/bot_movement.c:191:10: warning: unused variable ‘dot_prod’ [-Wunused-variable]
    float dot_prod = 0.0f;
          ^
[80/99] Compiling C object 'qwprogs@sha/src_race.c.o'.
../src/race.c: In function ‘race_finish_capture’:
../src/race.c:3766:9: warning: unused variable ‘race_time’ [-Wunused-variable]
   float race_time = race_match_mode() ? player_match_info[player_num].best_time : race.currentrace[player_num].time;
         ^
[99/99] Linking target qwprogs.so.
```

Check the output binary file:

```bash
$ find build_${TARGET}/ -type f -name "qwprogs.*" -exec file {} \;
build_linux-armv7hl/qwprogs.so: ELF 32-bit LSB shared object, ARM, EABI5 version 1 (SYSV), dynamically linked, BuildID[sha1]=5d5ff9fd0172ef1aa929c1704a6a16b68906641f, not stripped

```

In ``build_*/`` there will be ``qwprogs.so``  or ``qwprogs.dll`` binary, change permissions to executable and copy it to quake/ktx/ directory to start quake server with ktx mod enabled.


Known issues:

- When using cross compiling between 32bit and 64bit architecture make sure to reinstall *dev packages or run in chroot. See ``.travis.yml`` lines, there is ``apt-get remove`` command for this, because curl and pcre are in dependency but not required.
- When changing architecture builds, for example for arm, apt-get will install/remove conflicting packages. Don't be surprised that you compile ``linux-amd64``, then ``linux-armv7hl`` and then back ``linux-amd64`` and it does not work because files are missing :)


Building KTX as VM bytecode
---------------------------

If your qw server is [MVDSV] you may want to build KTX as a quake virtual machine. Virtual machine building needs Quake 3 bytecode assembly tools, namely q3asm and q3lcc. We packaged those inside the tools directory. It is up to you to build them.

But, if you have trouble building it may be easier, you grab the ioquake3 source code from github and build it. Ioquake3 is a maintained and advanced quake 3 client based on the quake 3 source code. It has lots of new features, stability and cats.

```
git clone https://github.com/ioquake/ioq3/
cd ioq3
make
```

q3asm and q3lcc are now inside the build/ directory. Copy them to a path where your shell can find it.

Get back to the ktx source directory and type:

```
./configure
make build-vm
```

Configure should have found q3asm and q3lcc. After make, you should find qwprogs.vm in the directory. Copy it to your ktx game directory of your quakeworld server.



License
-------
All files inside this repository are licensed under the Gnu General Publice License v2 (GPLv2). You wil find a copy of the license in the LICENSE file.



[KTX]:https://github.com/qwassoc/ktx
[MVDSV]:https://github.com/qwassoc/mvdsv
