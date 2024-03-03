# XDDSP

A not-so-simple C++ template library for creating complex DSP networks.

## Description

I had lots of DSP code snippets lying around and I wanted an easy way to tie them together into DSP networks.

My goals where:
 - A common base class for components
 - Named inputs and outputs for each component
 - Maximising code-encapsulation and reusability
 - Portability
 - As many opportunities for automatic optimisation as possible
 - Independence from other libraries (except for the C++ standard library)

At this early stage, my code is not readable or well commented. I will be creating a series of YouTube videos where I explain the code and will be building examples using Juce.

## Usage

I will be attempting to structure the code so that this repository can be included as a sub-repository in other projects easily.

Your project just needs to compile and link XDDSP.cpp and include the XDDSP.h header file

    #include "[path_to_library]/XDDSP.h"

