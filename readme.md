# fea_getopt
[![Build status](https://ci.appveyor.com/api/projects/status/v2exjtkw5699bpi4/branch/master?svg=true)](https://ci.appveyor.com/project/p-groarke/fea-getopt/branch/master)
[![Build Status](https://travis-ci.org/p-groarke/fea_getopt.svg?branch=master)](https://travis-ci.org/p-groarke/fea_getopt)

Header only c++17 argument parsing.

## Examples

```c++

```

## Build
`fea_getopt` is a header only library with dependencies to the stl and another header only libraries; fea_utils and fea_state_machines.

The unit tests depend on gtest. They are not built by default. Use conan to install the dependencies when running the test suite.

### Windows
```
mkdir build && cd build
..\conan.bat
cmake .. -DBUILD_TESTING=On && cmake --build . --config debug
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
