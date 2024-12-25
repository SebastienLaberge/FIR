#pragma once

#include <ProjHeader.h>
#include <types.h>

#include <string>

class ProjInterfileReader
{
public:

  // Initialize reader by specifying path to a header file
  // This reads the header file and fills data structures
  ProjInterfileReader(const std::string& headerFileName);

  // Get information contained in or derived from header file
  inline ProjHeader getHeader();
  inline ProjGeometry getGeometry();

  // Read the data file pointed to by the header file
  void readData(types::BinValue** dataArray);

  // Static method to write a volume to file
  static void writeProjInterfile(
    const std::string& outputProjFile,
    const ProjHeader& header,
    types::BinValue** dataArray);

private:

  std::string mDataFileName;

  ProjHeader mHeader;
  ProjGeometry mGeometry;
};

#include <ProjInterfileReader.inl>
