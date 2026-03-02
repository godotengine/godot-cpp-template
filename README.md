# godot-object-compiler-template
This repository serves as a quickstart template for GDExtension development with Godot 4.0+ and Godot Object Compiler.

## Contents
* Preconfigured source files for C++ development of the GDExtension ([src/](./src/))
* An empty Godot project in [project/](./project), to test the GDExtension
* godot-cpp as a submodule (`godot-cpp/`)
* godot-object-compiler as a submodule (`godot-object-compiler/`)
* GitHub Issues template ([.github/ISSUE_TEMPLATE.yml](./.github/ISSUE_TEMPLATE.yml))
* GitHub CI/CD workflows to publish your library packages when creating a release ([.github/workflows/builds.yml](./.github/workflows/builds.yml))
* An SConstruct file with various functions, such as boilerplate for [Adding documentation](https://docs.godotengine.org/en/stable/tutorials/scripting/cpp/gdextension_docs_system.html)

## Usage - Template

To use this template, log in to GitHub and click the green "Use this template" button at the top of the repository page. This will let you create a copy of this repository with a clean git history.

To get started with your new GDExtension, do the following:

* clone your repository to your local computer
* initialize the godot-cpp and godot-object-compiler git submodules via `git submodule update --init`
* change the name of the compiled library file inside the [SConstruct](./SConstruct) file by modifying the `libname` string.
  * change the paths of the to be loaded library name inside the [project/bin/example.gdextension](./project/bin/example.gdextension) file, by replacing `EXTENSION-NAME` with the name you chose for `libname`.
* change the `entry_symbol` string inside [project/bin/example.gdextension](./project/bin/example.gdextension) file.
  * rename the `example_library_init` function in [src/register_types.cpp](./src/register_types.cpp) to the same name you chose for `entry_symbol`.
* change the name of the `project/bin/example.gdextension` file

Now, you can build the project with the following command:

```shell
scons
```

If the build command worked, you can test it with the [project](./project) project. Import it into Godot, open it, and launch the main scene. You should see it print the following line in the console:

```
Type: 24
```

## Usage - Godot Object Compiler
This repository comes reconfigured with Godot Object Compiler in both CMake and SConstruct.

The Godot Object Compiler is a code generation tool for GDExtensions. It generates bindings and other builderplate code for efficent development in C++ while maintaining full configurability via expressive macros generated directly from the godot-cpp source used to build your extension.

This repository contains a simple example class that is bound using Godot Object Compiler, in this case binding the print_type function using
the GODOT_FUNCTION attribute macro.

```cpp
	GODOT_CLASS();
	class ExampleClass : public RefCounted {
		GODOT_GENERATED_BODY();

	public:
		ExampleClass() = default;
		~ExampleClass() override = default;

		GODOT_FUNCTION();
		void print_type(const Variant &p_variant) const;
	};
```

Check out the [documentation](https://godot-object-compiler.readthedocs.io/latest/) for more information.

## Configuring an IDE
You can develop your own extension with any text editor and by invoking scons on the command line, but if you want to work with an IDE (Integrated Development Environment), you can use a compilation database file called `compile_commands.json`. Most IDEs should automatically identify this file, and self-configure appropriately.
To generate the database file, you can run one of the following commands in the project root directory:
```shell
# Generate compile_commands.json while compiling
scons compiledb=yes

# Generate compile_commands.json without compiling
scons compiledb=yes compile_commands.json
```

## Usage - Actions

This repository comes with continuous integration (CI) through a GitHub action that tests building the GDExtension.
It triggers automatically for each pushed change. You can find and edit it in [builds.yml](.github/workflows/ci.yml).

There is also a workflow ([make_build.yml](.github/workflows/make_build.yml)) that builds the GDExtension for all supported platforms that you can use to create releases.
You can trigger this workflow manually from the `Actions` tab on GitHub.
After it is complete, you can find the file `godot-cpp-template.zip` in the `Artifacts` section of the workflow run.
