# mjs

## Introduction
_mjs_ is an interpreter for [ECMAScript 1st
edition](https://www.ecma-international.org/publications/files/ECMA-ST-ARCH/ECMA-262,%201st%20edition,%20June%201997.pdf)
(a.k.a. Javascript 1997) written in C++17. It was written as a hobby
project for the purpose of gaining a better understanding of how
Javascript actually works. As such, it neither strives to be fast,
secure nor particularly beautiful. In fact, some things are downright
terribly implemented; also no real programs were actually tested.

Most of ES1 should be implemented (to some degree...), but see the
[TODO](TODO.md) file for details. Also be aware that you probably
haven't programmed in ES1 before and will find most modern conveniences
(e.g. array and object literals) missing from the specification.

## Building

You need a modern C++ compiler that supports C++17 and CMake 3.7 or
later. It has currently been tested with the following compilers:

* Visual C++ 2017 15.8.8 on Windows 10
* GCC 7.1.0 (x64 distribution from [Stephan T. Lavavej](https://nuwen.net/mingw.html)) on Windows 10
* GCC 8.2.0 (on x64 Linux, Debian testing as of October 2018)

To run the tests build the `check` target.

The interpreter is built in the `src` directory. It accepts a Javascript
file on the command line or starts in REPL mode if no argument is given.

These steps should work for most people:

    mkdir build
    cd build
    cmake .. # Here you may want to use e.g. CXX=g++-8 cmake ..
    cmake --build .
    cmake --build . --target check

## Support

Forget about it :) Feel free to raise an issue on GitHub, but don't
expect anything to come of it.
