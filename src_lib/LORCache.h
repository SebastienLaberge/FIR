#include <ProjData.h>

#include <tuple>

struct LOR
{
  // Maximum value of unsigned int: non-valid
  unsigned short int crystal1;
  unsigned short int crystal2;
};

class LORCache
{
public:

  LORCache(const ProjData& proj, int nSubsets);

  ~LORCache();

  // Set subset and segment
  // Return nBinsForCurrentSubsetAndSegment
  int setSubsetAndSegment(int subset, int segment);

  // Outputs:
  // valid, projIndex,
  // crystalAxialCoord1, crystalAngCoord1,
  // crystalAxialCoord2, crystalAngCoord2;
  std::tuple<bool, int, int, int, int, int> getLOR(
    int index) const;

  void disableLOR(int index);

private:

  int mNSubsets, mNViewsPerSubset;
  int mNCrystalsPerRing, mSegOffset;

  // [segment]
  int* mNBinsPerViewForEachSegment;

  // [subset][segment][bin]
  LOR*** mCrystalArray;

  int mCurrentSubset, mCurrentSegment;
  int mNBinsPerViewForCurrentSegment;
  LOR* mCrystalArrayForCurrentSubsetAndSegment;
};
