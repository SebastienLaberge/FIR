#include <fstream>
#include <string>
#include <vector>

void writeKey(std::ofstream& os, const std::string& keyName);

template<typename T>
void writeKey(
  std::ofstream& os,
  const std::string& keyName,
  const T& keyValue);

void writeKey(
  std::ofstream& os,
  const std::string& keyName,
  double keyValue);

template<typename T>
void writeKey(
  std::ofstream& os,
  const std::string& keyName,
  const std::vector<T>& keyValue);

#include <writeKeys.inl>
