-- In-game snapshot plugins (Windows only): write `<game>/Data/hkdump_snapshots/<exe>.hksnap`.
-- Hand written xSE ABI, no game library: they only copy `.rdata` / `.data` of the game exe.
if not is_plat("windows") then
  return
end

---@param name string plugin / target name
---@param loader string source dir under src/ (f4se, skse64, skse)
---@param arch "x64"|"x86"
---@param prefixdir string install dir inside the mod archive
local function snapshot_plugin(name, loader, arch, prefixdir)
  target(name, function ()
    set_kind("shared")
    set_arch(arch)
    add_deps("hkdump-snapshot-" .. arch)
    add_files("src/common/*.cpp", "src/" .. loader .. "/*.cpp")
    add_headerfiles("src/common/*.h")
    add_includedirs("src/common")
    add_defines("HKDUMP_PLUGIN_NAME=\"" .. name .. "\"")
    add_rules("zip.package", {prefixdir = prefixdir})
  end)
end

snapshot_plugin("hkdump_f4se", "f4se", "x64", "Data/F4SE/Plugins") -- Fallout 4 / VR
snapshot_plugin("hkdump_skse64", "skse64", "x64", "Data/SKSE/Plugins") -- Skyrim SE / AE / VR
snapshot_plugin("hkdump_skse", "skse", "x86", "Data/SKSE/Plugins") -- Skyrim LE (32-bit)
