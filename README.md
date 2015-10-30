KTX: a Quakeworld&trade; server mod
===

[KTX] is a popular **Quakeworld&trade;** server modification, adding numerous features to the core features of the server.

Although it had been developed to be **Quakeworld&trade;** server agnostic, it has over the years been developed very close to [MVDSV] to which it has become an extent, thus compatibility with other **Quakeworld™** servers might not have been maintained.

Current code status: [![Build Status](https://drone.io/github.com/qwassoc/ktx/status.png)](https://drone.io/github.com/qwassoc/ktx/latest)

Features
----
*To be detailled*


Build from source
----

Linux:

```
./configure
make build-dl
```

Now, you should have the qwprogs.so. Copy it to your ktx game directory of your quakeworld server.


Building KTX as VM bytecode
----

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
----
All files inside this repository are licensed under the Gnu General Publice License v2 (GPLv2). You wil find a copy of the license in the LICENSE file.



[KTX]:https://github.com/qwassoc/ktx
[MVDSV]:https://github.com/qwassoc/mvdsv
