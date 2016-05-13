# Developer Lecture: Fly Compiler

## Installation

If you don't have it already, install [Homebrew][]. Then the following commands to install pre-requisites for the compiler:

    brew update
    brew bundle

## Development

The build system for compiling the compiler utilizes [CMake][] and [Ninja][]. However, there is a script to run the build easily:

    scripts/build

The produced executable will be output to `build/flyc`.

[Homebrew]: http://brew.sh/
[CMake]: https://cmake.org/
[Ninja]: https://ninja-build.org/
