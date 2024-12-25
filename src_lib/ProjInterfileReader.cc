#include <ProjInterfileReader.h>

#include <KeyParser.h>
#include <console.h>
#include <macros.h>
#include <tools.h>
#include <writeKeys.h>

#include <filesystem>
#include <fstream>
#include <vector>

ProjInterfileReader::ProjInterfileReader(
  const std::string& headerFileName)
{
  // Set default values
  mHeader.setDefaults();

  KeyParser kp;

  kp.addStartKey("PROJECTION DATA PARAMETERS");

  kp.addKey("name of data file", &mDataFileName);

  kp.addKey("number of rings", &mHeader.nRings);
  kp.addKey(
    "number of crystals per ring",
    &mHeader.nCrystalsPerRing);
  kp.addKey("segment span", &mHeader.segmentSpan);
  kp.addKey("number of segments", &mHeader.nSegments);
  kp.addKey(
    "number of tangential coordinates",
    &mHeader.nTangCoords);

  kp.addStopKey("END OF PROJECTION DATA PARAMETERS");

  kp.parse(headerFileName);

  // If path to data file is relative, prepend path to header
  addPath(headerFileName, mDataFileName);

  // Check header values
  mHeader.check();

  // Compute derived parameters
  mGeometry.fill(mHeader);
}

void ProjInterfileReader::readData(types::BinValue** dataArray)
{
  // Open data file
  std::ifstream is;
  is.open(mDataFileName);
  if (!is.is_open())
  {
    error("Couldn't open file ", mDataFileName);
  }

  // Read data file into buffer
  std::vector<types::BinValue> temp(mGeometry.nBins);
  is.read(
    (char*)(&(temp[0])),
    mGeometry.nBins * sizeof(types::BinValue));

  if (!is)
  {
    error(
      "The number of bins that were read from the data file (",
      is.gcount(),
      ") is inferior to that expected from the header file (",
      mGeometry.nBins,
      ")");
  }

  is.close();

  // Copy buffer into mDataArray
  int elementsRead = 0;
  LOOP(seg, -mGeometry.segOffset, mGeometry.segOffset)
  {
    const auto elementsToRead =        //
      mGeometry.nViews *               //
      mGeometry.getNAxialCoords(seg) * //
      mHeader.nTangCoords;

    LOOP(i, 0, elementsToRead - 1)
    {
      dataArray[seg + mGeometry.segOffset][i] =
        (types::BinValue)temp[elementsRead + i];
    }

    elementsRead += elementsToRead;
  }
}

static void writeData(
  const std::string& outputProjDataFile,
  const ProjHeader& header,
  const ProjGeometry& geometry,
  types::BinValue** dataArray)
{
  // Open data file
  std::ofstream os;
  os.open(outputProjDataFile);
  if (!os.is_open())
  {
    error("Couldn't create file ", outputProjDataFile);
  }

  // Copy projection data into buffer
  std::vector<types::BinValue> temp(geometry.nBins);
  int elementsWritten = 0;
  LOOP(seg, -geometry.segOffset, geometry.segOffset)
  {
    const auto elementsToWrite =      //
      geometry.nViews *               //
      geometry.getNAxialCoords(seg) * //
      header.nTangCoords;

    LOOP(i, 0, elementsToWrite - 1)
    {
      temp[elementsWritten + i] =
        dataArray[seg + geometry.segOffset][i];
    }

    elementsWritten += elementsToWrite;
  }

  // Write projection data into data file
  os.write(
    (char*)(&(temp[0])),
    geometry.nBins * sizeof(types::BinValue));

  os.close();
}

void ProjInterfileReader::writeProjInterfile(
  const std::string& outputProjFile,
  const ProjHeader& header,
  types::BinValue** dataArray)
{
  // Derive projection geometry from header information
  // Note: This is regenerated instead of being given as an
  // input parameter in order to ensure that the geometry data
  // is valid without having to check it.
  ProjGeometry geometry;
  geometry.fill(header);

  std::filesystem::path outputProjHeaderFile(outputProjFile);
  outputProjHeaderFile.replace_extension(".hs");

  std::filesystem::path outputProjDataFile(outputProjFile);
  outputProjDataFile.replace_extension(".s");

  // Open header file
  std::ofstream os;
  os.open(outputProjHeaderFile);
  if (!os.is_open())
  {
    error("Couldn't create file ", outputProjHeaderFile);
  }

  // Fill header file

  writeKey(os, "PROJECTION DATA PARAMETERS");

  writeKey(
    os,
    "name of data file",
    outputProjDataFile.filename().string());

  writeKey(os, "number of rings", header.nRings);
  writeKey(
    os,
    "number of crystals per ring",
    header.nCrystalsPerRing);

  writeKey(os, "segment span", header.segmentSpan);

  writeKey(os, "number of segments", header.nSegments);
  writeKey(
    os,
    "number of tangential coordinates",
    header.nTangCoords);

  writeKey(os, "END OF PROJECTION DATA PARAMETERS");

  os.close();

  // Write data file
  writeData(outputProjDataFile, header, geometry, dataArray);
}
