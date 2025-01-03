#include <Siddon.h>
#include <VolData.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <array>

namespace
{
const double TOLERANCE = 1e-7;

void CheckPath(
  const int line,
  const Siddon& siddon,
  std::array<double, 3> point1,
  std::array<double, 3> point2,
  std::vector<double> expectedCoords,
  std::vector<double> expectedLengths)
{
  const auto pathElementsArray =
    siddon.getThreadLocalPathElements();

  siddon.computePath(
    point1[0],
    point1[1],
    point1[2],
    point2[0],
    point2[1],
    point2[2],
    pathElementsArray);

  const auto expectedLength = expectedCoords.size();
  auto actualLength = 0;

  for (auto pathIndex = 0;
       pathElementsArray[pathIndex].coord != -1;
       ++pathIndex, ++actualLength)
  {
    ASSERT_LT(pathIndex, expectedLength)
      << "For test at line " << line << ", "
      << "resulting path is longer than expected "
      << "(expected length: " << expectedLength << ")";

    const auto actualCoord = pathElementsArray[pathIndex].coord;
    const auto expectedCoord = expectedCoords[pathIndex];
    ASSERT_EQ(actualCoord, expectedCoord)
      << "For test at line " << line << ", "
      << "invalid coordinate at path index " << pathIndex << " "
      << "(expected value: " << expectedCoord << ", "
      << "actual value: " << actualCoord << ")";

    const auto actualLength =
      pathElementsArray[pathIndex].length;
    const auto expectedLength = expectedLengths[pathIndex];
    ASSERT_LT(std::abs(actualLength - expectedLength), TOLERANCE)
      << "For test at line " << line << ", "
      << "invalid length at path index " << pathIndex << " "
      << "(expected value: " << expectedLength << ", "
      << "actual value: " << actualLength << ")";
  }

  ASSERT_EQ(actualLength, expectedLength)
    << "For test at line " << line << ", "
    << "resulting path is shorter than expected "
    << "(expected length: " << expectedLength << ", "
    << "actual length: " << actualLength << ")";
}
}

TEST(SiddonUnitTest, OrthogonalPaths)
{
  const auto volSizeX = 3;
  const auto volSizeY = 3;
  const auto volSizeZ = 2;

  const auto voxelExtentX = 1.0;
  const auto voxelExtentY = 2.0;
  const auto voxelExtentZ = 3.0;

  // Centered volume
  const auto volOffsetX = -(volSizeX - 1) * voxelExtentX / 2.0;
  const auto volOffsetY = -(volSizeY - 1) * voxelExtentY / 2.0;
  const auto volOffsetZ = 0;

  const auto nFrames = 1;

  VolData vol({
    {    volSizeX,     volSizeY,     volSizeZ},
    {voxelExtentX, voxelExtentY, voxelExtentZ},
    {  volOffsetX,   volOffsetY,   volOffsetZ},
    nFrames
  });

  Siddon siddon(vol);

  const auto leftPlane = volOffsetX - voxelExtentX;
  const auto rightPlane =
    leftPlane + (volSizeX + 1) * voxelExtentX;

  const auto topPlane = volOffsetY - voxelExtentY;
  const auto bottomPlane =
    topPlane + (volSizeY + 1) * voxelExtentY;

  const auto zSlice0 = -voxelExtentZ / 2.0;
  const auto zSlice1 = voxelExtentZ / 2.0;

  const auto frontPlane = zSlice0 - voxelExtentZ;
  const auto backPlane = zSlice1 + voxelExtentZ;

  // Horizontal, slice 0

  CheckPath(
    __LINE__,
    siddon,
    {leftPlane, -voxelExtentY, zSlice0},
    {rightPlane, -voxelExtentY, zSlice0},
    {0, 1, 2},
    {voxelExtentX, voxelExtentX, voxelExtentX});

  CheckPath(
    __LINE__,
    siddon,
    {leftPlane, 0, zSlice0},
    {rightPlane, 0, zSlice0},
    {3, 4, 5},
    {voxelExtentX, voxelExtentX, voxelExtentX});

  CheckPath(
    __LINE__,
    siddon,
    {leftPlane, voxelExtentY, zSlice0},
    {rightPlane, voxelExtentY, zSlice0},
    {6, 7, 8},
    {voxelExtentX, voxelExtentX, voxelExtentX});

  // Horizontal, slice 1

  CheckPath(
    __LINE__,
    siddon,
    {leftPlane, -voxelExtentY, zSlice1},
    {rightPlane, -voxelExtentY, zSlice1},
    {9, 10, 11},
    {voxelExtentX, voxelExtentX, voxelExtentX});

  CheckPath(
    __LINE__,
    siddon,
    {leftPlane, 0, zSlice1},
    {rightPlane, 0, zSlice1},
    {12, 13, 14},
    {voxelExtentX, voxelExtentX, voxelExtentX});

  CheckPath(
    __LINE__,
    siddon,
    {leftPlane, voxelExtentY, zSlice1},
    {rightPlane, voxelExtentY, zSlice1},
    {15, 16, 17},
    {voxelExtentX, voxelExtentX, voxelExtentX});

  // Vertical, slice 0

  CheckPath(
    __LINE__,
    siddon,
    {-voxelExtentX, topPlane, zSlice0},
    {-voxelExtentX, bottomPlane, zSlice0},
    {0, 3, 6},
    {voxelExtentY, voxelExtentY, voxelExtentY});

  CheckPath(
    __LINE__,
    siddon,
    {0, topPlane, zSlice0},
    {0, bottomPlane, zSlice0},
    {1, 4, 7},
    {voxelExtentY, voxelExtentY, voxelExtentY});

  CheckPath(
    __LINE__,
    siddon,
    {voxelExtentX, topPlane, zSlice0},
    {voxelExtentX, bottomPlane, zSlice0},
    {2, 5, 8},
    {voxelExtentY, voxelExtentY, voxelExtentY});

  // Vertical, slice 1

  CheckPath(
    __LINE__,
    siddon,
    {-voxelExtentX, topPlane, zSlice1},
    {-voxelExtentX, bottomPlane, zSlice1},
    {9, 12, 15},
    {voxelExtentY, voxelExtentY, voxelExtentY});

  CheckPath(
    __LINE__,
    siddon,
    {0, topPlane, zSlice1},
    {0, bottomPlane, zSlice1},
    {10, 13, 16},
    {voxelExtentY, voxelExtentY, voxelExtentY});

  CheckPath(
    __LINE__,
    siddon,
    {voxelExtentX, topPlane, zSlice1},
    {voxelExtentX, bottomPlane, zSlice1},
    {11, 14, 17},
    {voxelExtentY, voxelExtentY, voxelExtentY});

  // Orthogonal

  CheckPath(
    __LINE__,
    siddon,
    {-voxelExtentX, -voxelExtentY, frontPlane},
    {-voxelExtentX, -voxelExtentY, backPlane},
    {0, 9},
    {voxelExtentZ, voxelExtentZ, voxelExtentZ});

  CheckPath(
    __LINE__,
    siddon,
    {0, -voxelExtentY, frontPlane},
    {0, -voxelExtentY, backPlane},
    {1, 10},
    {voxelExtentZ, voxelExtentZ, voxelExtentZ});

  CheckPath(
    __LINE__,
    siddon,
    {0, voxelExtentY, frontPlane},
    {0, voxelExtentY, backPlane},
    {7, 16},
    {voxelExtentZ, voxelExtentZ, voxelExtentZ});

  CheckPath(
    __LINE__,
    siddon,
    {voxelExtentX, voxelExtentY, frontPlane},
    {voxelExtentX, voxelExtentY, backPlane},
    {8, 17},
    {voxelExtentZ, voxelExtentZ, voxelExtentZ});

  // Starting from inside the volume and going backwards

  CheckPath(
    __LINE__,
    siddon,
    {0, -voxelExtentY, zSlice0},
    {leftPlane, -voxelExtentY, zSlice0},
    {1, 0},
    {voxelExtentX / 2.0, voxelExtentX});

  CheckPath(
    __LINE__,
    siddon,
    {-voxelExtentX, 0, zSlice0},
    {-voxelExtentX, topPlane, zSlice0},
    {3, 0},
    {voxelExtentY / 2.0, voxelExtentY});

  CheckPath(
    __LINE__,
    siddon,
    {-voxelExtentX, -voxelExtentY, zSlice1},
    {-voxelExtentX, -voxelExtentY, frontPlane},
    {9, 0},
    {voxelExtentZ / 2.0, voxelExtentZ});

  // Stopping inside the volume and going backwards

  CheckPath(
    __LINE__,
    siddon,
    {rightPlane, -voxelExtentY, zSlice0},
    {0, -voxelExtentY, zSlice0},
    {2, 1},
    {voxelExtentX, voxelExtentX / 2.0});

  CheckPath(
    __LINE__,
    siddon,
    {-voxelExtentX, bottomPlane, zSlice0},
    {-voxelExtentX, 0, zSlice0},
    {6, 3},
    {voxelExtentY, voxelExtentY / 2.0});

  CheckPath(
    __LINE__,
    siddon,
    {-voxelExtentX, -voxelExtentY, backPlane},
    {-voxelExtentX, -voxelExtentY, zSlice0},
    {9, 0},
    {voxelExtentZ, voxelExtentZ / 2.0});
}
