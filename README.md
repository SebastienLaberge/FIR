# FIR

## Description

FIR (Faithful Iterative Reconstruction) is a work-in-progress library for implementing iterative tomographic reconstruction algorithms featuring a precise modeling of a PET scanner's geometry.

The library is implemented in C++, is compiled using CMake and is tested using the gtest library.

Included are implementations of the following executables:
- forward projection,
- backprojection, and
- the OSEM reconstruction algorithm.

Pending the completion of a proper Python wrapper, a Python package called PyInteractor is included for preparing input files and launching executables from the Python language.

## Dependencies

### Software tools and libraries

- CMake
- gtest

### Python packages available on the Python Package Index

- numpy

## Content

### src_bin/

This directory contains the source code for executables implemented using the FIR library.

- ForwardProj.cc  
  => Forward projection from image space to tomographic space

- BackProj.cc  
  => Backprojection from tomographic space to image space

- OSEM.cc  
  => Image reconstruction using Ordered Subset Expectation Maximization

### src_lib/

This directory contains the source code of the FIR library proper.

#### I/O

- KeyParser.h/.cc
- writeKeys.h/.inl

#### Common

- macros.h
- console.h/.inl
- types.h/.inl
- tools.h/.inl/.cc

#### Voxelized volume data structure

- VolHeader.h/.inl/.cc
- VolInterfileReader.h/.inl/.cc
- VolData.h/.inl/.cc

#### Scanner data structure

- ScannerHeader.h/.cc
- ScannerInterfileReader.h/.cc
- ScannerData.h/.inl/.cc

#### Projection data structure

- ProjHeader.h/.inl/.cc
- ProjInterfileReader.h/.inl/.cc
- ProjData.h/.inl/.cc

#### LOR computation

- Siddon.h/.cc
- LORCache.h/.cc

#### Main operations

- operations.h/.cc
- projections.h/.cc
- reconAlgos.h/.cc

### src_test/

This directory contains code for testing the FIR library.

- ProjHeaderUnitTest.cc
- ProjInterfileReaderUnitTest.cc

### PyInterface/

This directory contains a Python package allowing interaction with the FIR executables from the Python language.

#### Interaction utilities

- call_exec.py
- interfile_io.py

#### Main class proxys

- vol.py
- scanner.py
- proj.py

#### Conversions

- vol_conversions.py
- proj_reader.py

#### Executable callers

- forward.py
- backward.py
- OSEM.py

#### Display

- scanner_generator.py
