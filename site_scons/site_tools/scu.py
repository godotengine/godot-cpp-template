import pathlib
from SCons.Variables import BoolVariable


def exists(env):
    return True

def options(opts):
    opts.Add(
        BoolVariable(
            "scu_build",
            "Use single compilation unit build.",
            False
        )
    )

def generate(env, sources):
    if not env.get('scu', False):
        return

    scu_path = pathlib.Path('src/gen/scu.cpp')
    scu_path.parent.mkdir(exist_ok=True)

    scu_path.write_text(
        '\n'.join(
            f'#include "{pathlib.Path(source.path).relative_to("src")}"'
            for source in sources
        ) + '\n'
    )

    sources[:] = [str(scu_path.absolute())]
