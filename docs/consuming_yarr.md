# CMake Compatibility

Yarr is a fully CMake-compatible project.  It tries to follow modern CMake best practices:

- Targets are defined using target_* commands.

- Namespaced targets (Yarr::) are provided.

- Configuration files (YarrConfig.cmake) are generated for easy find_package() usage.

- Installation exports a complete CMake package.

- Relative RPATH is configured for correct runtime library lookup after installation.

Supported minimum CMake version: 3.14.
# Installation

Here a short summary of the installation with a focus on installation as a dependency. Yarr can be installed into a separate directory from the build directory by setting the standard CMAKE_INSTALL_PREFIX during configuration:

cmake -S /path/to/yarr -B /path/to/build -DCMAKE_INSTALL_PREFIX=/desired/install/path

cmake --build /path/to/build

cmake --install /path/to/build

The install will place:

- Libraries into ${CMAKE_INSTALL_LIBDIR} (e.g., lib or lib64)

- Headers into ${CMAKE_INSTALL_INCLUDEDIR}

- CMake package config files into ${CMAKE_INSTALL_PREFIX}/cmake

Default if no install path is set is an in-source install. Yarr will try to avoid overwriting files which are under git control ie not all parts will be installed. If Yarr is added with add_subdirectory (or FetchContent) to a parent project then by default the parent project install path will be used for installation. This behaviour can be overriden by setting YARR_FORCE_OWN_INSTALL_PREFIX to true (install in own source directory even if a sub build was detected).
    
By default C++17 will be used but another version can be set and forced by (e.g.):

- -DCMAKE_CXX_STANDARD=17

# How to consume Yarr as a dependency

Yarr supports several consumption methods which should cover most of the standard ways. Everything that is optional to be compiled is organized in form of components which are described in the bottom. In the following examples only random components are chosen.

## find_package()

After (separate) installation one can consume Yarr using:

find_package(Yarr REQUIRED COMPONENTS Spec Util)

target_link_libraries(MyTarget PRIVATE Yarr::Yarr Yarr::Spec Yarr::Util)

One must set CMAKE_PREFIX_PATH or Yarr_DIR if CMake cannot automatically find it:

cmake -DYarr_DIR=/path/to/install/cmake ...

## add_subdirectory()

If one wants to embed Yarr directly as a subproject (for development or tight coupling), it can be added via:

```
add_subdirectory(/path/to/yarr)
target_link_libraries(MyTarget PRIVATE Yarr::Yarr Yarr::Spec Yarr::Util)
```

No installation is needed in this case. All targets get automatically exposed to the parent project and be default all of the parent project will be applied.

## FetchContent

One can also use FetchContent to fetch Yarr at configure time:

```
include(FetchContent)

FetchContent_Declare(
  Yarr
  SOURCE_DIR /path/to/local/yarr # or GIT_REPOSITORY https://gitlab.cern.ch/YARR/YARR.git
  # for further options check the cmake manual
)

FetchContent_MakeAvailable(Yarr)

target_link_libraries(MyTarget PRIVATE Yarr::Yarr Yarr::Spec Yarr::Util)
```

FetchContent does not perform a full installation step — Yarr becomes part of the parent project’s build tree and all targets get exposed.

## ExternalProject_Add

One can also use the older ExternalProject_Add which can be useful for a super build approach. I.e. 

```
ExternalProject_Add(Yarr
  SOURCE_DIR /path/to/local/yarr # or GIT_REPOSITORY https://gitlab.cern.ch/YARR/YARR.git
  CMAKE_ARGS
    -DCMAKE_INSTALL_PREFIX=${YARR_INSTALL_DIR}
    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
  INSTALL_COMMAND ${CMAKE_COMMAND} --install . --prefix ${YARR_INSTALL_DIR}
)
```

In this case the installation is isolated and one has to use find_package or inject directly the directories. For an example of the super build architecture have a look at the external_project_superbuild test.

# Exported Targets and Components

The following CMake targets are exported by Yarr (executables not listed; fully printed during cmake execution):
| Name space | Target              | Type                       | Comments                                                     | Component   |
|------------|---------------------|----------------------------|--------------------------------------------------------------|-------------|
| Yarr::     | Scan                | dynamic library            |                                                              |             |
| Yarr::     | Util                | dynamic library            |                                                              |             |
| Yarr::     | Util_interface      | interface library          | target exposing only Util headers used for cyclic dependency |             |
| Yarr::     | Yarr                | dynamic library            |                                                              |             |
| Yarr::     | Yarr_interface      | interface library          | target exposing only Yarr headers used for cyclic dependency |             |
| Yarr::     | Bdaq                | dynamic library            |                                                              | Bdaq        |
| Yarr::     | Emu                 | dynamic library            |                                                              | Emu         |
| Yarr::     | FelixClient         | dynamic library            |                                                              | FelixClient |
| Yarr::     | NetioHW             | dynamic library            |                                                              | NetioHW     |
| Yarr::     | Spec                | dynamic library            |                                                              | Spec        |
| Yarr::     | Fei4                | dynamic library            |                                                              | Fei4        |
| Yarr::     | Fei4Emu             | dynamic library            |                                                              | Fei4Emu     |
| Yarr::     | Itkpixv2            | dynamic library            |                                                              | Itkpixv2    |
| Yarr::     | Itkpixv2Emu         | dynamic library            |                                                              | Itkpixv2Emu |
| Yarr::     | Rd53a               | dynamic library            |                                                              | Rd53a       |
| Yarr::     | Rd53aEmu            | dynamic library            |                                                              | Rd53aEmu    |
| Yarr::     | Rd53b               | dynamic library            |                                                              | Rd53b       |
| Yarr::     | Star                | dynamic library            |                                                              | Star        |
| Yarr::     | StarEmu             | dynamic library            |                                                              | StarEmu     |
| tbb::      | tbb                 | imported static library    |                                                              |             |
| tbb::      | malloc              | imported static library    |                                                              |             |
| tbb::      | malloc_proxy        | imported static library    |                                                              |             |
|            | yarrspdlog          | imported interface library |                                                              |             |
|            | tbb_2020            | utility target             | for ExternalProject_Add only; makes tbb:: available          |             |
|            | felix_client_thread | utility target             | for ExternalProject_Add only                                 |             |
|            | netio4              | utility target             | for ExternalProject_Add only                                 |             |
|            | felixbase4          | utility target             | for ExternalProject_Add only                                 |             |

These are organized into CMake components, so one can selectively request them using find_package(Yarr COMPONENTS ...).

This is the dependency graph:
![yarr libraries dependency graph](images/yarr_lib_dependency.png)

Dependency graph created by:
- cmake --graphviz=yarr_lib_dependency.dot ..
- dot -Tpng -o yarr_lib_dependency.png yarr_lib_dependency.dot
