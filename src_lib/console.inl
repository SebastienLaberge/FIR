#pragma once

#include <console.h>

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <utility>

void echo(const std::string& message)
{
  std::cout << message << std::endl;
}

template<typename... Args>
void print(Args&&... args)
{
  std::stringstream message;
  ((message << std::forward<Args>(args)), ...);
  message << std::endl;
  std::cout << message.str();
}

template<typename T>
void printValue(const std::string& name, const T& value)
{
  std::cout << name << ": " << value << std::endl;
}

template<typename T>
void printQuotedValue(const std::string& name, const T& value)
{
  std::cout << name << ": \"" << value << "\"" << std::endl;
}

template<typename T>
void printVector(
  const std::string& name,
  const std::vector<T>& values)
{
  std::cout << name << ": { ";

  bool first = true;
  for (const auto& value : values)
  {
    if (first)
    {
      first = false;
    }
    else
    {
      printf(", ");
    }
    std::cout << value;
  }
  std::cout << " }" << std::endl;
}

void printEmptyLine()
{
  std::cout << std::endl;
}

template<typename... Args>
void warning(Args&&... args)
{
  std::stringstream message;
  message << "WARNING: ";
  ((message << std::forward<Args>(args)), ...);
  message << std::endl;
  std::cout << message.str();
}

template<typename... Args>
void error(Args&&... args)
{
  std::stringstream message;
  message << "ERROR: ";
  ((message << std::forward<Args>(args)), ...);
  message << std::endl;
  throw std::runtime_error(message.str());
}
