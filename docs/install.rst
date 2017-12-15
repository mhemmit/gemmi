
Installation
============

.. highlight:: none

C++ library
-----------

It is a header-only library. You need to ensure that
the ``include`` and ``third_party`` directories are in your include path
when compiling your program. For example::

    git clone https://github.com/project-gemmi/gemmi.git
    c++ -std=c++11 -Igemmi/include -Igemmi/third_party -O2 my_program.cpp

If you want Gemmi to uncompress gzipped files on the fly
(i.e. if you ``#include <gemmi/gz.hpp>``)
you will also need to link your program with the zlib library.

.. _install_py:

Python 2.7/3.x module
---------------------

To install the gemmi module you need pip, git and not too old
C++ compiler (GCC 4.8+, Clang 3.4+, MSVC 2015+, ICC 16+)::

    pip install git+https://github.com/project-gemmi/gemmi.git

(when the project is more mature and has regular releases, it will be simply
``pip install gemmi``).

Alternatively, clone the `project <https://github.com/project-gemmi/gemmi/>`_
or download a zip file, and from the top-level directory do::

    pip install .

On Windows Python 3.5+ should automatically find an appropriate compiler
(MSVC 2015+) . If the compiler is not installed, pip shows a message
with a download link.
For Python 2.7 pip prefers MSVC 2008, which is too old to compile gemmi.
You may still use MSVC 2015 or 2017, but before invoking pip you need to
set the compiler environment with one of these commands::

    "%VS140COMNTOOLS%\..\..\VC\vcvarsall.bat" x64
    "%VS140COMNTOOLS%\..\..\VC\vcvarsall.bat"

If you'd like to use PyPy instead of CPython
we support PyPy2.7 >= 5.7 (although we test it only occasionally).

Fortran 2003+
-------------

TODO

Utilities
---------

The library comes with a growing number of small command-line programs.
When the project is more mature we will provide binaries for Windows, Mac
and Linux. At this moment the utilities are tested only Linux and Mac
and need to be compiled from source::

    git clone https://github.com/project-gemmi/gemmi.git
    cd gemmi/src
    make