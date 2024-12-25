#include <LORCache.h>

#include <macros.h>

#include <cstdlib>

// TODO: Replace this with a use of something from limits.h
constexpr unsigned short int INVALID = -1;

LORCache::LORCache(const ProjData& proj, int nSubsets_in)
{
  // Check and save nSubsets
  proj.checkNSubsets(nSubsets_in);
  nSubsets = nSubsets_in;

  // Number of views for each subset
  nViewsPerSubset = proj.getGeometry().nViews / nSubsets;

  // Copies of some proj attributes
  nCrystalsPerRing = proj.getHeader().nCrystalsPerRing;
  mSegOffset = proj.getGeometry().segOffset;

  // Initialize vector for number of bins per view for each
  // segment
  nBinsPerViewVector =
    (int*)std::malloc(proj.getHeader().nSegments * sizeof(int));
  LOOP_SEG(seg, proj)
  nBinsPerViewVector[seg + mSegOffset] =
    proj.getGeometry().getNAxialCoords(seg) *
    proj.getHeader().nTangCoords;

  // Allocate and fill crystal array
  mCrystalArray = (LOR***)std::malloc(nSubsets * sizeof(LOR**));
  LOOP(subset, 0, nSubsets - 1)
  {
    mCrystalArray[subset] = (LOR**)std::malloc(
      proj.getHeader().nSegments * sizeof(LOR*));

    LOOP_SEG(seg, proj)
    {
      const int nBinsForCurrentSubsetAndSegment =
        nViewsPerSubset * nBinsPerViewVector[seg + mSegOffset];

      mCrystalArray[subset][seg + mSegOffset] =
        (LOR*)std::malloc(
          nBinsForCurrentSubsetAndSegment * sizeof(LOR));

      auto index = 0;

      LOOP(subsetView, 0, nViewsPerSubset - 1)
      LOOP_AXIAL(axialCoord, proj, seg)
      LOOP_TANG(tangCoord, proj)
      {
        const int view = subset + subsetView * nSubsets;

        const auto [crystalAxialCoord1, crystalAxialCoord2] =
          proj.getCrystalAxialCoord(seg, axialCoord);

        const auto [crystalAngCoord1, crystalAngCoord2] =
          proj.getCrystalAngCoord(view, tangCoord);

        mCrystalArray[subset][seg + mSegOffset][index]
          .crystal1 = crystalAxialCoord1 * nCrystalsPerRing +
          crystalAngCoord1;

        mCrystalArray[subset][seg + mSegOffset][index]
          .crystal2 = crystalAxialCoord2 * nCrystalsPerRing +
          crystalAngCoord2;

        index++;
      }
    }
  }
}

LORCache::~LORCache()
{
  LOOP(subset, 0, nSubsets - 1)
  {
    LOOP(seg, -mSegOffset, mSegOffset)
    std::free(mCrystalArray[subset][seg + mSegOffset]);

    std::free(mCrystalArray[subset]);
  }

  std::free(mCrystalArray);
}

int LORCache::setSubsetAndSegment(int subset, int seg)
{
  mCurrentSubset = subset;
  mCurrentSegment = seg;
  nBinsPerViewForCurrentSegment =
    nBinsPerViewVector[mCurrentSegment + mSegOffset];

  mCrystalArrayForCurrentSubsetAndSegment =
    mCrystalArray[mCurrentSubset][mCurrentSegment + mSegOffset];

  const auto nBinsForCurrentSubsetAndSegment =
    nViewsPerSubset * nBinsPerViewForCurrentSegment;

  return nBinsForCurrentSubsetAndSegment;
}

std::tuple<bool, int, int, int, int, int> LORCache::getLOR(
  int index) const
{
  const auto binIndex = nSubsets == 1 ?
    index :
    mCurrentSubset * nBinsPerViewForCurrentSegment +
      index / nBinsPerViewForCurrentSegment * nSubsets *
        nBinsPerViewForCurrentSegment +
      index % nBinsPerViewForCurrentSegment;

  const auto lor =
    mCrystalArrayForCurrentSubsetAndSegment[index];

  const auto valid = lor.crystal1 != INVALID;

  // TODO: Address what happens when left uninitialized
  int crystalAxialCoord1, crystalAngCoord1;
  int crystalAxialCoord2, crystalAngCoord2;

  if (valid)
  {
    crystalAxialCoord1 =
      static_cast<int>(lor.crystal1 / nCrystalsPerRing);

    crystalAngCoord1 =
      static_cast<int>(lor.crystal1 % nCrystalsPerRing);

    crystalAxialCoord2 =
      static_cast<int>(lor.crystal2 / nCrystalsPerRing);

    crystalAngCoord2 =
      static_cast<int>(lor.crystal2 % nCrystalsPerRing);
  }

  return {
    valid,
    binIndex,
    crystalAxialCoord1,
    crystalAngCoord1,
    crystalAxialCoord2,
    crystalAngCoord2};
}

void LORCache::disableLOR(int index)
{
  mCrystalArrayForCurrentSubsetAndSegment[index].crystal1 =
    INVALID;
}
