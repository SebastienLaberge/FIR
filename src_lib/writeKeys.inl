#pragma once

#include <writeKeys.h>

template<typename T>
void writeKey(
  std::ofstream& os,
  const std::string& keyName,
  const T& keyValue)
{
  os << keyName << " := " << keyValue << std::endl;
}

template<typename T>
void writeKey(
  std::ofstream& os,
  const std::string& keyName,
  const std::vector<T>& keyValue)
{
  os << keyName << " := {";

  const int size = keyValue.size();

  for (int i = 0; i < size; ++i)
  {
    os << keyName << keyValue[i] << std::endl;

    if (i != size - 1)
    {
      os << keyName << ",";
    }
  }

  os << keyName << "}" << std::endl;
}
