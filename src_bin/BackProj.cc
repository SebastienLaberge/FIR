#include <ScannerData.h>
#include <console.h>
#include <projections.h>
#include <tools.h>

#include <iostream>

// Input 1: Input projection header file
// Input 2: Scanner file
// Input 3: Output volume template header file
// Input 4: Output volume file name (no extension)

int main(int argc, char** argv)
{
  try
  {
    if (argc < 4)
    {
      error("Requires four input arguments");
    }

    // Initialize input projection
    ProjData inputProj(
      argv[1],
      ProjData::ConstructionMode::READ_DATA);

    // Initialize scanner
    ScannerData scanner(argv[2]);

    // Initialize output projection
    VolData outputVol(
      argv[3],
      VolData::ConstructionMode::ALLOCATE);

    // Execute back-projection
    projections::backward(inputProj, scanner, outputVol);

    // Save result
    outputVol.write(argv[4]);
  }
  catch (const std::exception& ex)
  {
    std::cerr << ex.what();
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
