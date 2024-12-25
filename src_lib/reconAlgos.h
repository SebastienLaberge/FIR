#include <ProjData.h>
#include <ScannerData.h>
#include <VolData.h>

#include <optional>
#include <string>
#include <vector>

struct OSEMCoreParams
{
  // Reconstruction parameters
  int nIterations{1};
  int nSubsets{1};

  // Save parameters
  int saveInterval{0};

  // Operation parameters
  float cutRadius{0.0};
  int convolutionInterval{0};
  std::vector<float> fwhmXYZ{0.0, 0.0, 0.0};
};

namespace reconAlgos
{
// Iterative reconstruction
// -> biasProj is given as pointer to allow a default value
// (no bias)
void OSEM(
  const ProjData& inputProj,
  const ScannerData& scanner,
  VolData& outputVol,
  const std::string& outputVolFileName,
  const OSEMCoreParams& params,
  const VolData& sensitivityMap,
  const std::optional<ProjData>& biasProj);

void OSEM_ResoReco(
  const ProjData& inputProj,
  const ScannerData& scanner,
  VolData& outputVol,
  const std::string& outputVolFileName,
  const OSEMCoreParams& params,
  VolData& sensitivityMap,
  const std::optional<ProjData>& biasProj);
}
