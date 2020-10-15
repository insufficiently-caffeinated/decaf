# Decaf - Prototype Symbolic Execution Engine

## Getting Dependencies
You'll need to set the `CMAKE_TOOLCHAIN_FILE` variable according to your
vscode install directory. Once that's done you can just run cmake as you
would normally to generate your build system.

The first build will take a while as it's going to build all the dependencies
and install them within a `vcpkg_installed` directory within the repo folder.
Once that's done you shouldn't have to worry about it anymore.

## Installation instructions

### macOS

- Install homebrew
  - Follow the instructions at <https://brew.sh>
- Install dependencies with homebrew
  - `brew update && brew install cmake boost llvm fmt z3`
- Install gtest
  - `git clone https://github.com/google/googletest` or `git clone git@github.com:google/googletest.git`
  - `cd googletest`
  - `mkdir build`
  - `cd build`
  - `cmake ..`
  - `make`
  - `make install`
- Run cmake and make
  - Find the path to llvm with `brew info llvm`, it should look something like `/usr/local/Cellar/llvm/10.0.1_1`
  - Run cmake, replacing `-DLLVM_DIR=...` with the path to llvm
    - `mkdir build`
    - `cd build`
    - `cmake .. -DLLVM_DIR=/usr/local/Cellar/llvm/10.0.1_1/lib/cmake/llvm/ -DCMAKE_C_COMPILER=/usr/bin/llvm-gcc -DCMAKE_CXX_COMPILER=/usr/bin/llvm-g++ -DCMAKE_CXX_FLAGS=-D_GNU_SOURCE=1`
    - `make`
