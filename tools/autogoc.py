import os
import platform
from os import environ

from SCons.Environment import Environment
from SCons.Node import FS


def normalize_path(path):
    if isinstance(path, FS.File):
        path = str(path)

    if isinstance(path, FS.Dir):
        path = str(path)

    if not isinstance(path, str):
        return None

    return os.path.abspath(path)


def normalize_paths(paths):
    result = [normalize_path(path) for path in paths]
    result = [valid for valid in result if valid is not None]
    return result


def in_generated_sources(sources, root_path, generated_path):
    root_norm = normalize_path(root_path)
    gen_norm = normalize_path(generated_path)
    result = [
        f"{gen_norm}/generated_register_types.cpp",
    ]

    if root_norm is None or gen_norm is None:
        return []

    for source in sources:
        normalized = normalize_path(source)
        if (
            normalized
            and normalized.startswith(root_norm)
            and normalized.endswith(".cpp")
        ):
            relative = normalized.removeprefix(root_norm)
            relative = relative.replace(".cpp", ".generated.cpp")
            result.append(f"{generated_path}{relative}")

    return result


def find_goc(env: Environment):
    exec_name = "goc"
    if platform.system() == "Windows":
        exec_name += ".exe"

    if os.path.exists("godot-object-compiler"):
        build_dir = normalize_path("godot-object-compiler/build")
        exec_path = normalize_path(f"godot-object-compiler/build/{exec_name}")
        command = f"\
            mkdir -p {build_dir} &&\
            cmake -B{build_dir} -Sgodot-object-compiler -DCMAKE_BUILD_TYPE=Release && \
            cmake --build {build_dir}"

        if platform.system() == "Windows":
            command = f"\
                if not exists {build_dir} mkdir {build_dir} &&\
                cmake -B{build_dir} -Sgodot-object-compiler && \
                cmake --build {build_dir} --config Release"

        env.Command(
            exec_path,
            source=[],
            action=command,
        )

        print(f"Using built GOC executable: {exec_path}")
        return exec_path

    environment_var = environ.get("GOC_EXECUTABLE")
    if environment_var is not None:
        print(f"Using GOC executable: {environment_var}")
        return environment_var

    if os.path.exists(exec_name):
        return normalize_path(exec_name)

    raise Exception(
        "GOC executable not found. Set the GOC_EXECUTABLE environment variable or add godot-object-compiler as a submodule to your project root."
    )


def find_godot_cpp_path(include_dirs):
    for dir in include_dirs:
        if "godot-cpp" in dir:
            return dir.split("godot-cpp")[0] + "godot-cpp"

    raise Exception("Could no find godot-cpp in include paths.")


def create_goc_shared_library(env: Environment, lib_name: str, source, root_path: str):
    goc_path = find_goc(env)
    goc_dir = normalize_path(".goc")

    generated_dir = normalize_path(".goc/generated")
    cache_dir = normalize_path(".goc/cache")
    root_dir = normalize_path(root_path)
    sources = normalize_paths(source)
    current_includes = env.Dictionary("CPPPATH")
    include_dirs = normalize_paths(current_includes)
    generated_source = in_generated_sources(source, root_dir, generated_dir)
    comma = ","

    run_goc = env.Command(
        generated_source,
        source=[],
        action=f"{goc_path} generate \
            -P={goc_dir} \
            -C={cache_dir} \
            -G={generated_dir} \
            -I={comma.join(str(f) for f in include_dirs)} \
            -S={comma.join(str(f) for f in sources)} \
            -R={root_dir}",
    )
    env.AlwaysBuild(run_goc)
    env.Alias("goc_generated", generated_source)
    env.Depends(run_goc, goc_path)

    godot_cpp_path = find_godot_cpp_path(include_dirs)
    godot_cpp_lib = (
        godot_cpp_path + "/bin/libgodot-cpp" + env["suffix"] + env["LIBSUFFIX"]
    )

    env.Depends(run_goc, godot_cpp_lib)

    final_source = []
    final_source.extend(generated_source)
    final_source.extend(sources)
    env.Depends(source, generated_source)
    env.AppendUnique(CPPPATH=[generated_dir])

    return env.SharedLibrary(lib_name, source=final_source)
