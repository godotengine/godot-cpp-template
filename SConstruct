#!/usr/bin/env python
import os
import sys

from methods import print_error


if not (os.path.isdir("godot-cpp") and os.listdir("godot-cpp")):
    print_error("""godot-cpp is not available within this folder, as Git submodules haven"t been initialized.
Run the following command to download godot-cpp:

    git submodule update --init --recursive""")
    sys.exit(1)


# ============================= General Lib Info =============================

libname = "EXTENSION-NAME"
projectdir = "demo"

gdextension_tool = Tool("gdextension")

# ============================= Setup Options =============================

# Load variables from custom.py, in case someone wants to store their own arguments.
# See https://scons.org/doc/production/HTML/scons-user.html#app-tools // search custom.py
customs = ["custom.py"]
customs = [os.path.abspath(path) for path in customs]
opts = Variables(customs, ARGUMENTS)

gdextension_tool.options(opts)

# Remove our custom options to avoid passing to godot-cpp; godot-cpp has its own check for unknown options.
for opt in opts.options:
    ARGUMENTS.pop(opt.key, None)

# ============================= Setup godot-cpp =============================

godot_cpp_env = SConscript("godot-cpp/SConstruct", {"customs": customs})

gdextension_env = godot_cpp_env.Clone()
opts.Update(gdextension_env)
Help(opts.GenerateHelpText(gdextension_env))

# ============================= Setup Targets =============================

sources = []
targets = []

gdextension_tool.generate(gdextension_env, godot_cpp_env, sources)

file = "{}{}{}".format(libname, godot_cpp_env["suffix"], godot_cpp_env["SHLIBSUFFIX"])
filepath = ""

if godot_cpp_env["platform"] == "macos" or godot_cpp_env["platform"] == "ios":
    filepath = "{}.framework/".format(godot_cpp_env["platform"])
    file = "{}.{}.{}".format(libname, godot_cpp_env["platform"], godot_cpp_env["target"])

libraryfile = "bin/{}/{}{}".format(godot_cpp_env["platform"], filepath, file)
library = gdextension_env.SharedLibrary(
    libraryfile,
    source=sources,
)
targets.append(library)

targets.append(gdextension_env.Install("{}/bin/{}/{}".format(projectdir, godot_cpp_env["platform"], filepath), library))

Default(targets)
