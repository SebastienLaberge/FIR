#include <KeyParser.h>
#include <ProjData.h>
#include <ScannerData.h>
#include <VolData.h>
#include <console.h>
#include <operations.h>
#include <projections.h>
#include <tools.h>

#include <iostream>
#include <numeric>
#include <optional>
#include <string>
#include <vector>

struct Params
{
  Params(const char* paramFile);
  bool checkFrames(int nFrames);
  void printContent();

  // Mandatory
  std::string inputVolFile;
  std::string scannerFile;
  std::string outputProjHeader;
  std::string outputProjFileName;

  // Optional
  std::vector<int> frames;
  std::string maskVolFile;
};

int main(int argc, char** argv)
{
  try
  {
    printEmptyLine();
    echo("=== FIR_ForwardProj ===");
    printEmptyLine();

    const auto nThreads = getNThreads();
    printValue("Number of threads", nThreads);
    printEmptyLine();

    // Check shell parameters
    if (argc < 2)
    {
      error("Parameter file missing");
    }

    Params params(argv[1]);

    // Read input volume
    VolData inputVol(
      params.inputVolFile,
      VolData::ConstructionMode::READ_DATA);

    // Check frame parameter
    const auto singleFrame =
      params.checkFrames(inputVol.getNFrames());

    // Read mask if provided
    std::optional<VolData> maskVol = std::nullopt;
    if (!params.maskVolFile.empty())
    {
      maskVol = VolData(
        params.maskVolFile,
        VolData::ConstructionMode::READ_DATA);
    }

    // Initialize scanner
    ScannerData scanner(params.scannerFile);

    // Initialize output projection
    ProjData outputProj(
      params.outputProjHeader,
      ProjData::ConstructionMode::ALLOCATE);

    for (const auto frameIndex : params.frames)
    {
      inputVol.setActiveFrame(frameIndex);

      // Apply mask if provided
      if (maskVol.has_value())
      {
        operations::applyMask(inputVol, *maskVol);
      }

      // Execute forward projection
      projections::forward(inputVol, scanner, outputProj);

      // Get name for saved projection
      const auto outputProjFileName = singleFrame ?
        params.outputProjFileName :
        params.outputProjFileName + "_frame_" +
          std::to_string(frameIndex);

      // Save projection
      outputProj.write(outputProjFileName);
    }
  }
  catch (const std::exception& ex)
  {
    std::cerr << ex.what();
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

Params::Params(const char* paramFile)
{
  KeyParser kp;

  kp.addStartKey("!FORWARD PROJECTION PARAMETERS");

  kp.addKey("input volume file", &inputVolFile);
  kp.addKey("scanner file", &scannerFile);
  kp.addKey("output projection header", &outputProjHeader);
  kp.addKey("output projection file name", &outputProjFileName);

  kp.addKey("frames to project", &frames);

  kp.addKey("mask volume file", &maskVolFile);

  kp.addStopKey("!END OF FORWARD PROJECTION PARAMETERS");

  kp.parse(paramFile);

  if (inputVolFile.empty())
  {
    error("No input volume file provided");
  }

  if (scannerFile.empty())
  {
    error("No scanner file provided");
  }

  if (outputProjHeader.empty())
  {
    error("No output projection header provided");
  }

  if (outputProjHeader.empty())
  {
    error("No output projection file name provided");
  }
}

bool Params::checkFrames(int nFrames)
{
  bool singleFrame;

  if (frames.empty())
  {
    // No frames provided: Take all frames

    singleFrame = nFrames == 1;

    frames = std::vector<int>(nFrames);
    std::iota(frames.begin(), frames.end(), 0);
  }
  else
  {
    // Frames provided: Check their value

    singleFrame = false;

    for (const auto frameIndex : frames)
    {
      if (!(frameIndex >= 0 && frameIndex < nFrames))
      {
        error(
          "Frame indices must be between 0 and ",
          nFrames - 1);
      }
    }
  }

  return singleFrame;
}

void Params::printContent()
{
  printEmptyLine();
  echo("=== Forward projection parameters ===");
  printEmptyLine();

  printValue("input volume file", inputVolFile);
  printValue("scanner file", scannerFile);
  printValue("output projection header", outputProjHeader);
  printValue("output projection file name", outputProjFileName);
  printVector("frames to project", frames);
  printValue("mask volume file", maskVolFile);
  printEmptyLine();
}
