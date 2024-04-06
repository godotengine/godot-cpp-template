# godot-cpp template
This repository serves as a quickstart template for GDExtension development with Godot 4.0+.

## Contents
* An empty Godot project (`demo/`)
* godot-cpp as a submodule (`godot-cpp/`)
* GitHub Issues template (`.github/ISSUE_TEMPLATE.yml`)
* GitHub CI/CD to publish your library packages when creating a release (`.github/workflows/builds.yml`)
* preconfigured source files for C++ development of the GDExtension (`src/`)

## Usage - Actions

The actions builds `godot-cpp` at a specified location, and then builds the `gdextension` at a configurable location. It builds for desktop, mobile and web and allows for configuration on what platforms you need. It also supports configuration for debug and release builds, and for double builds.

The action uses SConstruct for both godot-cpp and the GDExtension that is built.

To reuse the build actions, in a github actions yml file, do the following:

```name: Build GDExtension
on:
  workflow_call:
  push:

jobs:
  build:
    strategy:
      fail-fast: false
      matrix:
        include:
          - platform: linux
            arch: x86_64
            os: ubuntu-20.04
          - platform: windows
            arch: x86_32
            os: windows-latest
          - platform: windows
            arch: x86_64
            os: windows-latest
          - platform: macos
            arch: universal
            os: macos-latest
          - platform: android
            arch: arm64
            os: ubuntu-20.04
          - platform: android
            arch: arm32
            os: ubuntu-20.04
          - platform: android
            arch: x86_64
            os: ubuntu-20.04
          - platform: android
            arch: x86_32
            os: ubuntu-20.04
          - platform: ios
            arch: arm64
            os: macos-latest
          - platform: web
            arch: wasm32
            os: ubuntu-20.04

    runs-on: ${{ matrix.os }}
    steps:
      - name: Checkout
        uses: actions/checkout@v3
        with:
          submodules: true
          fetch-depth: 0
      - name: 🔗 GDExtension Build
        uses: godotengine/godot-cpp-template/.github/actions/build@main
        with:
          platform: ${{ matrix.platform }}
          arch: ${{ matrix.arch }}
          float-precision: single
          build-target-type: template_release
      - name: Mac Sign
        if: ${{ matrix.platform == 'macos' && env.APPLE_CERT_BASE64 }}
        env:
          APPLE_CERT_BASE64: ${{ secrets.APPLE_CERT_BASE64 }}
        uses: godotengine/godot-cpp-template/.github/actions/sign@main
        with:
          FRAMEWORK_PATH: bin/macos/macos.framework
          APPLE_CERT_BASE64: ${{ secrets.APPLE_CERT_BASE64 }}
          APPLE_CERT_PASSWORD: ${{ secrets.APPLE_CERT_PASSWORD }}
          APPLE_DEV_PASSWORD: ${{ secrets.APPLE_DEV_PASSWORD }}
          APPLE_DEV_ID: ${{ secrets.APPLE_DEV_ID }}
          APPLE_DEV_TEAM_ID: ${{ secrets.APPLE_DEV_TEAM_ID }}
          APPLE_DEV_APP_ID: ${{ secrets.APPLE_DEV_APP_ID }}
      - name: Upload Artifact
        uses: actions/upload-artifact@v3
        with:
          name: GDExtension
          path: |
            ${{ github.workspace }}/bin/**

```

## Usage - Signing

You will need:

- A Mac
- An Apple ID enrolled in Apple Developer Program (99 USD per year)
- A `Resources/Info.plist` in the `framework` folder. Take the one in this project as an example. Be careful to set CFBundleExecutable to the **EXACT** lib name, otherwise it won't work. Also, don't put strange names in the CFBundleName and other such places. Try to only use letters and spaces. Errors will be extremly vague if not impossible to debug.

For the actions you will need to set the following inputs. Store them as secrets in GitHub:

- APPLE_CERT_BASE64
- APPLE_CERT_PASSWORD
- APPLE_DEV_ID
- APPLE_DEV_TEAM_ID
- APPLE_DEV_PASSWORD
- APPLE_DEV_APP_ID

You will find here a guide on how to create all of them. Go to [developer.apple.com](developer.apple.com):

- Create an Apple ID if you don’t have one already.
- Use your Apple ID to register in the Apple Developer Program.
- Accept all agreements from the Apple Developer Page.

### APPLE_DEV_ID - Apple ID

- Your email used for your Apple ID.

- APPLE_DEV_ID = email@provider.com

### APPLE_DEV_TEAM_ID - Apple Team ID

- Go to [developer.apple.com](https://developer.apple.com). Go to account.
- Go to membership details. Copy Team ID.

- APPLE_DEV_TEAM_ID = `1ABCD23EFG`

### APPLE_DEV_PASSWORD - Apple App-Specific Password

- Create [Apple App-Specific Password](https://support.apple.com/en-us/102654). Copy the password.

- APPLE_DEV_PASSWORD = `abcd-abcd-abcd-abcd`

### APPLE_CERT_BASE64 and APPLE_CERT_PASSWORD and APPLE_DEV_APP_ID

- Go to [developer.apple.com](https://developer.apple.com). Go to account.
- Go to certificates.
- Click on + at Certificates tab. Create Developer ID Application. Click Continue.
- Leave profile type as is. [Create a certificate signing request from a mac](https://developer.apple.com/help/account/create-certificates/create-a-certificate-signing-request). You can use your own name and email address. Save the file to disk. You will get a file called `CertificateSigningRequest.certSigningRequest`. Upload it to the Developer ID Application request. Click Continue.
- Download the certificate. You will get a file `developerID_application.cer`.
- On a Mac, right click and select open. Add it to the login keychain. In the Keychain Access app that opened, login Keychain tab, go to Keys, sort by date modified, expand your key (the key should have name you entered at common name `Common Name`), right click the expanded certificate, get info, and copy the text at Details -> Subject Name -> Common Name.
Eg.
- APPLE_DEV_APP_ID = `Developer ID Application: Common Name (1ABCD23EFG)`

- Then, select the certificate, right click and click export. At file format select p12. When exporting, set a password for the certificate. This will be APPLE_CERT_PASSWORD. You will get a `Certificates.p12` file.

Eg.
- APPLE_CERT_PASSWORD = `<password_set_when_exporting_p12>`

- Then you need to make a base64 file out of it, by running:
```
base64 -i Certificates.p12 -o Certificates.base64
```

- Copy the contents of the generated file:
Eg.
- APPLE_CERT_BASE64 = `...`(A long text file)

- While still

## Usage - Template

To use this template, log in to GitHub and click the green "Use this template" button at the top of the repository page.
This will let you create a copy of this repository with a clean git history. Make sure you clone the correct branch as these are configured for development of their respective Godot development branches and differ from each other. Refer to the docs to see what changed between the versions.

For getting started after cloning your own copy to your local machine, you should: 
* initialize the godot-cpp git submodule via `git submodule update --init`
* change the name of your library
  * change the name of the compiled library file inside the `SConstruct` file by modifying the `libname` string.
  * change the pathnames of the to be loaded library name inside the `demo/bin/example.gdextension` file. By replacing `libgdexample` to the name specified in your `SConstruct` file.
  * change the name of the `demo/bin/example.gdextension` file
* change the `entry_symbol` string inside your `demo/bin/your-extension.gdextension` file to be configured for your GDExtension name. This should be the same as the `GDExtensionBool GDE_EXPORT` external C function. As the name suggests, this sets the entry function for your GDExtension to be loaded by the Godot editors C API.
* register the classes you want Godot to interact with inside the `register_types.cpp` file in the initialization method (here `initialize_gdextension_types`) in the syntax `ClassDB::register_class<CLASS-NAME>();`.
