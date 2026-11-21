-- hkdump-snapshot: `.hksnap` format + PE image sections. Shared: written by the plugins, read by hkdump.
-- Platform independent; no Havok knowledge.

--- Declare one snapshot library.
---@param name string target name
---@param arch string? fixed arch (nil: configured arch)
local function snapshot_lib(name, arch)
  target(name, function ()
    set_kind("static")
    if arch then
      set_arch(arch)
      set_default(false) -- built on demand by the plugins
    end
    add_files("src/*.cpp")
    add_headerfiles("src/*.h")
    add_includedirs("src", {public = true})
    on_package(function (target) end) -- internal library: nothing to ship

    if not arch then
      if is_plat("windows") then
        -- test binary: `main` lives in Catch2Main.lib (the linker cannot infer the subsystem by itself).
        -- Policy instead of `{force = true}`: the test clone of this target drops the per-flag `force`.
        set_policy("check.auto_ignore_flags", false)
        add_ldflags("/subsystem:console")
      end
      -- `xmake test`: this library + tests/ + Catch2's main as a binary
      add_tests("unit", {
        kind = "binary",
        files = "tests/*.cpp",
        packages = "catch2",
        timeout = 20 * 1000,
        trim_output = false,
      })
    end
  end)
end

snapshot_lib("hkdump-snapshot")
if is_plat("windows") then
  -- The plugins are pinned to the game arch (x64 / x86 for Skyrim LE), independent of `xmake f -a`.
  snapshot_lib("hkdump-snapshot-x64", "x64")
  snapshot_lib("hkdump-snapshot-x86", "x86")
end
