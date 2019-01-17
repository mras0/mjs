# mjs

## Introduction
_mjs_ is an interpreter for the ECMAScript (a.k.a. Javascript)
[1st](https://www.ecma-international.org/publications/files/ECMA-ST-ARCH/ECMA-262,%201st%20edition,%20June%201997.pdf)
,
[3rd](http://www.ecma-international.org/publications/files/ECMA-ST-ARCH/ECMA-262,%203rd%20edition,%20December%201999.pdf)
and
[5(.1)th](http://www.ecma-international.org/ecma-262/5.1/ECMA-262.pdf)
editions written in C++17. It was written as a hobby project for the
purpose of gaining a better understanding of how Javascript actually
works. As such, it neither strives to be fast, secure nor particularly
beautiful.  In fact, some things are downright terribly implemented;
also no real programs were actually tested.

All ES1 and ES3 features should be implemented (to some
degree...), and full ES5.1 support is getting there, but conformance is
always work in progress.

See the [TODO](TODO.md) file some of the known pain points and the ES5
notes for missing features.

## Building

You need a modern C++ compiler that supports C++17 and CMake 3.7 or
later. It has currently been tested with the following compilers:

* Visual C++ 2017 15.9.5 on Windows 10
* GCC 7.1.0 (x64 distribution from [Stephan T. Lavavej](https://nuwen.net/mingw.html)) on Windows 10
* GCC 8.2.0 (on x64 Linux, Debian testing as of October 2018)
* Clang 7.0.0 (on Windows 10 and x64 Linux)

To run the tests build the `check` target.

The interpreter is built in the `src` directory. It accepts a Javascript
file on the command line or starts in REPL mode if no argument is given.
The ECMAScript version can be chosen by supplying it as a command line
argument (e.g. `mjs -es1`).

These steps should work for most people:

    mkdir build
    cd build
    cmake .. # Here you may want to use e.g. CXX=g++-8 cmake ..
    cmake --build .
    cmake --build . --target check

Several debugging features can also be enabled. Use your favorite CMake
GUI to view and toggle them (they are all off by default).

## Documentation

The project is currently light on the documentation front, but here is
what's available so far:

* [Implementing a Garbage Collector in
  C++](https://mras0.github.io/mjs/doc/gc/initial.html) about how the
  garbage collector was implemented
* [Supporting ECMAScript 3rd Edition](doc/es3/es3.md) on the work done
  to support ES3 (very incomplete)
* [Supporting ECMAScript 5th Edition](doc/es5/es5.md) on the work done
  to support ES5 (very incomplete)

## Support

Forget about it :) Feel free to raise an issue on GitHub, but don't
expect anything to come of it.
