# Decaf - Prototype Symbolic Execution Engine

## Getting Dependencies
You'll need to set the `CMAKE_TOOLCHAIN_FILE` variable according to your
vscode install directory. Once that's done you can just run cmake as you
would normally to generate your build system.

The first build will take a while as it's going to build all the dependencies
and install them within a `vcpkg_installed` directory within the repo folder.
Once that's done you shouldn't have to worry about it anymore.
