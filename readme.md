# fea_getopt
<!-- [![Build status](https://ci.appveyor.com/api/projects/status/08nrt1tbdau7o3jq/branch/master?svg=true)](https://ci.appveyor.com/project/p-groarke/fea-utils/branch/master) -->

Header only c++17 argument parsing.

## Examples

```c++

```

## Build
`fea_getopt` is a header only library with dependencies to the stl and another header only lib, fea_utils.

The unit tests depend on gtest. They are not built by default. Use conan to install the dependencies when running the test suite.

### Windows
```
mkdir build && cd build
..\conan.bat
cmake .. -A x64 -DBUILD_TESTING=On && cmake --build . --config debug
bin\fea_getopt_tests.exe

// Optionally
cmake --build . --target install
```

### Unixes
```
mkdir build && cd build
..\conan.sh
cmake .. -DBUILD_TESTING=On && cmake --build . --config debug
bin\fea_getopt_tests.exe

// Optionally
cmake --build . --target install
```
