#pragma once

#include <string>

/// String utilities

std::string strToUpper(const std::string& in);

/// Endianness utilities

// Returns true if current system is little-endian
bool systemIsLittleEndian();

// Swap endianness of input
template<typename T>
T swapEndianness(T u);

/// Filesystem utilities

// If path to data file is relative, prepend path to directory
// containing header file (changed in-place)
// TODO: Platform-independant implementation (std::filesystem)
void addPath(
  const std::string& headerFile,
  std::string& dataFile);

/// Multi-threading utilities

int getNThreads();

int getCurrentThread();

#include <tools.inl>
