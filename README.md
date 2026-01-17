
# My Experimental GameEngine

It's gonna be a big hole needs to be filled.

Tools used:

* cmake
* ninjia
* invoke

Build steps
***********

The task builds using ``cmake`` and ``Ninja``.

We're using [`invoke`](https://www.pyinvoke.org/) to avoid having to type the same things repeatedly on the
command line. It makes use of the ``tasks.py`` file in the root of our repo.


```

   $ inv --list
   Available tasks:

     build       Run builds via cmake.
     clean       Clean build directory.
     clean-all   Clean build and install directory.
     config      Run cmake configure.
     info        Show project info.
     install     Run install via cmake.
```

Build
=====

```
   # First time
   invoke config build
   # subsequently
   invoke build
```

Pre-requisites
==============

- ``gcc``

- ``cmake``

- ``ninja``

- ``invoke`` - Python and can be installed via ``pipx install invoke`` (`pipx`
  installs tools in an isolated virtual environment.)

One-time setup for vcpkg
========================

```
   git clone https://github.com/microsoft/vcpkg.git
   cd vcpkg
   ./bootstrap-vcpkg.sh -disableMetrics
```
Engine dependency
========================
```
vcpkg install glfw3:x64-mingw-static

```
Install vulkan sdk

[`Download`](https://vulkan.lunarg.com/sdk/home#windows) the vulkan sdk and follow the instructions for installation.
