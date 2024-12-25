#pragma once

#include <string>
#include <vector>

/// Print to console

inline void echo(const std::string& message);

template<typename... Args>
void print(Args&&... args);

template<typename T>
void printValue(const std::string& name, const T& value);

template<typename T>
void printQuotedValue(const std::string& name, const T& value);

template<typename T>
void printVector(
  const std::string& name,
  const std::vector<T>& values);

inline void printEmptyLine();

/// Errors and warnings

template<typename... Args>
void warning(Args&&... args);

template<typename... Args>
void error(Args&&... args);

#include <console.inl>
