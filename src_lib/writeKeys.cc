#include <writeKeys.h>

#include <cstdio>

void writeKey(std::ofstream& os, const std::string& keyName)
{
  os << keyName << " :=" << std::endl;
}

// const's must be added because methods that write keys are
// declared as constant
void writeKey(
  std::ofstream& os,
  const std::string& keyName,
  double keyValue)
{
  char str[32];
  std::sprintf(str, "%.7f", keyValue);

  os << keyName << " := " << str << std::endl;
}
