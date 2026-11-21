-- Workspace root (like a Cargo `[workspace]`): shared settings + member list.
-- Each member owns its `xmake.lua`, `src/` and `tests/` (`add_tests`: run by `xmake test`).
--
--   crates/snapshot            `.hksnap` format + PE image (shared: written by plugin, read by hkdump)
--   crates/plugin              in-game snapshot plugins (Windows only)
--   crates/hkdump              CLI: snapshot -> serde-hkx class JSON (+ its hkClass decoder)
set_xmakever("2.8.5") -- set minimum xmake version

set_project("havok_layout_dumper")
set_version("0.1.0")
set_license("MIT OR Apache-2.0")

set_languages("c++23")
set_warnings("allextra")
set_encodings("utf-8")

add_rules("mode.debug", "mode.releasedbg", "mode.release")
add_rules("plugin.vsxmake.autoupdate")

-- `xmake fmt` / `xmake lint` / `xmake lint:fix` (see scripts/xmake/tasks.lua)
-- built-ins: `xmake test` (unit tests of every member), `xmake package` (build/packages/<target>-<version>.zip)
includes("scripts/xmake/tasks.lua")
includes("scripts/xmake/rules.lua")

-- `[workspace.dependencies]`
add_requires("catch2 3.16.1") -- For testing

-- `[workspace] members`
includes(
  "crates/snapshot",
  "crates/plugin",
  "crates/hkdump"
)
