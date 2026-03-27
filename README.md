# okay-engine

`okay-engine` is a lightweight game engine that targets Windows, and Linux (particularly the Raspberry Pi), and (somewhat) MacOS. It was primarily developed to power `Northwestern Formula Racing`'s dashboard for NFR26, which runs off of a headless Rapsiban OS. Check out that project [here.](https://github.com/NU-Formula-Racing/daq-dash-26)

It is built using OpenGL 3.0 ES and C++20. For MacOS, slight hackery is employed to use OpenGL 3.3, so support may vary.

`okay-engine` is not intended to be a fully-featured game-engine, rather, it is an exercise of recreational programming and learning.

## Installation

### Prerequistes

Before installing `okay-engine` you must have:
* CMake, at least version 3.13
* Ninja
* Python 3 & pip
* Git
* A C++20 compiler, preferably Clang

A lot of the tooling (the shell scripts in `scripts/`) are designed with `bash` in mind, so please install that if you'd like to use those.

Any text editor will work, an integrated development enviornment like Visual Studio is untested.

### Overview

`okay-engine` is installed in a single location on your computer. With the installation, you get a few things:

* The `okay-engine` core, which contains the all the C++ and build files needed to build an `okay-engine` game. This also includes dependencies like `glm` and `glfw` for your target platform.
* The `okay-engine` CLI, which is a set of python scripts that are used for building, running, and packaging games
* The `okay` game, which is a test game you can build to test your installation.

### Steps

1. Clone the `okay-engine` repo.

```bash
git clone https://github.com/evan-bertis-sample/okay-engine.git --recursive
```

The recursive flag is important, as some needed libraries have been included as submodules.


2. Install the `okay-engine` CLI. This essentially just adds an alias within your `.bashrc` to point to `okay.py`.

```bash
cd okay-engine
source ./okay/scripts/install.bash
```

3. Try out the demo!

```bash
okay demo
```

### Integrating Intellisense

To enable intellisense, you must first build an `okay` project. For this repo, there is a demo project that you can build. Once any `okay` project is built, a `compile_commands.json` file is created in the build directory that can be used by your IDE to provide intellisense.

If intellisense does not work after building and restarting your IDE, then you can try running `okay index` in the root of the project. This will iterate through all of the `compile_commands.json` files in your project, and combine them into a single file. This new `compile_commands.json` file is placed into the root of the project, rather than the build directory. For repos that have multiple `okay` projects, this `compile_commands.json` will be inaccurate, as it will contain an amalgamation of all of the `compile_commands.json` files in the repo. For the most part, this should not be a problem, as you ideally keep a single `okay` project per repo/workspace.