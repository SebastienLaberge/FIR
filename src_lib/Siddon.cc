#include <Siddon.h>

#include <console.h>
#include <macros.h>
#include <tools.h>

#include <cmath>
#include <cstdlib>

constexpr types::SpatialCoord ALPHA_MIN{0.0};
constexpr types::SpatialCoord ALPHA_MAX{1.0};

constexpr int DIR_NEG{-1};
constexpr int DIR_POS{1};

constexpr int X_DIM{0};
constexpr int Y_DIM{1};
constexpr int Z_DIM{2};

Siddon::Siddon(const VolData& vol)
{
  const auto& header = vol.getHeader();

  const auto& volSize = header.volSize;
  const auto& voxelExtent = header.voxelExtent;

  mRowSize = volSize.nPixelsX;
  mSliceSize = volSize.nPixelsX * volSize.nPixelsY;

  mVolSizeM1 = SizeTriplet(
    volSize.nPixelsX - 1,
    volSize.nPixelsY - 1,
    volSize.nSlices - 1);

  mVoxelExtent = SizeTriplet(
    voxelExtent.pixelWidth,
    voxelExtent.pixelHeight,
    voxelExtent.sliceThickness);

  // Set low and high planes

  const auto& volExtent = vol.getVolExtent();

  mLowPlanes = CoordTriplet(
    header.volOffset.x - voxelExtent.pixelWidth / 2,
    header.volOffset.y - voxelExtent.pixelHeight / 2,
    header.volOffset.z - volExtent.volDepth / 2);

  mHighPlanes = CoordTriplet(
    std::get<X_DIM>(mLowPlanes) + volExtent.sliceWidth,
    std::get<Y_DIM>(mLowPlanes) + volExtent.sliceHeight,
    std::get<Z_DIM>(mLowPlanes) + volExtent.volDepth);

  // Get maximum length of path element array
  mMaxPathLength =
    volSize.nPixelsX + volSize.nPixelsY + volSize.nSlices;

  const auto nPathElements = getNThreads() * mMaxPathLength;

  // Allocate path element array
  mPathElementArray = (types::PathElement*)std::malloc(
    nPathElements * sizeof(types::PathElement));
}

Siddon::~Siddon()
{
  // Free path element array
  std::free(mPathElementArray);
}

void Siddon::printContent() const
{
  // Border plane position (used by Siddon)
  printValue("Low plane (x)", std::get<X_DIM>(mLowPlanes));
  printValue("Low plane (y)", std::get<Y_DIM>(mLowPlanes));
  printValue("Low plane (z)", std::get<Z_DIM>(mLowPlanes));
  printValue("High plane (x)", std::get<X_DIM>(mHighPlanes));
  printValue("High plane (y)", std::get<Y_DIM>(mHighPlanes));
  printValue("High plane (z)", std::get<Z_DIM>(mHighPlanes));
}

types::PathElement* Siddon::getThreadLocalPathElements() const
{
  return mPathElementArray +
    getCurrentThread() * mMaxPathLength;
}

bool Siddon::computePathBetweenCrystals(
  const ScannerData& scanner,
  int crysAxialCoord1,
  int crysAngCoord1,
  int crysAxialCoord2,
  int crysAngCoord2,
  types::PathElement* pathElementsArray) const
{
  // Get access to crystal positions
  const auto* crystalXYPositionVector =
    scanner.getCrystalXYPositionVector();
  const auto* sliceZPositionVector =
    scanner.getSliceZPositionVector();

  return computePath(
    crystalXYPositionVector[crysAngCoord1].x,
    crystalXYPositionVector[crysAngCoord1].y,
    sliceZPositionVector[crysAxialCoord1],
    crystalXYPositionVector[crysAngCoord2].x,
    crystalXYPositionVector[crysAngCoord2].y,
    sliceZPositionVector[crysAxialCoord2],
    pathElementsArray);
}

bool Siddon::computePath(
  types::SpatialCoord crys1X,
  types::SpatialCoord crys1Y,
  types::SpatialCoord crys1Z,
  types::SpatialCoord crys2X,
  types::SpatialCoord crys2Y,
  types::SpatialCoord crys2Z,
  types::PathElement* pathElementsArray) const
{
  // Default empty path if the LOR doesn't intersect the volume
  pathElementsArray[0].coord = -1;

  // The line of response (LOR) going from crys1 (x1, y1, z1)
  // to crys2 (x2, y2, z2) is parameterized as:
  //   x(alpha) = x1 + (x2 - x1) * alpha = x1 + diffX * alpha
  //   Similarly for Y and Z
  // => alpha goes from 0 to 1 when moving from crys1 to crys2
  //
  // The value of alpha for a given value of x, y or z is thus:
  //   alpha = (x - x1) / diffX
  //   Similarly for Y and Z
  //
  // dir for a dimension D:
  //   +1 or -1 depending on whether crys2 is reached from
  //   crys1 by increasing D (+1) or decreasing D (-1)
  //
  // alphaMin and alphaMax for a dimension D:
  //   Values of alpha for which the LOR intersects the planes
  //   of constant D at the edge of the volume:
  //   alphaMin for the one closest to crys1 and
  //   alphaMax for the one farthest from crys1

  // Setup in X
  const auto setupXout = dimSetup<X_DIM>(crys1X, crys2X);
  if (!setupXout.has_value())
  {
    return false;
  }
  const auto setupX = setupXout.value();

  // Setup in Y
  auto setupYout = dimSetup<Y_DIM>(crys1Y, crys2Y);
  if (!setupYout.has_value())
  {
    return false;
  }
  const auto setupY = setupYout.value();

  // Setup in Z
  const auto setupZout = dimSetup<Z_DIM>(crys1Z, crys2Z);
  if (!setupZout.has_value())
  {
    return false;
  }
  const auto setupZ = setupZout.value();

  // Values of alpha producing the points where the LOR enters
  // and exits the volume
  const auto [alphaMin, alphaMax] =
    getEntryExit(setupX, setupY, setupZ);
  if (alphaMin >= alphaMax)
  {
    return false;
  }

  // Find the variations of alpha necessary to travel between
  // neighboring inter-voxel planes for a given dimension
  const CoordTriplet dAlpha{
    std::get<X_DIM>(mVoxelExtent) / ABS(setupX.diff),
    std::get<Y_DIM>(mVoxelExtent) / ABS(setupY.diff),
    std::get<Z_DIM>(mVoxelExtent) / ABS(setupZ.diff)};

  // Find the constant d12 such that alpha * d12 gives distance
  // from crys1 to the point described by the value of alpha
  const auto d12 = std::sqrt(
    setupX.diff * setupX.diff + setupY.diff * setupY.diff +
    setupZ.diff * setupZ.diff);

  // Initialize position:
  // Find the voxel where the LOR enters the volume
  IndexTriplet position{
    getStartInd<X_DIM>(crys1X, setupX.diff, alphaMin),
    getStartInd<Y_DIM>(crys1Y, setupY.diff, alphaMin),
    getStartInd<Z_DIM>(crys1Z, setupZ.diff, alphaMin)};

  // For each non-cancelled dimension, get the value of alpha
  // for which the LOR touches the second plane along that
  // dimension after entering the volume
  CoordTriplet alphaDim{
    prepareDim<X_DIM>(crys1X, setupX, position, dAlpha),
    prepareDim<Y_DIM>(crys1Y, setupY, position, dAlpha),
    prepareDim<Z_DIM>(crys1Z, setupZ, position, dAlpha)};

  int pathInd{0};
  types::SpatialCoord previousAlpha{alphaMin};
  while (previousAlpha < alphaMax)
  {
    // Update nextAlpha to the next value for which the LOR
    // touches a plane between neighboring voxels
    const auto nextAlpha = MIN(
      alphaMax,
      MIN(
        std::get<X_DIM>(alphaDim),
        MIN(
          std::get<Y_DIM>(alphaDim),
          std::get<Z_DIM>(alphaDim))));

    // Write new path element after making sure it is within
    // the boundaries of the volume
    if (checkIndices(position))
    {
      pathElementsArray[pathInd].coord =
        getLinearCoord(position);

      pathElementsArray[pathInd].length =
        (nextAlpha - previousAlpha) * d12;

      ++pathInd;
    }

    updateDim<X_DIM>(
      alphaDim,
      position,
      nextAlpha,
      dAlpha,
      setupX.dir);
    updateDim<Y_DIM>(
      alphaDim,
      position,
      nextAlpha,
      dAlpha,
      setupY.dir);
    updateDim<Z_DIM>(
      alphaDim,
      position,
      nextAlpha,
      dAlpha,
      setupZ.dir);

    previousAlpha = nextAlpha;
  }

  // Terminator
  pathElementsArray[pathInd].coord = -1;

  return true;
}

template<int Dim>
std::optional<Siddon::Setup> Siddon::dimSetup(
  types::SpatialCoord crys1,
  types::SpatialCoord crys2) const
{
  const auto lowPlane = std::get<Dim>(mLowPlanes);
  const auto highPlane = std::get<Dim>(mHighPlanes);

  auto diff = crys2 - crys1;

  int dir;
  types::SpatialCoord alphaMin, alphaMax, nextAlpha;
  if (ABS(diff) > EPSILON)
  {
    if (diff > 0)
    {
      // LOR entering volume from low plane
      dir = DIR_POS;
      alphaMin = (lowPlane - crys1) / diff;
      alphaMax = (highPlane - crys1) / diff;
    }
    else
    {
      // LOR entering volume from high plane
      dir = DIR_NEG;
      alphaMin = (highPlane - crys1) / diff;
      alphaMax = (lowPlane - crys1) / diff;
    }

    // Indicates that the dimension is not "cancelled"
    nextAlpha = ALPHA_MIN;
  }
  else
  {
    // The LOR has no component along the current dimension

    if (crys1 < lowPlane || crys1 > highPlane)
    {
      // The LOR doesn't intersect the volume: skip it
      return std::nullopt;
    }

    // The LOR intersects the volume: Set special values

    diff = EPSILON;
    dir = DIR_NEG;

    // Widest allowed range of alpha so that those values do not
    // interfere in the computation of the point of entry
    alphaMin = ALPHA_MIN;
    alphaMax = ALPHA_MAX;

    // Indicates that the dimension is "cancelled"
    nextAlpha = ALPHA_MAX;
  }

  return Setup{diff, dir, alphaMin, alphaMax, nextAlpha};
}

std::tuple<types::SpatialCoord, types::SpatialCoord> Siddon::
  getEntryExit(
    const Setup& setupX,
    const Setup& setupY,
    const Setup& setupZ) const
{
  const auto alphaMin = MAX(
    setupX.alphaMin,
    MAX(setupY.alphaMin, MAX(setupZ.alphaMin, ALPHA_MIN)));

  const auto alphaMax = MIN(
    setupX.alphaMax,
    MIN(setupY.alphaMax, MIN(setupZ.alphaMax, ALPHA_MAX)));

  return {alphaMin, alphaMax};
}

template<int Dim>
int Siddon::getStartInd(
  types::SpatialCoord crys1,
  types::SpatialCoord diff,
  types::SpatialCoord alphaMin) const
{
  auto ind = static_cast<int>(
    (crys1 + diff * alphaMin - std::get<Dim>(mLowPlanes)) /
    std::get<Dim>(mVoxelExtent));

  // Make sure the initial coordinates are within the volume
  ind = MAX(0, MIN(ind, std::get<Dim>(mVolSizeM1)));

  return ind;
}

template<int Dim>
types::SpatialCoord Siddon::prepareDim(
  types::SpatialCoord crys1,
  const Siddon::Setup& setup,
  IndexTriplet& position,
  const Siddon::CoordTriplet& dAlpha) const
{
  auto alpha = setup.nextAlpha;

  if (alpha < ALPHA_MAX)
  {
    const auto len = // RENAME
      std::get<Dim>(mLowPlanes) +
      std::get<Dim>(mVoxelExtent) * std::get<Dim>(position) -
      crys1;

    alpha = len / setup.diff;
  }

  if (setup.dir > 0)
  {
    alpha += std::get<Dim>(dAlpha);
  }

  return alpha;
}

template<int Dim>
void Siddon::updateDim(
  CoordTriplet& alphaDim,
  IndexTriplet& position,
  types::SpatialCoord nextAlpha,
  const Siddon::CoordTriplet& dAlpha,
  int dir) const
{
  if (ABS(std::get<Dim>(alphaDim) - nextAlpha) < EPSILON)
  {
    std::get<Dim>(alphaDim) += std::get<Dim>(dAlpha);
    std::get<Dim>(position) += dir;
  }
}

bool Siddon::checkIndices(const IndexTriplet& position) const
{
  return std::get<X_DIM>(position) >= 0 &&
    std::get<X_DIM>(position) <= std::get<X_DIM>(mVolSizeM1) &&

    std::get<Y_DIM>(position) >= 0 &&
    std::get<Y_DIM>(position) <= std::get<Y_DIM>(mVolSizeM1) &&

    std::get<Z_DIM>(position) >= 0 &&
    std::get<Z_DIM>(position) <= std::get<Z_DIM>(mVolSizeM1);
}

types::Index Siddon::getLinearCoord(
  const IndexTriplet& position) const
{
  return std::get<X_DIM>(position) +
    std::get<Y_DIM>(position) * mRowSize +
    std::get<Z_DIM>(position) * mSliceSize;
}
