#pragma once

#include <ProjData.h>
#include <ScannerData.h>
#include <VolData.h>

namespace projections
{
void forward(
  const VolData& inputVol,
  const ScannerData& scanner,
  ProjData& outputProj);

void backward(
  const ProjData& inputProj,
  const ScannerData& scanner,
  VolData& outputVol,
  int nSubsets = 1);

void computeSensitivityVol(
  const ProjData& proj,
  const ScannerData& scanner,
  VolData& initializedSensVol,
  int nSubsets = 1);
}
