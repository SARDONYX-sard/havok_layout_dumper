-- `hkdump <snapshot.hksnap> <out dir>`: snapshot (Skyrim LE / SE, Fallout 4) -> serde-hkx class JSON
-- src/: CLI (main.cpp) + x86 / x64 hkClass decoder (scan, signature, serde-hkx class JSON)

target("hkdump", function ()
  set_kind("binary")
  add_deps("hkdump-snapshot")
  add_files("src/*.cpp")
  add_headerfiles("src/**.h")
  add_includedirs("src")
  add_rules("zip.package")

  if is_plat("windows") then
    -- console app; also required by the test binary, whose `main` lives in Catch2Main.lib
    -- (the linker cannot infer the subsystem by itself). Policy instead of `{force = true}`:
    -- the test clone of this target drops the per-flag `force` and would warn.
    set_policy("check.auto_ignore_flags", false)
    add_ldflags("/subsystem:console")
  end

  -- `xmake test`: the same target without main.cpp, plus tests/ and Catch2's main
  add_tests("unit", {
    files = "tests/*.cpp",
    remove_files = "src/main.cpp",
    packages = "catch2",
    timeout = 20 * 1000,
    trim_output = false,
  })
end)
