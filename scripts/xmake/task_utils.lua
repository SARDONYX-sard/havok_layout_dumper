-- Shared helpers for scripts/xmake/tasks.lua (script scope module, used via `import`).
-- xmake module exports are plain global functions.
---@diagnostic disable: lowercase-global

--- Run `xmake <args...>` as a child process (raises on failure).
---@param args string[] xmake arguments (without `xmake`)
function exec_xmake(args)
  print("> xmake %s", table.concat(args, " "))
  os.execv(os.programfile(), args)
end

--- Own C++ sources (workspace members under crates/).
---@return string[] files absolute paths
function source_files()
  local files = {}
  for _, pattern in ipairs({"crates/**.cpp", "crates/**.h"}) do
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

--- C++ standard of the project for clang (`c++23`), from `set_languages` (`c++23` / `cxx23`).
---@return string std clang `-std=` value
function project_cxx_std()
  import("core.project.project")

  for _, target in pairs(project.targets()) do
    for _, lang in ipairs(table.wrap(target:get("languages"))) do
      local ver = lang:match("^c%+%+(%w+)$") or lang:match("^cxx(%w+)$")
      if ver then
        return "c++" .. ver
      end
    end
  end
  return "c++23"
end

--- Run `xmake check clang.tidy` (parallel) over the own sources, on a compile database (build/tidy)
--- whose C++ standard flag clang understands.
---@param explicit string? clang-tidy path (auto detect when nil)
---@param fix boolean apply fixes (`--fix`)
function clang_tidy(explicit, fix)
  import("core.base.json")

  local tool = find_llvm_tool("clang-tidy", explicit)

  local dbdir = path.absolute("build/tidy")
  exec_xmake({"project", "-k", "compile_commands", dbdir})

  -- The MSVC `/std:` spelling depends on the xmake / MSVC versions (`c++23preview`, `c++23`, `c++latest`)
  -- and clang-cl silently falls back to C++14 on one it does not know: pass the standard to clang directly.
  -- GCC style drivers already use `-std=`, which clang understands.
  local stdflag = "/clang:-std=" .. project_cxx_std()
  local dbfile = path.join(dbdir, "compile_commands.json")
  local entries = json.loadfile(dbfile)
  for _, entry in ipairs(entries) do
    local compiler = path.basename(entry.arguments[1]):lower()
    if compiler == "cl" or compiler == "clang-cl" then
      local args = {entry.arguments[1], stdflag}
      for idx, arg in ipairs(entry.arguments) do
        if idx > 1 and not arg:startswith("/std:") and not arg:startswith("-std:") then
          table.insert(args, arg)
        end
      end
      entry.arguments = args
    end
  end
  json.savefile(dbfile, entries)

  -- `xmake check clang.tidy` looks clang-tidy up on PATH only (else installs the llvm package):
  -- put the detected one first. `-f`: tests/ sources only exist in `add_tests` clones, not in a target.
  local argv = {"check", "clang.tidy", "--quiet", "--compdb=" .. dbdir, "-f", "crates/**.cpp"}
  if fix then
    table.join2(argv, {"--fix", "--jobs=1"}) -- serial: parallel jobs would rewrite shared headers concurrently
  end
  local pathenv = path.directory(tool) .. path.envsep() .. (os.getenv("PATH") or "")
  print("> xmake %s (%s)", table.concat(argv, " "), tool)
  os.execv(os.programfile(), argv, {envs = {PATH = pathenv}})
end
