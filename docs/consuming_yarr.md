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

```
cmake -S /path/to/yarr -B /path/to/build -DCMAKE_INSTALL_PREFIX=/desired/install/path

cmake --build /path/to/build

cmake --install /path/to/build
```

The install will place:

- Libraries into ${CMAKE_INSTALL_LIBDIR} (e.g., lib or lib64)

- Headers into ${CMAKE_INSTALL_INCLUDEDIR}

- CMake package config files into ${CMAKE_INSTALL_PREFIX}/cmake

Default if no install path is set is an in-source install. Yarr will try to avoid overwriting files which are under git control ie not all parts will be installed. If Yarr is added with add_subdirectory (or FetchContent) to a parent project then by default the parent project install path will be used for installation. This behaviour can be overriden by setting YARR_FORCE_OWN_INSTALL_PREFIX to true (install in own source directory even if a sub build was detected).
    
By default C++17 will be used but another version can be set and forced by (e.g.):

```
-DCMAKE_CXX_STANDARD=17
```

# How to consume Yarr as a dependency

Yarr supports several consumption methods which should cover most of the standard ways. Everything that is optional to be compiled is organized in form of components which are described in the bottom. In the following examples only random components are chosen.

## find_package()

After (separate) installation one can consume Yarr using:

```
find_package(Yarr REQUIRED COMPONENTS Spec Util)

target_link_libraries(MyTarget PRIVATE Yarr::Yarr Yarr::Spec Yarr::Util)
```

One must set CMAKE_PREFIX_PATH or Yarr_DIR if CMake cannot automatically find it:

```
cmake -DYarr_DIR=/path/to/install/cmake ...
```

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

## Binary config

You can provide a binary configuration tag to install output targets in a tdaq-like
structure.

```
set(YARR_INSTALL_BIN_CONFIG "${BINARY_TAG}" CACHE INTERNAL "Pass config to YARR")
FetchContent_Declare(
  yarr
  ...
)
```

This includes installing debug info in `$TAG/lib/.debug/libName.so.debug`.

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
| Yarr::     | ItsdaqFW            | dynamic library            |                                                              | ItsdaqFW    |
| Yarr::     | Rd53a               | dynamic library            |                                                              | Rd53a       |
| Yarr::     | Rd53aEmu            | dynamic library            |                                                              | Rd53aEmu    |
| Yarr::     | Rd53b               | dynamic library            |                                                              | Rd53b       |
| Yarr::     | Star                | dynamic library            |                                                              | Star        |
| Yarr::     | StarEmu             | dynamic library            |                                                              | StarEmu     |
| Yarr::     | _pyyarr             | dynamic library            |                                                              |             |
|            | felixbase4          | imported interface library | needed for NetioHW                                           |             |
|            | felix-interface     | imported interface library | needed for FelixCLient                                       |             |
| tbb::      | tbb                 | imported static library    | needed for NetioHW                                           |             |
| tbb::      | malloc              | imported static library    | needed for NetioHW                                           |             |
| tbb::      | malloc_proxy        | imported static library    | needed for NetioHW                                           |             |
|            | felix-client-thread | imported dynamic library   | needed for FelixClient                                       |             |
|            | netio               | imported dynamic library   | needed for NetioHW                                           |             |
|            | pybind11_headers    | imported interface library | only if python bindings are enabled                          |             |
|            | yarrspdlog          | imported interface library | needed by Yarr, currently modified version                   |             |
|            | tbb_2020            | utility target             | for ExternalProject_Add only; makes tbb:: available          |             |
|            | libfabric_ext_build | utility target             | for ExternalProject_Add only; if built-in libfabric for NetioHW |          |

These are organized into CMake components, so one can selectively request them using find_package(Yarr COMPONENTS ...).

This is the dependency graph:
![yarr libraries dependency graph](images/yarr_lib_dependency.png)

Dependency graph created by:

- cmake --graphviz=yarr_lib_dependency.dot ..
- dot -Tpng -o yarr_lib_dependency.png yarr_lib_dependency.dot

# Options and Dependencies
## YARR general
YARR is using currently a patched spdlog version. It defines an own target ALIAS called yarrspdlog. If a target spdlog::spdlog is already available due to being integrated in another project it will be used. Otherwise it will an installed version unless the variable "YARR_FORCE_FETCHCONTENT_SPDLOG" is set to true. As the last resort the patched version is installed. Both the yarrspdlog ALIAS as well as the real target SPDLOG::SPDLOG in case YARR install it on its own are provided to downstream users.
## Python
If the python bindings are enabled by "YARR_ENABLE_PYTHON" pybind11 is downloaded and added to the YARR project making it directly available. Installation is called by the internal function "YARR_ADD_PYBIND11()".
## BDAQ
BDAQ requires the BOOST::system library.
## NetioHW
NetioHW depends on felixbase4, netio4 and tbb. A patched cmake build file is used for most dependencies. netio4 depends on libfabric and some tests depend on ZeroMQ. Tests are by default not built and have to be enabled using the option "NETIO4_BUILD_TESTS" which means that ZeroMQ is be fault not required. felixbase4 depends also on its own directly on netio4.

If a target libfabric::libfabric is provided by a parent project it will be used. This target has been defined here as libfabric is autotools based and does not provide any cmake integration. Otherwise it tries to find an installed version of the libfabric library using the PKG system (can be steered by adjusting PKG_CONFIG_PATH) or searching standard system paths as fallback. If the library is not found on the system or if the option "NETIO4_FORCE_USE_BUILTIN_LIBFABRIC" is set to true, a standalone version will be downloaded and built and the target libfabric::libfabric will provide the static version to the netio4 library. Neither the libfabric library nor its target are installed and only used by netio4.

For ZeroMQ a similar strategy is used although it is simpler as ZeroMQ is cmake based and offers proper export functionality. If the standard ZeroMQ target libzmq is found - provided by a parent project - then it will be used. Otherwise it will be attempted to find a system version of the library unless the variable "NETIO4_FORCE_USE_BUILTIN_ZEROMQ" is set. As a last ressort the library will be downloaded, built and installed. An internal target netio4zmq is defined in all cases but neither netio4zmq nor libzmq are exported.

The netio library's target netio and the targets for the test binaries (if requested to be built) are exported and be used be downstream consumers. A namespace netio:: is defined. Installation of netio is defined in cmake/CMakeLists.txt.external and encapsulated in a function which is called by "YARR_ADD_NETIO()". Multiple calls are harmless.

tbb in the 2020 version before oneAPI is downloaded and built using the function "YARR_ADD_TBB()". The build system is completely patched as it did not support cmake export in that version yet. Three targets are defined: tbb::tbb, tbb::tbbmalloc and tbb::tbbmalloc_proxy providing the static version. The targets are not made available to downstream users.

felixbase4 depends on netio4. It is downloaded and installed using the function "YARR_ADD_FELIX()". The target felixbase4 in the namespace felixbase4 is defined and made available to downstream users.
## FelixClient
FelixClient depends on felix-client-thread which is installed by the function "YARR_ADD_FELIX_CLIENT()" together with its dependency felix-interface. Both of the build systems are patched and the export the targets "felix-client-thread" and "felix-interface" both within the namespace "felix::".
