#pragma once

#include <VolInterfileReader.h>

void VolInterfileReaderParams::setDefaults()
{
  dataFileName = std::string("");

  dataOffset = 0;

  dataTypeAsString = std::string("");
  dataType = DataTypeEnum::NONE;

  bytesPerPixel = 0;

  dataByteOrderAsString = std::string("");
  dataByteOrder = DataByteOrderEnum::NONE;
}

bool VolInterfileReader::hasDataFile()
{
  return !mParams.dataFileName.empty();
}

VolHeader VolInterfileReader::getHeader()
{
  return mHeader;
}

VolGeometry VolInterfileReader::getGeometry()
{
  return mGeometry;
}
