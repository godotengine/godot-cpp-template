import pathlib
from SCons.Tool import Tool


def exists(env):
    return True

def options(opts):
    # Add custom options here
    # opts.Add(
    #     "custom_option",
    #     "Custom option help text",
    #     "default_value"
    # )

    scu_tool = Tool("scu")
    scu_tool.options(opts)

def generate(env, godot_cpp_env, sources):
    scu_tool = Tool("scu")

    # read custom options values
    # custom_option = env["custom_option"]

    env.Append(CPPPATH=["src/"])

    sources.extend([
        f for f in env.Glob("src/*.cpp") + env.Glob("src/**/*.cpp")
        # Generated files will be added selectively and maintained by tools.
        if not "/gen/" in str(f.path)
    ])

    scu_tool.generate(env, sources)

    if godot_cpp_env["target"] in ["editor", "template_debug"]:
        try:
            doc_data = godot_cpp_env.GodotCPPDocData("src/gen/doc_data.gen.cpp", source=env.Glob("doc_classes/*.xml"))
            sources.append(doc_data)
        except AttributeError:
            print("Not including class reference as we're targeting a pre-4.3 baseline.")
