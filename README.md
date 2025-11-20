# okay-engine

Okay Engine is best described as a lightwork C++17 game engine that is able to target Windows and Linux-based systems. It was primarily developed to power `Northwestern Formula Racing`'s dashboard for NFR26, which runs off of a headless Rapsiban OS.

## Installation

### Prerequistes

Before installing `okay-engine` you must have:
* CMake, at least version 3.13
* Python 3 & pip
* Git

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
source ./okay/scipts/install.bash
```

1. Try to run the okay game!

```bash
cd okay-game
okay build
okay run
```