-- npm-script like tasks (built-ins cover the rest: `xmake test`, `xmake package`).
--
--   xmake fmt        clang-format -i
--   xmake lint       clang-format --dry-run -Werror + clang-tidy
--   xmake lint:fix   clang-format -i + clang-tidy --fix
--
-- Tools are auto detected (PATH, then LLVM bundled with Visual Studio); both must be >= 22.
-- Override with `--clang-format=<path>` / `--clang-tidy=<path>`.

--- Shared `set_menu` options: {short, long, kind, default, description}.
---@type [string?, string, string, any, string][]
local tool_options = {
  {nil, "clang-format", "kv", nil, "clang-format executable (default: auto detect)."},
  {nil, "clang-tidy", "kv", nil, "clang-tidy executable (default: auto detect)."},
}

task("fmt")
  set_category("plugin")
  on_run(function ()
    import("core.base.option")
    import("task_utils", {rootdir = os.scriptdir()})
    task_utils.clang_format(option.get("clang-format"), true)
  end)
  set_menu {
    usage = "xmake fmt [options]",
    description = "Format sources with clang-format.",
    options = tool_options,
  }
task_end()

task("lint")
  set_category("plugin")
  on_run(function ()
    import("core.base.option")
    import("task_utils", {rootdir = os.scriptdir()})
    task_utils.clang_format(option.get("clang-format"), false)
    task_utils.clang_tidy(option.get("clang-tidy"), false)
  end)
  set_menu {
    usage = "xmake lint [options]",
    description = "Check formatting (clang-format) and run clang-tidy.",
    options = tool_options,
  }
task_end()

task("lint:fix")
  set_category("plugin")
  on_run(function ()
    import("core.base.option")
    import("task_utils", {rootdir = os.scriptdir()})
    task_utils.clang_tidy(option.get("clang-tidy"), true)
    task_utils.clang_format(option.get("clang-format"), true) -- after tidy: re-format its fixes
  end)
  set_menu {
    usage = "xmake lint:fix [options]",
    description = "Apply clang-tidy fixes and format sources.",
    options = tool_options,
  }
task_end()
