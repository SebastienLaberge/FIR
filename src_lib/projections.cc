#include <projections.h>

#include <LORCache.h>
#include <Siddon.h>
#include <console.h>
#include <macros.h>

#include <iostream>

// TODO: Get rid of this
const bool DEBUG{false};

namespace projections
{
void forward(
  const VolData& inputVol,
  const ScannerData& scanner,
  ProjData& outputProj)
{
  // Check proj data dimensions
  scanner.checkProjData(outputProj);

  Siddon siddon(inputVol);

  LOOP_SEG(seg, outputProj)
  {
    std::cout << "Computing segment " << seg << std::endl;

    const auto nBinsPerViewForCurrentSegment =
      outputProj.getGeometry().getNAxialCoords(seg) *
      outputProj.getHeader().nTangCoords;

    types::PathElement* threadLocalPathElements = nullptr;

    // Parallelization over views
#pragma omp parallel for firstprivate(threadLocalPathElements)
    LOOP_VIEW(view, outputProj)
    {
      if (threadLocalPathElements == nullptr)
      {
        threadLocalPathElements =
          siddon.getThreadLocalPathElements();
      }

      auto binIndex = view * nBinsPerViewForCurrentSegment;

      LOOP_AXIAL(axialCoord, outputProj, seg)
      LOOP_TANG(tangCoord, outputProj)
      {
        const auto [crystalAxialCoord1, crystalAxialCoord2] =
          outputProj.getCrystalAxialCoord(seg, axialCoord);

        const auto [crystalAngCoord1, crystalAngCoord2] =
          outputProj.getCrystalAngCoord(view, tangCoord);

        siddon.computePathBetweenCrystals(
          scanner,
          crystalAxialCoord1,
          crystalAngCoord1,
          crystalAxialCoord2,
          crystalAngCoord2,
          threadLocalPathElements);

        // Compute line integral
        const auto line =
          inputVol.computeLineIntegral(threadLocalPathElements);

        // Put result in ProjData
        BIN(outputProj, seg, binIndex) = line;

        binIndex++;

        // Print info about current projection bin
        if (DEBUG)
        {
          scanner.printBinInfo(
            seg,
            view,
            axialCoord,
            tangCoord,
            crystalAngCoord1,
            crystalAxialCoord1,
            crystalAngCoord2,
            crystalAxialCoord2,
            inputVol,
            threadLocalPathElements,
            line);
        }
      }
    }
  }
}

void backward(
  const ProjData& inputProj,
  const ScannerData& scanner,
  VolData& outputVol,
  int nSubsets)
{
  // Check proj data dimensions
  scanner.checkProjData(inputProj);

  // Check number of subsets
  inputProj.checkNSubsets(nSubsets);

  // Check number of frames allocated
  outputVol.checkNFrames(nSubsets);

  // Initialize siddon algorithm and LOR list
  Siddon siddon(outputVol);
  LORCache cache(inputProj, nSubsets);

  echo("Back-projection");

  // Initialize output volume
  outputVol.setAllVoxelsAllFrames(0.0);

  if (nSubsets == 1)
  {
    std::cout << "Segment:";
  }

  // Sub-iterations
  LOOP(subset, 0, nSubsets - 1)
  {
    if (nSubsets > 1)
    {
      std::cout <<                 //
        "subset " << subset + 1 << //
        " of " << nSubsets << std::endl;
      std::cout << "Segment:";
    }

    outputVol.setActiveFrame(subset);

    LOOP_SEG(seg, inputProj)
    {
      std::cout << " " << seg << std::flush;

      auto nBinsForCurrentSubsetAndSegment =
        cache.setSubsetAndSegment(subset, seg);

      types::PathElement* threadLocalPathElements{nullptr};

#pragma omp parallel for firstprivate(threadLocalPathElements)
      LOOP(index, 0, nBinsForCurrentSubsetAndSegment - 1)
      {
        if (threadLocalPathElements == nullptr)
        {
          threadLocalPathElements =
            siddon.getThreadLocalPathElements();
        }

        // Get LOR crystals (seg and binIndex assigned even if
        // LOR is skipped)
        const auto
          [valid,
           binIndex,
           crystalAxialCoord1,
           crystalAngCoord1,
           crystalAxialCoord2,
           crystalAngCoord2] = cache.getLOR(index);

        // TODO: Check if valid should be used

        // Apply siddon algorithm
        siddon.computePathBetweenCrystals(
          scanner,
          crystalAxialCoord1,
          crystalAngCoord1,
          crystalAxialCoord2,
          crystalAngCoord2,
          threadLocalPathElements);

        // Add line integral to volume
        outputVol.projectLineIntegral(
          threadLocalPathElements,
          BIN(inputProj, seg, binIndex));
      }
    }

    printEmptyLine();
  }
}

void computeSensitivityVol(
  const ProjData& proj,
  const ScannerData& scanner,
  VolData& outputSensitivityVol,
  int nSubsets)
{
  ProjData ones(
    proj,
    ProjData::ConstructionMode::INITIALIZE,
    1.0);
  backward(ones, scanner, outputSensitivityVol, nSubsets);
}
}
