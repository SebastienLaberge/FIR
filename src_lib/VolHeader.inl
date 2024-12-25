#pragma once

#include <VolHeader.h>

bool VolHeader::operator==(const VolHeader& rhs) const
{
  // The number of frames is not compared
  return volSize == rhs.volSize &&
         voxelExtent == rhs.voxelExtent &&
         volOffset == rhs.volOffset;
}

bool VolHeader::operator!=(const VolHeader& rhs) const
{
  return !(*this == rhs);
}

VolHeader& VolHeader::operator=(const VolHeader& rhs)
{
  volSize = rhs.volSize;
  voxelExtent = rhs.voxelExtent;
  volOffset = rhs.volOffset;
  nFrames = rhs.nFrames;

  return *this;
}

void VolHeader::setDefaults()
{
  // Set those parameters to an invalid value to trigger an
  // error if no valid value is substituted before check is
  // called
  volSize.nPixelsX = 0;
  volSize.nPixelsY = 0;
  volSize.nSlices = 0;
  voxelExtent.pixelWidth = 0.0;
  voxelExtent.pixelHeight = 0.0;
  voxelExtent.sliceThickness = 0.0;

  // Should be set to something else at least in x and y, but
  // any value is considered valid
  volOffset.x = 0.0;
  volOffset.y = 0.0;
  volOffset.z = 0.0;

  // Assume a single frame unless something else is specified
  nFrames = 1;
}

VolGeometry& VolGeometry::operator=(const VolGeometry& rhs)
{
  volExtent = rhs.volExtent;
  nVoxelsPerFrame = rhs.nVoxelsPerFrame;
  nVoxelsTotal = rhs.nVoxelsTotal;

  return *this;
}
