#include <reconAlgos.h>

#include <LORCache.h>
#include <Siddon.h>
#include <console.h>
#include <macros.h>
#include <operations.h>

#include <tuple>

inline std::tuple<int, types::VoxelValue> getLine(
  int index,
  LORCache& cache,
  const Siddon& siddon,
  const ScannerData& scanner,
  types::PathElement* threadLocalPathElements,
  bool firstIter,
  const VolData& outputVol)
{
  // Get LOR crystals (seg and binIndex assigned even if LOR is
  // skipped)
  const auto
    [validSaved,
     binIndex,
     crystalAxialCoord1,
     crystalAngCoord1,
     crystalAxialCoord2,
     crystalAngCoord2] = cache.getLOR(index);

  types::VoxelValue line;
  if (validSaved)
  {
    // Apply siddon algorithm
    const auto valid = siddon.computePathBetweenCrystals(
      scanner,
      crystalAxialCoord1,
      crystalAngCoord1,
      crystalAxialCoord2,
      crystalAngCoord2,
      threadLocalPathElements);

    // Disable LOR for future iterations
    if (firstIter && !valid)
    {
      cache.disableLOR(index);
    }

    // Compute line integral
    line = valid ?
      outputVol.computeLineIntegral(threadLocalPathElements) :
      0.0;
  }
  else
  {
    line = 0.0;
  }

  return {binIndex, line};
}

namespace reconAlgos
{
void OSEM(
  const ProjData& inputProj,
  const ScannerData& scanner,
  VolData& outputVol,
  const std::string& outputVolFileName,
  const OSEMCoreParams& params,
  const VolData& sensitivityMap,
  const std::optional<ProjData>& biasProj)
{
  echo("OSEM:");

  // Check proj data dimensions
  scanner.checkProjData(inputProj);

  // Check number of subsets
  inputProj.checkNSubsets(params.nSubsets);

  const auto convolveFlag = //
    params.convolutionInterval > 0 && params.fwhmXYZ[0] > 0.0 &&
    params.fwhmXYZ[1] > 0.0 && params.fwhmXYZ[2] > 0.0;

  // Allocate empty volume for back-projection
  VolData backProj(
    outputVol,
    VolData::ConstructionMode::INITIALIZE,
    0.0);

  // Initialize siddon algorithm and LOR list
  LORCache cache(inputProj, params.nSubsets);
  Siddon siddon(outputVol);

  // Cut circle at the center of the image
  operations::cutCircle(outputVol, params.cutRadius);

  const auto nSubiterations =
    params.nIterations * params.nSubsets;

  // Main iterations
  LOOP(iter, 0, params.nIterations - 1)
  {
    print("Iteration ", iter + 1, " of ", params.nIterations);

    // Sub-iterations
    LOOP(subset, 0, params.nSubsets - 1)
    {
      const auto subiter = iter * params.nSubsets + subset + 1;

      if (params.nSubsets > 1)
      {
        print(
          "  Sub-iteration ",
          subset + 1,
          " of ",
          params.nSubsets);
      }

      LOOP_SEG(seg, inputProj)
      {
        const auto nBinsForCurrentSubsetAndSegment =
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

          auto [binIndex, line] = getLine(
            index,
            cache,
            siddon,
            scanner,
            threadLocalPathElements,
            iter == 0,
            outputVol);

          // Add bias if biasProj is provided
          if (biasProj != std::nullopt)
          {
            line += BIN(*biasProj, seg, binIndex);
          }

          // Compute ratio with input projection and project
          // into backProj
          if (line > EPSILON)
          {
            backProj.projectLineIntegral(
              threadLocalPathElements,
              BIN(inputProj, seg, binIndex) / line);
          }
        }
      }

      // Divide backProj by sensitivity
      sensitivityMap.setActiveFrame(subset);
      backProj /= sensitivityMap;

      // Multiply output volume by backProj
      outputVol *= backProj;

      // Convolve output image with a gaussian kernel
      if (
        convolveFlag &&
        subiter % params.convolutionInterval == 0)
      {
        operations::convolve(
          outputVol,
          params.fwhmXYZ,
          params.cutRadius);
      }

      // Cut circle at the center of the image
      operations::cutCircle(outputVol, params.cutRadius);

      // Reset backProj to zero for next iteration
      if (subiter != nSubiterations)
      {
        backProj.setAllVoxels(0.0);
      }

      // Save intermediate result if requested
      if (
        params.saveInterval > 0 &&
        subiter % params.saveInterval == 0 &&
        subiter != nSubiterations)
      {
        const auto intermediateVolFileName = outputVolFileName +
          "_subiter_" + std::to_string(subiter);

        outputVol.write(intermediateVolFileName);
      }
    }
  }
}

void OSEM_ResoReco(
  const ProjData& inputProj,
  const ScannerData& scanner,
  VolData& outputVol,
  const std::string& outputVolFileName,
  const OSEMCoreParams& params,
  VolData& sensitivityMap,
  const std::optional<ProjData>& biasProj)
{
  echo("OSEM_ResoReco:");

  // Check proj data dimensions
  scanner.checkProjData(inputProj);

  // Check number of subsets
  inputProj.checkNSubsets(params.nSubsets);

  const auto convolveFlag = params.convolutionInterval > 0 &&
    params.fwhmXYZ[0] > 0.0 && params.fwhmXYZ[1] > 0.0 &&
    params.fwhmXYZ[2] > 0.0;

  // Initialize siddon algorithm and LOR list
  LORCache cache(inputProj, params.nSubsets);
  Siddon siddon(outputVol);

  // Initialize empty volume for back-projection
  VolData backProj(
    outputVol,
    VolData::ConstructionMode::INITIALIZE,
    0.0);

  // Cut circle at the center of the image
  operations::cutCircle(outputVol, params.cutRadius);

  // Convolve sensitivity image with a gaussian kernel
  operations::convolve(
    sensitivityMap,
    params.fwhmXYZ,
    params.cutRadius);

  // Initialize copy of output volume used for blurring
  VolData blur(outputVol, VolData::ConstructionMode::READ_DATA);

  const auto nSubiterations =
    params.nIterations * params.nSubsets;

  // Main iterations
  LOOP(iter, 0, params.nIterations - 1)
  {
    print("Iteration ", iter + 1, " of ", params.nIterations);

    // Sub-iterations
    LOOP(subset, 0, params.nSubsets - 1)
    {
      const auto subiter = iter * params.nSubsets + subset + 1;

      if (params.nSubsets > 1)
      {
        print(
          "  Sub-iteration ",
          subset + 1,
          " of ",
          params.nSubsets);
      }

      operations::convolve(
        blur,
        params.fwhmXYZ,
        params.cutRadius);

      LOOP_SEG(seg, inputProj)
      {
        const auto nBinsForCurrentSubsetAndSegment =
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

          auto [binIndex, line] = getLine(
            index,
            cache,
            siddon,
            scanner,
            threadLocalPathElements,
            iter == 0,
            outputVol);

          // Add bias if biasProj is provided
          if (biasProj != std::nullopt)
          {
            line += BIN(*biasProj, seg, binIndex);
          }

          // Compute ratio with input projection and project
          // into backProj
          if (line > EPSILON)
          {
            backProj.projectLineIntegral(
              threadLocalPathElements,
              BIN(inputProj, seg, binIndex) / line);
          }
        }
      }

      // Convolve back-projection with a gaussian kernel
      operations::convolve(
        backProj,
        params.fwhmXYZ,
        params.cutRadius);

      // Divide backProj by sensitivity
      sensitivityMap.setActiveFrame(subset);
      outputVol /= sensitivityMap;

      // Multiply output volume by backProj
      outputVol *= backProj;

      // Reset backProj to zero for next iteration
      if (subiter != nSubiterations)
      {
        backProj.setAllVoxels(0.0);
      }

      // Convolve output image with a gaussian kernel
      if (
        convolveFlag &&
        subiter % params.convolutionInterval == 0)
      {
        operations::convolve(
          outputVol,
          params.fwhmXYZ,
          params.cutRadius);
      }

      // Cut circle at the center of the image
      operations::cutCircle(outputVol, params.cutRadius);

      // Set blur to output volume for next iteration
      if (subiter != nSubiterations)
      {
        blur.assign(outputVol);
      }

      // Save intermediate result if requested
      if (
        params.saveInterval > 0 &&
        subiter % params.saveInterval == 0 &&
        subiter != nSubiterations)
      {
        const auto intermediateVolFileName = outputVolFileName +
          "_subiter_" + std::to_string(subiter);

        outputVol.write(intermediateVolFileName);
      }
    }
  }
}
}
