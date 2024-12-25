#pragma once

#include <ProjHeader.h>
#include <types.h>

#include <string>
#include <tuple>

// Bin indices and their range:
// seg        : [-segOffset, segOffset]
// view       : [0, nViews[
// axialCoord : [0, nAxialCoords[seg + segOffset][
// tangCoord  : [-tangCoordOffset,
//               -tangCoordOffset + nTangCoords]
//
// How to obtain a bin value in mDataArray:
// => mDataArray
//      [seg + segOffset]
//      [view * nAxialCoords[seg + segOffset] * nTangCoords +
//       axialCoord * nTangCoords +
//       tangCoord + tangCoordOffset]

class ProjData
{
public:

  enum class ConstructionMode
  {
    // Allocate data without initializing it
    ALLOCATE,

    // Initialize all voxels to a value provided (default: 0.0)
    INITIALIZE,

    // Read data file provided in header (error if absent)
    READ_DATA
  };

  // Empty projection
  ProjData();

  // From header file
  ProjData(
    const std::string& inputProjFile,
    ConstructionMode mode = ConstructionMode::READ_DATA,
    types::BinValue initValue = 0.0);

  // Empty copy of another projection
  ProjData(
    const ProjData& proj,
    ConstructionMode mode = ConstructionMode::READ_DATA,
    types::BinValue initValue = 0.0);

  // Destructor
  ~ProjData();

  // Read projecton (use if empty constructor was used)
  void read(
    const std::string& headerFileName,
    ConstructionMode mode = ConstructionMode::READ_DATA,
    types::BinValue initValue = 0.0);

  // Copy another projection
  void copy(
    const ProjData& proj,
    ConstructionMode mode = ConstructionMode::READ_DATA,
    types::BinValue initValue = 0.0);

  // Write projection in interfile format
  void write(const std::string& outputProjFile) const;

  // Print class content
  void printContent() const;

  // Issue error if number of subsets is incorrect
  void checkNSubsets(int nSubsets) const;

  // Set all bins to the same value (default: zero)
  void setAllBins(types::BinValue value = 0.0);

  // Get crystal coordinates from projection bin coordinates

  // Output: {crystalAxialCoord1, crystalAxialCoord2}
  std::tuple<int, int> getCrystalAxialCoord(
    int seg,
    int axialCoord) const;

  // Output: {crystalAngCoord1, crystalAngCoord2}
  std::tuple<int, int> getCrystalAngCoord(
    int view,
    int tangCoord) const;

  // Get projection bin coordinates from crystal coordinates
  // Returns false if the LOR falls outside the current
  // projection
  bool getBinCoordinates(
    int crystalAxialCoord1,
    int crystalAngCoord1,
    int crystalAxialCoord2,
    int crystalAngCoord2,
    int* seg,
    int* view,
    int* axialCoord,
    int* tangCoord) const;

  // Bin-by-bin arithmetics
  ProjData& operator*=(const ProjData& inputProj);
  void exponential();

  // Rebinning weighting
  // Divide each bin by the number of ring pairs associated
  void rebinWeight();

  inline const ProjHeader& getHeader() const;
  inline const ProjGeometry& getGeometry() const;
  inline types::BinValue** getDataArray() const;

  // Set and get single bin value

  inline void setBin(
    int seg,
    int view,
    int axialCoord,
    int tangCoord,
    types::BinValue value);

  inline types::BinValue getBin(
    int seg,
    int view,
    int axialCoord,
    int tangCoord) const;

  inline void incrementBin(
    int seg,
    int view,
    int axialCoord,
    int tangCoord);

  inline void weightBin(
    int seg,
    int view,
    int axialCoord,
    int tangCoord,
    types::BinValue weight);

private:

  inline std::tuple<int, int> getInd(
    int seg,
    int view,
    int axialCoord,
    int tangCoord) const;

  // Copy parsed and calculated parameters from another volume
  void copyParameters(const ProjData& proj);

  // Set data (TODO: Separate initialization from allocation)
  void allocate(
    bool initialize = true,
    types::BinValue initValue = 0.0);
  void deallocate();

  ProjHeader mHeader;
  ProjGeometry mGeometry;

  types::BinValue** mDataArray;

  // Bin weights (multiplies each bin during projection)
  types::BinValue** mBinWeights;
};

#include <ProjData.inl>
