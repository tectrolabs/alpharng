# mlkem-native (ML-KEM-1024)

Post-quantum key encapsulation (FIPS 203 ML-KEM) used by the encapsulate-mlkem and decapsulate-mlkem tools.

- Upstream: https://github.com/pq-code-package/mlkem-native
- Version: release v2.0.0 (commit d1b2fe782888bdb761a50336012923180be7f502, 2026-08-07)
- License: Apache-2.0 OR ISC OR MIT (see `LICENSE`)

Only the portable C sources are included (no assembly backends).
Compile **only** `mlkem_native.c`; it includes everything under `src/`.
Local settings are at the top of `mlkem_native_config.h`
(parameter set 1024, `mlkem_` symbol prefix, OpenSSL `RAND_bytes()`
as the random source). All other files are unmodified upstream copies.

Public API (see `mlkem_native.h`):

    int mlkem_keypair(uint8_t pk[1568], uint8_t sk[3168]);
    int mlkem_enc(uint8_t ct[1568], uint8_t ss[32], const uint8_t pk[1568]);
    int mlkem_dec(uint8_t ss[32], const uint8_t ct[1568], const uint8_t sk[3168]);
