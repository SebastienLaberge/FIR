#include <VolData.h>

#include <vector>

namespace operations
{
void convolve(
  VolData& vol,
  std::vector<float> fwhmXYZ,
  float cutRadius);

// Convolution
void cutCircle(
  VolData& vol,
  float cutRadius = 0.0); // 0: No cut

// Apply mask: Volume maskVol of the same size.
// Keep voxels for which maskVol > 0
void applyMask(VolData& vol, VolData& maskVol);

// Convert Hounsfield units to mu map in mm^-1
void HounsfieldToMuMap(VolData& vol);
}
