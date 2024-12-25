#include <ScannerData.h>

#include <ProjData.h>
#include <ScannerInterfileReader.h>
#include <VolData.h>
#include <console.h>
#include <macros.h>

#include <cmath>
#include <cstdio>
#include <iostream>
#include <tuple>
#include <vector>

ScannerData::ScannerData() :
  mCrystalXYPositionVector{nullptr},
  mSliceZPositionVector{nullptr}
{
}

ScannerData::ScannerData(const std::string& scannerFile) :
  ScannerData()
{
  readScannerInterfileHeader(scannerFile, mHeader);
  mGeometry.fill(mHeader);

  computeCrystalXYPositionVector();
  computeSliceZPositionVector();
}

void ScannerData::checkProjData(const ProjData& proj) const
{
  const auto& header = proj.getHeader();

  if (header.nRings != mGeometry.nRings ||
      header.nCrystalsPerRing != mGeometry.nCrystalsPerRing)
  {
    error("Projection must have the same number of rings (",
          mGeometry.nRings,
          ") and the same number of crystals per ring (",
          mGeometry.nCrystalsPerRing,
          ") as the scanner");
  }
}

void ScannerData::printContent() const
{
  echo("= Scanner header:");
  printEmptyLine();

  echo("== Crystals:");
  printVector("crystal dimensions XYZ in mm",
			 mHeader.crystalDimsXYZ);
  printVector("crystal repeat numbers YZ",
			  mHeader.crystalRepeatNumbersYZ);
  printVector("inter-crystal distance YZ in mm",
			  mHeader.interCrystalDistanceYZ);
  printEmptyLine();

  echo("== Modules");
  printVector("module dimensions XYZ in mm",
			  mHeader.moduleDimsXYZ);
  printVector("module repeat numbers YZ",
			  mHeader.moduleRepeatNumbersYZ);
  printVector("inter-module distance YZ in mm",
			  mHeader.interModuleDistanceYZ);
  printEmptyLine();

  echo("== R-sectors:");
  printVector("rSector dimensions XYZ in mm",
			  mHeader.rSectorDimsXYZ);
  printValue("rSector repeat number",
			 mHeader.rSectorRepeatNumber);
  printValue("rSector inner radius in mm",
			 mHeader.rSectorInnerRadius);
  printEmptyLine();

  echo("== Translations:");
  printVector("crystal repeat vector YZ in mm",
			  mGeometry.crystalRepeatVectorYZ);
  printVector("module repeat vector YZ in mm",
			  mGeometry.moduleRepeatVectorYZ);
  printValue("rSector translation in X in mm",
			 mGeometry.rSectorTranslationX);
  printEmptyLine();

  echo("== Numbers:");
  printValue("number of rings", mGeometry.nRings);
  printValue("number of crystals per ring",
			 mGeometry.nCrystalsPerRing);
  printValue("number of crystals", mGeometry.nCrystals);
  printValue("number of slices", mGeometry.nSlices);
  printValue("crystal offset", mGeometry.crystalOffset);
  printEmptyLine();
}

void ScannerData::printCrystalPositions() const
{
  echo("Crystal positions in single ring (X , Y):");
  LOOP(crystalNumber, 0, mGeometry.nCrystalsPerRing - 1)
    std::printf(
      "%.5f , %.5f\n",
      mCrystalXYPositionVector[crystalNumber].x,
      mCrystalXYPositionVector[crystalNumber].y);
  printEmptyLine();

  std::cout << "Slice positions in Z:" << std::endl;
  LOOP(sliceNumber, 0, mGeometry.nSlices - 1)
    std::printf("%.5f\n", mSliceZPositionVector[sliceNumber]);
  printEmptyLine();
}

void ScannerData::printBinInfo(
  int seg, int view, int axialCoord, int tangCoord,
  int crystal1, int slice1, int crystal2, int slice2,
  const VolData& vol,
  types::PathElement* pathElementsArray,
  types::VoxelValue line) const
{
  // Bin coordinates
  printValue("seg", seg);
  printValue("view", view);
  printValue("axialCoord", axialCoord);
  printValue("tangCoord", tangCoord);
  printEmptyLine();

  // Crystal and slice coordinates
  printValue("crystal1", crystal1);
  printValue("slice1", slice1);
  printValue("crystal2", crystal2);
  printValue("slice2", slice2);
  printEmptyLine();
  std::cout << "X: ("
            << mCrystalXYPositionVector[crystal1].x
            << ",";
  std::cout << mCrystalXYPositionVector[crystal2].x
            << ")"
            << std::endl;
  std::cout << "Y: ("
            << mCrystalXYPositionVector[crystal1].y
            << ",";
  std::cout << mCrystalXYPositionVector[crystal2].y
            << ")"
            << std::endl;
  std::cout << "Z: ("
            << mSliceZPositionVector[slice1]
            << ",";
  std::cout << mSliceZPositionVector[slice2]
            << ")"
            << std::endl;
  printEmptyLine();

  // Path
  if (pathElementsArray)
  {
    std::cout << "{";
    for (auto pathIndex = 0;
         pathElementsArray[pathIndex].coord != -1;
         pathIndex++)
    {
      if (pathIndex != 0)
      {
        std::cout << ",";
      }
      std::cout << pathElementsArray[pathIndex].coord;
    }
    std::cout << "}" << std::endl;

    std::cout << "{";
    for (auto pathIndex = 0;
         pathElementsArray[pathIndex].coord != -1;
         pathIndex++)
    {
      if (pathIndex != 0)
      {
        std::cout << ",";
      }
      std::cout << pathElementsArray[pathIndex].length;
    }
    std::cout << "}" << std::endl << std::endl;

    std::cout << "{";
    for (int pathIndex = 0;
         pathElementsArray[pathIndex].coord != -1;
         pathIndex++)
    {
      if (pathIndex != 0)
      {
        std::cout << ",";
      }
      std::cout << vol.getDataArray()[
        pathElementsArray[pathIndex].coord];
    }
    std::cout << "}" << std::endl << std::endl;
  }

  // Path length
  printValue("line", line);
  printEmptyLine();
}

void ScannerData::getCrystalCoordinates(
  int rSectorID,
  int moduleID,
  int crystalID,
  int* ringCoord,
  int* crystalCoord) const
{
  // Ring coordinate
  const auto moduleRepeatY = mHeader.moduleRepeatNumbersYZ[0];
  const auto crystalRepeatY = mHeader.crystalRepeatNumbersYZ[0];
  const auto crystalRepeatZ = mHeader.crystalRepeatNumbersYZ[1];

  *ringCoord =
    (moduleID / moduleRepeatY) * crystalRepeatZ +
    crystalID / crystalRepeatY;

  // Crystal coordinate in ring
  auto crystalCoord_temp =
    rSectorID * moduleRepeatY * crystalRepeatY +
    (moduleID % moduleRepeatY) * crystalRepeatY +
    (crystalID % crystalRepeatY) -
    mGeometry.crystalOffset;

  if (crystalCoord_temp >= mGeometry.nCrystalsPerRing)
  {
    crystalCoord_temp -= mGeometry.nCrystalsPerRing;
  }

  *crystalCoord = crystalCoord_temp;
}

// angle is in radians
static std::tuple<types::SpatialCoord, types::SpatialCoord>
rotateXY(types::SpatialCoord xIn,
         types::SpatialCoord yIn,
         types::SpatialCoord angle)
{
  const auto cosAngle = std::cos(angle);
  const auto sinAngle = std::sin(angle);

  const auto xOut = xIn * cosAngle - yIn * sinAngle;
  const auto yOut = xIn * sinAngle + yIn * cosAngle;

  return {xOut, yOut};
}

void ScannerData::computeCrystalXYPositionVector()
{
  // Parameters for the crystal positions in Y within the first
  // r-sector, which is vertical and repeated rotationally

  const auto nCrystalsY = mHeader.crystalRepeatNumbersYZ[0];
  const auto crystalRepeatVectorY =
    mGeometry.crystalRepeatVectorYZ[0];

  const auto nModulesY = mHeader.moduleRepeatNumbersYZ[0];
  const auto moduleRepeatVectorY =
    mGeometry.moduleRepeatVectorYZ[0];

  // Y position of the first crystal of the first r-sector:
  // Half the distance between the first and the last crystal
  // of the first r-sector, on the negative Y side
  const auto firstCrystalCoordY =
    -((nModulesY - 1) * moduleRepeatVectorY +
      (nCrystalsY - 1) * crystalRepeatVectorY) / 2.0;

  // Position in Y of each crystal in the first r-sector
  const auto rSectorNCrystalsY = nModulesY * nCrystalsY;
  std::vector<types::SpatialCoord> rSectorY(rSectorNCrystalsY);
  LOOP(moduleIndex, 0, nModulesY - 1)
  LOOP(crystalIndex, 0, nCrystalsY - 1)
    rSectorY[moduleIndex * nCrystalsY + crystalIndex] =
      moduleIndex * moduleRepeatVectorY +
      crystalIndex * crystalRepeatVectorY +
      firstCrystalCoordY;

  // Crystal positions in XY for an entire ring
  mCrystalXYPositionVector =
    (types::SpatialCoords2D*)std::malloc(
      mGeometry.nCrystalsPerRing *
      sizeof(types::SpatialCoords2D));

  // Angular distance between r-sectors
  const auto angleInterval =
    2 * PI / mHeader.rSectorRepeatNumber;

  // Index of the first crystal of the first r-sector:
  // Such that the crystal with index 0 is the middle crystal of
  // the first r-sector that is on the X axis or close to it
  const auto firstCrystalIndex =
    mGeometry.nCrystalsPerRing - mGeometry.crystalOffset;

  // Find the position of each crystal from the middle crystal
  // of the first r-sector (index 0) up to but excluding the
  //first crystal of the first r-sector (first_crystal_index)
  LOOP(crystalIndex, 0, firstCrystalIndex - 1)
  {
    const auto x = mGeometry.rSectorTranslationX;

    const auto crystalIndexWithOffset =
      crystalIndex + mGeometry.crystalOffset;

    const auto y =
      rSectorY[crystalIndexWithOffset % rSectorNCrystalsY];

    const auto angle =
      (crystalIndexWithOffset / rSectorNCrystalsY) *
      angleInterval;

    const auto [xRot, yRot] = rotateXY(x, y, angle);

    mCrystalXYPositionVector[crystalIndex].x = xRot;
    mCrystalXYPositionVector[crystalIndex].y = yRot;
  }

  // Complete the first half of the first r-sector
  LOOP(
    crystalIndex,
    firstCrystalIndex,
    mGeometry.nCrystalsPerRing - 1)
  {
    mCrystalXYPositionVector[crystalIndex].x =
      mGeometry.rSectorTranslationX;

    mCrystalXYPositionVector[crystalIndex].y =
      rSectorY[crystalIndex - firstCrystalIndex];
  }
}

void ScannerData::computeSliceZPositionVector()
{
  // Parameters for crystal positions in Z within each r-sector

  const auto nCrystalsZ = mHeader.crystalRepeatNumbersYZ[1];
  const auto moduleRepeatVectorZ =
    mGeometry.moduleRepeatVectorYZ[1];

  const auto nModulesZ = mHeader.moduleRepeatNumbersYZ[1];
  const auto crystalRepeatVectorZ =
    mGeometry.crystalRepeatVectorYZ[1];

  // Z position of the first ring (and of the first slice):
  // Half the distance between the first ring and the last
  // ring, on the negative Z side
  const auto firstCrystalOffsetZ =
    -((nModulesZ - 1) * moduleRepeatVectorZ +
      (nCrystalsZ - 1) * crystalRepeatVectorZ) / 2.0;

  // Position in Z of each ring in a r-sector
  const auto rSectorNCrystalsZ = nModulesZ * nCrystalsZ;
  std::vector<types::SpatialCoord> ringZPositionVector(
    rSectorNCrystalsZ);
  LOOP(moduleIndex, 0, nModulesZ - 1)
    LOOP(crystalIndex, 0, nCrystalsZ - 1)
      ringZPositionVector[
        moduleIndex * nCrystalsZ + crystalIndex] =
        moduleIndex * moduleRepeatVectorZ +
        crystalIndex * crystalRepeatVectorZ +
        firstCrystalOffsetZ;

  // Slice positions in Z
  mSliceZPositionVector =
    (types::SpatialCoord*)std::malloc(
      mGeometry.nSlices *
      sizeof(types::SpatialCoord));
  LOOP(sliceIndex, 0, mGeometry.nSlices - 1)
  {
    mSliceZPositionVector[sliceIndex] =
      sliceIndex % 2 == 0 ?
      // Even slice: Aligned with a ring
      ringZPositionVector[sliceIndex / 2] :
      // Odd slice: Between two neighboring rings
      (ringZPositionVector[sliceIndex / 2] +
       ringZPositionVector[(sliceIndex + 1) / 2]) / 2.0;
  }
}
