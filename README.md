# godot-cpp template
This repository serves as a quickstart template for GDExtension development with Godot 4.0+.

## Contents
* An empty Godot project (`demo/`)
* godot-cpp as a submodule (`godot-cpp/`)
* GitHub Issues template (`.github/ISSUE_TEMPLATE.yml`)
* GitHub CI/CD to publish your library packages when creating a release (`.github/workflows/builds.yml`)
* preconfigured source files for C++ development of the GDExtension (`src/`)

## Usage
To use this template, log in to github and click the green "Use this template" button at the top of the repository page.
This will let you create a copy of this repository with a clean git history. Make sure you clone the correct branch as these are configured for development of their respective Godot development branches and differ from each other. Refer to the docs to see what changed between the versions.

For getting started after cloning your own copy to your local machine, you should: 
* initialize the godot-cpp git submodule via `git submodule update --init`
* change the name of your library
  * change the name of the compiled library file inside the `SConstruct` file by modifying the `libname` string.
  * change the pathnames of the to be loaded library name inside the `demo/bin/example.gdextension` file. By replacing `libgdexample` to the name specified in your `SConstruct` file.
  * change the name of the `demo/bin/example.gdextension` file
* change the `entry_symbol` string inside your `demo/bin/your-extension.gdextension` file to be configured for your GDExtension name. This should be the same as the `GDExtensionBool GDE_EXPORT` external C function. As the name suggests, this sets the entry function for your GDExtension to be loaded by the Godot editors C API.
* register the classes you want Godot to interact with inside the `register_types.cpp` file in the initialization method (here `initialize_gdextension_types`) in the syntax `ClassDB::register_class<CLASS-NAME>();`.
