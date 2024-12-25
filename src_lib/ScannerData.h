#pragma once

#include <ScannerHeader.h>
#include <types.h>

#include <string>

class ProjData;
class VolData;

class ScannerData
{
public:
  ScannerData(); // TODO: Remove?
  ScannerData(const std::string& scannerFile);

  // Check if projection data is compatible with scanner
  void checkProjData(const ProjData& proj) const;

  inline const types::SpatialCoords2D*
  getCrystalXYPositionVector() const;

  inline const types::SpatialCoord*
  getSliceZPositionVector() const;

  // Print scanner info
  void printContent() const;
  void printCrystalPositions() const;
  void printBinInfo(
    int seg, int view, int axialCoord, int tangCoord,
    int crystal1, int slice1, int crystal2, int slice2,
    const VolData& vol,
    types::PathElement* pathElementsArray = 0,
    types::VoxelValue line = 0) const;

  // Used in debugging
  void getCrystalCoordinates(
    int rSectorID,
    int moduleID,
    int crystalID,
    int* ring,
    int* crystal) const;

private:
  void computeCrystalXYPositionVector();
  void computeSliceZPositionVector();

  ScannerHeader mHeader;
  ScannerGeometry mGeometry;

  types::SpatialCoords2D* mCrystalXYPositionVector;
  types::SpatialCoord* mSliceZPositionVector;
};

#include <ScannerData.inl>
