# AlphaRNG utilities for Linux, FreeBSD and macOS

Source code for the command line utilities and the C/C++ API that access an
[AlphaRNG](https://tectrolabs.com/alpharng/) true random number generator over USB.
Communication with the device goes through a secure channel.

## Contents

| Program | Purpose |
| --- | --- |
| `alrng` | Download entropy or raw random bytes from a device to a file or named pipe |
| `alrngdiag` | Device diagnostics |
| `alperftest` | Performance test |
| `alseqgen` | Generate a random sequence of unique integers within a range |
| `bitgrps` | Statistical analysis of sticky bit groups in a file of random bytes |
| `sample`, `sample_c` | C++ and C examples of using the API |
| `run-alrng-pserver.sh` | Named pipe server script for Linux, see [below](#named-pipe-server) |
| `85-alpharng-usb-access.rules` | udev rules for non-root USB access on Linux |

The API lives in `api-inc/` (headers) and `api-src/` (sources). `AlphaRngApi.h`
is the C++ interface and `AlphaRngApiCWrapper.h` is the C interface.

Run any program with `-h` for its full list of options.

## Requirements

* A C++11 compiler (GCC or Clang)
* OpenSSL development files
  * Debian/Ubuntu/Raspberry Pi OS: `sudo apt install build-essential libssl-dev`
  * macOS: `brew install openssl`
  * FreeBSD: OpenSSL from base or `pkg install openssl`
* CMake 3.10 or newer, if building with CMake (`sudo apt install cmake`, `brew install cmake`)

## Building with CMake

From this directory:

```sh
cmake -S . -B build
cmake --build build -j4
```

The programs are created in `build/`, for example `./build/alrng`.

Optionally install them to `/usr/local/bin` (the `sample` programs are not installed):

```sh
sudo cmake --install build
```

Use `--prefix` to install elsewhere, for example `cmake --install build --prefix ~/.local`.
To remove what was installed (run this before deleting `build/`):

```sh
sudo cmake --build build --target uninstall
```

Notes:

* **macOS:** the Homebrew OpenSSL in `/opt/homebrew/opt/openssl` or `/usr/local/opt/openssl`
  is found automatically. For another location add `-DOPENSSL_ROOT_DIR=<path>` when configuring.
* **FreeBSD:** if the wrong OpenSSL is picked up, add `-DOPENSSL_ROOT_DIR=/usr/local`.
* To start over, delete the `build/` directory.

## Building with make

```sh
make              # Linux and macOS
sudo make install
```

On FreeBSD use `Makefile.bsd`:

```sh
make -f Makefile.bsd
sudo make -f Makefile.bsd install
```

`make clean` removes the build output. Both build systems produce the same programs.

## Running the tests

The unit tests need no device. They are built only when you ask for them:

```sh
cmake -S . -B build -DALRNG_BUILD_TESTS=ON
cmake --build build -j4
cd build && ctest --output-on-failure
```

Each test file in `tests/` is its own program (for example `build/tests/test_aes`), which you can also run
directly to see every test by name. They use a small framework in `tests/test_framework.h`, so there is nothing
extra to install.

| Suite | What it checks |
| --- | --- |
| `sha` | SHA-256 and SHA-512 against known answers |
| `hmac` | HMAC-SHA256, HMAC-SHA1 and HMAC-MD5 against known answers, and key handling |
| `aes` | AES-128/256-GCM round trips, rejection of tampered data and tags, and agreement with OpenSSL |
| `rsa` | The embedded device public keys, key files, generated keys, and encryption and decryption |
| `health` | The Repetition Count and Adaptive Proportion health tests, with good and with stuck data |
| `range_sequence` | Unique, in-range, evenly distributed random sequences, using a repeatable entropy source |
| `app_arguments` | Command line option parsing and error reporting |

The classes that talk to a device (`AlphaRngApi`, `UsbSerialDevice` and `ShaEntropyExtractor`) are not covered,
because they need an AlphaRNG device.

A test that documents a known, unfixed defect is declared with `TEST_KNOWN_BUG`. It is reported as `XFAIL` and
does not fail the run. Once the defect is fixed, the test reports `XPASS` and fails the run until the marker is removed.

## FreeBSD performance

Download speed is lower on FreeBSD than on Linux. In `alperftest`, a FreeBSD 14.3 system measured about 6.5 MB/s
for the plain (no MAC, no cipher) test, against about 25 MB/s on Ubuntu with the same device.

The limit is in FreeBSD's `umodem` serial driver, which reads the device through a single 1 KB USB transfer
at a time, not in these utilities or in the device. The USB link itself runs at full high speed.

## Linux USB access

To use the device without root, install the udev rules and re-plug the device:

```sh
sudo cp 85-alpharng-usb-access.rules /etc/udev/rules.d/
sudo udevadm control --reload-rules && sudo udevadm trigger
```

## Usage examples

List the available devices:

```sh
alrng -l
```

Download 1 MB of entropy bytes to `rnd.bin` using AES-128-GCM and an HMAC-SHA256 digest:

```sh
alrng -e -o rnd.bin -n 1000000 -c aes128 -m hmacSha256
```

Download 1 MB without the secure channel (`-c none`):

```sh
alrng -e -o rnd.bin -n 1000000 -c none
```

Download 1 MB of raw, unprocessed bytes:

```sh
alrng -r -o rnd.bin -n 1000000
```

Check a downloaded file for groups of at least 10 consecutive sticky bits:

```sh
bitgrps -sb -i rnd.bin -gs 10
```

Run the diagnostics and performance tests:

```sh
alrngdiag
alperftest
```

## Named pipe server

`run-alrng-pserver.sh` keeps `/tmp/alpharng` filled with random bytes from the device,
so other programs can read from it like a file. It expects the programs in `/usr/local/bin`.
To start it at boot, add this line with `sudo crontab -e`:

```
@reboot /usr/local/bin/run-alrng-pserver.sh >> /var/log/run-alrng-pserver.log 2>&1
```

The script runs `alrng` with `-c none`, which disables the secure channel. Remove that option
and review the access rights set in the script if security matters for your setup.

## More information

* [AlphaRNG quick start guide](https://tectrolabs.com/docs/alpharng/quick-start/)
* [AlphaRNG documentation](https://tectrolabs.com/docs/alpharng/)
* [Top level README](../../README.md)
