# JetBrains Rider C++ support files (.sln folder)

Rider uses these files to resolve C++, provide code insight, and create/run build configs. They also supply MSBuild/NMake metadata so projects index and build correctly.

- godot-cpp-example.sln
  - Solution that groups the C++ projects and lists all Configuration|Platform pairs shown in the Solution Configuration selector.

- gdext.vcxproj
  - GDExtension C++ project. Rider reads compiler options/defines/includes and also calls scons to build the extension library (.dll/.so/.dylib).

- demo.vcxproj
  - Auxiliary project linking `demo` game contents. Rider plugins for Godot support provide language support, run configurations and other features for such projects.

- targets/JetBrains.Rider.Cpp.targets
  - MSBuild target that resolves C++ standard library headers.

- targets/nmake.substitution.props (similar to the one in the Godot Engine)
  - Used when MSBuild doesn't contain MSVC targets; substitutes Build/Rebuild/Clean targets and passes correct include/lib paths.
  - Reference: [godot/misc/msvs/nmake.substitution.props](https://github.com/godotengine/godot/blob/master/misc/msvs/nmake.substitution.props)

Notes: Rider uses MSBuild to read configurations/props/items/targets. Files are MSBuild/VS-compatible but mainly meant for Rider’s C++ support.
