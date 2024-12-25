#pragma once

#include <ScannerData.h>

const types::SpatialCoords2D*
ScannerData::getCrystalXYPositionVector() const
{
  return mCrystalXYPositionVector;
}

const types::SpatialCoord*
ScannerData::getSliceZPositionVector() const
{
  return mSliceZPositionVector;
}
