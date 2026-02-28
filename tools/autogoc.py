import os

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


def create_goc_shared_library(env: Environment, lib_name: str, source, root_path: str):
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
        action=f"goc generate \
            -P={goc_dir} \
            -C={cache_dir} \
            -G={generated_dir} \
            -I={comma.join(str(f) for f in include_dirs)} \
            -S={comma.join(str(f) for f in sources)} \
            -R={root_dir}",
    )
    env.AlwaysBuild(run_goc)
    env.Alias("goc_generated", generated_source)
    env.Depends(run_goc, "godot-cpp/bin/libgodot-cpp.linux.template_debug.x86_64.a")
    # env.Depends(run_goc, source)
    # env.SideEffect(generated_source, run_goc)

    final_source = []
    final_source.extend(generated_source)
    final_source.extend(sources)
    env.Depends(source, generated_source)

    env.AppendUnique(CPPPATH=[generated_dir])

    extension = env.SharedLibrary(lib_name, source=final_source)
    return extension
