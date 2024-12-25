#include <ScannerHeader.h>

#include <console.h>

void ScannerHeader::setDefaults()
{
  // rSectorRepeatNumber and rSectorInnerRadius are the only
  // mandatory parameters

  // Crystals
  crystalDimsXYZ.resize(3, 0.0);
  crystalRepeatNumbersYZ.resize(2, 1);
  interCrystalDistanceYZ.resize(2, 0.0);

  // Modules
  moduleDimsXYZ.resize(3, 0.0); // If set to 0: Get min value
  moduleRepeatNumbersYZ.resize(2, 1);
  interModuleDistanceYZ.resize(2, 0.0);

  // rSector
  rSectorDimsXYZ.resize(3, 0.0); // If set to 0: Get min value
  rSectorRepeatNumber = 0;  // Causes error if kept unchanged
  rSectorInnerRadius = 0.0; // Causes error if kept unchanged
}

void ScannerHeader::check()
{
  // crystalDimsXYZ:
  // - Must contain three elements
  // - All elements must be >= 0
  // - All elements default to 0
  // - Can only be 0 in Y if crystal repeat number in Y is 1
  // - Can only be 0 in Z if crystal repeat number in Z is 1

  if (crystalDimsXYZ.size() != 3)
  {
    error("\"crystal dimensions XYZ in mm\" must contain three "
          "elements");
  }

  if (
    crystalDimsXYZ[0] < 0 || //
    crystalDimsXYZ[1] < 0 || //
    crystalDimsXYZ[2] < 0)
  {
    error("Elements of \"crystal dimensions XYZ in mm\" must "
          "not be negative");
  }

  // crystalRepeatNumbersYZ:
  // - Must contain two elements
  // - All elements must be > 0
  // - All elements default to 1

  if (crystalRepeatNumbersYZ.size() != 2)
  {
    error("\"crystal repeat numbers YZ\" must contain two "
          "elements");
  }

  if (
    crystalRepeatNumbersYZ[0] <= 0 ||
    crystalRepeatNumbersYZ[1] <= 0)
  {
    error(
      "Elements of \"crystal repeat numbers YZ\" must be > 0");
  }

  // interCrystalDistanceYZ:
  // - Must contain two elements
  // - All elements must be >= 0
  // - All elements default to 0

  if (interCrystalDistanceYZ.size() != 2)
  {
    error("\"inter-crystal distance YZ in mm\" must contain "
          "two elements");
  }

  if (
    interCrystalDistanceYZ[0] < 0 ||
    interCrystalDistanceYZ[1] < 0)
  {
    error("Elements of \"inter-crystal distance YZ in mm\" "
          "must not be negative");
  }

  // moduleDimsXYZ:
  // - Must contain two elements
  // - All elements must be >= 0
  // - All elements default to 0
  // - All elements must be greater than the underlying crystal
  // - A value of 0 is replaced by the smallest possible value

  if (moduleDimsXYZ.size() != 3)
  {
    error("\"module dimensions XYZ in mm\" must contain three "
          "elements");
  }

  if (
    moduleDimsXYZ[0] < 0 || //
    moduleDimsXYZ[1] < 0 || //
    moduleDimsXYZ[2] < 0)
  {
    error("Elements of \"module dimensions XYZ in mm\" must "
          "not be negative");
  }

  // moduleRepeatNumbersYZ:
  // - Must contain two elements
  // - All elements must be > 0
  // - All elements default to 1

  if (moduleRepeatNumbersYZ.size() != 2)
  {
    error(
      "\"module repeat numbers YZ\" must contain two elements");
  }

  if (
    moduleRepeatNumbersYZ[0] <= 0 ||
    moduleRepeatNumbersYZ[1] <= 0)
  {
    error(
      "Elements of \"module repeat numbers YZ\" must be > 0");
  }

  // interModuleDistanceYZ:
  // - Must contain two elements
  // - All elements must be >= 0

  if (interModuleDistanceYZ.size() != 2)
  {
    error("\"inter-module distance YZ in mm\" must contain two "
          "elements");
  }

  if (
    interModuleDistanceYZ[0] < 0 ||
    interModuleDistanceYZ[1] < 0)
  {
    error("Elements of \"inter-module distance YZ in mm\" must "
          "not be negative");
  }

  // rSectorDimsXYZ:
  // - Must contain two elements
  // - All elements must be >= 0
  // - All elements default to 0
  // - All elements must be greater than the underlying module
  // - A value of 0 is replaced by the smallest possible value

  if (rSectorDimsXYZ.size() != 3)
  {
    error("\"rSector dimensions XYZ in mm\" must contain three "
          "elements");
  }

  if (
    rSectorDimsXYZ[0] < 0 || //
    rSectorDimsXYZ[1] < 0 || //
    rSectorDimsXYZ[2] < 0)
  {
    error("Elements of \"rSector dimensions XYZ in mm\" must "
          "not be negative");
  }

  // rSectorRepeatNumber:
  // - Single number
  // - Must be > 0
  // - Mandatory (no default)

  if (rSectorRepeatNumber <= 0)
  {
    error("\"rSector repeat number\" must be present and > 0");
  }

  // rSectorInnerRadius:
  // - Single number
  // - Must be > 0
  // - Mandatory (no default)

  if (rSectorInnerRadius <= 0)
  {
    error(
      "\"rSector inner radius in mm\" must be present and > 0");
  }

  // For dimensions Y and Z, check that crystals have non-zero
  // dimension if crystal repeat number is greater than 1

  if (crystalRepeatNumbersYZ[0] > 1 && crystalDimsXYZ[1] == 0)
  {
    error("Crystal repeat number in Y cannot be > 1 if crystal "
          "dimension in Y is zero");
  }

  if (crystalRepeatNumbersYZ[1] > 1 && crystalDimsXYZ[2] == 0)
  {
    error("Crystal repeat number in Z cannot be > 1 if crystal "
          "dimension in Z is zero");
  }

  // Default and minimum module size: Fits tightly on crystals

  const auto minModuleDimsX = crystalDimsXYZ[0];

  if (moduleDimsXYZ[0] == 0.0)
  {
    moduleDimsXYZ[0] = minModuleDimsX;
  }
  else if (moduleDimsXYZ[0] < minModuleDimsX)
  {
    error("Module dimension in X must be >= ", minModuleDimsX);
  }

  const auto minModuleDimsY =
    crystalDimsXYZ[1] * crystalRepeatNumbersYZ[0] +
    interCrystalDistanceYZ[0] * (crystalRepeatNumbersYZ[0] - 1);

  if (moduleDimsXYZ[1] == 0.0)
  {
    moduleDimsXYZ[1] = minModuleDimsY;
  }
  else if (moduleDimsXYZ[1] < minModuleDimsY)
  {
    error("Module dimension in Y must be >= ", minModuleDimsY);
  }

  const auto minModuleDimsZ =
    crystalDimsXYZ[2] * crystalRepeatNumbersYZ[1] +
    interCrystalDistanceYZ[1] * (crystalRepeatNumbersYZ[1] - 1);

  if (moduleDimsXYZ[2] == 0.0)
  {
    moduleDimsXYZ[2] = minModuleDimsZ;
  }
  else if (moduleDimsXYZ[2] < minModuleDimsZ)
  {
    error("Module dimension in Z must be >= ", minModuleDimsZ);
  }

  // For dimensions Y and Z, check that modules have non-zero
  // dimension if module repeat number is greater than 1

  if (moduleRepeatNumbersYZ[0] > 1 && moduleDimsXYZ[1] == 0)
  {
    error("Module repeat number in Y cannot be > 1 if module "
          "dimension in Y is zero");
  }

  if (moduleRepeatNumbersYZ[1] > 1 && moduleDimsXYZ[2] == 0)
  {
    error("Module repeat number in Z cannot be > 1 if module "
          "dimension in Z is zero");
  }

  // Default and minimum rSector size: Fits tightly on modules

  const auto minrSectorDimsX = moduleDimsXYZ[0];

  if (rSectorDimsXYZ[0] == 0)
  {
    rSectorDimsXYZ[0] = minrSectorDimsX;
  }
  else if (rSectorDimsXYZ[0] < minrSectorDimsX)
  {
    error(
      "rSector dimension in X must be >= ",
      minrSectorDimsX);
  }

  const auto minrSectorDimsY =
    moduleDimsXYZ[1] * moduleRepeatNumbersYZ[0] +
    interModuleDistanceYZ[0] * (moduleRepeatNumbersYZ[0] - 1);

  if (rSectorDimsXYZ[1] == 0)
  {
    rSectorDimsXYZ[1] = minrSectorDimsY;
  }
  else if (rSectorDimsXYZ[1] < minrSectorDimsY)
  {
    error(
      "rSector dimension in Y must be >= ",
      minrSectorDimsY);
  }

  const auto minrSectorDimsZ =
    moduleDimsXYZ[2] * moduleRepeatNumbersYZ[1] +
    interModuleDistanceYZ[1] * (moduleRepeatNumbersYZ[1] - 1);

  if (rSectorDimsXYZ[2] == 0)
  {
    rSectorDimsXYZ[2] = minrSectorDimsZ;
  }
  else if (rSectorDimsXYZ[2] < minrSectorDimsZ)
  {
    error(
      "rSector dimension in Z must be >= ",
      minrSectorDimsZ);
  }
}

void ScannerGeometry::fill(const ScannerHeader& header)
{
  // Crystal repeat vector
  crystalRepeatVectorYZ.resize(2);
  crystalRepeatVectorYZ[0] =
    header.crystalDimsXYZ[1] + header.interCrystalDistanceYZ[0];
  crystalRepeatVectorYZ[1] =
    header.crystalDimsXYZ[2] + header.interCrystalDistanceYZ[1];

  // Module repeat vector
  moduleRepeatVectorYZ.resize(2);
  moduleRepeatVectorYZ[0] =
    header.moduleDimsXYZ[1] + header.interModuleDistanceYZ[0];
  moduleRepeatVectorYZ[1] =
    header.moduleDimsXYZ[2] + header.interModuleDistanceYZ[1];

  // Translation of reference rSector from origin along x
  rSectorTranslationX =
    header.rSectorInnerRadius + header.rSectorDimsXYZ[0] / 2;

  // Numbers
  nCrystalsPerRing = //
    header.crystalRepeatNumbersYZ[0] *
    header.moduleRepeatNumbersYZ[0] *
    header.rSectorRepeatNumber;
  nRings = //
    header.crystalRepeatNumbersYZ[1] *
    header.moduleRepeatNumbersYZ[1];
  nCrystals = nRings * nCrystalsPerRing;

  // Number of slices
  nSlices = 2 * nRings - 1;

  // Crystal offset
  crystalOffset = //
    header.moduleRepeatNumbersYZ[0] *
    header.crystalRepeatNumbersYZ[0] / 2;
}
