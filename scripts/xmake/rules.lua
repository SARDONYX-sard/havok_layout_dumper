-- `xmake package` / `xmake install` support: ship the target file (+ pdb) under a prefix directory.
--
--   add_rules("zip.package", {prefixdir = "Data/F4SE/Plugins"})
--
-- `xmake package` -> build/packages/<target>-<version>-<plat>-<arch>.zip
-- `xmake install -o <dir>` -> <dir>/<prefixdir>/<target file>
rule("zip.package")
  on_config(function (target)
    local conf = target:extraconf("rules", "zip.package") or {}
    local prefix = conf.prefixdir or ""
    target:add("installfiles", target:targetfile(), {prefixdir = prefix})
    if target:is_plat("windows") then -- pdb
      target:add("installfiles", target:symbolfile(), {prefixdir = prefix})
    end
  end)

  on_install(function (target)
    local srcfiles, dstfiles = target:installfiles()
    for idx, srcfile in ipairs(srcfiles or {}) do
      os.trycp(srcfile, dstfiles[idx])
    end
  end)

  on_package(function (target)
    import("core.project.config")
    import("utils.archive")

    local root = path.join(os.tmpdir(), "zip.package", target:name())
    os.tryrm(root)

    local srcfiles, dstfiles = target:installfiles(root)
    for idx, srcfile in ipairs(srcfiles or {}) do
      os.trycp(srcfile, dstfiles[idx])
    end

    local archive_name = string.format(
      "%s-%s-%s-%s.zip", target:name(), target:version() or "0.0.0", target:plat(), target:arch()
    )
    local archive_path = path.absolute(path.join(config.builddir(), "packages", archive_name))
    os.tryrm(archive_path)

    local olddir = os.cd(root)
    local files = os.files("**")
    os.cd(olddir)
    archive.archive(archive_path, files, {curdir = root})
    print("packed %s", archive_path)
  end)
rule_end()
