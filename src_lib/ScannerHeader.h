#pragma once

#include <types.h>

#include <vector>

struct ScannerHeader
{
  // Optional parameters: Set to zero (default) to compute
  // value in post-processing

  // Crystals

  // X and Y can be omitted. All default to value: 0
  std::vector<types::SpatialCoord> crystalDimsXYZ;
  // Default value: {1, 1}
  std::vector<int> crystalRepeatNumbersYZ;
  // Default value: {0, 0}
  std::vector<types::SpatialCoord> interCrystalDistanceYZ;

  // Modules

  // Default value: Fitting tightly on crystals
  std::vector<types::SpatialCoord> moduleDimsXYZ;
  // Default value: {1, 1}
  std::vector<int> moduleRepeatNumbersYZ;
  // Default value: {0, 0}
  std::vector<types::SpatialCoord> interModuleDistanceYZ;

  // rSectors

  // Default value: Fitting tightly on modules
  std::vector<types::SpatialCoord> rSectorDimsXYZ;
  // Mandatory
  int rSectorRepeatNumber;
  // Mandatory (X translation - crystalDimsXYZ[0] / 2)
  types::SpatialCoord rSectorInnerRadius;

  // Fill structure with default values
  // Note: Must be executed before filling with specific values
  void setDefaults();

  // Check validity of values and throw if there is a problem
  // Note: Must be executed after filling with specific values
  void check();
};

struct ScannerGeometry
{
  // Translation vectors and distances to build scanner geometry
  std::vector<types::SpatialCoord> crystalRepeatVectorYZ;
  std::vector<types::SpatialCoord> moduleRepeatVectorYZ;
  types::SpatialCoord rSectorTranslationX;

  // Numbers of geometric elements
  int nRings, nCrystalsPerRing, nCrystals;

  // Number of slices
  int nSlices;

  // Offset necessary so that crystal numbering within a ring
  // starts in the middle of the first r-sector
  int crystalOffset;

  // Fill structure using information contained in header
  // Note: Must be executed before using the structure
  void fill(const ScannerHeader& header);
};
