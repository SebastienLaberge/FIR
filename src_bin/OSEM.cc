#include <KeyParser.h>
#include <ProjData.h>
#include <ScannerData.h>
#include <VolData.h>
#include <console.h>
#include <operations.h>
#include <projections.h>
#include <reconAlgos.h>
#include <tools.h>

#include <fstream>
#include <iostream>
#include <optional>
#include <string>

// Notes on parameter file:
//
// PSF_OSEM paramFile.params recomSensFlag recomAttenCorrFlag
//
// Parameters file cannot be ommited
//
// Flags can be 0 or 1 (anything that doesn't begin with 0 is
// interpreted as 1). All flags default to 1 if absent.
//
//   recomSensFlag: Recompute sensitivity volume
//   recomAttenCorrFlag: Recompute attenuation correction
//     factors if an attenuation volume is provided
//
// Parameter file:
//
// 1: Parameters "input projection file", "scanner file",
//    "output volume header" and "output volume file name" are
//     required.
//
// 2: -If volume header provided by parameter
//     "output volume header" links to a data file,
//     that data is used as a first approximation.
//    -The dimensions of the volume specified by the header will
//     be those of the output volume.
//
// 3: -Parameters "number of iterations" and "number of subsets"
//     each default to 1 if absent.
//    -Parameter "save interval" defaults to 0 if absent.
//    -If 0, only the final image is saved.
//
// 4: -Volume provided by parameter "sensitivity map volume" has
//     to have the same dimensions as the output volume.
//    -If absent, sensitivity is computed but not saved.
//    -If present but the file is not found, sensitivity is
//     computed and saved with the name provided.
//    -If present and the file is found, the file is read and
//     used in the reconstruction, which speeds up the
//     execution.
//    -In any case, the sensitivity is computed if the recompute
//     sensitivity flag is provided as a shell argument and
//     saved if 'attenuation volume file' is provided. This
//     should be done if the scanner or number of subsets is
//     changed from the last time sensitivity was computed.
//
// 5: -Volume provided by parameter "attenuation volume in HU"
//     does not have to be the same size as the output volume.
//    -The attenuation volume must be in Housfield units
//    -Projection provided by parameter "attenuation correction
//     factors" must have the same dimensions as the input
//     projection.
//
// 6: -Projection provided by parameter "bias projection" must
//     have the same dimensions as the input projection.
//    -If absent, no bias is added to the projection.

struct Params
{
  Params(const char* paramFile);
  void printContent();

  // Main files (mandatory)
  std::string inputProjFile;
  std::string scannerFile;
  std::string outputVolHeader;
  std::string outputVolFileName;

  // Core parameters (optional with default values)
  OSEMCoreParams algoParams;

  // Optional files

  // Sensitivity
  std::string sensVolFile;

  // Bias
  std::string biasProjFile;

  // Attenuation
  std::string attenVolHUFile;
  std::string attenCorrFactorsFile;
};

int main(int argc, char** argv)
{
  try
  {
    printEmptyLine();
    echo("=== FIR_OSEM ===");
    printEmptyLine();

    // Print number of threads
    const auto nThreads = getNThreads();
    printValue("Number of threads", nThreads);
    printEmptyLine();

    //// 1) Manage input parameters

    // Check number of parameters
    if (argc < 2)
    {
      error("Parameter file missing");
    }

    // Read parameters
    Params params(argv[1]);
    // params.printContent();

    // Check if optional files are provided
    const auto sensVolFileProvided =
      !params.sensVolFile.empty();
    const auto attenVolHUFileProvided =
      !params.attenVolHUFile.empty();
    const auto attenCorrFactorsFileProvided =
      !params.attenCorrFactorsFile.empty();

    // Retrieve flags
    auto recomputeSensitivityFlag = true;
    auto recomputeAttenuationCorrectionFlag = true;
    if (argc > 2)
    {
      recomputeSensitivityFlag =
        argv[2][0] == '0' ? false : true;
    }
    if (argc > 3)
    {
      recomputeAttenuationCorrectionFlag =
        argv[3][0] == '0' ? false : true;
    }

    // If sensitivity is being asked not to be recomputed,
    // recompute it anyway if sensitivity map file is not
    // provided or if it is provided but doesn't exist.
    if (!recomputeSensitivityFlag)
    {
      if (!sensVolFileProvided)
      {
        recomputeSensitivityFlag = true;
      }
      else
      {
        // TODO: Find a better way to check if it exists
        std::ifstream f(params.sensVolFile);
        const auto sensFileExists = f.good();
        recomputeSensitivityFlag = !sensFileExists;
      }
    }

    // Check if attenuation factors file is exists if provided
    bool attenCorrFactorsFileExists;
    if (attenCorrFactorsFileProvided)
    {
      // TODO: Find a better way to check if it exists
      std::ifstream f(params.attenCorrFactorsFile);
      attenCorrFactorsFileExists = f.good();
    }
    else
    {
      attenCorrFactorsFileExists = false;
    }

    //// 2) Prepare main data structures

    // Read input projection
    ProjData inputProj(
      params.inputProjFile,
      ProjData::ConstructionMode::READ_DATA);
    inputProj.checkNSubsets(params.algoParams.nSubsets);
    // inputProj.printContent();

    // Read scanner
    ScannerData scanner(params.scannerFile);
    // scanner.printContent();

    // Initialize output volume
    // 1) If no volume file is provided, fill with ones
    // 2) If volume file is provided, read the volume and use it
    VolData outputVol(
      params.outputVolHeader,
      VolData::ConstructionMode::READ_DATA_IF_PROVIDED,
      1.0);
    printEmptyLine();
    // outputVol.printContent();

    // Get sensitivity map
    VolData sensVol;
    if (recomputeSensitivityFlag)
    {
      echo("Computing sensitivity map");
      printEmptyLine();

      sensVol.allocateAsMultiVol(
        outputVol,
        params.algoParams.nSubsets);

      projections::computeSensitivityVol(
        inputProj,
        scanner,
        sensVol,
        params.algoParams.nSubsets);

      // Save sensitivity map
      if (sensVolFileProvided)
      {
        printQuotedValue(
          "Saving sensitivity map to file",
          params.sensVolFile);
        printEmptyLine();

        sensVol.write(params.sensVolFile);
      }
    }
    else
    {
      printQuotedValue(
        "Reading sensitivity map from file",
        params.sensVolFile);
      printEmptyLine();

      sensVol.read(
        params.sensVolFile,
        VolData::ConstructionMode::READ_DATA);

      if (sensVol.getHeader() != outputVol.getHeader())
      {
        error("Sensitivity volume provided doesn't fit with "
              "output volume provided");
      }
    }
    // sensVol.printContent();

    // Read bias projection if provided
    std::optional<ProjData> biasProj = std::nullopt;
    if (!params.biasProjFile.empty())
    {
      printQuotedValue(
        "Reading bias projection from file",
        params.biasProjFile);
      printEmptyLine();

      biasProj = ProjData(
        params.biasProjFile,
        ProjData::ConstructionMode::READ_DATA);
    }

    //// 3) Do pre-processing

    // Apply attenuation correction if HU volume is provided
    // (error later if absent) or if attenuation factors file
    // is provided and exists.
    if (attenVolHUFileProvided || attenCorrFactorsFileExists)
    {
      // Exception 1: recomputeAttenuationCorrectionFlag == 0
      // but correction factors absent
      if (
        !recomputeAttenuationCorrectionFlag &&
        attenVolHUFileProvided && !attenCorrFactorsFileExists)
      {
        recomputeAttenuationCorrectionFlag = true;
      }

      // Exception 2: recomputeAttenuationCorrectionFlag == 1
      // but HU volume absent
      if (
        recomputeAttenuationCorrectionFlag &&
        !attenVolHUFileProvided && attenCorrFactorsFileExists)
      {
        recomputeAttenuationCorrectionFlag = false;
      }

      // Get attenuation correction factors
      ProjData attenCorrFactors;
      if (recomputeAttenuationCorrectionFlag)
      {
        echo("Computing attenuation correction factors");
        printEmptyLine();

        // Open attenuation volume in Hounsfield units and
        // convert to attenuation factors in mm^-1 (mu map)
        VolData muMap(
          params.attenVolHUFile,
          VolData::ConstructionMode::READ_DATA);
        operations::HounsfieldToMuMap(muMap);
        operations::cutCircle(
          muMap,
          params.algoParams.cutRadius);

        // Compute exponential of line integrals of mu
        // => The attenuation correction factors are the
        //    inverse of the attenuation factors, given by
        //    exp(-lineIntegralOfMuMap)
        attenCorrFactors.copy(
          inputProj,
          ProjData::ConstructionMode::INITIALIZE);
        projections::forward(muMap, scanner, attenCorrFactors);
        attenCorrFactors.exponential();

        // Save attenuation correction factors if file name
        // provided
        if (attenCorrFactorsFileProvided)
        {
          printQuotedValue(
            "Saving attenuation correction factors to file",
            params.attenCorrFactorsFile);
          printEmptyLine();

          attenCorrFactors.write(params.attenCorrFactorsFile);
        }
      }
      else
      {
        printQuotedValue(
          "Reading attenuation correction factors from file",
          params.attenCorrFactorsFile);
        printEmptyLine();

        attenCorrFactors.read(
          params.attenCorrFactorsFile,
          ProjData::ConstructionMode::READ_DATA);
      }

      // Multiply inputProj by attenCorrFactors
      inputProj *= attenCorrFactors;
    }

    // TODO: Provide that parameter somehow
    const auto resoRecoFlag = false;

    //// 4) Execute reconstruction
    if (!resoRecoFlag)
    {
      reconAlgos::OSEM(
        inputProj,
        scanner,
        outputVol,
        params.outputVolFileName,
        params.algoParams,
        sensVol,
        biasProj);
    }
    else
    {
      reconAlgos::OSEM_ResoReco(
        inputProj,
        scanner,
        outputVol,
        params.outputVolFileName,
        params.algoParams,
        sensVol,
        biasProj);
    }

    //// 5) Save reconstructed volume

    printQuotedValue(
      "Saving reconstructed volume to file",
      params.outputVolFileName);
    printEmptyLine();

    outputVol.write(params.outputVolFileName);
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
  // TODO: Check fwhmXYZ

  KeyParser kp;

  kp.addStartKey("!OSEM PARAMETERS");

  // Main files
  kp.addKey("input projection file", &inputProjFile);
  kp.addKey("scanner file", &scannerFile);
  kp.addKey("output volume header", &outputVolHeader);
  kp.addKey("output volume file name", &outputVolFileName);

  // Core parameters

  // Reconstruction parameters
  kp.addKey("number of iterations", &algoParams.nIterations);
  kp.addKey("number of subsets", &algoParams.nSubsets);

  // Save parameters
  kp.addKey("save interval", &algoParams.saveInterval);

  // Operation parameters
  kp.addKey("cut radius in mm", &algoParams.cutRadius);
  kp.addKey(
    "convolution interval",
    &algoParams.convolutionInterval);
  kp.addKey("convolution FHWM XYZ in mm", &algoParams.fwhmXYZ);

  // Optional files

  // Sensitivity
  kp.addKey("sensitivity map volume", &sensVolFile);

  // Bias
  kp.addKey("bias projection", &biasProjFile);

  // Attenuation
  kp.addKey("attenuation volume in HU", &attenVolHUFile);
  kp.addKey(
    "attenuation correction factors",
    &attenCorrFactorsFile);

  kp.addStopKey("!END OF OSEM PARAMETERS");

  kp.parse(paramFile);

  // Check mandatory parameters
  if (inputProjFile.empty())
  {
    error("No input projection file provided");
  }
  if (scannerFile.empty())
  {
    error("No scanner file provided");
  }
  if (outputVolHeader.empty())
  {
    error("No output volume header provided");
  }
  if (outputVolHeader.empty())
  {
    error("No output volume file name provided");
  }
}

void Params::printContent()
{
  printEmptyLine();
  echo("= OSEM parameters");
  printEmptyLine();

  echo("== Main files");
  printValue("input projection file", inputProjFile);
  printValue("scanner file", scannerFile);
  printValue("output volume header", outputVolHeader);
  printValue("output volume file name", outputVolFileName);
  printEmptyLine();

  echo("== Core parameters");
  printEmptyLine();

  echo("=== Recontruction parameters");
  printValue("number of iterations", algoParams.nIterations);
  printValue("number of subsets", algoParams.nSubsets);
  printEmptyLine();

  echo("=== Save parameters");
  printValue("save interval", algoParams.saveInterval);
  printEmptyLine();

  echo("=== Operation parameters");
  printValue("cut radius in mm", algoParams.cutRadius);
  printValue(
    "convolution interval",
    algoParams.convolutionInterval);
  printVector("convolution FHWM XYZ in mm", algoParams.fwhmXYZ);
  printEmptyLine();

  echo("== Optional files");
  printEmptyLine();

  echo("=== Sensitivity");
  printValue("sensitivity map volume", sensVolFile);
  printEmptyLine();

  echo("=== Bias");
  printValue("bias projection", biasProjFile);
  printEmptyLine();

  echo("=== Attenuation");
  printValue("attenuation volume in HU", attenVolHUFile);
  printValue(
    "attenuation correction factors",
    attenCorrFactorsFile);
  printEmptyLine();
}
