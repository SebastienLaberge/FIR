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

  int nSubsets, nViewsPerSubset;
  int nCrystalsPerRing, mSegOffset;

  int mCurrentSubset, mCurrentSegment;
  int nBinsPerViewForCurrentSegment;
  int* nBinsPerViewVector;

  LOR*** mCrystalArray;
  LOR* mCrystalArrayForCurrentSubsetAndSegment;
};
