#include <VolInterfileReader.h>

#include <KeyParser.h>
#include <console.h>
#include <macros.h>
#include <tools.h>
#include <writeKeys.h>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <optional>

namespace
{
constexpr auto SHORT_FLOAT{"SHORT FLOAT"};
constexpr auto LONG_FLOAT{"LONG FLOAT"};
constexpr auto FLOAT{"FLOAT"};
constexpr auto UNSIGNED_INTEGER{"UNSIGNED INTEGER"};
constexpr auto SIGNED_INTEGER{"SIGNED INTEGER"};

constexpr auto LITTLEENDIAN = "LITTLEENDIAN";
constexpr auto BIGENDIAN = "BIGENDIAN";
}

void VolInterfileReaderParams::check()
{
  if (dataOffset < 0)
  {
    error("Data offset must not be negative");
  }

  // Convert enum strings to ALL CAPS
  dataTypeAsString = strToUpper(dataTypeAsString);
  dataByteOrderAsString = strToUpper(dataByteOrderAsString);

  // Set data type enum
  std::optional<int> bytesPerPixelFallbackDefault{std::nullopt};
  if (!dataTypeAsString.compare(SHORT_FLOAT))
  {
    dataType = DataTypeEnum::FLOAT;

    if (bytesPerPixel != 4)
    {
      bytesPerPixelFallbackDefault = 4;
    }
  }
  else if (!dataTypeAsString.compare(LONG_FLOAT))
  {
    dataType = DataTypeEnum::FLOAT;

    if (bytesPerPixel != 8)
    {
      bytesPerPixelFallbackDefault = 8;
    }
  }
  else if (!dataTypeAsString.compare(FLOAT))
  {
    dataType = DataTypeEnum::FLOAT;

    if (!(bytesPerPixel == 4 || bytesPerPixel == 8))
    {
      bytesPerPixelFallbackDefault = 4;
    }
  }
  else if (!dataTypeAsString.compare(UNSIGNED_INTEGER))
  {
    dataType = DataTypeEnum::UNSIGNED_INTEGER;

    if (!(bytesPerPixel == 1 || bytesPerPixel == 2 ||
          bytesPerPixel == 4 || bytesPerPixel == 8))
    {
      bytesPerPixelFallbackDefault = 4;
    }
  }
  else if (!dataTypeAsString.compare(SIGNED_INTEGER))
  {
    dataType = DataTypeEnum::SIGNED_INTEGER;

    if (!(bytesPerPixel == 1 || bytesPerPixel == 2 ||
          bytesPerPixel == 4 || bytesPerPixel == 8))
    {
      bytesPerPixelFallbackDefault = 4;
    }
  }
  else
  {
    error("Unrecognized data type name ", dataTypeAsString);
  }

  // Set bytes per pixel to default for type if value is invalid
  if (bytesPerPixelFallbackDefault.has_value())
  {
    bytesPerPixel = bytesPerPixelFallbackDefault.value();

    warning(
      "Invalid value for bytes per pixel. "
      "Using default for datatype ",
      dataTypeAsString,
      ", which is ",
      bytesPerPixel,
      ".");
  }

  // Set data byte order enum
  if (!dataByteOrderAsString.compare(BIGENDIAN))
  {
    dataByteOrder = DataByteOrderEnum::BIGENDIAN;
  }
  else if (!dataByteOrderAsString.compare(LITTLEENDIAN))
  {
    dataByteOrder = DataByteOrderEnum::LITTLEENDIAN;
  }
  else if (dataByteOrderAsString.empty())
  {
    // If ommited, select default according to current machine

    const auto isLittle = systemIsLittleEndian();

    dataByteOrderAsString = isLittle ? LITTLEENDIAN : BIGENDIAN;
    dataByteOrder = isLittle ? //
      DataByteOrderEnum::LITTLEENDIAN :
      DataByteOrderEnum::BIGENDIAN;

    warning(
      "Invalid value for imagedata byte order. "
      "Using default for current machine (",
      LITTLEENDIAN,
      ").");
  }
  else
  {
    error("Unrecognized byte order", dataByteOrderAsString);
  }
}

VolInterfileReader::VolInterfileReader(
  const std::string& headerFileName)
{
  KeyParser kp;

  kp.addStartKey("!INTERFILE");

  // Parameters for interfile reader
  kp.addKey("name of data file", &mParams.dataFileName);
  kp.addKey("data offset in bytes", &mParams.dataOffset);
  kp.addKey("number format", &mParams.dataTypeAsString);
  kp.addKey(
    "number of bytes per pixel",
    &mParams.bytesPerPixel);
  kp.addKey(
    "imagedata byte order",
    &mParams.dataByteOrderAsString);

  // Parameters for volume header

  // Volume size
  // Note: number of slices is used for the 3rd dimension
  // Note: The Gate simulator also accepts "number of images"
  kp.addKey("matrix size [1]", &mHeader.volSize.nPixelsX);
  kp.addKey("matrix size [2]", &mHeader.volSize.nPixelsY);
  kp.addKey("number of slices", &mHeader.volSize.nSlices);

  // Voxel dimensions
  // Note: slice thickness is used for the 3rd dimension
  kp.addKey(
    "scaling factor (mm/pixel) [1]",
    &mHeader.voxelExtent.pixelWidth);
  kp.addKey(
    "scaling factor (mm/pixel) [2]",
    &mHeader.voxelExtent.pixelHeight);
  kp.addKey(
    "slice thickness (pixels)",
    &mHeader.voxelExtent.sliceThickness);

  // First pixel offset
  // Note: Not used by the Gate simulator
  kp.addKey(
    "first pixel offset (mm) [1]",
    &mHeader.volOffset.x);
  kp.addKey(
    "first pixel offset (mm) [2]",
    &mHeader.volOffset.y);
  kp.addKey(
    "first pixel offset (mm) [3]",
    &mHeader.volOffset.z);

  // Number of frames
  kp.addKey("number of time frames", &mHeader.nFrames);

  kp.addStopKey("!END OF INTERFILE");

  // Set defaults
  mParams.setDefaults();
  mHeader.setDefaults();

  // Parse
  kp.parse(headerFileName.c_str());

  // If path to data file is relative, prepend path to header
  addPath(headerFileName, mParams.dataFileName);

  // Check values
  mParams.check();
  mHeader.check();

  // Compute derived parameters
  mGeometry.fill(mHeader);
}

template<typename VoxT>
static void readDataTemplate(
  const VolInterfileReaderParams& params,
  const VolHeader& header,
  const VolGeometry& geometry,
  std::vector<types::VoxelValue*>& frameVector)
{
  // Open data file
  std::ifstream stream;
  stream.open(params.dataFileName);
  if (!stream.is_open())
  {
    error("Couldn't open file ", params.dataFileName);
  }

  // Buffer large enough to store voxel data for one frame
  std::vector<VoxT> buffer(geometry.nVoxelsPerFrame);

  // Determine if endianness needs to be swapped
  const auto endiannessToBeSwapped = systemIsLittleEndian() ?
    params.dataByteOrder != DataByteOrderEnum::LITTLEENDIAN :
    params.dataByteOrder != DataByteOrderEnum::BIGENDIAN;

  // Skip a number of bytes equal to data offset
  stream.seekg(params.dataOffset, std::ios::beg);

  // Read data file into buffer and copy buffer into volume
  // Note: This is done frame by frame to use less memory
  LOOP(frame, 0, header.nFrames - 1)
  {
    // Read frame data
    stream.read(
      (char*)(&buffer[0]),
      geometry.nVoxelsPerFrame * sizeof(VoxT));

    // Throw if EOF found too early
    if (!stream)
    {
      error(
        "The number of pixels that were read from the data "
        "file (",
        stream.gcount(),
        ") is inferior to the number expected from the ",
        "header file (",
        geometry.nVoxelsTotal,
        ")");
    }

    // Copy buffer into mDataArray
    LOOP(i, 0, geometry.nVoxelsPerFrame - 1)
    {
      auto value = buffer[i];

      if (endiannessToBeSwapped)
      {
        value = swapEndianness(value);
      }

      frameVector[frame][i] =
        static_cast<types::VoxelValue>(value);
    }
  }
}

void VolInterfileReader::readData(
  std::vector<types::VoxelValue*>& frameVector)
{
  switch (mParams.dataType)
  {
  case DataTypeEnum::UNSIGNED_INTEGER:

    switch (mParams.bytesPerPixel)
    {
    case 1:
      readDataTemplate<std::uint8_t>(
        mParams,
        mHeader,
        mGeometry,
        frameVector);
      break;

    case 2:
      readDataTemplate<std::uint16_t>(
        mParams,
        mHeader,
        mGeometry,
        frameVector);
      break;

    case 4:
      readDataTemplate<std::uint32_t>(
        mParams,
        mHeader,
        mGeometry,
        frameVector);
      break;

    case 8:
      readDataTemplate<std::uint64_t>(
        mParams,
        mHeader,
        mGeometry,
        frameVector);
      break;
    }
    break;

  case DataTypeEnum::SIGNED_INTEGER:

    switch (mParams.bytesPerPixel)
    {
    case 1:
      readDataTemplate<std::int8_t>(
        mParams,
        mHeader,
        mGeometry,
        frameVector);
      break;

    case 2:
      readDataTemplate<std::int16_t>(
        mParams,
        mHeader,
        mGeometry,
        frameVector);
      break;

    case 4:
      readDataTemplate<std::int32_t>(
        mParams,
        mHeader,
        mGeometry,
        frameVector);
      break;

    case 8:
      readDataTemplate<std::int64_t>(
        mParams,
        mHeader,
        mGeometry,
        frameVector);
      break;
    }
    break;

  case DataTypeEnum::FLOAT:

    switch (mParams.bytesPerPixel)
    {
    case 4:
      readDataTemplate<float>(
        mParams,
        mHeader,
        mGeometry,
        frameVector);
      break;

    case 8:
      readDataTemplate<double>(
        mParams,
        mHeader,
        mGeometry,
        frameVector);
      break;
    }
    break;

  default:
    error("Invalid value for data type enum");
  }
}

template<typename VoxT>
static void writeVolInterfileDataTemplate(
  const std::vector<types::VoxelValue*>& frameVector,
  const std::string& outputVolDataFile,
  DataByteOrderEnum outputDataByteOrder,
  int nVoxelsPerFrame,
  int nFrames)
{
  // Open data file
  std::ofstream stream;
  stream.open(outputVolDataFile);
  if (!stream.is_open())
  {
    error("Couldn't create file ", outputVolDataFile);
  }

  // Determine if endianness needs to be swapped
  const auto endiannessToBeSwapped = systemIsLittleEndian() ?
    outputDataByteOrder != DataByteOrderEnum::LITTLEENDIAN :
    outputDataByteOrder != DataByteOrderEnum::BIGENDIAN;

  // Copy volume data into buffer and write buffer to file
  // Note: This is done frame by frame to use less memory
  std::vector<VoxT> buffer(nVoxelsPerFrame);
  LOOP(frame, 0, nFrames - 1)
  {
    LOOP(i, 0, nVoxelsPerFrame - 1)
    {
      auto value = static_cast<VoxT>(frameVector[frame][i]);

      if (endiannessToBeSwapped)
      {
        value = swapEndianness(value);
      }

      buffer[i] = value;
    }

    // Write buffer into data file
    stream.write(
      (char*)(&(buffer[0])),
      nVoxelsPerFrame * sizeof(VoxT));
  }
}

static void writeVolInterfileData(
  const std::vector<types::VoxelValue*>& frameVector,
  const std::string& outputVolDataFile,
  const VolInterfileReaderParams& params,
  int nVoxelsPerFrame,
  int nFrames)
{
  switch (params.dataType)
  {
  case DataTypeEnum::NONE:

    break;

  case DataTypeEnum::UNSIGNED_INTEGER:

    switch (params.bytesPerPixel)
    {
    case 1:
      writeVolInterfileDataTemplate<std::uint8_t>(
        frameVector,
        outputVolDataFile,
        params.dataByteOrder,
        nVoxelsPerFrame,
        nFrames);
      break;

    case 2:
      writeVolInterfileDataTemplate<std::uint16_t>(
        frameVector,
        outputVolDataFile,
        params.dataByteOrder,
        nVoxelsPerFrame,
        nFrames);
      break;

    case 4:
      writeVolInterfileDataTemplate<std::uint32_t>(
        frameVector,
        outputVolDataFile,
        params.dataByteOrder,
        nVoxelsPerFrame,
        nFrames);
      break;

    case 8:
      writeVolInterfileDataTemplate<std::uint64_t>(
        frameVector,
        outputVolDataFile,
        params.dataByteOrder,
        nVoxelsPerFrame,
        nFrames);
      break;
    }
    break;

  case DataTypeEnum::SIGNED_INTEGER:

    switch (params.bytesPerPixel)
    {
    case 1:
      writeVolInterfileDataTemplate<std::int8_t>(
        frameVector,
        outputVolDataFile,
        params.dataByteOrder,
        nVoxelsPerFrame,
        nFrames);
      break;

    case 2:
      writeVolInterfileDataTemplate<std::int16_t>(
        frameVector,
        outputVolDataFile,
        params.dataByteOrder,
        nVoxelsPerFrame,
        nFrames);
      break;

    case 4:
      writeVolInterfileDataTemplate<std::int32_t>(
        frameVector,
        outputVolDataFile,
        params.dataByteOrder,
        nVoxelsPerFrame,
        nFrames);
      break;

    case 8:
      writeVolInterfileDataTemplate<std::int64_t>(
        frameVector,
        outputVolDataFile,
        params.dataByteOrder,
        nVoxelsPerFrame,
        nFrames);
      break;
    }
    break;

  case DataTypeEnum::FLOAT:

    switch (params.bytesPerPixel)
    {
    case 4:
      writeVolInterfileDataTemplate<float>(
        frameVector,
        outputVolDataFile,
        params.dataByteOrder,
        nVoxelsPerFrame,
        nFrames);
      break;

    case 8:
      writeVolInterfileDataTemplate<double>(
        frameVector,
        outputVolDataFile,
        params.dataByteOrder,
        nVoxelsPerFrame,
        nFrames);
      break;
    }
    break;
  }
}

static void writeVolInterfileHeader(
  const std::string& outputVolHeaderFile,
  const VolInterfileReaderParams& params,
  const VolHeader& header)
{
  // Open header file
  std::ofstream stream;
  stream.open(outputVolHeaderFile);
  if (!stream.is_open())
  {
    error("Couldn't create file ", outputVolHeaderFile);
  }

  writeKey(stream, "!INTERFILE");

  stream << std::endl;
  writeKey(stream, "name of data file", params.dataFileName);
  writeKey(stream, "data offset in bytes", params.dataOffset);
  writeKey(stream, "number format", params.dataTypeAsString);
  writeKey(
    stream,
    "number of bytes per pixel",
    params.bytesPerPixel);
  writeKey(
    stream,
    "imagedata byte order",
    params.dataByteOrderAsString);

  stream << std::endl;
  constexpr int nDims = 3;
  writeKey(stream, "number of dimensions", nDims);

  stream << std::endl;
  writeKey(stream, "matrix size [1]", header.volSize.nPixelsX);
  writeKey(stream, "matrix size [2]", header.volSize.nPixelsY);
  writeKey(stream, "matrix size [3]", header.volSize.nSlices);
  writeKey(stream, "number of slices", header.volSize.nSlices);

  stream << std::endl;
  writeKey(
    stream,
    "scaling factor (mm/pixel) [1]",
    header.voxelExtent.pixelWidth);
  writeKey(
    stream,
    "scaling factor (mm/pixel) [2]",
    header.voxelExtent.pixelHeight);
  writeKey(
    stream,
    "scaling factor (mm/pixel) [3]",
    header.voxelExtent.sliceThickness);
  writeKey(
    stream,
    "slice thickness (pixels)",
    header.voxelExtent.sliceThickness);

  stream << std::endl;
  writeKey(
    stream,
    "first pixel offset (mm) [1]",
    header.volOffset.x);
  writeKey(
    stream,
    "first pixel offset (mm) [2]",
    header.volOffset.y);
  writeKey(
    stream,
    "first pixel offset (mm) [3]",
    header.volOffset.z);

  stream << std::endl;
  writeKey(stream, "number of time frames", header.nFrames);

  stream << std::endl;
  writeKey(stream, "!END OF INTERFILE");
}

void VolInterfileReader::writeVolInterfile(
  const std::string& outputVolFile,
  const VolHeader& header,
  const std::vector<types::VoxelValue*>& frameVector)
{
  std::filesystem::path outputVolHeaderFile(outputVolFile);
  outputVolHeaderFile.replace_extension(".h33");

  std::filesystem::path outputVolDataFile(outputVolFile);
  outputVolDataFile.replace_extension(".i33");

  // Interfile reader parameters
  VolInterfileReaderParams params;
  params.setDefaults();
  params.dataFileName = outputVolDataFile.filename();
  params.dataTypeAsString = FLOAT;
  params.bytesPerPixel = 4;
  params.dataByteOrderAsString = LITTLEENDIAN;
  params.check();

  const auto nVoxelsPerFrame = //
    header.volSize.nPixelsX *  //
    header.volSize.nPixelsY *  //
    header.volSize.nSlices;

  writeVolInterfileHeader(outputVolHeaderFile, params, header);

  writeVolInterfileData(
    frameVector,
    outputVolDataFile,
    params,
    nVoxelsPerFrame,
    header.nFrames);
}
