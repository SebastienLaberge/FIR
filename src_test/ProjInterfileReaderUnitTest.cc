#include <ProjInterfileReader.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

const std::string PROJ_HEADER_FILE{"TestResources/demo.hs"};

TEST(ProjInterfileReaderUnitTest, Constructor)
{
  ProjInterfileReader reader(PROJ_HEADER_FILE);

  std::cout << testing::TempDir() << std::endl;
}
