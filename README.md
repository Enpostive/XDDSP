# XDDSP

A not-so-simple C++ template library for creating complex DSP networks.

## Description

I had lots of DSP code snippets lying around and I wanted an easy way to tie them together into DSP networks.

My goals where:
 - A common base class for components.
 - Component inputs and outputs to have human readable names.
 - Maximising code-encapsulation and reusability.
 - Portability.
 - As many opportunities for automatic optimisation as possible.
 - Independence from other libraries (except for the C++ standard library).

I am in the process of creating doxygen documentation for the library. Watch this space.

## Usage

I will be attempting to structure the code so that this repository can be included as a sub-repository in other projects easily.

Your project just needs to include the XDDSP.h header file and use the XDDSP_GLOBAL macro in one place to include the optional global data structures used by some of the code.

