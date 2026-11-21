set_xmakever("2.8.2") -- set minimum xmake version

-- set project
local PLUGIN_NAME <const> = "HavokLayoutDumperFO4"

set_project("havok_layout_dumper_fo4")
set_version("0.1.0")
set_license("MIT")

set_languages("c++23")
set_warnings("allextra")
set_encodings("utf-8")

-- add rules
add_rules("mode.debug", "mode.releasedbg", "mode.release")
add_rules("plugin.vsxmake.autoupdate")

-- `xmake fmt` / `xmake lint` / `xmake lint:fix` (see scripts/xmake/tasks.lua)
-- built-ins: `xmake test` (unit tests), `xmake package` (build/packages/<plugin>-<version>.zip)
includes("scripts/xmake/tasks.lua")

-- Local package repository (packages/c/commonlibf4-ng).
-- Move `repo/` to its own git repo to share between projects:
--   add_repositories("fo4-repo https://github.com/<you>/xmake-repo-fo4.git main")
add_repositories("local-repo repo")

-- Like Cargo `git = ..., rev = ...`: the version is the CommonLibF4 git commit.
add_requires("commonlibf4-ng 15f504ea")

-- F4SE independent core: scanning, signature, JSON, file output (unit tested)
target("hkdump-core", function ()
  set_kind("static")
  add_files("src/core/**.cpp")
  add_headerfiles("src/core/**.h")
  add_includedirs("src/core", {public = true})
  on_package(function (target) end) -- internal library: nothing to ship
end)

-- F4SE plugin: only the game specific glue (image / sections, log dir, messaging)
target(PLUGIN_NAME, function ()
  add_deps("hkdump-core")
  add_packages("commonlibf4-ng")

  add_rules("@commonlibf4-ng/plugin", {
    name = PLUGIN_NAME,
    author = "SARDONYX",
    description = "Dumps Havok hkClass reflection data as serde-hkx class JSON"
  })

  add_files("src/plugin/**.cpp")
  add_headerfiles("src/plugin/**.h")
  add_includedirs("src/plugin")
  set_pcxxheader("src/plugin/PCH.h")
end)

add_requires("catch2 3.16.1") -- For testing

-- `xmake test` (or `xmake run test_hkdump`)
target("test_hkdump", function ()
  set_default(false) -- To exclude `xmake build`
  set_kind("binary")
  add_deps("hkdump-core")
  add_packages("catch2")

  add_files("tests/**.cpp")
  add_headerfiles("tests/**.h")
  add_includedirs("tests")
  add_tests("default", {timeout = 20 * 1000, trim_output = false})
end)
