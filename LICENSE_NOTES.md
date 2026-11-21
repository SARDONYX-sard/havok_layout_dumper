# License

## This project

Licensed under either of

- Apache License, Version 2.0 ([LICENSE-APACHE](LICENSE-APACHE))
- MIT License ([LICENSE-MIT](LICENSE-MIT))

at your option. `SPDX-License-Identifier: MIT OR Apache-2.0`

Unless you explicitly state otherwise, any contribution intentionally submitted for inclusion
in this project shall be dual licensed as above, without any additional terms or conditions.

### Why this license

The license follows from the projects this one builds on:

- The output format and the verification data come from [serde-hkx], which is
  `MIT OR Apache-2.0`. Using the same terms keeps the dumper and its consumer interchangeable.
- Every other source of layout information is permissive and compatible with both options:
  [HKX2Library] (MIT), [hkxcmd] (BSD) and [CommonLibF4] (MIT).
- No code, header or binary from the Havok SDK, the games or the script extenders is included,
  so none of their licenses has to be inherited (see below).

## Third party notices

### Referenced, not included

| Project                                | License           | How it was used                                                                                                                                                |
| -------------------------------------- | ----------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| [serde-hkx] (`assets/classes`)         | MIT OR Apache-2.0 | Target JSON format; Skyrim class data used as golden values for the signature algorithm                                                                        |
| [d-merge-serde-hkx] (`assets/classes`) | MIT OR Apache-2.0 | x86 / x64 offsets of `hkClass`, `hkClassMember`, `hkClassEnum`, `hkClassEnumItem`                                                                              |
| [HKX2Library]                          | MIT (2021 kreny)  | Origin of the x64 class layout data in the serde-hkx assets                                                                                                    |
| [hkxcmd]                               | BSD               | Origin of the x86 class layout data in the serde-hkx assets                                                                                                    |
| [CommonLibF4]                          | MIT               | Reference for the F4SE plugin ABI and Fallout 4 runtime layouts                                                                                                |
| [F4SE] / [SKSE64] / [SKSE]             | see each project  | Plugin entry points and interface struct layouts (`PluginAPI.h`) reproduced by hand in `src/plugin` for interoperability. No script extender source is copied. |

### Build / test only (not shipped)

| Project  | License    |
| -------- | ---------- |
| [Catch2] | BSL-1.0    |
| [xmake]  | Apache-2.0 |

## Game and Havok data

- _Havok_ is a trademark of its owner. _Fallout 4_ and _The Elder Scrolls V: Skyrim_ are
  trademarks of ZeniMax Media Inc. / Bethesda Softworks. This project is not affiliated with or
  endorsed by them.
- The `hkClass` signature algorithm (`crates/hkdump/src/Signature.cpp`) is an independent
  reimplementation, validated against published class data ([serde-hkx]) and against the class
  signatures stored in Fallout 4's own `.hkx` files. No Havok code is part of this repository.
- Snapshots (`*.hksnap`) and the JSON generated from them are derived from **your copy** of the
  game. Do not redistribute snapshots: they contain raw memory of the game executable. The
  generated JSON only describes type layouts (names, offsets, sizes, signatures); whether you may
  publish it is your responsibility.

[serde-hkx]: https://github.com/SARDONYX-sard/serde-hkx
[d-merge-serde-hkx]: https://github.com/SARDONYX-forks/d-merge-serde-hkx
[HKX2Library]: https://github.com/ret2end/HKX2Library
[hkxcmd]: https://github.com/figment/hkxcmd
[CommonLibF4]: https://github.com/alandtse/CommonLibF4
[F4SE]: https://f4se.silverlock.org/
[SKSE64]: https://skse.silverlock.org/
[SKSE]: https://skse.silverlock.org/
[Catch2]: https://github.com/catchorg/Catch2
[xmake]: https://github.com/xmake-io/xmake
