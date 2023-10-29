from collections.abc import Mapping
from pathlib import Path
from typing import Iterator

import SCons.Node.FS
from SCons.Script import *

# Get the "Godot" folder name ahead of time
base_folder_path = str(os.path.abspath(Path(__file__).parent)) + "/"
base_folder_only = os.path.basename(os.path.normpath(base_folder_path))
# Listing all the folders we have converted
# for SCU in scu_builders.py
_scu_folders = set()


def add_to_vs_project(env, sources):
    for x in sources:
        if isinstance(x, SCons.Node.FS.Dir):
            sources.extend(x.glob("./*/*"))  # Recurse into subdir
            sources.extend(x.glob("./*.h"))
            sources.extend(x.glob("./*.?pp"))
            continue
        elif isinstance(x, SCons.Node.FS.File):
            fname = x.path
        elif isinstance(x, list):
            sources.extend(x)
            continue
        elif type(x) == type(""):
            fname = env.File(x).path
        else:
            fname = env.File(x)[0].path
        print(fname)
        pieces = fname.split(".")
        if len(pieces) > 0:
            basename = pieces[0]
            basename = basename.replace("\\\\", "/")
            if os.path.isfile(basename + ".h"):
                env.vs_incs += [basename + ".h"]
            elif os.path.isfile(basename + ".hpp"):
                env.vs_incs += [basename + ".hpp"]
            if os.path.isfile(basename + ".c"):
                env.vs_srcs += [basename + ".c"]
            elif os.path.isfile(basename + ".cpp"):
                env.vs_srcs += [basename + ".cpp"]


def find_visual_c_batch_file(env):
    from SCons.Tool.MSCommon.vc import (
        get_default_version,
        get_host_target,
        find_batch_file,
    )

    # Syntax changed in SCons 4.4.0.
    from SCons import __version__ as scons_raw_version

    scons_ver = env._get_major_minor_revision(scons_raw_version)

    version = get_default_version(env)

    if scons_ver >= (4, 4, 0):
        (host_platform, target_platform, _) = get_host_target(env, version)
    else:
        (host_platform, target_platform, _) = get_host_target(env)

    return find_batch_file(env, version, host_platform, target_platform)[0]


def generate_cpp_hint_file(filename):
    if os.path.isfile(filename):
        # Don't overwrite an existing hint file since the user may have customized it.
        pass
    else:
        try:
            with open(filename, "w") as fd:
                fd.write("#define GDCLASS(m_class, m_inherits)\n")
        except OSError:
            print("Could not write cpp.hint file.")


def generate_vs_project_target(env, original_args, godot_exec, sln_name):
    batch_file = find_visual_c_batch_file(env)
    filtered_args = original_args.copy()
    # Ignore the "vsproj" option to not regenerate the VS project on every build
    filtered_args.pop("vsproj", None)
    # The "platform" option is ignored because only the Windows platform is currently supported for VS projects
    filtered_args.pop("platform", None)
    # The "target" option is ignored due to the way how targets configuration is performed for VS projects (there is a separate project configuration for each target)
    filtered_args.pop("target", None)
    # The "progress" option is ignored as the current compilation progress indication doesn't work in VS
    filtered_args.pop("progress", None)

    if batch_file:
        print("found vs batch_file: %s" % batch_file)

        class ModuleConfigs(Mapping):
            # This version information (Win32, x64, Debug, Release) seems to be
            # required for Visual Studio to understand that it needs to generate an NMAKE
            # project. Do not modify without knowing what you are doing.
            PLATFORMS = ["Win32", "x64"]
            PLATFORM_IDS = ["x86_32", "x86_64"]
            CONFIGURATIONS = ["editor", "template_release", "template_debug"]
            DEV_SUFFIX = ".dev" if env["dev_build"] else ""

            @staticmethod
            def for_every_variant(value):
                return [value for _ in range(len(ModuleConfigs.CONFIGURATIONS) * len(ModuleConfigs.PLATFORMS))]

            def __init__(self):
                shared_targets_array = []
                self.names = []
                self.arg_dict = {
                    "variant": [],
                    "runfile": shared_targets_array,
                    "buildtarget": shared_targets_array,
                    "cpppaths": [],
                    "cppdefines": [],
                    "cmdargs": [],
                }
                self.add_mode()  # default

            def add_mode(
                    self,
                    name: str = "",
                    includes: str = "",
                    cli_args: str = "",
                    defines=None,
            ):
                if defines is None:
                    defines = []
                self.names.append(name)
                self.arg_dict["variant"] += [
                    f'{config}{f"_[{name}]" if name else ""}|{platform}'
                    for config in ModuleConfigs.CONFIGURATIONS
                    for platform in ModuleConfigs.PLATFORMS
                ]
                self.arg_dict["runfile"] += [
                    godot_exec
                    for config in ModuleConfigs.CONFIGURATIONS
                    for plat_id in ModuleConfigs.PLATFORM_IDS
                ]
                self.arg_dict["cpppaths"] += ModuleConfigs.for_every_variant(env["CPPPATH"] + [includes])
                self.arg_dict["cppdefines"] += ModuleConfigs.for_every_variant(list(env["CPPDEFINES"]) + defines)
                self.arg_dict["cmdargs"] += ModuleConfigs.for_every_variant(cli_args)

            def build_commandline(self, commands):
                configuration_getter = (
                        "$(Configuration"
                        + "".join([f'.Replace("{name}", "")' for name in self.names[1:]])
                        + '.Replace("_[]", "")'
                        + ")"
                )

                common_build_prefix = [
                    'cmd /V /C set "plat=$(PlatformTarget)"',
                    '(if "$(PlatformTarget)"=="x64" (set "plat=x86_amd64"))',
                    'call "' + batch_file + '" !plat!',
                ]

                # Windows allows us to have spaces in paths, so we need
                # to double quote off the directory. However, the path ends
                # in a backslash, so we need to remove this, lest it escape the
                # last double quote off, confusing MSBuild
                common_build_postfix = [
                    "--directory=\"$(ProjectDir.TrimEnd('\\'))\"",
                    "platform=windows",
                    f"target={configuration_getter}",
                    "progress=no",
                ]

                for arg, value in filtered_args.items():
                    common_build_postfix.append(f"{arg}={value}")

                result = " ^& ".join(common_build_prefix + [" ".join([commands] + common_build_postfix)])
                return result

            # Mappings interface definitions

            def __iter__(self) -> Iterator[str]:
                for x in self.arg_dict:
                    yield x

            def __len__(self) -> int:
                return len(self.names)

            def __getitem__(self, k: str):
                return self.arg_dict[k]

        for header in glob_recursive("**/*.h"):
            env.vs_incs.append(str(header))

        module_configs = ModuleConfigs()

        env["MSVSBUILDCOM"] = module_configs.build_commandline("scons debug_symbols=yes")
        env["MSVSREBUILDCOM"] = module_configs.build_commandline("scons debug_symbols=yes vsproj=yes")
        env["MSVSCLEANCOM"] = module_configs.build_commandline("scons --clean")
        if not env.get("MSVS"):
            env["MSVS"]["PROJECTSUFFIX"] = ".vcxproj"
            env["MSVS"]["SOLUTIONSUFFIX"] = ".sln"

        return env.MSVSProject(
            target=["#" + sln_name + env["MSVSPROJECTSUFFIX"]],
            incs=env.vs_incs,
            srcs=env.vs_srcs,
            auto_build_solution=1,
            **module_configs,
        )
    else:
        print("Could not locate Visual Studio batch file to set up the build environment. Not generating VS project.")


def glob_recursive(pattern, node="."):
    from SCons import Node
    from SCons.Script import Glob

    results = []
    for f in Glob(str(node) + "/*", source=True):
        if type(f) is Node.FS.Dir:
            results += glob_recursive(pattern, f)
    results += Glob(str(node) + "/" + pattern, source=True)
    return results


def dump(env):
    # Dumps latest build information for debugging purposes and external tools.
    from json import dump

    def non_serializable(obj):
        return "<<non-serializable: %s>>" % (type(obj).__qualname__)

    with open(".scons_env.json", "w") as f:
        dump(env.Dictionary(), f, indent=4, default=non_serializable)
