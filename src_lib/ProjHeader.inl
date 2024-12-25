#pragma once

#include <ProjHeader.h>

bool ProjHeader::operator==(const ProjHeader& rhs) const
{
  return nRings == rhs.nRings &&
    nCrystalsPerRing == rhs.nCrystalsPerRing &&
    segmentSpan == rhs.segmentSpan &&
    nSegments == rhs.nSegments &&
    nTangCoords == rhs.nTangCoords;
}

ProjHeader& ProjHeader::operator=(const ProjHeader& rhs)
{
  nRings = rhs.nRings;
  nCrystalsPerRing = rhs.nCrystalsPerRing;
  segmentSpan = rhs.segmentSpan;
  nSegments = rhs.nSegments;
  nTangCoords = rhs.nTangCoords;

  return *this;
}

void ProjHeader::setDefaults()
{
  // Set those parameters to an invalid value to trigger an
  // error if no valid value is substituted before calling check
  nRings = 0;
  nCrystalsPerRing = 0;

  // Default behavior: Keep only the central diagonal
  segmentSpan = 1;
  nSegments = 1;

  // This parameter is replaced with its maximum possible value
  // in the check() method if it is not set to a value greater
  // than zero beforehand.
  nTangCoords = 0;
}

void ProjGeometry::operator=(const ProjGeometry& other)
{
  nBins = other.nBins;
  nViews = other.nViews;
  nAxialCoords = other.nAxialCoords;
  segOffset = other.segOffset;
  tangCoordOffset = other.tangCoordOffset;
  halfSegmentSpan = other.halfSegmentSpan;
  maxRingDiff = other.maxRingDiff;
}

int ProjGeometry::getNAxialCoords(int seg) const
{
  return nAxialCoords[seg + segOffset];
}
