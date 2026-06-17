# Maestro

**NOTE: This side project is a work-in-progress and is not a substitute for
mature and actively maintained server libraries.**

Maestro is a barebones web server library. The goal of this project is to
develop a networking application using C++ without external frameworks,
including a hand-rolled standards-compliant HTTP parser and handler.

## Requirements

- A C++23+ compiler
- [CMake](https://cmake.org/)
- [GoogleTest](https://github.com/google/googletest) (installed automatically when building for the first time)

## Building

Ensure the current working directory is the root directory.

```
mkdir build
cd build
cmake ..
cmake --build .
```

## Testing

Build the project and ensure the current working directory is `build`.

```
ctest
```
