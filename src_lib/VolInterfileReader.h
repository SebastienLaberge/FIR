#pragma once

#include <VolData.h>
#include <types.h>

#include <string>
#include <vector>

enum class DataTypeEnum
{
  NONE,
  UNSIGNED_INTEGER, // 1, 2, 4 or 8 bytes per pixel
  SIGNED_INTEGER,   // 1, 2, 4 or 8 bytes per pixel
  FLOAT,            // 4 or 8 bytes per pixel
};

enum class DataByteOrderEnum
{
  NONE,
  LITTLEENDIAN, // Least significant byte is read first
  BIGENDIAN     // Most significant byte is read first
};

struct VolInterfileReaderParams
{
  // Path to the file containing the data
  std::string dataFileName;

  // Offset in the data file where the voxel data starts
  int dataOffset;

  // Data type for voxel values
  // Note: The string is set first and check() sets the enum
  std::string dataTypeAsString;
  DataTypeEnum dataType;

  // Number of bytes per voxel
  int bytesPerPixel;

  // Order of bytes for each voxel
  // Note: The string is set first and check() sets the enum
  std::string dataByteOrderAsString;
  DataByteOrderEnum dataByteOrder;

  // Fill structure with default values
  // Note: Must be executed before filling with specific values
  inline void setDefaults();

  // Check validity of values and throw if there is a problem
  // Note: Must executed after filling with specific values
  void check();
};

class VolInterfileReader
{
public:

  // Initialize reader by specifying path to a header file
  // This reads the header file and fills data structures
  VolInterfileReader(const std::string& headerFileName);

  // Get information contained in or derived from header file
  inline bool hasDataFile();
  inline VolHeader getHeader();
  inline VolGeometry getGeometry();

  // Read the data file pointed to by the header file
  void readData(std::vector<types::VoxelValue*>& frameVector);

  // Static method to write a volume to file
  static void writeVolInterfile(
    const std::string& outputVolFile,
    const VolHeader& header,
    const std::vector<types::VoxelValue*>& frameVector);

private:

  void checkVoxelType();

  VolInterfileReaderParams mParams;

  VolHeader mHeader;
  VolGeometry mGeometry;
};

#include <VolInterfileReader.inl>
