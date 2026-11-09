# AlphaRNG Software Kit

This repository contains the AlphaRNG Software Kit, which provides access to the AlphaRNG device over a secure channel when used with Linux, macOS, FreeBSD, and 64-bit Windows.

The AlphaRNG is a true random number generator that plugs directly into any Linux, macOS, FreeBSD, or 64-bit Windows computer via USB.
The AlphaRNG device can be integrated with hardware platforms such as Raspberry Pi 4/5 and with ARM based hardware design solutions that run embedded Linux.

Starting with v3.0, the kit also supports the AlphaRNG PQ. It uses the same hardware as the AlphaRNG but sets up its secure session with ML-KEM-1024 (FIPS 203) post-quantum key exchange instead of RSA. The kit detects the device type automatically.

* [AlphaRNG quick start guide](https://tectrolabs.com/docs/alpharng/quick-start/)
* [AlphaRNG Documentation](https://tectrolabs.com/docs/alpharng/)
* [More about AlphaRNG](https://tectrolabs.com/alpharng/)
* [More about AlphaRNG PQ](https://tectrolabs.com/alpharng-pq/)

## Contents

* `linux` contains all necessary files and source code for building the `alrandom` kernel module/driver used with Linux distributions. The driver allows concurrent access to AlphaRNG entropy data streams from user space.
* `linux-and-macOS/alrng` contains all necessary files and source code for building the `alrng`, `alseqgen`, `alrngdiag`, `alperftest`, `bitgrps`, `sample` and `sample_c` utilities used with Linux, FreeBSD, and macOS systems, the optional OpenSSL 4 provider, and the unit tests. It also includes the `run-alrng-pserver.sh` script for running a named pipe server on Linux based systems.
* `windows-x64` contains all necessary files and source code for building the `alrng.exe`, `alseqgen.exe`, `alrngdiag.exe`, `alperftest.exe`, `entropy-server.exe`, `entropy-client-test.exe`, `entropy-client-sample.exe`, `sample.exe` and `sample_c.exe` utilities for Windows 10/11 (64-bit) and Windows Server 2016/2019 (64-bit) using Visual Studio 2019 or newer.
* `windows-dll` contains all necessary files and source code for building the `AlphaRNG-64.dll` library for Windows 10/11 (64-bit) and Windows Server 2016/2019 (64-bit) using Visual Studio 2019 or newer. Windows applications built with different programming languages can concurrently access the AlphaRNG entropy server through a unified API.



## License

The AlphaRNG Software Kit is provided under the terms in [LICENSE](LICENSE). It includes third-party software, mlkem-native for ML-KEM-1024, and uses OpenSSL. Their copyright notices and license terms are in [THIRD-PARTY-NOTICES.txt](THIRD-PARTY-NOTICES.txt), which must be included with any distribution of the kit.

## Authors

Andrian Belinski 
