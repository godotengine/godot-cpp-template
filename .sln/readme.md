# JetBrains Rider C++ support files (.sln folder)

Rider uses these files to resolve C++, provide code insight, and create/run build configs. They also supply MSBuild/NMake metadata so projects index and build correctly.

- godot-cpp-example.sln
	- Solution that groups the C++ projects and lists all Configuration|Platform pairs shown in the Solution Configuration selector.

- gdext.vcxproj
	- GDExtension C++ project. Rider reads compiler options/defines/includes and also calls scons to build the extension library (.dll/.so/.dylib).

- targets/JetBrains.Rider.Cpp.targets (for non-Windows)
	- MSBuild target that resolves C++ standard library headers.

- targets/nmake.substitution.props (for non-Windows)
	- Substitutes Build/Rebuild/Clean targets.
	- Reference: [godot/misc/msvs/nmake.substitution.props](https://github.com/godotengine/godot/blob/master/misc/msvs/nmake.substitution.props)


Sln to vcxproj relation:
- The .sln file lists all Configuration|Platform pairs in SolutionConfigurationPlatforms (e.g., Debug|linux-x86_64).
- The same pairs are mapped to the gdext project in ProjectConfigurationPlatforms, so the project gets the exact Configuration and Platform selected in the IDE.
- In gdext.vcxproj, $(Configuration) toggles Debug/Release props, while $(Platform) is matched by Condition blocks to set GodotPlatform and Arch.
- Those values feed the NMake commands, which call scons with platform=$(GodotPlatform) arch=$(Arch) target=$(GodotTemplate).
- Result: choosing a Solution configuration in Rider/VS selects the matching vcxproj config and builds the right target for your OS/arch.
- sln and vcxproj are meant to be manually edited in Rider.

Notes:
- Full `godot-cpp-template` folder can be linked by context menu of the solution root (Add | Existing folder)
- Auxiliary project linking `demo` game contents can be added optionally in the `demo` folder. Rider plugins for Godot would provide language support, run configurations and other features for such projects.
- Theoretically there is an option to use non-msvc toolchain on Windows, but there are known problems with it in Rider debugger [RIDER-106816], so for now for this project on Windows MSBuild with MSVC toolchain is a requirement to work in Rider.
