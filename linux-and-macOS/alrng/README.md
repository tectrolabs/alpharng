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
| `alpharng.so` | OpenSSL 4 provider that uses AlphaRNG as the random source (optional, see [below](#openssl-provider-openssl-40-and-later)) |
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

## OpenSSL provider (OpenSSL 4.0 and later)

OpenSSL 4.0 removed engines. The AlphaRNG provider lets OpenSSL use an AlphaRNG device instead: when it is named as the
random provider, `RAND_bytes()` and `RAND_priv_bytes()` return bytes from the device. It is built only when you ask for
it, and needs OpenSSL 4.0 or a 3.5.x release that has `OSSL_FUNC_PROVIDER_RANDOM_BYTES`; configuring fails with a message
if the OpenSSL found does not have it.

```sh
cmake -S . -B build -DALRNG_BUILD_PROVIDER=ON -DOPENSSL_ROOT_DIR=<path to OpenSSL 4>
cmake --build build
```

This creates `build/alpharng.so` (`build/alpharng.dylib` on macOS). Name it in the OpenSSL config file (`openssl.cnf`)
and set `OPENSSL_CONF` to that file:

```
openssl_conf = openssl_init

[openssl_init]
random = random_sect
providers = provider_sect

[provider_sect]
default = default_sect
alpharng = alpharng_sect

[default_sect]
activate = 1

[alpharng_sect]
module = /path/to/alpharng.so
activate = 1

[random_sect]
random_provider = alpharng
```

To check it, run `openssl list -providers` (it should list `AlphaRNG`) and `openssl rand -hex 16`.

### Settings

Lines in the provider section (`[alpharng_sect]` above) choose how the provider uses the device. They use the same
values as the options of `alrng`, and all are optional:

| Setting | Values | Default | Like `alrng` |
| --- | --- | --- | --- |
| `cipher` | `aes256`, `aes128`, `none` | `aes256` | `-c` |
| `mac` | `none`, `hmacSha256`, `hmacSha160`, `hmacMD5` | `none` | `-m` |
| `rsa` | `RSA2048`, `RSA1024`, `RSA3072`, `RSA4096`; ignored when `key_file` is given | `RSA2048` | `-p` |
| `key_file` | path of a device public key file. If given, it is used instead of the key built into the API, and `rsa` is ignored | the key built into the API | `-k` |
| `device` | device number, 0 to 25 | `0` | `-d` |
| `data` | `entropy` (the device's processed output), `sha256` or `sha512` (extracted from the raw noise) | `entropy` | `-e`, `-x`, `-X` |
| `stat_tests` | `on`, `off` | `on` | `-dt` turns them off |

For example, to use the second device with HMAC-SHA256 and SHA-512 extraction:

```
[alpharng_sect]
module = /path/to/alpharng.so
activate = 1
mac = hmacSha256
device = 1
data = sha512
```

A value that is not allowed, or a device or key file that cannot be used, stops the provider from loading, so the
device is never used with settings other than the ones asked for. The reason is printed to the standard error output,
and OpenSSL keeps its own random source (see the notes below). `cipher = none` sends the random bytes over USB
unencrypted. A program can read the settings in use from the provider's `buildinfo` parameter
(`OSSL_PROVIDER_get_params`, `OSSL_PROV_PARAM_BUILDINFO`).

Things to know:

* **Keep `random` before `providers`** in `[openssl_init]`. With OpenSSL 4.0.3, listing `providers` first makes the provider
  get unloaded again as soon as `random_provider` is read, and OpenSSL then uses its own random source.
* **Load the `default` provider too**, as in the config above. The AlphaRNG API itself needs OpenSSL's random generator and
  AES while it starts, and OpenSSL provides them through the default provider. Without it the provider cannot start.
* **Load the config before the first random call.** OpenSSL answers the very first `RAND_bytes()` call of a process from
  its own source if it has not read the config yet. Tools such as `openssl` read it first. In your own program, call
  `OPENSSL_init_crypto(OPENSSL_INIT_LOAD_CONFIG, NULL)` before generating random numbers.
* **A missing device is not an error for OpenSSL.** If the provider cannot start, for example because no AlphaRNG is found,
  it is not loaded and OpenSSL silently keeps using its own source. Applications that must use the device should check
  `OSSL_PROVIDER_available(NULL, "alpharng")`. If the provider loads and the device then fails, `RAND_bytes()` returns 0.
* **One device per process.** A process uses the device chosen by `device` (default 0) and holds it from the moment OpenSSL
  loads the provider until the process ends. While it is held, `alrng` and other programs cannot use that device. To use
  several devices, run separate processes, each with its own config and its own `device = N`. The number counts all
  connected devices, in the same order as `alrng -l`, whether or not they are in use. A process that asks for a device
  that is already in use cannot start the provider: it prints `Could not lock device` and OpenSSL silently keeps using
  its own random source. The process that holds the device is not affected. A single process cannot combine the output
  of two devices.
* Starting the provider sets up the secure session with the device, which takes about 0.7 seconds on a Raspberry Pi 5
  with the default `RSA2048` and several seconds with `RSA4096`.
* The AlphaRNG API calls `RAND_bytes()` itself for its session keys. While the provider is reading from the device,
  those calls are answered by OpenSSL's built-in generator, not by the device.
* Speed is set by the device: about 0.5 MB/s on a Raspberry Pi 5 and on an Ubuntu PC, the same as `alrng -e`.

The provider tests (`-DALRNG_BUILD_TESTS=ON` together with the provider option) use a stand-in provider with a known
byte stream instead of a device, so they need no hardware. The provider was run end to end with OpenSSL 4.0.3 and an
AlphaRNG device on a Raspberry Pi 5, on Ubuntu 22.04 and on macOS, and with two devices on a Mac.

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
