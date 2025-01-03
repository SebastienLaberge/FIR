#include <LORCache.h>

#include <macros.h>

#include <cstdlib>

// TODO: Replace this with a use of something from limits.h
constexpr unsigned short int INVALID = -1;

LORCache::LORCache(const ProjData& proj, int nSubsets):
  mNSubsets{nSubsets},
  mNViewsPerSubset{proj.getGeometry().nViews / nSubsets},
  mNCrystalsPerRing{proj.getHeader().nCrystalsPerRing},
  mSegOffset{proj.getGeometry().segOffset}
{
  // Check number of subsets
  proj.checkNSubsets(nSubsets);

  // Get and save the number of bins per view for each segment
  mNBinsPerViewForEachSegment =
    (int*)std::malloc(proj.getHeader().nSegments * sizeof(int));
  LOOP_SEG(seg, proj)
  {
    mNBinsPerViewForEachSegment[seg + mSegOffset] =
      proj.getGeometry().getNAxialCoords(seg) *
      proj.getHeader().nTangCoords;
  }

  // Allocate and fill the crystal array
  mCrystalArray =
    (LOR***)std::malloc(mNSubsets * sizeof(LOR**));
  LOOP(subset, 0, mNSubsets - 1)
  {
    mCrystalArray[subset] = (LOR**)std::malloc(
      proj.getHeader().nSegments * sizeof(LOR*));

    LOOP_SEG(seg, proj)
    {
      const auto nBinsForCurrentSubsetAndSegment =
        mNViewsPerSubset *
        mNBinsPerViewForEachSegment[seg + mSegOffset];

      mCrystalArray[subset][seg + mSegOffset] =
        (LOR*)std::malloc(
          nBinsForCurrentSubsetAndSegment * sizeof(LOR));

      auto binIndex = 0;

      LOOP(subview, 0, mNViewsPerSubset - 1)
      {
        const auto view = subview * mNSubsets + subset;

        LOOP_AXIAL(axialCoord, proj, seg)
        {
          const auto [crystalAxialCoord1, crystalAxialCoord2] =
            proj.getCrystalAxialCoord(seg, axialCoord);

          LOOP_TANG(tangCoord, proj)
          {
            const auto [crystalAngCoord1, crystalAngCoord2] =
              proj.getCrystalAngCoord(view, tangCoord);

            auto* currentCrystals =
              &mCrystalArray[subset][seg + mSegOffset]
                            [binIndex];

            currentCrystals->crystal1 =
              crystalAxialCoord1 * mNCrystalsPerRing +
              crystalAngCoord1;

            currentCrystals->crystal2 =
              crystalAxialCoord2 * mNCrystalsPerRing +
              crystalAngCoord2;

            ++binIndex;
          }
        }
      }
    }
  }
}

LORCache::~LORCache()
{
  LOOP(subset, 0, mNSubsets - 1)
  {
    LOOP(seg, -mSegOffset, mSegOffset)
    {
      std::free(mCrystalArray[subset][seg + mSegOffset]);
    }

    std::free(mCrystalArray[subset]);
  }

  std::free(mCrystalArray);
}

int LORCache::setSubsetAndSegment(int subset, int seg)
{
  mCurrentSubset = subset;
  mCurrentSegment = seg;

  mNBinsPerViewForCurrentSegment =
    mNBinsPerViewForEachSegment[mCurrentSegment + mSegOffset];

  mCrystalArrayForCurrentSubsetAndSegment =
    mCrystalArray[mCurrentSubset][mCurrentSegment + mSegOffset];

  const auto nBinsForCurrentSubsetAndSegment =
    mNViewsPerSubset * mNBinsPerViewForCurrentSegment;

  return nBinsForCurrentSubsetAndSegment;
}

std::tuple<bool, int, int, int, int, int> LORCache::getLOR(
  int index) const
{
  int binIndex;
  if (mNSubsets == 1)
  {
    binIndex = index;
  }
  else
  {
    const auto divIndex =
      std::div(index, mNBinsPerViewForCurrentSegment);

    const auto view =
      divIndex.quot * mNSubsets + mCurrentSubset;

    binIndex =
      view * mNBinsPerViewForCurrentSegment + divIndex.rem;
  }

  const auto lor =
    mCrystalArrayForCurrentSubsetAndSegment[index];

  const auto valid = lor.crystal1 != INVALID;

  // TODO: Address what happens when left uninitialized
  int crystalAxialCoord1, crystalAngCoord1;
  int crystalAxialCoord2, crystalAngCoord2;

  if (valid)
  {
    const auto div1 = std::div(lor.crystal1, mNCrystalsPerRing);
    crystalAxialCoord1 = div1.quot;
    crystalAngCoord1 = div1.rem;

    const auto div2 = std::div(lor.crystal2, mNCrystalsPerRing);
    crystalAxialCoord2 = div2.quot;
    crystalAngCoord2 = div2.rem;
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
