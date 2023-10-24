#!/usr/bin/env python

from sconsutil import *

# libgdextension name
libname = "gdextension"
# godot binary to run demo project
godot_exec = f'godot\\Godot_v4.1.2-stable_win64.exe'
# godot demo project
projectdir = "demo"
# sln filename
sln_dir = os.getcwd()
sln_name = os.path.basename(sln_dir)

# default targets
default_targets = []


def normalize_path(val, env):
    return val if os.path.isabs(val) else os.path.join(env.Dir("#").abspath, val)


def validate_parent_dir(key, val, env):
    if not os.path.isdir(normalize_path(os.path.dirname(val), env)):
        raise UserError("'%s' is not a directory: %s" % (key, os.path.dirname(val)))


localEnv = Environment(tools=["default"], PLATFORM="")

customs = ["custom.py"]
customs = [os.path.abspath(path) for path in customs]

opts = Variables(customs, ARGUMENTS)
opts.Add(
    BoolVariable(
        key="compiledb",
        help="Generate compilation DB (`compile_commands.json`) for external tools",
        default=localEnv.get("compiledb", False),
    )
)
opts.Add(
    PathVariable(
        key="compiledb_file",
        help="Path to a custom `compile_commands.json` file",
        default=localEnv.get("compiledb_file", "compile_commands.json"),
        validator=validate_parent_dir,
    )
)
# vsproj
opts.Add(
    BoolVariable(
        key="vsproj",
        help="Generate a Visual Studio solution",
        default=False
    )
)
opts.Update(localEnv)

Help(opts.GenerateHelpText(localEnv))

env = localEnv.Clone()
env["compiledb"] = False

env.Tool("compilation_db")
compilation_db = env.CompilationDatabase(
    normalize_path(localEnv["compiledb_file"], localEnv)
)
env.Alias("compiledb", compilation_db)

env = SConscript("godot-cpp/SConstruct", {"env": env, "customs": customs})
env.Append(CPPPATH=["src/"])
env.libgdextension_sources = Glob("src/*.cpp")

file = "{}{}{}".format(libname, env["suffix"], env["SHLIBSUFFIX"])

if env["platform"] == "macos":
    platlibname = "{}.{}.{}".format(libname, env["platform"], env["target"])
    file = "{}.framework/{}".format(env["platform"], platlibname, platlibname)

libraryfile = "bin/{}/{}".format(env["platform"], file)
library = env.SharedLibrary(
    libraryfile,
    source=env.libgdextension_sources,
)
default_targets += [library]

projectlib = "{}/bin/{}/lib{}".format(projectdir, env["platform"], file)


def copy_bin_to_projectdir(target, source, env):
    import shutil

    for sourcefile in source:
        for targetfile in target:
            shutil.copyfile(sourcefile.get_path(), targetfile.get_path())


copy = env.Command(projectlib, libraryfile, copy_bin_to_projectdir)
default_targets += [copy]

if localEnv.get("compiledb", False):
    default_targets += [compilation_db]

# generate vs project if needed
if localEnv.get("vsproj", False):
    if os.name != "nt":
        print("Error: The `vsproj` option is only usable on Windows with Visual Studio.")
        Exit(255)

    # enable msvs construct
    env.Tool("msvs")

    env["CPPPATH"] = [Dir(path) for path in env["CPPPATH"]]
    env.vs_incs = []
    env.vs_srcs = []

    add_to_vs_project(env, env.libgodot_sources)
    add_to_vs_project(env, env.libgdextension_sources)

    vsproj = generate_vs_project_target(env, ARGUMENTS, godot_exec, sln_name)
    generate_cpp_hint_file("cpp.hint")

    env.Alias("vsproj", vsproj)
    default_targets += [vsproj]

dump(env)
Default(*default_targets)
