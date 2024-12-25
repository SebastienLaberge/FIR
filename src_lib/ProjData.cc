#include <ProjData.h>

#include <ProjInterfileReader.h>
#include <console.h>
#include <macros.h>

#include <cassert>
#include <cmath>
#include <cstdlib>

ProjData::ProjData():
  mDataArray{nullptr}
{
}

ProjData::ProjData(
  const std::string& inputProjFile,
  ConstructionMode mode,
  types::BinValue initValue):
  mDataArray{nullptr}
{
  read(inputProjFile, mode, initValue);
}

ProjData::ProjData(
  const ProjData& proj,
  ConstructionMode mode,
  types::BinValue initValue):
  mDataArray{nullptr}
{
  copy(proj, mode, initValue);
}

ProjData::~ProjData()
{
  deallocate();
}

void ProjData::read(
  const std::string& headerFileName,
  ConstructionMode mode,
  types::BinValue initValue)
{
  // Deallocate in case it has already been allocated
  deallocate();

  ProjInterfileReader reader(headerFileName);
  mHeader = reader.getHeader();
  mGeometry = reader.getGeometry();

  switch (mode)
  {
  case ConstructionMode::ALLOCATE:

    allocate(false);
    break;

  case ConstructionMode::INITIALIZE:

    allocate(true, initValue);
    break;

  case ConstructionMode::READ_DATA:

    allocate(false);
    reader.readData(mDataArray);
    break;
  }
}

void ProjData::copy(
  const ProjData& proj,
  ConstructionMode mode,
  types::BinValue initValue)
{
  copyParameters(proj);

  // Deallocate in case it has already been allocated
  deallocate();

  switch (mode)
  {
  case ConstructionMode::ALLOCATE:

    allocate(false);
    break;

  case ConstructionMode::INITIALIZE:

    allocate(true, initValue);
    break;

  case ConstructionMode::READ_DATA:

    allocate(false);

#pragma omp parallel for
    LOOP(seg, -mGeometry.segOffset, mGeometry.segOffset)
    {
      const auto nBinsInSegment =        //
        mGeometry.getNAxialCoords(seg) * //
        mGeometry.nViews *               //
        mHeader.nTangCoords;

      LOOP(binIndex, 0, nBinsInSegment - 1)
      {
        mDataArray[seg + mGeometry.segOffset][binIndex] =
          proj.mDataArray[seg + mGeometry.segOffset][binIndex];
      }
    }
    break;
  }
}

void ProjData::write(const std::string& outputProjFile) const
{
  if (mDataArray == nullptr)
  {
    error("Projection data is not allocated");
  }

  ProjInterfileReader::writeProjInterfile(
    outputProjFile,
    mHeader,
    mDataArray);
}

void ProjData::printContent() const
{
  echo("= Projection header:");
  printEmptyLine();

  echo("== Scanner basic geometry:");
  printValue("number of rings", mHeader.nRings);
  printValue(
    "number of crystals per ring",
    mHeader.nCrystalsPerRing);
  printEmptyLine();

  echo("== Michelogram compression:");
  printValue("segment span", mHeader.segmentSpan);
  printEmptyLine();

  echo("== Projection dimensions:");
  printValue("number of segments", mHeader.nSegments);
  printVector("number of axial coords", mGeometry.nAxialCoords);
  printValue("number of views", mGeometry.nViews);
  printValue(
    "number of tangential coordinates",
    mHeader.nTangCoords);
  printEmptyLine();

  echo("== Others:");
  printValue("total number of bins", mGeometry.nBins);
  printValue("segment offset", mGeometry.segOffset);
  printValue(
    "tangential coordinate offset",
    mGeometry.tangCoordOffset);
  printValue(
    "half the segment span",
    mGeometry.halfSegmentSpan);
  printValue("maximum ring difference", mGeometry.maxRingDiff);
  printEmptyLine();
}

void ProjData::checkNSubsets(int nSubsets) const
{
  if (mGeometry.nViews % nSubsets != 0)
  {
    error(
      "Number of subsets must be a divisor of ",
      mGeometry.nViews);
  }
}

void ProjData::setAllBins(types::BinValue value)
{
  if (mDataArray == nullptr)
  {
    error("Projection not allocated");
  }

#pragma omp parallel for
  LOOP(seg, -mGeometry.segOffset, mGeometry.segOffset)
  {
    const auto nBinsInSegment =        //
      mGeometry.getNAxialCoords(seg) * //
      mGeometry.nViews *               //
      mHeader.nTangCoords;

    LOOP(binIndex, 0, nBinsInSegment - 1)
    {
      mDataArray[seg + mGeometry.segOffset][binIndex] = value;
    }
  }
}

std::tuple<int, int> ProjData::getCrystalAxialCoord(
  int seg,
  int axialCoord) const
{
  int crystalAxialCoord1, crystalAxialCoord2;

  if (mHeader.segmentSpan == 1)
  {
    // Span 1
    if (seg == 0)
    {
      // Central segment
      crystalAxialCoord1 = crystalAxialCoord2 = 2 * axialCoord;
    }
    else if (seg > 0)
    {
      crystalAxialCoord1 = 2 * (axialCoord + seg);
      crystalAxialCoord2 = 2 * axialCoord;
    }
    else
    {
      crystalAxialCoord1 = 2 * axialCoord;
      crystalAxialCoord2 = 2 * (axialCoord - seg);
    }
  }
  else if (seg == 0)
  {
    // Span > 1, central segment
    crystalAxialCoord1 = crystalAxialCoord2 = axialCoord;
  }
  else
  {
    // Span > 1, lateral segment
    const auto midSegRingDiff = ABS(seg) * mHeader.segmentSpan;

    if (axialCoord < mGeometry.halfSegmentSpan)
    {
      // Initial portion
      const auto ringDiff =
        midSegRingDiff - mGeometry.halfSegmentSpan + axialCoord;

      if (seg > 0)
      {
        crystalAxialCoord1 = 2 * ringDiff;
        crystalAxialCoord2 = 0;
      }
      else
      {
        crystalAxialCoord1 = 0;
        crystalAxialCoord2 = 2 * ringDiff;
      }
    }
    else
    {
      const auto invertedAxialCoord =
        mGeometry.getNAxialCoords(seg) - axialCoord - 1;

      if (invertedAxialCoord < mGeometry.halfSegmentSpan)
      {
        // Final portion
        const auto ringDiff =         //
          midSegRingDiff -            //
          mGeometry.halfSegmentSpan + //
          invertedAxialCoord;

        const auto sliceMax = 2 * mHeader.nRings - 2;

        if (seg > 0)
        {
          crystalAxialCoord1 = sliceMax;
          crystalAxialCoord2 = sliceMax - 2 * ringDiff;
        }
        else
        {
          crystalAxialCoord1 = sliceMax - 2 * ringDiff;
          crystalAxialCoord2 = sliceMax;
        }
      }
      else
      {
        // Central portion
        const auto adjustedAxialCoord =
          axialCoord - mGeometry.halfSegmentSpan;

        if (seg > 0)
        {
          crystalAxialCoord1 =
            2 * midSegRingDiff + adjustedAxialCoord;
          crystalAxialCoord2 = adjustedAxialCoord;
        }
        else
        {
          crystalAxialCoord1 = adjustedAxialCoord;
          crystalAxialCoord2 =
            2 * midSegRingDiff + adjustedAxialCoord;
        }
      }
    }
  }

  return {crystalAxialCoord1, crystalAxialCoord2};
}

std::tuple<int, int> ProjData::getCrystalAngCoord(
  int view,
  int tangCoord) const
{
  // With transaxial compression (max nTangBins is
  // nCrystalsPerRing / 2 - 1)
  // int crystalAngCoord1 = view          + tangCoord;
  // int crystalAngCoord2 = view + nViews - tangCoord;

  const auto crystalTranslation =
    tangCoord >= 0 ? tangCoord / 2 : (tangCoord - 1) / 2;

  const auto oddBinAdjustment = ABS(tangCoord) % 2;

  auto crystalAngCoord1 =
    view + crystalTranslation + oddBinAdjustment;

  auto crystalAngCoord2 =
    view + mGeometry.nViews - crystalTranslation;

  if (crystalAngCoord1 < 0)
  {
    crystalAngCoord1 += mHeader.nCrystalsPerRing;
  }
  else if (crystalAngCoord1 >= mHeader.nCrystalsPerRing)
  {
    crystalAngCoord1 -= mHeader.nCrystalsPerRing;
  }

  if (crystalAngCoord2 < 0)
  {
    crystalAngCoord2 += mHeader.nCrystalsPerRing;
  }
  else if (crystalAngCoord2 >= mHeader.nCrystalsPerRing)
  {
    crystalAngCoord2 -= mHeader.nCrystalsPerRing;
  }

  return {crystalAngCoord1, crystalAngCoord2};
}

bool ProjData::getBinCoordinates(
  int crystalAxialCoord1,
  int crystalAngCoord1,
  int crystalAxialCoord2,
  int crystalAngCoord2,
  int* seg,
  int* view,
  int* axialCoord,
  int* tangCoord) const
{
  // Local copies of output variables
  int seg_loc, view_loc, axialCoord_loc, tangCoord_loc;

  // Absolute value of segment number
  auto absSeg = ABS(crystalAxialCoord1 - crystalAxialCoord2);

  // LOR falls outside allocated segments
  if (absSeg > mGeometry.maxRingDiff)
  {
    return false;
  }

  // Tangential coordinate:

  // sign1 = +1:
  //   sum in [0, nCrystalsPerRing / 2 [ U
  //          [3 * nCrystalsPerRing / 2,
  //           2 * (nCrystalsPerRing - 1)]
  // sign1 = -1:
  //   sum in [nCrystalsPerRing / 2, 3 * nCrystalsPerRing / 2[
  const auto sum = crystalAngCoord1 + crystalAngCoord2;
  const auto sign1 = //
    sum >= mHeader.nCrystalsPerRing / 2 &&
      sum < 3 * mHeader.nCrystalsPerRing / 2 ?
    -1 :
    +1;

  tangCoord_loc = sign1 *
    (ABS(crystalAngCoord2 - crystalAngCoord1) -
     mHeader.nCrystalsPerRing / 2);

  // LOR falls outside allocated tangential coordinates
  if (
    tangCoord_loc < -mGeometry.tangCoordOffset ||
    tangCoord_loc >=
      -mGeometry.tangCoordOffset + mHeader.nTangCoords)
  {
    return false;
  }

  // Axial coordinate:
  if (mHeader.segmentSpan == 1)
  {
    axialCoord_loc =
      (crystalAxialCoord1 + crystalAxialCoord2 - absSeg) / 2;
  }
  else
  {
    // Modify absSeg to consider segment fusion
    const auto inCentralSeg =
      absSeg <= mGeometry.halfSegmentSpan;

    absSeg = inCentralSeg ? //
      0 :
      1 +
        (absSeg - mGeometry.halfSegmentSpan - 1) /
          mHeader.segmentSpan;

    // TODO: Find better variable name
    const auto m = inCentralSeg ?
      0 :
      1 + mGeometry.halfSegmentSpan +
        (absSeg - 1) * mHeader.segmentSpan;

    axialCoord_loc =
      crystalAxialCoord1 + crystalAxialCoord2 - m;
  }

  // TODO: Find a better variable name
  const auto n = sum + mHeader.nCrystalsPerRing / 2;

  // View
  view_loc = (n % mHeader.nCrystalsPerRing) / 2;

  // Segment number sign
  seg_loc = absSeg;
  if (seg_loc != 0)
  {
    const auto sign2 =
      crystalAngCoord1 < crystalAngCoord2 ? +1 : -1;

    const auto u = ABS(tangCoord_loc) % 2 == 0 ?
      // Even bin: half the tangCoord offset to
      // reach tangCoord = 0
      -tangCoord_loc / 2 :
      // Odd bin: half the tangCoord offset to
      // reach tangCoord = 1
      -(tangCoord_loc - 1) / 2;

    // Crystals of parallel LOR closest to center
    // (tangCoord = 0 or 1)
    auto c1 = crystalAngCoord1 - sign1 * sign2 * u;
    auto c2 = crystalAngCoord2 + sign1 * sign2 * u;

    if (c1 >= mHeader.nCrystalsPerRing)
    {
      c1 = c1 - mHeader.nCrystalsPerRing;
    }
    else if (c1 < 0)
    {
      c1 = c1 + mHeader.nCrystalsPerRing;
    }

    if (c2 >= mHeader.nCrystalsPerRing)
    {
      c2 = c2 - mHeader.nCrystalsPerRing;
    }
    else if (c2 < 0)
    {
      c2 = c2 + mHeader.nCrystalsPerRing;
    }

    const auto segSign =
      crystalAxialCoord1 < crystalAxialCoord2 ? -1 : +1;

    const auto segFlip = c1 < c2 ? +1 : -1;

    seg_loc = seg_loc * segSign * segFlip;
  }

  // Set output
  *seg = seg_loc;
  *view = view_loc;
  *axialCoord = axialCoord_loc;
  *tangCoord = tangCoord_loc;

  return true;
}

ProjData& ProjData::operator*=(const ProjData& inputProj)
{
  assert(mHeader == inputProj.mHeader);

  LOOP(seg, -mGeometry.segOffset, mGeometry.segOffset)
  {
    const auto maxInd =                //
      mGeometry.nViews *               //
      mGeometry.getNAxialCoords(seg) * //
      mHeader.nTangCoords;

    const auto segWithOffset = seg + mGeometry.segOffset;

#pragma omp parallel for
    LOOP(binIndex, 0, maxInd - 1)
    {
      if (
        mDataArray[segWithOffset][binIndex] > EPSILON &&
        inputProj.mDataArray[segWithOffset][binIndex] > EPSILON)
      {
        mDataArray[segWithOffset][binIndex] *=
          inputProj.mDataArray[segWithOffset][binIndex];
      }
      else
      {
        mDataArray[seg + mGeometry.segOffset][binIndex] = 0.0;
      }
    }
  }

  return *this;
}

void ProjData::exponential()
{
  LOOP(seg, -mGeometry.segOffset, mGeometry.segOffset)
  {
    const auto maxInd =                //
      mGeometry.nViews *               //
      mGeometry.getNAxialCoords(seg) * //
      mHeader.nTangCoords;

    const auto segWithOffset = seg + mGeometry.segOffset;

#pragma omp parallel for
    LOOP(binIndex, 0, maxInd - 1)
    {
      if (mDataArray[segWithOffset][binIndex] > EPSILON)
      {
        mDataArray[segWithOffset][binIndex] =
          std::exp(mDataArray[segWithOffset][binIndex]);
      }
      else
      {
        mDataArray[segWithOffset][binIndex] = 1.0;
      }
    }
  }
}

void ProjData::rebinWeight()
{
  if (mHeader.segmentSpan != 1)
  {
    LOOP(ringSum, 0, 2 * (mHeader.nRings - 1))
    {
      auto localMaxRingDiff = //
        ringSum <= mHeader.nRings - 1 ?
        ringSum :
        2 * (mHeader.nRings - 1) - ringSum;

      localMaxRingDiff =
        MIN(localMaxRingDiff, mGeometry.maxRingDiff);

      if ((ringSum + localMaxRingDiff) % 2)
      {
        localMaxRingDiff--;
      }

      // Find starting point
      auto seg = -mGeometry.segOffset - 1;
      auto nextRingDiff = -mGeometry.maxRingDiff - 1;
      while (nextRingDiff < -localMaxRingDiff)
      {
        seg++;
        nextRingDiff += mHeader.segmentSpan;
      }

      // Go through ring differences
      int weight = 1;
      for (auto ringDiff = -localMaxRingDiff;
           ringDiff <= localMaxRingDiff;
           ringDiff += 2)
      {
        if (
          ringDiff == localMaxRingDiff ||
          ringDiff + 1 >= nextRingDiff)
        {
          if (weight > 1)
          {
            const auto axialCoord = !seg ?
              ringSum :
              ringSum - (mHeader.segmentSpan + 1) / 2 -
                (ABS(seg) - 1) * mHeader.segmentSpan;

            LOOP(view, 0, mGeometry.nViews - 1)
            {
              LOOP(
                tangCoord,
                -mGeometry.tangCoordOffset,
                -mGeometry.tangCoordOffset +
                  mHeader.nTangCoords - 1)
              {
                weightBin(
                  seg,
                  view,
                  axialCoord,
                  tangCoord,
                  1.0 / weight);
              }
            }
          }

          weight = 1;
          seg++;
          nextRingDiff = nextRingDiff + mHeader.segmentSpan;
        }
        else
        {
          weight++;
        }
      }
    }
  }
}

void ProjData::copyParameters(const ProjData& proj)
{
  mHeader = proj.mHeader;
  mGeometry = proj.mGeometry;
}

void ProjData::allocate(
  bool initialize,
  types::BinValue initValue)
{
  // Clear mDataArray in case it is already allocated
  deallocate();

  // Allocate mDataArray and fill with zeros
  mDataArray = (types::BinValue**)std::malloc(
    mHeader.nSegments * sizeof(types::BinValue*));

  LOOP(seg, -mGeometry.segOffset, mGeometry.segOffset)
  {
    const auto binsToWrite = mGeometry.nViews *
      mGeometry.getNAxialCoords(seg) * mHeader.nTangCoords;

    mDataArray[seg + mGeometry.segOffset] =
      (types::BinValue*)std::malloc(
        binsToWrite * sizeof(types::BinValue));

    if (initialize)
    {
      LOOP(i, 0, binsToWrite - 1)
      {
        mDataArray[seg + mGeometry.segOffset][i] = initValue;
      }
    }
  }
}

void ProjData::deallocate()
{
  if (mDataArray != nullptr)
  {
#pragma omp parallel for
    LOOP(seg, -mGeometry.segOffset, mGeometry.segOffset)
    {
      std::free(mDataArray[seg + mGeometry.segOffset]);
    }
  }

  mDataArray = nullptr;
}
