package("commonlibf4-ng")
    set_homepage("https://github.com/alandtse/CommonLibF4")
    set_description("A reverse engineered library for Fallout 4 (F4/NG/VR multi-targeting).")
    set_license("MIT")

    -- Git source only: the requested version is used as a git ref (commit sha / tag / branch).
    --   add_requires("commonlibf4-ng 15f504eacd90df1e586767c02f21a5f329c3f821")
    add_urls("https://github.com/alandtse/CommonLibF4.git")

    add_configs("fallout_f4",   {description = "Enable runtime support for Fallout 4 (pre-NG)", default = true,  type = "boolean"})
    add_configs("fallout_f4ng", {description = "Enable runtime support for Fallout 4 NG",       default = true,  type = "boolean"})
    add_configs("fallout_f4vr", {description = "Enable runtime support for Fallout 4 VR",       default = true,  type = "boolean"})
    add_configs("f4se_xbyak",   {description = "Enable trampoline support for Xbyak",           default = false, type = "boolean"})

    add_deps("rsm-binary-io", "rsm-mmio")
    add_deps("spdlog", {configs = {header_only = false, wchar = true, std_format = true}})

    add_syslinks("advapi32", "bcrypt", "d3d11", "d3dcompiler", "dbghelp", "dxgi", "ole32", "shell32", "user32", "version")

    on_load("windows|x64", function (package)
        if package:config("fallout_f4") then
            package:add("defines", "ENABLE_FALLOUT_F4=1")
        end
        if package:config("fallout_f4ng") then
            package:add("defines", "ENABLE_FALLOUT_NG=1")
        end
        if package:config("fallout_f4vr") then
            package:add("defines", "ENABLE_FALLOUT_VR=1")
            package:add("deps", "rapidcsv")
        end
        if package:config("f4se_xbyak") then
            package:add("defines", "F4SE_SUPPORT_XBYAK=1")
            package:add("deps", "xbyak")
        end

        -- public flags of the upstream `commonlibf4-ng` target
        package:add("cxxflags", "/EHsc", "/permissive-")
        package:add("cxxflags", "/wd4200", "/wd4201", "/wd4324", "/we4715")
    end)

    on_install("windows|x64", function (package)
        -- Upstream root xmake.lua already declares its requires and only builds
        -- the library by default (tools / RTTIDump are set_default(false)).
        local configs = {
            fallout_f4   = package:config("fallout_f4"),
            fallout_f4ng = package:config("fallout_f4ng"),
            fallout_f4vr = package:config("fallout_f4vr"),
            f4se_xbyak   = package:config("f4se_xbyak"),
        }
        import("package.tools.xmake").install(package, configs, {target = "commonlibf4-ng"})
    end)

    on_test("windows|x64", function (package)
        assert(package:has_cxxincludes("F4SE/F4SE.h", {configs = {languages = "c++23"}}))
    end)
