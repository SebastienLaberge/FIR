#pragma once

#include <ScannerData.h>
#include <VolData.h>
#include <types.h>

#include <optional>
#include <tuple>

class Siddon
{
public:

  // Triplet types
  using SizeTriplet =
    std::tuple<types::Size, types::Size, types::Size>;
  using IndexTriplet =
    std::tuple<types::Index, types::Index, types::Index>;
  using CoordTriplet = //
    std::tuple<
      types::SpatialCoord,
      types::SpatialCoord,
      types::SpatialCoord>;

  Siddon(const VolData& vol, const ScannerData& scanner);

  ~Siddon();

  void printContent() const;

  // Call to get path element vector for current thread
  types::PathElement* getThreadLocalPathElements() const;

  // Compute LOR path from scanner coordinates of crystal pair
  // Provide path element vector for current thread
  // Returns true if LOR crosses volume, false otherwise
  bool computePath(
    int crysAxialCoord1,
    int crysAngCoord1,
    int crysAxialCoord2,
    int crysAngCoord2,
    types::PathElement* pathElementsArray) const;

  // Compute LOR path from spatial coordinates of crystal pair
  // Provide path element vector for current thread
  // Returns true if LOR crosses volume, false otherwise
  bool computePathCore(
    types::SpatialCoord crys1_X,
    types::SpatialCoord crys1_Y,
    types::SpatialCoord crys1_Z,
    types::SpatialCoord crys2_X,
    types::SpatialCoord crys2_Y,
    types::SpatialCoord crys2_Z,
    types::PathElement* pathElementsArray) const;

private:

  struct Setup
  {
    types::SpatialCoord diff;
    int dir;
    types::SpatialCoord alphaMin;
    types::SpatialCoord alphaMax;
    types::SpatialCoord nextAlpha; // TODO: Rename
  };

  template<int Dim>
  inline std::optional<Siddon::Setup> dimSetup(
    types::SpatialCoord crys1,
    types::SpatialCoord crys2) const;

  inline std::tuple<types::SpatialCoord, types::SpatialCoord>
  getEntryExit(
    const Setup& setupX,
    const Setup& setupY,
    const Setup& setupZ) const;

  template<int Dim>
  inline int getStartInd(
    types::SpatialCoord crys1,
    types::SpatialCoord diff,
    types::SpatialCoord alphaMin) const;

  template<int Dim>
  inline types::SpatialCoord prepareDim(
    types::SpatialCoord crys1,
    const Siddon::Setup& setup,
    IndexTriplet& position,
    const Siddon::CoordTriplet& dAlpha) const;

  template<int Dim>
  inline void updateDim(
    CoordTriplet& alphaDim,
    IndexTriplet& position,
    types::SpatialCoord nextAlpha,
    const Siddon::CoordTriplet& dAlpha,
    int dir) const;

  inline bool checkIndices(const IndexTriplet& position) const;

  inline types::Index getLinearCoord(
    const IndexTriplet& position) const;

  // Copies of elements from ScannerData (Managed externally)
  const types::SpatialCoords2D* mCrystalXYPositionVector;
  const types::SpatialCoord* mSliceZPositionVector;

  // Volume dimensions
  types::Size mRowSize;
  types::Size mSliceSize;
  SizeTriplet mVolSizeM1;
  SizeTriplet mVoxelExtent;

  // Border plane position
  CoordTriplet mLowPlanes;
  CoordTriplet mHighPlanes;

  // Maximum length of a path element array for each thread
  int mMaxPathLength;

  // Array of path elements (Managed internally)
  types::PathElement* mPathElementArray;
};
