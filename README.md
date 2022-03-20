# AlphaRNG Software Kit

This repository contains the AlphaRNG Software Kit, which provides access to the AlphaRNG device when used with Linux, macOS, freeBSD and 64 bit Windows
over a secure channel.

AlphaRNG device is a true random number generator that plugs directly into any Linux, macOS, freeBSD and 64 bit Windows computer via USB. 
The AlphaRNG device works with virtual machines using software such as VMware Player, VMware ESXi, VirtualBox, and Parallels. 

* [AlphaRNG quick start guide](https://tectrolabs.com/docs/alpharng/quick-start/)
* [More about AlphaRNG](https://tectrolabs.com/alpharng/)

## Contents

* `linux-and-macOS/alrng` contains all necessary files and source code for building `alrng`, `alrngdiag`, `alperftest` and `sample` utilities used with Linux, FreeBSD and macOS distributions. It also includes the run-alrng-pserver.sh script for running a named pipe server on Linux based systems.
* `windows-x64` contains all necessary files and source code for building `alrng.exe`, `alrngdiag.exe`, `alperftest`, `entropy-server.exe`, `entropy-client-test`, `entropy-client-sample` and `sample.exe` utilities for Windows 10 (64 bit) and Windows Server 2016/2019 (64 bit) using Visual Studio 2019 or newer.
* `windows-dll` contains all necessary files and source code for building `AlphaRNG-64.dll` library for Windows 10 (64 bit) and Windows Server 2016/2019 (64 bit) using Visual Studio 2019 or newer. Windows application that are built using different programming languages can concurrently access AlphaRNG entropy server through a unified API.



## Authors

Andrian Belinski 
