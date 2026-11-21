-- Shared helpers for scripts/xmake/tasks.lua (script scope module, used via `import`).
-- xmake module exports are plain global functions.
---@diagnostic disable: lowercase-global

--- Run `xmake <args...>` as a child process (raises on failure).
---@param args string[] xmake arguments (without `xmake`)
function exec_xmake(args)
  print("> xmake %s", table.concat(args, " "))
  os.execv(os.programfile(), args)
end

--- Own C++ sources (src/, tests/).
---@return string[] files absolute paths
function source_files()
  local files = {}
  for _, pattern in ipairs({"src/**.cpp", "src/**.h", "tests/**.cpp", "tests/**.h"}) do
    table.join2(files, os.files(path.join(os.projectdir(), pattern)))
  end
  return files
end

--- Locate an LLVM tool: explicit path > PATH > LLVM bundled with Visual Studio.
--- Raises when not found.
---@param name string tool name, e.g. `"clang-tidy"`
---@param explicit string? path given by the user (`--<name>=<path>`)
---@return string program executable path
function find_llvm_tool(name, explicit)
  import("lib.detect.find_program")

  if explicit then
    return explicit
  end
  local program = find_program(name)
  if program then
    return program
  end

  -- vswhere: <VS>/VC/Tools/Llvm/x64/bin/<name>.exe
  local vswhere = path.join(os.getenv("ProgramFiles(x86)") or "", "Microsoft Visual Studio/Installer/vswhere.exe")
  if os.isfile(vswhere) then
    local vsdir = os.iorunv(vswhere, {"-latest", "-property", "installationPath"}):trim()
    local candidate = path.join(vsdir, "VC/Tools/Llvm/x64/bin", name .. ".exe")
    if os.isfile(candidate) then
      return candidate
    end
  end
  raise("%s not found: install LLVM >= 22 or pass --%s=<path>", name, name)
end

--- Run clang-format over `source_files()`.
---@param explicit string? clang-format path (auto detect when nil)
---@param fix boolean `true`: format in place (`-i`), `false`: check only (`--dry-run -Werror`)
function clang_format(explicit, fix)
  local tool = find_llvm_tool("clang-format", explicit)
  local files = source_files()
  local argv = fix and {"-i"} or {"--dry-run", "-Werror"}
  table.join2(argv, files)
  print("> %s %s (%d files)", tool, fix and "-i" or "--dry-run -Werror", #files)
  os.execv(tool, argv)
end

--- Run clang-tidy over the compile database (build/tidy), with MSVC precompiled header flags removed.
---@param explicit string? clang-tidy path (auto detect when nil)
---@param fix boolean apply fixes (`--fix`)
function clang_tidy(explicit, fix)
  import("core.base.json")
  import("core.project.config")

  local tool = find_llvm_tool("clang-tidy", explicit)

  config.load()
  local mode = config.mode()
  local dbdir = path.absolute("build/tidy")
  exec_xmake({"project", "-k", "compile_commands", dbdir})

  -- clang-tidy cannot read MSVC precompiled headers (/Yu, /Yc, /Fp): drop them and
  -- force-include the original PCH source instead of the generated copy under build/.gens.
  local dbfile = path.join(dbdir, "compile_commands.json")
  local files = {}
  local entries = {}
  for _, entry in ipairs(json.loadfile(dbfile)) do
    local args = {}
    for _, arg in ipairs(entry.arguments) do
      if arg:startswith("/Yu") or arg:startswith("/Yc") or arg:startswith("/Fp") then
        -- skip
      elseif arg:startswith("/FI") and path.unix(arg):find("/.gens/", 1, true) then
        local rel = path.unix(arg):match("/" .. mode .. "/(.+)$")
        table.insert(args, "/FI" .. path.join(os.projectdir(), rel))
      else
        table.insert(args, arg)
      end
    end
    entry.arguments = args
    table.insert(entries, entry)

    -- own sources only (skip generated plugin.cpp etc.)
    local file = path.absolute(entry.file, entry.directory)
    if not path.unix(file):find("/.gens/", 1, true) then
      table.insert(files, file)
    end
  end
  json.savefile(dbfile, entries)

  local argv = {"-p", dbdir, "--quiet"}
  if fix then
    table.insert(argv, "--fix")
  end
  table.join2(argv, files)
  print("> %s%s (%d files)", tool, fix and " --fix" or "", #files)
  os.execv(tool, argv)
end
